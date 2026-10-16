#include <Arduino.h>
#include "headers/TWI.h"

// ============================================================
// Инициализация аппаратного интерфейса TWI
// ============================================================

void TWI::init()
{
    TWBR = 72;
    TWSR = 0x00;
    TWCR = (1 << TWEN);
}

// ============================================================
// Генерация START
// ============================================================

uint8_t TWI::start()
{
    TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);

    while (!(TWCR & (1 << TWINT)));

    return TWSR & 0xF8;
}

// ============================================================
// Генерация STOP
// ============================================================

void TWI::stop()
{
    TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);

    while (TWCR & (1 << TWSTO));
}

// ============================================================
// Передача одного байта
// ============================================================

uint8_t TWI::write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)));

    return TWSR & 0xF8;
}

// ============================================================
// Чтение байта с ACK
// ============================================================

uint8_t TWI::readAck()
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    while (!(TWCR & (1 << TWINT)));

    return TWDR;
}

// ============================================================
// Чтение байта с NACK
// ============================================================

uint8_t TWI::readNack()
{
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)));

    return TWDR;
}

// ============================================================
// Чтение одного регистра — возврат значения или кода ошибки
// ============================================================

uint8_t TWI::readReg(uint8_t addr, uint8_t reg)
{
    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, 1);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, 2);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, 3);

    status = start();
    CHECK_TWI(status, 0x10, [&]() { stop(); }, 4);

    status = write((addr << 1) | 1);
    CHECK_TWI(status, 0x40, [&]() { stop(); }, 5);

    uint8_t value = readNack();

    stop();

    return value;
}

// ============================================================
// Чтение одного регистра — вариант с отдельным статусом
// ============================================================

uint8_t TWI::readReg(
    uint8_t addr,
    uint8_t reg,
    uint8_t& value
)
{
    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, 1);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, 2);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, 3);

    status = start();
    CHECK_TWI(status, 0x10, [&]() { stop(); }, 4);

    status = write((addr << 1) | 1);
    CHECK_TWI(status, 0x40, [&]() { stop(); }, 5);

    value = readNack();

    stop();

    return 0;
}

// ============================================================
// Запись одного регистра
// ============================================================

uint8_t TWI::writeReg(
    uint8_t addr,
    uint8_t reg,
    uint8_t value
)
{
    uint8_t status;

    status = start();
    CHECK_TWI(status, 0x08, [&]() { stop(); }, 1);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, 2);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, 3);

    status = write(value);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, 4);

    stop();

    return 0;
}

// ============================================================
// Чтение нескольких последовательных регистров
// ============================================================

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
    CHECK_TWI(status, 0x08, [&]() { stop(); }, 2);

    status = write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { stop(); }, 3);

    status = write(reg);
    CHECK_TWI(status, 0x28, [&]() { stop(); }, 4);

    status = start();
    CHECK_TWI(status, 0x10, [&]() { stop(); }, 5);

    status = write((addr << 1) | 1);
    CHECK_TWI(status, 0x40, [&]() { stop(); }, 6);

    for (uint8_t i = 0; i < len - 1; i++) {
        buf[i] = readAck();
    }

    buf[len - 1] = readNack();

    stop();

    return 0;
}