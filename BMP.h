#ifndef BMP_H
#define BMP_H

#include "TWI.h"
#define BME280_ADDR 0x76

typedef struct 
{
    uint8_t  addr;
    uint16_t dig_T1;  int16_t dig_T2, dig_T3;
    uint16_t dig_P1;  int16_t dig_P2, dig_P3, dig_P4, dig_P5,
                                dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;  int16_t dig_H2;  uint8_t dig_H3;
    int16_t  dig_H4, dig_H5;  int8_t dig_H6;
} bme_calib;


uint8_t init_BMP();
char checkMode();
uint8_t readTemp(uint32_t& res,uint8_t osrs, uint8_t mode);
uint8_t readHum(uint32_t& res,uint8_t osrs, uint8_t mode);
uint8_t readPressure(uint32_t& res,uint8_t osrs, uint8_t mode);
uint8_t setConfig(uint8_t t_sb, uint8_t filter, uint8_t spi3w_en);
int8_t   checkStatus(char mode);
uint8_t reset();
uint8_t readCalib(bme_calib& data);
uint8_t calculateTemp(uint8_t& res);
uint8_t calculateHum(uint8_t& res);
uint8_t calculatePressure(uint8_t& res);

#endif