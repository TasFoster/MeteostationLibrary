#include "TWI.h"
#include <Arduino.h>
#include <util/atomic.h>

void mydelay(uint16_t ms) { delayMicroseconds(ms * 1000); }

namespace {
constexpr uint32_t kTimerTickUs = 64;
constexpr uint32_t kMicrosecondsPerMillisecond = 1000;
static_assert(F_CPU == 16000000UL, "Timer4 timeouts require 16 MHz");
}  // namespace

uint16_t TWI::timerTicks() const {
  uint16_t ticks;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ticks = TCNT4;
  }
  return ticks;
}

uint32_t TWI::mymillis(uint16_t started_at) const {
  const uint16_t elapsed_ticks =
      static_cast<uint16_t>(timerTicks() - started_at);
  return static_cast<uint32_t>(elapsed_ticks) * kTimerTickUs /
         kMicrosecondsPerMillisecond;
}

void TWI::init() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR4B = 0;
    TCCR4A = 0;
    TCCR4C = 0;
    TIMSK4 = 0;
    TCNT4 = 0;
    TIFR4 = _BV(TOV4);
    TCCR4B = _BV(CS42) | _BV(CS40);  // Normal, делитель 1024.
  }

  TWBR = TWI_BIT_RATE_REGISTER_VALUE;
  TWSR = TWI_PRESCALER_BITS;
  TWCR = (1 << TWEN);
  timeout_ = false;
}

uint8_t TWI::start() {
  timeout_ = false;

  TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);

  if (!waitForTwint()) {
    return TIMEOUT;
  }

  return TWSR & TWI_STATUS_MASK;
}

void TWI::stop() {
  if (timeout_) {
    return;
  }

  TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);

  waitForStopClear();
}

uint8_t TWI::write(uint8_t data) {
  timeout_ = false;

  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);

  if (!waitForTwint()) {
    return TIMEOUT;
  }

  return TWSR & TWI_STATUS_MASK;
}

uint8_t TWI::readAck() {
  timeout_ = false;

  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

  if (!waitForTwint()) {
    return TWI_INVALID_DATA;
  }

  return TWDR;
}

uint8_t TWI::readNack() {
  timeout_ = false;

  TWCR = (1 << TWINT) | (1 << TWEN);

  if (!waitForTwint()) {
    return TWI_INVALID_DATA;
  }

  return TWDR;
}

uint8_t TWI::readReg(uint8_t addr, uint8_t reg, uint8_t &value) {
  uint8_t status;

  status = start();
  if (status != TWI_START_TRANSMITTED) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_START_ERROR);
  }

  status = write((addr << TWI_ADDRESS_SHIFT) | TWI_WRITE_BIT);
  if (status != TWI_ADDRESS_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT
                    : static_cast<uint8_t>(TWI_READ_ADDRESS_WRITE_ERROR);
  }

  status = write(reg);
  if (status != TWI_DATA_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_REGISTER_ERROR);
  }

  status = start();
  if (status != TWI_REPEATED_START_TRANSMITTED) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_RESTART_ERROR);
  }

  status = write((addr << TWI_ADDRESS_SHIFT) | TWI_READ_BIT);
  if (status != TWI_ADDRESS_READ_ACK) {
    stop();
    return timeout_ ? TIMEOUT
                    : static_cast<uint8_t>(TWI_READ_ADDRESS_READ_ERROR);
  }

  value = readNack();
  if (timeout_) {
    return TIMEOUT;
  }

  stop();

  return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_SUCCESS);
}

uint8_t TWI::writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
  uint8_t status;

  status = start();
  if (status != TWI_START_TRANSMITTED) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_WRITE_START_ERROR);
  }

  status = write((addr << TWI_ADDRESS_SHIFT) | TWI_WRITE_BIT);
  if (status != TWI_ADDRESS_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_WRITE_ADDRESS_ERROR);
  }

  status = write(reg);
  if (status != TWI_DATA_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_WRITE_REGISTER_ERROR);
  }

  status = write(value);
  if (status != TWI_DATA_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_WRITE_DATA_ERROR);
  }

  stop();

  return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_WRITE_SUCCESS);
}

uint8_t TWI::readRegs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
  if (len == 0 || buf == nullptr) {
    return TWI_READ_INVALID_ARGUMENT;
  }

  uint8_t status;

  status = start();
  if (status != TWI_START_TRANSMITTED) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_START_ERROR);
  }

  status = write((addr << TWI_ADDRESS_SHIFT) | TWI_WRITE_BIT);
  if (status != TWI_ADDRESS_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT
                    : static_cast<uint8_t>(TWI_READ_ADDRESS_WRITE_ERROR);
  }

  status = write(reg);
  if (status != TWI_DATA_WRITE_ACK) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_REGISTER_ERROR);
  }

  status = start();
  if (status != TWI_REPEATED_START_TRANSMITTED) {
    stop();
    return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_RESTART_ERROR);
  }

  status = write((addr << TWI_ADDRESS_SHIFT) | TWI_READ_BIT);
  if (status != TWI_ADDRESS_READ_ACK) {
    stop();
    return timeout_ ? TIMEOUT
                    : static_cast<uint8_t>(TWI_READ_ADDRESS_READ_ERROR);
  }

  for (uint8_t i = 0; i < len - 1; i++) {
    buf[i] = readAck();
    if (timeout_) {
      return TIMEOUT;
    }
  }

  buf[len - 1] = readNack();
  if (timeout_) {
    return TIMEOUT;
  }

  stop();

  return timeout_ ? TIMEOUT : static_cast<uint8_t>(TWI_READ_SUCCESS);
}

bool TWI::waitForTwint() {
  const uint16_t started_at = timerTicks();

  while (!(TWCR & (1 << TWINT))) {
    if (mymillis(started_at) >= TIMEOUT_MS) {
      timeout_ = true;
      TWCR = TWI_DISABLED;
      return false;
    }
  }

  return true;
}

bool TWI::waitForStopClear() {
  const uint16_t started_at = timerTicks();

  while (TWCR & (1 << TWSTO)) {
    if (mymillis(started_at) >= TIMEOUT_MS) {
      timeout_ = true;
      TWCR = TWI_DISABLED;
      return false;
    }
  }

  return true;
}