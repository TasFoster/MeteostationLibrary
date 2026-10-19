#include "headers/TWI.h"
#include "headers/BMP.h"
#include "headers/SerialOutput.h"
#include "headers/MeteoService.h"
#include "headers/TWILCD_output.h"

TWI twi;
BME280 sensor(twi, 0x76);
TWILCD_output lcd_output(0x27, twi);

MeteoService meteo(sensor, lcd_output);

void setup()
{
    Serial.begin(9600);
    Serial.println("Program started");

    twi.init();
    Serial.println("TWI initialized");

    if (sensor.begin() != 0)
    {
        Serial.println("BME280 initialization failed!");
        while (true) {}
    }

    Serial.println("BME280 initialized!");
}

void loop()
{
    meteo.display('h');


    delay(2000);
}