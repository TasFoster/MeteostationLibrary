#ifndef BMP_H
#define BMP_H

#include <Arduino.h>
#include "TWI.h"

struct bme_calib {
    uint8_t addr = 0;

    uint16_t dig_T1 = 0;
    int16_t dig_T2 = 0;
    int16_t dig_T3 = 0;

    uint16_t dig_P1 = 0;
    int16_t dig_P2 = 0;
    int16_t dig_P3 = 0;
    int16_t dig_P4 = 0;
    int16_t dig_P5 = 0;
    int16_t dig_P6 = 0;
    int16_t dig_P7 = 0;
    int16_t dig_P8 = 0;
    int16_t dig_P9 = 0;

    uint8_t dig_H1 = 0;
    int16_t dig_H2 = 0;
    uint8_t dig_H3 = 0;
    int16_t dig_H4 = 0;
    int16_t dig_H5 = 0;
    int8_t dig_H6 = 0;

    int32_t t_fine = 0;
};

class BME280 {
private:
    TWI& bus;
    bme_calib data;

    uint8_t readCalib();
    int8_t checkStatus(char mode);

public:
    explicit BME280(TWI& twi, uint8_t address = 0x76);

    uint8_t begin();
    uint8_t reset();

    char checkMode();

    uint8_t setConfig(
        uint8_t t_sb,
        uint8_t filter,
        uint8_t spi3w_en
    );

    uint8_t readTemp(
        uint32_t& res,
        uint8_t osrs,
        uint8_t mode
    );

    uint8_t readHum(
        uint32_t& res,
        uint8_t osrs_h,
        uint8_t osrs_t,
        uint8_t mode
    );

    uint8_t readPressure(
        uint32_t& res,
        uint8_t osrs_p,
        uint8_t osrs_t,
        uint8_t mode
    );

    double calculateTemp(uint32_t adc_T);
    double calculateHum(uint32_t adc_H);
    double calculatePressure(uint32_t adc_P);
};

#endif