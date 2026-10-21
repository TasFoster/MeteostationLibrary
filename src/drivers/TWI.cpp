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

  TWBR = kTwiBitRateRegisterValue;
  TWSR = kTwiPrescalerBits;
  TWCR = (1 << TWEN);
  timeout_ = false;
}

uint8_t TWI::start() {
  timeout_ = false;

  TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);

  if (!waitForTwint()) {
    return kTimeout;
  }

  return TWSR & kTwiStatusMask;
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
    return kTimeout;
  }

  return TWSR & kTwiStatusMask;
}

uint8_t TWI::readAck() {
  timeout_ = false;

  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

  if (!waitForTwint()) {
    return kTwiInvalidData;
  }

  return TWDR;
}

uint8_t TWI::readNack() {
  timeout_ = false;

  TWCR = (1 << TWINT) | (1 << TWEN);

  if (!waitForTwint()) {
    return kTwiInvalidData;
  }

  return TWDR;
}

uint8_t TWI::readReg(uint8_t addr, uint8_t reg, uint8_t &value) {
  uint8_t status;

  status = start();
  if (status != kTwiStartTransmitted) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadStartError);
  }

  status = write((addr << kTwiAddressShift) | kTwiWriteBit);
  if (status != kTwiAddressWriteAck) {
    stop();
    return timeout_ ? kTimeout
                    : static_cast<uint8_t>(kTwiReadAddressWriteError);
  }

  status = write(reg);
  if (status != kTwiDataWriteAck) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadRegisterError);
  }

  status = start();
  if (status != kTwiRepeatedStartTransmitted) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadRestartError);
  }

  status = write((addr << kTwiAddressShift) | kTwiReadBit);
  if (status != kTwiAddressReadAck) {
    stop();
    return timeout_ ? kTimeout
                    : static_cast<uint8_t>(kTwiReadAddressReadError);
  }

  value = readNack();
  if (timeout_) {
    return kTimeout;
  }

  stop();

  return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadSuccess);
}

uint8_t TWI::writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
  uint8_t status;

  status = start();
  if (status != kTwiStartTransmitted) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiWriteStartError);
  }

  status = write((addr << kTwiAddressShift) | kTwiWriteBit);
  if (status != kTwiAddressWriteAck) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiWriteAddressError);
  }

  status = write(reg);
  if (status != kTwiDataWriteAck) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiWriteRegisterError);
  }

  status = write(value);
  if (status != kTwiDataWriteAck) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiWriteDataError);
  }

  stop();

  return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiWriteSuccess);
}

uint8_t TWI::readRegs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
  if (len == 0 || buf == nullptr) {
    return kTwiReadInvalidArgument;
  }

  uint8_t status;

  status = start();
  if (status != kTwiStartTransmitted) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadStartError);
  }

  status = write((addr << kTwiAddressShift) | kTwiWriteBit);
  if (status != kTwiAddressWriteAck) {
    stop();
    return timeout_ ? kTimeout
                    : static_cast<uint8_t>(kTwiReadAddressWriteError);
  }

  status = write(reg);
  if (status != kTwiDataWriteAck) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadRegisterError);
  }

  status = start();
  if (status != kTwiRepeatedStartTransmitted) {
    stop();
    return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadRestartError);
  }

  status = write((addr << kTwiAddressShift) | kTwiReadBit);
  if (status != kTwiAddressReadAck) {
    stop();
    return timeout_ ? kTimeout
                    : static_cast<uint8_t>(kTwiReadAddressReadError);
  }

  for (uint8_t i = 0; i < len - 1; i++) {
    buf[i] = readAck();
    if (timeout_) {
      return kTimeout;
    }
  }

  buf[len - 1] = readNack();
  if (timeout_) {
    return kTimeout;
  }

  stop();

  return timeout_ ? kTimeout : static_cast<uint8_t>(kTwiReadSuccess);
}

bool TWI::waitForTwint() {
  const uint16_t started_at = timerTicks();

  while (!(TWCR & (1 << TWINT))) {
    if (mymillis(started_at) >= kTimeoutMs) {
      timeout_ = true;
      TWCR = kTwiDisabled;
      return false;
    }
  }

  return true;
}

bool TWI::waitForStopClear() {
  const uint16_t started_at = timerTicks();

  while (TWCR & (1 << TWSTO)) {
    if (mymillis(started_at) >= kTimeoutMs) {
      timeout_ = true;
      TWCR = kTwiDisabled;
      return false;
    }
  }

  return true;
}