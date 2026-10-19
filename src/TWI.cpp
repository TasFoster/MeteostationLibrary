#include <Arduino.h>
#include "TWI.h"

void TWI::init()
{
    TWBR = 72;
    TWSR = 0x00;
    TWCR = (1 << TWEN);
    timeout_ = false;
}

uint8_t TWI::start()
{
    timeout_ = false;

    TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);

    if (!waitForTwint()) {
        return TIMEOUT;
    }

    return TWSR & 0xF8;
}

void TWI::stop()
{
    if (timeout_) {
        return;
    }

    TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);

    waitForStopClear();
}

uint8_t TWI::write(uint8_t data)
{
    timeout_ = false;

    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!waitForTwint()) {
        return TIMEOUT;
    }

    return TWSR & 0xF8;
}

uint8_t TWI::readAck()
{
    timeout_ = false;

    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    if (!waitForTwint()) {
        return 0;
    }

    return TWDR;
}

uint8_t TWI::readNack()
{
    timeout_ = false;

    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!waitForTwint()) {
        return 0;
    }

    return TWDR;
}

uint8_t TWI::readReg(uint8_t addr, uint8_t reg)
{
    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, timeout_ ? TIMEOUT : 1);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, timeout_ ? TIMEOUT : 2);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, timeout_ ? TIMEOUT : 3);

    status = start();
    CHECK_TWI(status, 0x10, [&]() { stop(); }, timeout_ ? TIMEOUT : 4);

    status = write((addr << 1) | 1);
    CHECK_TWI(status, 0x40, [&]() { stop(); }, timeout_ ? TIMEOUT : 5);

    uint8_t value = readNack();
    if (timeout_) {
        return TIMEOUT;
    }

    stop();
    if (timeout_) {
        return TIMEOUT;
    }

    return value;
}

uint8_t TWI::readReg(
    uint8_t addr,
    uint8_t reg,
    uint8_t& value
)
{
    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, timeout_ ? TIMEOUT : 1);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, timeout_ ? TIMEOUT : 2);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, timeout_ ? TIMEOUT : 3);

    status = start();
    CHECK_TWI(status, 0x10, [&]() { stop(); }, timeout_ ? TIMEOUT : 4);

    status = write((addr << 1) | 1);
    CHECK_TWI(status, 0x40, [&]() { stop(); }, timeout_ ? TIMEOUT : 5);

    value = readNack();
    if (timeout_) {
        return TIMEOUT;
    }

    stop();

    return timeout_ ? TIMEOUT : 0;
}

uint8_t TWI::writeReg(
    uint8_t addr,
    uint8_t reg,
    uint8_t value
)
{
    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, timeout_ ? TIMEOUT : 1);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, timeout_ ? TIMEOUT : 2);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, timeout_ ? TIMEOUT : 3);

    status = write(value);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, timeout_ ? TIMEOUT : 4);

    stop();

    return timeout_ ? TIMEOUT : 0;
}

uint8_t TWI::readRegs(
    uint8_t addr,
    uint8_t reg,
    uint8_t* buf,
    uint8_t len
)
{
    if (len == 0 || buf == nullptr) {
        return 1;
    }

    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, timeout_ ? TIMEOUT : 2);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, timeout_ ? TIMEOUT : 3);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, timeout_ ? TIMEOUT : 4);

    status = start();
    CHECK_TWI(status, 0x10, [&]() { stop(); }, timeout_ ? TIMEOUT : 5);

    status = write((addr << 1) | 1);
    CHECK_TWI(status, 0x40, [&]() { stop(); }, timeout_ ? TIMEOUT : 6);

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

    return timeout_ ? TIMEOUT : 0;
}

bool TWI::waitForTwint()
{
    const unsigned long startedAt = millis();

    while (!(TWCR & (1 << TWINT))) {
        if (millis() - startedAt >= TIMEOUT_MS) {
            timeout_ = true;
            TWCR = 0;
            return false;
        }
    }

    return true;
}

bool TWI::waitForStopClear()
{
    const unsigned long startedAt = millis();

    while (TWCR & (1 << TWSTO)) {
        if (millis() - startedAt >= TIMEOUT_MS) {
            timeout_ = true;
            TWCR = 0;
            return false;
        }
    }

    return true;
}