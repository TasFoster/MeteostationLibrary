#include "headers/BMP.h"

BME280::BME280(TWI& twi, uint8_t address)
    : bus(twi)
{
    data = {};
    data.addr = address;
}


// Инициализация датчика
uint8_t BME280::begin()
{
    uint8_t status = reset();

    CHECK_TWI(status, 0, []() {}, status);

    status = readCalib();

    if (status != 0) {
        return status + 10;
    }

    return 0;
}


// Сброс датчика
uint8_t BME280::reset()
{
    uint8_t status = bus.writeReg(data.addr, 0xE0, 0xB6);

    CHECK_TWI(status, 0, []() {}, status);

    unsigned long startTime = millis();

    while (true) {
        int8_t result = checkStatus('u');

        if (result == -1) {
            return 0;
        }

        if (result == -2) {
            return 7;
        }

        if (millis() - startTime > 1000) {
            return 6;
        }
    }
}


// Проверка состояния датчика
// 'u' — обновление калибровочных данных
// 'm' — выполнение измерения
int8_t BME280::checkStatus(char mode)
{
    uint8_t status = 0;

    if (bus.readReg(data.addr, 0xF3, status) != 0) {
        return -2;
    }

    if (mode == 'u') {
        return (status & 0x01) ? 0 : -1;
    }

    if (mode == 'm') {
        return (status & 0x08) ? 0 : -1;
    }

    return -2;
}


// Проверка режима работы
char BME280::checkMode()
{
    uint8_t ctrl_meas = 0;

    if (bus.readReg(data.addr, 0xF4, ctrl_meas) != 0) {
        return '?';
    }

    uint8_t mode = ctrl_meas & 0x03;

    if (mode == 0) {
        return 's'; // Sleep
    }

    if (mode == 1 || mode == 2) {
        return 'f'; // Forced
    }

    return 'n'; // Normal
}


// Настройка регистра CONFIG
uint8_t BME280::setConfig(
    uint8_t t_sb,
    uint8_t filter,
    uint8_t spi3w_en
)
{
    if (t_sb > 7 || filter > 7 || spi3w_en > 1) {
        return 1;
    }

    uint8_t config =
        (t_sb << 5) |
        (filter << 2) |
        spi3w_en;

    return bus.writeReg(data.addr, 0xF5, config);
}


// Чтение температуры
uint8_t BME280::readTemp(
    uint32_t& res,
    uint8_t osrs,
    uint8_t mode
)
{
    if (osrs > 5 || mode < 1 || mode > 3) {
        return 1;
    }

    uint8_t status = setConfig(5, 2, 0);

    CHECK_TWI(status, 0, []() {}, status);

    // Отключаем измерение влажности
    status = bus.writeReg(data.addr, 0xF2, 0);

    CHECK_TWI(status, 0, []() {}, status);

    // CTRL_MEAS: osrs_t [7:5], osrs_p [4:2], mode [1:0]
    uint8_t ctrl_meas = (osrs << 5) | mode;

    status = bus.writeReg(data.addr, 0xF4, ctrl_meas);

    CHECK_TWI(status, 0, []() {}, status);

    // В forced mode ожидаем завершения измерения
    if (mode != 3) {
        unsigned long startTime = millis();

        while (checkStatus('m') == 0) {
            if (millis() - startTime > 1000) {
                return 6;
            }
        }

        if (checkStatus('m') == -2) {
            return 7;
        }
    }

    uint8_t buf[3];

    status = bus.readRegs(data.addr, 0xFA, buf, 3);

    CHECK_TWI(status, 0, []() {}, status);

    res =
        ((uint32_t)buf[0] << 12) |
        ((uint32_t)buf[1] << 4) |
        ((uint32_t)buf[2] >> 4);

    return 0;
}


