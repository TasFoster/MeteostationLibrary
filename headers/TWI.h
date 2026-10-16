#ifndef TWI_H
#define TWI_H

#define CHECK_TWI(actual, expected, onError, err_id) \
    do {                                             \
        if ((actual) != (expected)) {                \
            onError();                               \
            return err_id;                           \
        }                                            \
    } while (0)

#include <Arduino.h>

class TWI {
public:
    // Инициализация интерфейса TWI
    void init();

    // Управление передачей
    uint8_t start();
    void stop();

    // Передача и чтение байтов
    uint8_t write(uint8_t data);
    uint8_t readAck();
    uint8_t readNack();

    // Работа с регистрами устройства
    uint8_t readReg(uint8_t addr, uint8_t reg);
    uint8_t readReg(uint8_t addr, uint8_t reg, uint8_t& value);
    uint8_t writeReg(uint8_t addr, uint8_t reg, uint8_t value);
    uint8_t readRegs(uint8_t addr, uint8_t reg,
                     uint8_t* buf, uint8_t len);
};

#endif