#ifndef TWILCD_OUTPUT_H
#define TWILCD_OUTPUT_H

#define LCD_CLEAR          0x01
#define LCD_HOME           0x02
#define LCD_ENTRY_RIGHT    0x06
#define LCD_DISPLAY_ON     0x0C
#define LCD_CURSOR_ON      0x0E
#define LCD_CURSOR_BLINK   0x0F
#define LCD_SHIFT_LEFT     0x18
#define LCD_SHIFT_RIGHT    0x1C
#define LCD_4BIT_2LINE      0x28
#define LCD_ROW1_START     0x80
#define LCD_ROW2_START     0xC0

#include "OutputInterface.h"
#include <Arduino.h>
#include "../drivers/TWI.h"

class TWILCD_output : public OutputInterface
{
private:
    uint8_t addr;
    TWI& twi;

    uint8_t zero = 0 | (1 << 3) ;
    uint8_t one = (1 << 2) | (1 << 3);

    uint8_t cur_row = 0;
    uint8_t cur_column = 0;
    
    uint8_t send_symbol(char s);
    uint8_t send_command(uint8_t c);
    void send_nibble(uint8_t nibble);

public:
    TWILCD_output(uint8_t addr, TWI& twi);
    ~TWILCD_output();
    uint8_t init(uint8_t cursorMode);
    void print(String data) override;
    uint8_t clear();
    uint8_t setCursor(uint8_t row, uint8_t column);
};

#endif