// Чтение влажности
uint8_t BME280::readHum(
    uint32_t& res,
    uint8_t osrs_h,
    uint8_t osrs_t,
    uint8_t mode
)
{
    if (osrs_h > 5 || osrs_t > 5 || mode < 1 || mode > 3) {
        return 1;
    }

    uint8_t status = setConfig(5, 2, 0);

    CHECK_TWI(status, 0, []() {}, status);

    // CTRL_HUM необходимо записывать перед CTRL_MEAS
    status = bus.writeReg(data.addr, 0xF2, osrs_h);

    CHECK_TWI(status, 0, []() {}, status);

    uint8_t ctrl_meas = (osrs_t << 5) | mode;

    status = bus.writeReg(data.addr, 0xF4, ctrl_meas);
    delay(10);
    CHECK_TWI(status, 0, []() {}, status);

    if (mode != 3) {
        unsigned long startTime = millis();

        while (checkStatus('m') == 0) {
            if (millis() - startTime > 1000) {
                return 6;
            }
        }

        if (checkStatus('m') == -2) {
            return 7;
        }
    }

    uint8_t buf[2];

    status = bus.readRegs(data.addr, 0xFD, buf, 2);

    CHECK_TWI(status, 0, []() {}, status);

    res =
        ((uint32_t)buf[0] << 8) |
        buf[1];

    return 0;
}


// Чтение давления
uint8_t BME280::readPressure(
    uint32_t& res,
    uint8_t osrs_p,
    uint8_t osrs_t,
    uint8_t mode
)
{
    if (osrs_p > 5 || osrs_t > 5 || mode < 1 || mode > 3) {
        return 1;
    }

    uint8_t status = setConfig(5, 2, 0);

    CHECK_TWI(status, 0, []() {}, status);

    // Отключаем измерение влажности
    status = bus.writeReg(data.addr, 0xF2, 0);

    CHECK_TWI(status, 0, []() {}, status);

    uint8_t ctrl_meas =
        (osrs_t << 5) |
        (osrs_p << 2) |
        mode;

    status = bus.writeReg(data.addr, 0xF4, ctrl_meas);

    CHECK_TWI(status, 0, []() {}, status);

    if (mode != 3) {
        unsigned long startTime = millis();

        while (checkStatus('m') == 0) {
            if (millis() - startTime > 1000) {
                return 6;
            }
        }

        if (checkStatus('m') == -2) {
            return 7;
        }
    }

    uint8_t buf[3];

    status = bus.readRegs(data.addr, 0xF7, buf, 3);

    CHECK_TWI(status, 0, []() {}, status);

    res =
        ((uint32_t)buf[0] << 12) |
        ((uint32_t)buf[1] << 4) |
        ((uint32_t)buf[2] >> 4);

    return 0;
}


// Чтение калибровочных коэффициентов
uint8_t BME280::readCalib()
{
    uint8_t buf1[26];
    uint8_t buf2[7];

    uint8_t status = bus.readRegs(data.addr, 0x88, buf1, 26);

    CHECK_TWI(status, 0, []() {}, status);

    status = bus.readRegs(data.addr, 0xE1, buf2, 7);

    CHECK_TWI(status, 0, []() {}, status);

    // Температура
    data.dig_T1 = (uint16_t)buf1[1] << 8 | buf1[0];
    data.dig_T2 = (int16_t)((uint16_t)buf1[3] << 8 | buf1[2]);
    data.dig_T3 = (int16_t)((uint16_t)buf1[5] << 8 | buf1[4]);

    // Давление
    data.dig_P1 = (uint16_t)buf1[7] << 8 | buf1[6];
    data.dig_P2 = (int16_t)((uint16_t)buf1[9] << 8 | buf1[8]);
    data.dig_P3 = (int16_t)((uint16_t)buf1[11] << 8 | buf1[10]);
    data.dig_P4 = (int16_t)((uint16_t)buf1[13] << 8 | buf1[12]);
    data.dig_P5 = (int16_t)((uint16_t)buf1[15] << 8 | buf1[14]);
    data.dig_P6 = (int16_t)((uint16_t)buf1[17] << 8 | buf1[16]);
    data.dig_P7 = (int16_t)((uint16_t)buf1[19] << 8 | buf1[18]);
    data.dig_P8 = (int16_t)((uint16_t)buf1[21] << 8 | buf1[20]);
    data.dig_P9 = (int16_t)((uint16_t)buf1[23] << 8 | buf1[22]);

    // Влажность
    data.dig_H1 = buf1[25];

    data.dig_H2 = (int16_t)((uint16_t)buf2[1] << 8 | buf2[0]);
    data.dig_H3 = buf2[2];

    // dig_H4 и dig_H5 — 12-битные знаковые значения
    int16_t h4 =
        (int16_t)((int16_t)(int8_t)buf2[3] * 16 +
                  (buf2[4] & 0x0F));

    int16_t h5 =
        (int16_t)((int16_t)(int8_t)buf2[5] * 16 +
                  (buf2[4] >> 4));

    data.dig_H4 = h4;
    data.dig_H5 = h5;
    data.dig_H6 = (int8_t)buf2[6];

    return 0;
}


