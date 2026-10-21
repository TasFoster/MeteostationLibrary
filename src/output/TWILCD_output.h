#ifndef TWILCD_OUTPUT_H
#define TWILCD_OUTPUT_H

#include "../drivers/TWI.h"
#include "OutputInterface.h"
#include <Arduino.h>

enum LcdCommand : uint8_t {
  kLcdClear = 0x01,
  kLcdHome = 0x02,
  kLcdEntryRight = 0x06,
  kLcdDisplayOn = 0x0C,
  kLcdCursorOn = 0x0E,
  kLcdCursorBlink = 0x0F,
  kLcdShiftLeft = 0x18,
  kLcdShiftRight = 0x1C,
  kLcd4Bit2Line = 0x28,
  kLcdRow1Start = 0x80,
  kLcdRow2Start = 0xC0,
  kLcdInit8Bit = 0x03,
  kLcdInit4Bit = 0x02
};

enum LcdCursorMode : uint8_t { kLcdCursorSteady = 0, kLcdCursorBlinking = 1 };

enum LcdTransferError : uint8_t {
  kLcdSuccess = 0,
  kLcdStartError = 1,
  kLcdAddressError = 2,
  kLcdHighDataError = 3,
  kLcdHighEnableError = 4,
  kLcdHighLatchError = 5,
  kLcdLowDataError = 6,
  kLcdLowEnableError = 7,
  kLcdLowLatchError = 8
};

enum LcdCursorError : uint8_t { kLcdCursorInvalidPosition = 1 };

enum LcdControlBits : uint8_t {
  kLcdRegisterSelect = 0x01,
  kLcdEnable = 0x04,
  kLcdBacklight = 0x08
};

enum LcdRow : uint8_t { kLcdFirstRow = 0, kLcdSecondRow = 1 };

constexpr uint8_t kLcdColumns = 16;
constexpr uint8_t kLcdRows = 2;
constexpr uint8_t kLcdHighNibbleMask = 0xF0;
constexpr uint8_t kLcdLowNibbleMask = 0x0F;
constexpr uint8_t kLcdNibbleShift = 4;
constexpr uint16_t kLcdPowerOnDelayMs = 50;
constexpr uint16_t kLcdCommandDelayMs = 2;
constexpr uint16_t kLcdInitNibbleDelayMs = 3;

class TWILCD_output : public OutputInterface {
 public:
  TWILCD_output(uint8_t addr, TWI &twi);
  ~TWILCD_output();
  uint8_t init(uint8_t cursorMode);
  void print(String data) override;
  uint8_t clear();
  uint8_t setCursor(uint8_t row, uint8_t column);

 private:
  uint8_t send_symbol(char s);
  uint8_t send_command(uint8_t c);
  void send_nibble(uint8_t nibble);

  uint8_t addr_;
  TWI &twi_;
};

#endif