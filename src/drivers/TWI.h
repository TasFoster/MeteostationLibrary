#ifndef TWI_H
#define TWI_H

#include <Arduino.h>

constexpr uint8_t kTwiStatusMask = 0xF8;
constexpr uint8_t kTwiBitRateRegisterValue = 72;
constexpr uint8_t kTwiPrescalerBits = 0x00;
constexpr uint8_t kTwiReadBit = 1;
constexpr uint8_t kTwiWriteBit = 0;
constexpr uint8_t kTwiAddressShift = 1;
constexpr uint8_t kTwiDisabled = 0;
constexpr uint8_t kTwiInvalidData = 0;

void mydelay(uint16_t ms);


enum TwiHardwareStatus : uint8_t {
  kTwiStartTransmitted = 0x08,
  kTwiRepeatedStartTransmitted = 0x10,
  kTwiAddressWriteAck = 0x18,
  kTwiDataWriteAck = 0x28,
  kTwiAddressReadAck = 0x40
};

enum TwiReadError : uint8_t {
  kTwiReadSuccess = 0,
  kTwiReadStartError = 1,
  kTwiReadAddressWriteError = 2,
  kTwiReadRegisterError = 3,
  kTwiReadRestartError = 4,
  kTwiReadAddressReadError = 5,
  kTwiReadInvalidArgument = 6
};

enum TwiWriteError : uint8_t {
  kTwiWriteSuccess = 0,
  kTwiWriteStartError = 1,
  kTwiWriteAddressError = 2,
  kTwiWriteRegisterError = 3,
  kTwiWriteDataError = 4
};

class TWI {
 public:
  static constexpr uint8_t kTimeout = 0xFF;
  static constexpr uint32_t kTimeoutMs = 25;

  // Текущее значение Timer4, в тиках.
  uint16_t timerTicks() const;

  // Прошедшие миллисекунды; переполнение учтено.
  // started_at — тики timerTicks(); интервал < 4,194304 с.
  // Опрос регулярный. Сначала init(); Timer4 занят таймаутами.
  uint32_t mymillis(uint16_t started_at) const;

  // TWI и Timer4. Прерывания Timer4 выключены.
  void init();

  // Управление передачей
  uint8_t start();
  void stop();

  // Передача и чтение байтов
  uint8_t write(uint8_t data);
  uint8_t readAck();
  uint8_t readNack();

  // Регистры устройства
  uint8_t readReg(uint8_t addr, uint8_t reg, uint8_t &value);
  uint8_t writeReg(uint8_t addr, uint8_t reg, uint8_t value);
  uint8_t readRegs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

  // Проверять сразу после операции.
  // START/write/чтение сбрасывают флаг; STOP сохраняет.
  bool timedOut() const { return timeout_; }

 private:
  bool waitForTwint();
  bool waitForStopClear();

  bool timeout_ = false;
};

#endif