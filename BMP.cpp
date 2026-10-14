#include <Arduino.h>
#include "BMP.h"

bme_calib data{BME280_ADDR};

uint8_t init_BMP(){
    reset();
    readCalib(data);
    return 0;
}

char checkMode(){
    uint8_t mode = twi_read_reg(BME280_ADDR, 0xF4) & 0x03 ;
    if (mode == 0) return 's';
    if (mode == 1 || mode == 2) return 'f';
    if (mode == 3) return 'n';
    return '?';
}

uint8_t readTemp(uint32_t& res,uint8_t osrs, uint8_t mode){
    if (mode == 0 || mode > 3) return 1;
    setConfig(0b101, 2, 0);
    uint8_t start = (mode) | ((0 & 0x07)) << 2 | ((osrs & 0x07) << 5);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF2, 0), 0, 1);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF4, start), 0, 2);

    if (mode != 3) while(checkStatus('m') != -1);

    uint8_t b[3];
    TWI_CHECK(twi_read_regs(BME280_ADDR, 0xFA, b, 3), 0, 3);

    res = ((uint32_t)b[0] << 12) | ((uint32_t)b[1] << 4) | ((uint32_t)b[2] >> 4);

    return 0;
}

uint8_t readHum(uint32_t& res,uint8_t osrs, uint8_t mode)
{
    if (mode == 0 || mode > 3) return 1;
    setConfig(0b101, 2, 0);
    uint8_t start = (mode) | ((0 & 0x07)) << 2 | ((0 & 0x07) << 5);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF2, osrs), 0, 1);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF4, start), 0, 2);

    if (mode != 3) while(checkStatus('m') != -1);

    uint8_t b[2];
    TWI_CHECK(twi_read_regs(BME280_ADDR, 0xFD, b, 2), 0, 3);

    res = ((uint32_t)b[0] << 8) | ((uint32_t)b[1]);

    return 0;
}

uint8_t readPressure(uint32_t& res,uint8_t osrs, uint8_t mode)
{
    if (mode == 0 || mode > 3) return 1;
    setConfig(0b101, 2, 0);
    uint8_t start = (mode) | ((osrs & 0x07)) << 2 | ((0 & 0x07) << 5);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF2, 0), 0, 1);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF4, start), 0, 2);

    if (mode != 3) while(checkStatus('m') != -1);

    uint8_t b[3];
    TWI_CHECK(twi_read_regs(BME280_ADDR, 0xF7, b, 3), 0, 3);

    res = ((uint32_t)b[0] << 12) | ((uint32_t)b[1] << 4) | ((uint32_t)b[2] >> 4);

    return 0;
}


uint8_t setConfig(uint8_t t_sb, uint8_t filter, uint8_t spi3w_en){
    uint8_t conf = (spi3w_en & 1) | ((filter & 0x07)) << 2 | ((t_sb & 0x07) << 5);
    TWI_CHECK(twi_write_reg(BME280_ADDR, 0xF5, conf), 0, 1);
    return 0;
}

int8_t checkStatus(char mode)
{
    uint8_t s = twi_read_reg(BME280_ADDR, 0xF3);

    if (mode == 'u' && (s & (1 << 0)) != 0)
    {
        return 0;
    }
    else if (mode == 'm' && (s & (1 << 3)) != 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
uint8_t reset()
{
    Serial.println("Reset: start");

    uint8_t result = twi_write_reg(BME280_ADDR, 0xE0, 0xB6);

    Serial.print("Reset: write result = ");
    Serial.println(result);

    if (result != 0)
    {
        Serial.println("Reset: I2C write failed");
        return result;
    }

    Serial.println("Reset: command sent");

    uint32_t startTime = millis();

    while (checkStatus('u') != -1)
    {
        if (millis() - startTime > 1000)
        {
            Serial.println("Reset: timeout");
            return 2;
        }
    }

    Serial.println("Reset: complete");
    return 0;
}
uint8_t readCalib(bme_calib& data){
    uint8_t block1[26];
    uint8_t block2[7];
    TWI_CHECK(twi_read_regs(data.addr, 0x88, block1, 26), 0, 1);
    TWI_CHECK(twi_read_regs(data.addr, 0xE1, block2, 7), 0, 2);

    data.dig_T1 = (uint16_t)block1[0] | (uint16_t)block1[1] << 8;
    data.dig_T2 = (int16_t)((uint16_t)block1[2] | ((uint16_t)block1[3] << 8));
    data.dig_T3 = (int16_t)((uint16_t)block1[4] | ((uint16_t)block1[5] << 8));

    data.dig_P1 = (uint16_t)block1[6]  | ((uint16_t)block1[7]  << 8);
    data.dig_P2 = (int16_t)((uint16_t)block1[8]  | ((uint16_t)block1[9]  << 8));
    data.dig_P3 = (int16_t)((uint16_t)block1[10] | ((uint16_t)block1[11] << 8));
    data.dig_P4 = (int16_t)((uint16_t)block1[12] | ((uint16_t)block1[13] << 8));
    data.dig_P5 = (int16_t)((uint16_t)block1[14] | ((uint16_t)block1[15] << 8));
    data.dig_P6 = (int16_t)((uint16_t)block1[16] | ((uint16_t)block1[17] << 8));
    data.dig_P7 = (int16_t)((uint16_t)block1[18] | ((uint16_t)block1[19] << 8));
    data.dig_P8 = (int16_t)((uint16_t)block1[20] | ((uint16_t)block1[21] << 8));
    data.dig_P9 = (int16_t)((uint16_t)block1[22] | ((uint16_t)block1[23] << 8));

    data.dig_H1 = (uint8_t)block1[25];
    data.dig_H2 = (int16_t)((uint16_t)block2[0] | ((uint16_t)block2[1] << 8));
    data.dig_H3 = (uint8_t)block2[2];
    data.dig_H4 = ((int16_t)(int8_t)block2[3] << 4) | (block2[4] & 0x0F);
    data.dig_H5 = ((int16_t)(int8_t)block2[5] << 4) | (block2[4] >> 4); 
    data.dig_H6 = (int8_t)block2[6]; 
    return 0;      

}

uint8_t calculateTemp(uint8_t& res);
uint8_t calculateHum(uint8_t& res);
uint8_t calculatePressure(uint8_t& res);