// Компенсация температуры
double BME280::calculateTemp(uint32_t adc_T)
{
    double var1;
    double var2;

    var1 =
        ((double)adc_T / 16384.0 -
         (double)data.dig_T1 / 1024.0) *
        (double)data.dig_T2;

    var2 =
        ((double)adc_T / 131072.0 -
         (double)data.dig_T1 / 8192.0);

    var2 =
        var2 * var2 * (double)data.dig_T3;

    data.t_fine = (int32_t)(var1 + var2);

    return (var1 + var2) / 5120.0;
}


// Компенсация влажности
double BME280::calculateHum(uint32_t adc_H)
{
    uint32_t rawTemp = 0;

    if (readTemp(rawTemp, 1, 1) != 0) {
        return -1.0;
    }

    calculateTemp(rawTemp);

    double v =
        (double)data.t_fine - 76800.0;

    v =
        ((double)adc_H -
         ((double)data.dig_H4 * 64.0 +
          (double)data.dig_H5 / 16384.0 * v)) *
        ((double)data.dig_H2 / 65536.0 *
         (1.0 +
          (double)data.dig_H6 / 67108864.0 * v *
          (1.0 +
           (double)data.dig_H3 / 67108864.0 * v)));

    v *=
        1.0 -
        (double)data.dig_H1 * v / 524288.0;

    if (v > 100.0) {
        v = 100.0;
    }
    else if (v < 0.0) {
        v = 0.0;
    }

    return v;
}


// Компенсация давления
double BME280::calculatePressure(uint32_t adc_P)
{
    uint32_t rawTemp = 0;

    if (readTemp(rawTemp, 1, 1) != 0) {
        return -1.0;
    }

    calculateTemp(rawTemp);

    double var1 =
        (double)data.t_fine / 2.0 - 64000.0;

    double var2 =
        var1 * var1 *
        (double)data.dig_P6 / 32768.0;

    var2 +=
        var1 * (double)data.dig_P5 * 2.0;

    var2 =
        var2 / 4.0 +
        (double)data.dig_P4 * 65536.0;

    var1 =
        ((double)data.dig_P3 * var1 * var1 / 524288.0 +
         (double)data.dig_P2 * var1) / 524288.0;

    var1 =
        (1.0 + var1 / 32768.0) *
        (double)data.dig_P1;

    if (var1 == 0.0) {
        return -1.0;
    }

    double pressure = 1048576.0 - (double)adc_P;

    pressure =
        (pressure - var2 / 4096.0) *
        6250.0 / var1;

    var1 =
        (double)data.dig_P9 *
        pressure * pressure / 2147483648.0;

    var2 =
        pressure *
        (double)data.dig_P8 / 32768.0;

    pressure +=
        (var1 + var2 +
         (double)data.dig_P7) / 16.0;

    return pressure;
}


// =====================================================
// Реализация интерфейса TempStorage
// =====================================================


// Получить температуру в градусах Цельсия
uint8_t BME280::readTemperature(double& temperature)
{
    uint32_t rawTemperature = 0;

    uint8_t status = readTemp(rawTemperature, 1, 1);

    CHECK_TWI(status, 0, []() {}, status);

    temperature = calculateTemp(rawTemperature);

    return 0;
}


// Получить давление в паскалях
uint8_t BME280::readPressure(double& pressure)
{
    uint32_t rawPressure = 0;

    uint8_t status = readPressure(rawPressure, 1, 1, 1);

    CHECK_TWI(status, 0, []() {}, status);

    pressure = calculatePressure(rawPressure);

    if (pressure < 0) {
        return 10;
    }

    return 0;
}


// Получить относительную влажность в процентах
uint8_t BME280::readHumidity(double& humidity)
{
    uint32_t rawHumidity = 0;

    uint8_t status = readHum(rawHumidity, 1, 1, 1);

    CHECK_TWI(status, 0, []() {}, status);

    humidity = calculateHum(rawHumidity);

    if (humidity < 0) {
        return 10;
    }

    return 0;
}