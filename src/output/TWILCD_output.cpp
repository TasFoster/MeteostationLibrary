#include "TWILCD_output.h"

TWILCD_output::TWILCD_output(uint8_t addr, TWI &twi) : addr(addr), twi(twi) {}

TWILCD_output::~TWILCD_output() {}

uint8_t TWILCD_output::init(uint8_t cursorMode) {
  mydelay(LCD_POWER_ON_DELAY_MS);

  send_nibble(LCD_INIT_8BIT);
  send_nibble(LCD_INIT_8BIT);
  send_nibble(LCD_INIT_8BIT);
  send_nibble(LCD_INIT_4BIT);

  uint8_t status = send_command(LCD_4BIT_2LINE);
  if (status != LCD_SUCCESS)
    return status;

  status = send_command(LCD_DISPLAY_ON);
  if (status != LCD_SUCCESS)
    return status;

  status = send_command(LCD_CLEAR);
  if (status != LCD_SUCCESS)
    return status;

  status = send_command(LCD_ENTRY_RIGHT);
  if (status != LCD_SUCCESS)
    return status;

  status = send_command(cursorMode == LCD_CURSOR_BLINKING ? LCD_CURSOR_BLINK
                                                          : LCD_CURSOR_ON);
  if (status != LCD_SUCCESS)
    return status;

  status = send_command(LCD_HOME);
  if (status != LCD_SUCCESS)
    return status;

  return LCD_SUCCESS;
}

void TWILCD_output::print(String data) {
  String ref1 = data;
  String ref2 = "";

  if (data.length() > LCD_COLUMNS) {
    ref1 = data.substring(0, LCD_COLUMNS);
    ref2 = data.substring(LCD_COLUMNS);
  }

  send_command(LCD_CLEAR);
  send_command(LCD_HOME);

  for (char i : ref1) {
    send_symbol(i);
  }

  setCursor(LCD_SECOND_ROW, 0);

  for (char i : ref2) {
    send_symbol(i);
  }
}

uint8_t TWILCD_output::clear() { return send_command(LCD_CLEAR); }

uint8_t TWILCD_output::setCursor(uint8_t row, uint8_t column) {
  if (row >= LCD_ROWS || column >= LCD_COLUMNS)
    return LCD_CURSOR_INVALID_POSITION;

  cur_row = row;
  cur_column = column;

  uint8_t address;

  if (row == LCD_FIRST_ROW)
    address = LCD_ROW1_START + column;
  else
    address = LCD_ROW2_START + column;

  return send_command(address);
}

uint8_t TWILCD_output::send_symbol(char s) {
  uint8_t status;

  uint8_t b1 = (s & LCD_HIGH_NIBBLE_MASK) | LCD_BACKLIGHT | LCD_REGISTER_SELECT;
  uint8_t b2 = ((s & LCD_LOW_NIBBLE_MASK) << LCD_NIBBLE_SHIFT) | LCD_BACKLIGHT |
               LCD_REGISTER_SELECT;

  status = twi.start();
  if (status != TWI_START_TRANSMITTED) {
    twi.stop();
    return LCD_START_ERROR;
  }

  status = twi.write(addr << TWI_ADDRESS_SHIFT);
  if (status != TWI_ADDRESS_WRITE_ACK) {
    twi.stop();
    return LCD_ADDRESS_ERROR;
  }

  // Старшая половина
  status = twi.write(b1);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_HIGH_DATA_ERROR;
  }

  status = twi.write(b1 | LCD_ENABLE);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_HIGH_ENABLE_ERROR;
  }

  status = twi.write(b1);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_HIGH_LATCH_ERROR;
  }

  // Младшая половина
  status = twi.write(b2);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_LOW_DATA_ERROR;
  }

  status = twi.write(b2 | LCD_ENABLE);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_LOW_ENABLE_ERROR;
  }

  status = twi.write(b2);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_LOW_LATCH_ERROR;
  }

  twi.stop();
  mydelay(LCD_COMMAND_DELAY_MS);

  return LCD_SUCCESS;
}

uint8_t TWILCD_output::send_command(uint8_t c) {
  uint8_t status;

  uint8_t b1 = (c & LCD_HIGH_NIBBLE_MASK) | LCD_BACKLIGHT;
  uint8_t b2 = ((c & LCD_LOW_NIBBLE_MASK) << LCD_NIBBLE_SHIFT) | LCD_BACKLIGHT;

  status = twi.start();
  if (status != TWI_START_TRANSMITTED) {
    twi.stop();
    return LCD_START_ERROR;
  }

  status = twi.write(addr << TWI_ADDRESS_SHIFT);
  if (status != TWI_ADDRESS_WRITE_ACK) {
    twi.stop();
    return LCD_ADDRESS_ERROR;
  }

  // Старшая половина
  status = twi.write(b1);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_HIGH_DATA_ERROR;
  }

  status = twi.write(b1 | LCD_ENABLE);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_HIGH_ENABLE_ERROR;
  }

  status = twi.write(b1);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_HIGH_LATCH_ERROR;
  }

  // Младшая половина
  status = twi.write(b2);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_LOW_DATA_ERROR;
  }

  status = twi.write(b2 | LCD_ENABLE);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_LOW_ENABLE_ERROR;
  }

  status = twi.write(b2);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return LCD_LOW_LATCH_ERROR;
  }

  twi.stop();
  mydelay(LCD_COMMAND_DELAY_MS);

  return LCD_SUCCESS;
}

void TWILCD_output::send_nibble(uint8_t nibble) {
  uint8_t data =
      ((nibble & LCD_LOW_NIBBLE_MASK) << LCD_NIBBLE_SHIFT) | LCD_BACKLIGHT;
  uint8_t status;

  status = twi.start();
  if (status != TWI_START_TRANSMITTED) {
    twi.stop();
    return;
  }

  status = twi.write(addr << TWI_ADDRESS_SHIFT);
  if (status != TWI_ADDRESS_WRITE_ACK) {
    twi.stop();
    return;
  }

  status = twi.write(data);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return;
  }

  status = twi.write(data | LCD_ENABLE);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return;
  }

  status = twi.write(data);
  if (status != TWI_DATA_WRITE_ACK) {
    twi.stop();
    return;
  }

  twi.stop();
  mydelay(LCD_INIT_NIBBLE_DELAY_MS);
}