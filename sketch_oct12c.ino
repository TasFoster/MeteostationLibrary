#include <Arduino.h>
#include "headers/TWI.h"

TWI twi;

void setup()
{
    Serial.begin(9600);
    twi.init();

    Serial.println("I2C Scanner started");

    for (uint8_t addr = 0x01; addr <= 0x7E; addr++)
    {
        // START
        uint8_t status = twi.start();

        if (status != 0x08)
        {
            twi.stop();
            continue;
        }

        // Отправляем адрес устройства в режиме записи.
        // Метод write() ожидает адрес уже со сдвигом на 1 бит.
        status = twi.write(addr << 1);

        if (status == 0x18)
        {
            Serial.print("Device found at address: 0x");

            if (addr < 0x10)
                Serial.print("0");

            Serial.println(addr, HEX);
        }

        twi.stop();
    }

    Serial.println("Scan complete");
}

void loop()
{
}