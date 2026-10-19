#include "TWILCD_output.h"

TWILCD_output::TWILCD_output(uint8_t addr, TWI& twi)
    : addr(addr), twi(twi)
{
}

TWILCD_output::~TWILCD_output()
{
}

uint8_t TWILCD_output::init(uint8_t cursorMode)
{
    delay(50);

    send_nibble(0x03);
    send_nibble(0x03);
    send_nibble(0x03);
    send_nibble(0x02);

    uint8_t status = send_command(LCD_4BIT_2LINE);
    CHECK_TWI(status, 0, []() {}, status);

    status = send_command(LCD_DISPLAY_ON);
    CHECK_TWI(status, 0, []() {}, status);

    status = send_command(LCD_CLEAR);
    CHECK_TWI(status, 0, []() {}, status);

    status = send_command(LCD_ENTRY_RIGHT);
    CHECK_TWI(status, 0, []() {}, status);

    status = send_command(
        cursorMode == 1 ? LCD_CURSOR_BLINK : LCD_CURSOR_ON
    );
    CHECK_TWI(status, 0, []() {}, status);

    status = send_command(LCD_HOME);
    CHECK_TWI(status, 0, []() {}, status);

    return 0;
}

void TWILCD_output::print(String data)
{
    String ref1 = data;
    String ref2 = "";

    if (data.length() > 16)
    {
        ref1 = data.substring(0, 16);
        ref2 = data.substring(16);
    }

    send_command(LCD_CLEAR);
    send_command(LCD_HOME);

    for (char i : ref1)
    {
        send_symbol(i);
    }

    setCursor(1, 0);

    for (char i : ref2)
    {
        send_symbol(i);
    }
}

uint8_t TWILCD_output::clear()
{
    return send_command(LCD_CLEAR);
}

uint8_t TWILCD_output::setCursor(uint8_t row, uint8_t column)
{
    if (row > 1 || column > 15)
        return 1;

    cur_row = row;
    cur_column = column;

    uint8_t address;

    if (row == 0)
        address = LCD_ROW1_START + column;
    else
        address = LCD_ROW2_START + column;

    return send_command(address);
}

uint8_t TWILCD_output::send_symbol(char s)
{
    uint8_t status;

    uint8_t b1 = (s & 0xF0) | (1 << 3) | 1;
    uint8_t b2 = ((s & 0x0F) << 4) | (1 << 3) | 1;

    status = twi.start();
    CHECK_TWI(status, 0x08, [&]() { twi.stop(); }, 1);

    status = twi.write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { twi.stop(); }, 2);

    // Старшая половина
    status = twi.write(b1);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 3);

    status = twi.write(b1 | (1 << 2));
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 4);

    status = twi.write(b1);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 5);

    // Младшая половина
    status = twi.write(b2);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 6);

    status = twi.write(b2 | (1 << 2));
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 7);

    status = twi.write(b2);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 8);

    twi.stop();
    delay(2);

    return 0;
}

uint8_t TWILCD_output::send_command(uint8_t c)
{
    uint8_t status;

    uint8_t b1 = (c & 0xF0) | (1 << 3);
    uint8_t b2 = ((c & 0x0F) << 4) | (1 << 3);

    status = twi.start();
    CHECK_TWI(status, 0x08, [&]() { twi.stop(); }, 1);

    status = twi.write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { twi.stop(); }, 2);

    // Старшая половина
    status = twi.write(b1);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 3);

    status = twi.write(b1 | (1 << 2));
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 4);

    status = twi.write(b1);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 5);

    // Младшая половина
    status = twi.write(b2);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 6);

    status = twi.write(b2 | (1 << 2));
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 7);

    status = twi.write(b2);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, 8);

    twi.stop();
    delay(2);

    return 0;
}

void TWILCD_output::send_nibble(uint8_t nibble)
{
    uint8_t data = ((nibble & 0x0F) << 4) | (1 << 3);
    uint8_t status;

    status = twi.start();
    CHECK_TWI(status, 0x08, [&]() { twi.stop(); }, );

    status = twi.write(addr << 1);
    CHECK_TWI(status, 0x18, [&]() { twi.stop(); }, );

    status = twi.write(data);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, );

    status = twi.write(data | (1 << 2));
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, );

    status = twi.write(data);
    CHECK_TWI(status, 0x28, [&]() { twi.stop(); }, );

    twi.stop();
    delay(3);
}