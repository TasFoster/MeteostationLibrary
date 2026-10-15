#include <Arduino.h>
#include "TWI.h"
#include "BMP.h"

// Создаём объект для работы с шиной I2C
TWI twi;

// Создаём объект датчика BME280.
// Адрес 0x76 — стандартный вариант для нашей настройки.
BME280 sensor(twi, 0x76);


void setup() {
    Serial.begin(9600);

    Serial.println(F("Initializing TWI..."));

    // Инициализируем аппаратный интерфейс I2C
    twi.init();

    Serial.println(F("Initializing BME280..."));

    // Инициализируем датчик и считываем калибровочные коэффициенты
    uint8_t status = sensor.begin();

    if (status != 0) {
        Serial.print(F("BME280 initialization failed. Error: "));
        Serial.println(status);

        while (true) {
            // Останавливаем программу при ошибке инициализации
        }
    }

    Serial.println(F("BME280 initialized successfully!"));
    Serial.println();
}


void loop() {
    uint32_t rawTemp = 0;
    uint32_t rawPressure = 0;
    uint32_t rawHumidity = 0;

    uint8_t tempStatus;
    uint8_t pressureStatus;
    uint8_t humidityStatus;

    double temperature;
    double pressure;
    double humidity;


    // ----------------------------------------
    // 1. Читаем температуру
    // ----------------------------------------

    tempStatus = sensor.readTemp(rawTemp, 1, 1);

    if (tempStatus == 0) {
        temperature = sensor.calculateTemp(rawTemp);

        Serial.print(F("Temperature: "));
        Serial.print(temperature, 2);
        Serial.println(F(" *C"));
    }
    else {
        Serial.print(F("Temperature read error: "));
        Serial.println(tempStatus);
    }


    // ----------------------------------------
    // 2. Читаем давление
    // ----------------------------------------

    pressureStatus = sensor.readPressure(rawPressure, 1, 1, 1);

    if (pressureStatus == 0) {
        pressure = sensor.calculatePressure(rawPressure);

        if (pressure >= 0) {
            Serial.print(F("Pressure: "));
            Serial.print(pressure / 100.0, 2);
            Serial.println(F(" hPa"));
        }
        else {
            Serial.println(F("Pressure calculation error"));
        }
    }
    else {
        Serial.print(F("Pressure read error: "));
        Serial.println(pressureStatus);
    }


    // ----------------------------------------
    // 3. Читаем влажность
    // ----------------------------------------

    humidityStatus = sensor.readHum(rawHumidity, 1, 1, 1);
   
    if (humidityStatus == 0) {
        humidity = sensor.calculateHum(rawHumidity);

        if (humidity >= 0) {
            Serial.print(F("Humidity: "));
            Serial.print(humidity, 2);
            Serial.println(F(" %"));
        }
        else {
            Serial.println(F("Humidity calculation error"));
        }
    }
    else {
        Serial.print(F("Humidity read error: "));
        Serial.println(humidityStatus);
    }


    Serial.println(F("-----------------------------"));

    // Ждём секунду перед следующим измерением
    delay(1000);
}