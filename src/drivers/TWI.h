#ifndef TWI_H
#define TWI_H

#include <Arduino.h>

#define TWI_STATUS_MASK 0xF8
#define TWI_BIT_RATE_REGISTER_VALUE 72
#define TWI_PRESCALER_BITS 0x00
#define TWI_READ_BIT 1
#define TWI_WRITE_BIT 0
#define TWI_ADDRESS_SHIFT 1
#define TWI_DISABLED 0
#define TWI_INVALID_DATA 0
void mydelay(uint16_t ms);
enum TwiHardwareStatus : uint8_t {
  TWI_START_TRANSMITTED = 0x08,
  TWI_REPEATED_START_TRANSMITTED = 0x10,
  TWI_ADDRESS_WRITE_ACK = 0x18,
  TWI_DATA_WRITE_ACK = 0x28,
  TWI_ADDRESS_READ_ACK = 0x40
};

enum TwiReadError : uint8_t {
  TWI_READ_SUCCESS = 0,
  TWI_READ_START_ERROR = 1,
  TWI_READ_ADDRESS_WRITE_ERROR = 2,
  TWI_READ_REGISTER_ERROR = 3,
  TWI_READ_RESTART_ERROR = 4,
  TWI_READ_ADDRESS_READ_ERROR = 5,
  TWI_READ_INVALID_ARGUMENT = 6
};

enum TwiWriteError : uint8_t {
  TWI_WRITE_SUCCESS = 0,
  TWI_WRITE_START_ERROR = 1,
  TWI_WRITE_ADDRESS_ERROR = 2,
  TWI_WRITE_REGISTER_ERROR = 3,
  TWI_WRITE_DATA_ERROR = 4
};

class TWI {
 public:
  static constexpr uint8_t TIMEOUT = 0xFF;
  static constexpr uint32_t TIMEOUT_MS = 25;

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
  uint8_t readReg(uint8_t addr, uint8_t reg, uint8_t &value);
  uint8_t writeReg(uint8_t addr, uint8_t reg, uint8_t value);
  uint8_t readRegs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

  // Проверять сразу после операции.
  // START, write и чтение байта сбрасывают предыдущий признак.
  // STOP сохраняет его.
  bool timedOut() const { return timeout_; }

 private:
  bool waitForTwint();
  bool waitForStopClear();

  bool timeout_ = false;
};

#endif