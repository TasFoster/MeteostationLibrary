#ifndef TWILCD_OUTPUT_H
#define TWILCD_OUTPUT_H

#include "../drivers/TWI.h"
#include "OutputInterface.h"
#include <Arduino.h>

enum LcdCommand : uint8_t {
  LCD_CLEAR = 0x01,
  LCD_HOME = 0x02,
  LCD_ENTRY_RIGHT = 0x06,
  LCD_DISPLAY_ON = 0x0C,
  LCD_CURSOR_ON = 0x0E,
  LCD_CURSOR_BLINK = 0x0F,
  LCD_SHIFT_LEFT = 0x18,
  LCD_SHIFT_RIGHT = 0x1C,
  LCD_4BIT_2LINE = 0x28,
  LCD_ROW1_START = 0x80,
  LCD_ROW2_START = 0xC0,
  LCD_INIT_8BIT = 0x03,
  LCD_INIT_4BIT = 0x02
};

enum LcdCursorMode : uint8_t { LCD_CURSOR_STEADY = 0, LCD_CURSOR_BLINKING = 1 };

enum LcdTransferError : uint8_t {
  LCD_SUCCESS = 0,
  LCD_START_ERROR = 1,
  LCD_ADDRESS_ERROR = 2,
  LCD_HIGH_DATA_ERROR = 3,
  LCD_HIGH_ENABLE_ERROR = 4,
  LCD_HIGH_LATCH_ERROR = 5,
  LCD_LOW_DATA_ERROR = 6,
  LCD_LOW_ENABLE_ERROR = 7,
  LCD_LOW_LATCH_ERROR = 8
};

enum LcdCursorError : uint8_t { LCD_CURSOR_INVALID_POSITION = 1 };

enum LcdControlBits : uint8_t {
  LCD_REGISTER_SELECT = 0x01,
  LCD_ENABLE = 0x04,
  LCD_BACKLIGHT = 0x08
};

enum LcdRow : uint8_t { LCD_FIRST_ROW = 0, LCD_SECOND_ROW = 1 };

#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define LCD_HIGH_NIBBLE_MASK 0xF0
#define LCD_LOW_NIBBLE_MASK 0x0F
#define LCD_NIBBLE_SHIFT 4
#define LCD_POWER_ON_DELAY_MS 50
#define LCD_COMMAND_DELAY_MS 2
#define LCD_INIT_NIBBLE_DELAY_MS 3

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