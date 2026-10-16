#include "headers/TWILCD_output.h"

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
    if (status != 0) return status;

    status = send_command(LCD_DISPLAY_ON);
    if (status != 0) return status;

    status = send_command(LCD_CLEAR);
    if (status != 0) return status;

    status = send_command(LCD_ENTRY_RIGHT);
    if (status != 0) return status;

    status = send_command(
        cursorMode == 1 ? LCD_CURSOR_BLINK : LCD_CURSOR_ON
    );
    if (status != 0) return status;
    send_command(LCD_HOME);
    return 0;
}

void TWILCD_output::print(String data)
{
    String ref1 = data;
    String ref2 = "";
    if (data.length() > 16){
        ref1 = data.substring(0, 16);
        ref2 = data.substring(16);
    }
    send_command(LCD_CLEAR);
    send_command(LCD_HOME);
    for (char i : ref1){
        send_symbol(i);
    }
    setCursor(1,0);
    for (char i : ref2){
        send_symbol(i);
    }
}

uint8_t TWILCD_output::clear()
{
    send_command(LCD_CLEAR);
    return 0;
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
    CHECK_TWI(twi, status, 0x08, 1);

    status = twi.write(addr << 1);
    CHECK_TWI(twi, status, 0x18, 2);

    // Старшая половина
    status = twi.write(b1);
    CHECK_TWI(twi, status, 0x28, 3);

    status = twi.write(b1 | (1 << 2));
    CHECK_TWI(twi, status, 0x28, 4);

    status = twi.write(b1);
    CHECK_TWI(twi, status, 0x28, 5);

    // Младшая половина
    status = twi.write(b2);
    CHECK_TWI(twi, status, 0x28, 6);

    status = twi.write(b2 | (1 << 2));
    CHECK_TWI(twi, status, 0x28, 7);

    status = twi.write(b2);
    CHECK_TWI(twi, status, 0x28, 8);

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
    CHECK_TWI(twi, status, 0x08, 1);

    status = twi.write(addr << 1);
    CHECK_TWI(twi, status, 0x18, 2);

    // Старшая половина
    status = twi.write(b1);
    CHECK_TWI(twi, status, 0x28, 3);

    status = twi.write(b1 | (1 << 2));
    CHECK_TWI(twi, status, 0x28, 4);

    status = twi.write(b1);
    CHECK_TWI(twi, status, 0x28, 5);

    // Младшая половина
    status = twi.write(b2);
    CHECK_TWI(twi, status, 0x28, 6);

    status = twi.write(b2 | (1 << 2));
    CHECK_TWI(twi, status, 0x28, 7);

    status = twi.write(b2);
    CHECK_TWI(twi, status, 0x28, 8);

    twi.stop();
    delay(2);
    return 0;
}
void TWILCD_output::send_nibble(uint8_t nibble)
{
    uint8_t data = ((nibble & 0x0F) << 4) | (1 << 3);

    twi.start();
    twi.write(addr << 1);

    twi.write(data);
    twi.write(data | (1 << 2)); // E = 1
    twi.write(data);            // E = 0

    twi.stop();
    delay(3);
}