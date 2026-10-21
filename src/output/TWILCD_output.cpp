#include "TWILCD_output.h"

TWILCD_output::TWILCD_output(uint8_t addr, TWI &twi) : addr_(addr), twi_(twi) {}

TWILCD_output::~TWILCD_output() {}

uint8_t TWILCD_output::init(uint8_t cursorMode) {
  mydelay(kLcdPowerOnDelayMs);

  send_nibble(kLcdInit8Bit);
  send_nibble(kLcdInit8Bit);
  send_nibble(kLcdInit8Bit);
  send_nibble(kLcdInit4Bit);

  uint8_t status = send_command(kLcd4Bit2Line);
  if (status != kLcdSuccess)
    return status;

  status = send_command(kLcdDisplayOn);
  if (status != kLcdSuccess)
    return status;

  status = send_command(kLcdClear);
  if (status != kLcdSuccess)
    return status;

  status = send_command(kLcdEntryRight);
  if (status != kLcdSuccess)
    return status;

  status = send_command(cursorMode == kLcdCursorBlinking ? kLcdCursorBlink
                                                          : kLcdCursorOn);
  if (status != kLcdSuccess)
    return status;

  status = send_command(kLcdHome);
  if (status != kLcdSuccess)
    return status;

  return kLcdSuccess;
}

void TWILCD_output::print(String data) {
  String ref1 = data;
  String ref2 = "";

  if (data.length() > kLcdColumns) {
    ref1 = data.substring(0, kLcdColumns);
    ref2 = data.substring(kLcdColumns);
  }

  send_command(kLcdClear);
  send_command(kLcdHome);

  for (char i : ref1) {
    send_symbol(i);
  }

  setCursor(kLcdSecondRow, 0);

  for (char i : ref2) {
    send_symbol(i);
  }
}

uint8_t TWILCD_output::clear() { return send_command(kLcdClear); }

uint8_t TWILCD_output::setCursor(uint8_t row, uint8_t column) {
  if (row >= kLcdRows || column >= kLcdColumns)
    return kLcdCursorInvalidPosition;

  uint8_t address;

  if (row == kLcdFirstRow)
    address = kLcdRow1Start + column;
  else
    address = kLcdRow2Start + column;

  return send_command(address);
}

uint8_t TWILCD_output::send_symbol(char s) {
  uint8_t status;

  uint8_t b1 = (s & kLcdHighNibbleMask) | kLcdBacklight | kLcdRegisterSelect;
  uint8_t b2 = ((s & kLcdLowNibbleMask) << kLcdNibbleShift) | kLcdBacklight |
               kLcdRegisterSelect;

  status = twi_.start();
  if (status != kTwiStartTransmitted) {
    twi_.stop();
    return kLcdStartError;
  }

  status = twi_.write(addr_ << kTwiAddressShift);
  if (status != kTwiAddressWriteAck) {
    twi_.stop();
    return kLcdAddressError;
  }

  // Старшая половина
  status = twi_.write(b1);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdHighDataError;
  }

  status = twi_.write(b1 | kLcdEnable);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdHighEnableError;
  }

  status = twi_.write(b1);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdHighLatchError;
  }

  // Младшая половина
  status = twi_.write(b2);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdLowDataError;
  }

  status = twi_.write(b2 | kLcdEnable);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdLowEnableError;
  }

  status = twi_.write(b2);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdLowLatchError;
  }

  twi_.stop();
  mydelay(kLcdCommandDelayMs);

  return kLcdSuccess;
}

uint8_t TWILCD_output::send_command(uint8_t c) {
  uint8_t status;

  uint8_t b1 = (c & kLcdHighNibbleMask) | kLcdBacklight;
  uint8_t b2 = ((c & kLcdLowNibbleMask) << kLcdNibbleShift) | kLcdBacklight;

  status = twi_.start();
  if (status != kTwiStartTransmitted) {
    twi_.stop();
    return kLcdStartError;
  }

  status = twi_.write(addr_ << kTwiAddressShift);
  if (status != kTwiAddressWriteAck) {
    twi_.stop();
    return kLcdAddressError;
  }

  // Старшая половина
  status = twi_.write(b1);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdHighDataError;
  }

  status = twi_.write(b1 | kLcdEnable);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdHighEnableError;
  }

  status = twi_.write(b1);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdHighLatchError;
  }

  // Младшая половина
  status = twi_.write(b2);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdLowDataError;
  }

  status = twi_.write(b2 | kLcdEnable);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdLowEnableError;
  }

  status = twi_.write(b2);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return kLcdLowLatchError;
  }

  twi_.stop();
  mydelay(kLcdCommandDelayMs);

  return kLcdSuccess;
}

void TWILCD_output::send_nibble(uint8_t nibble) {
  uint8_t data =
      ((nibble & kLcdLowNibbleMask) << kLcdNibbleShift) | kLcdBacklight;
  uint8_t status;

  status = twi_.start();
  if (status != kTwiStartTransmitted) {
    twi_.stop();
    return;
  }

  status = twi_.write(addr_ << kTwiAddressShift);
  if (status != kTwiAddressWriteAck) {
    twi_.stop();
    return;
  }

  status = twi_.write(data);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return;
  }

  status = twi_.write(data | kLcdEnable);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return;
  }

  status = twi_.write(data);
  if (status != kTwiDataWriteAck) {
    twi_.stop();
    return;
  }

  twi_.stop();
  mydelay(kLcdInitNibbleDelayMs);
}