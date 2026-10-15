#include "headers/TWI.h"
#include "headers/BMP.h"
#include "headers/SerialOutput.h"
#include "headers/MeteoService.h"

TWI twi;
BME280 sensor(twi, 0x76);
SerialOutput serialOutput(9600);

MeteoService meteo(sensor, serialOutput);

void setup()
{
    Serial.begin(9600);
    Serial.println("Program started");

    twi.init();
    twi.write(0x27)
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
    meteo.display('t');
    meteo.display('p');
    meteo.display('h');

    delay(2000);
}