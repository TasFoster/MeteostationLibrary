#include "BMP.h"

void setup() {
    Serial.begin(9600);

    init_TWI();

    uint8_t status = init_BMP();

    if (status != 0) {
        Serial.println("BME280 initialization error!");
        return;
    }

    Serial.println("BME280 initialized!");
}

void loop()
{
    uint32_t rawHumidity = 0;

    uint8_t status = readHum(rawHumidity, 1, 1);

    if (status != 0)
    {
        Serial.println("Humidity reading error!");
        delay(1000);
        return;
    }

    Serial.print("Raw humidity: ");
    Serial.println(rawHumidity);

    delay(1000);
}