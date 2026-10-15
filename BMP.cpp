#include <Arduino.h>
#include "BMP.h"

// ============================================================
// Конструктор
// ============================================================

BME280::BME280(TWI& twi, uint8_t address)
    : bus(twi)
{
    data = {};
    data.addr = address;
}


// ============================================================
// Инициализация датчика
// ============================================================

uint8_t BME280::begin()
{
    uint8_t result = reset();

    if (result != 0) {
        return result;
    }

    result = readCalib();

    if (result != 0) {
        return result + 10;
    }

    return 0;
}


// ============================================================
// Сброс датчика
// ============================================================

uint8_t BME280::reset()
{
    // Регистр сброса 0xE0, команда 0xB6
    uint8_t result = bus.writeReg(data.addr, 0xE0, 0xB6);

    if (result != 0) {
        return result;
    }

    // После сброса BME280 обновляет внутренние данные
    uint32_t startTime = millis();

    while (true) {
        int8_t status = checkStatus('u');

        if (status == -1) {
            break;
        }

        if (status == -2) {
            return 7;
        }

        if (millis() - startTime > 1000) {
            return 6;
        }
    }

    return 0;
}


// ============================================================
// Проверка состояния датчика
// ============================================================
//
// mode == 'u' : проверка im_update, бит 0 регистра 0xF3
// mode == 'm' : проверка measuring, бит 3 регистра 0xF3
//
// Возвращает:
//  0  — процесс ещё идёт
// -1  — процесс завершён
// -2  — ошибка чтения регистра
// ============================================================

int8_t BME280::checkStatus(char mode)
{
    uint8_t status = 0;

    if (bus.readReg(data.addr, 0xF3, status) != 0) {
        return -2;
    }

    if (mode == 'u') {
        if (status & (1 << 0)) {
            return 0;
        }

        return -1;
    }

    if (mode == 'm') {
        if (status & (1 << 3)) {
            return 0;
        }

        return -1;
    }

    return -2;
}


// ============================================================
// Определение режима работы
// ============================================================
//
// Возвращает:
// 's' — Sleep
// 'f' — Forced
// 'n' — Normal
// '?' — ошибка чтения
// ============================================================

char BME280::checkMode()
{
    uint8_t value = 0;

    if (bus.readReg(data.addr, 0xF4, value) != 0) {
        return '?';
    }

    uint8_t mode = value & 0x03;

    if (mode == 0) {
        return 's';
    }

    if (mode == 1 || mode == 2) {
        return 'f';
    }

    if (mode == 3) {
        return 'n';
    }

    return '?';
}


// ============================================================
// Настройка регистра config (0xF5)
// ============================================================

uint8_t BME280::setConfig(
    uint8_t t_sb,
    uint8_t filter,
    uint8_t spi3w_en
)
{
    uint8_t conf =
          (spi3w_en & 0x01)
        | ((filter & 0x07) << 2)
        | ((t_sb & 0x07) << 5);

    return bus.writeReg(data.addr, 0xF5, conf);
}


// ============================================================
// Чтение температуры
// ============================================================
//
// res  — необработанное значение ADC
// osrs — oversampling температуры:
//        0 = выключен, 1 = x1, 2 = x2, ... 5 = x16
// mode — режим измерения:
//        1 = Forced, 2 = Forced, 3 = Normal
//
// Возвращает 0 при успехе, ненулевой код при ошибке.
// ============================================================

uint8_t BME280::readTemp(
    uint32_t& res,
    uint8_t osrs,
    uint8_t mode
)
{
    if (mode == 0 || mode > 3 || osrs > 5) {
        return 1;
    }

    uint8_t result = setConfig(5, 2, 0);

    if (result != 0) {
        return 2;
    }

    // Для измерения температуры отключаем измерение влажности
    result = bus.writeReg(data.addr, 0xF2, 0);

    if (result != 0) {
        return 3;
    }

    // ctrl_meas:
    // [7:5] osrs_t
    // [4:2] osrs_p = 0
    // [1:0] mode
    uint8_t ctrl_meas =
          ((osrs & 0x07) << 5)
        | (mode & 0x03);

    result = bus.writeReg(data.addr, 0xF4, ctrl_meas);

    if (result != 0) {
        return 4;
    }

    // В Forced mode ждём завершения измерения.
    if (mode != 3) {
        uint32_t startTime = millis();

        while (true) {
            int8_t status = checkStatus('m');

            if (status == -1) {
                break;
            }

            if (status == -2) {
                return 7;
            }

            if (millis() - startTime > 1000) {
                return 5;
            }
        }
    }

    // Считываем три регистра температуры: 0xFA–0xFC
    uint8_t b[3];

    result = bus.readRegs(data.addr, 0xFA, b, 3);

    if (result != 0) {
        return 6;
    }

    // Температура представлена 20-битным числом
    res =
          ((uint32_t)b[0] << 12)
        | ((uint32_t)b[1] << 4)
        | ((uint32_t)b[2] >> 4);

    return 0;
}


// ============================================================
// Чтение влажности
// ============================================================
//
// osrs_h — oversampling влажности
// osrs_t — oversampling температуры
// mode   — режим измерения
// ============================================================

uint8_t BME280::readHum(
    uint32_t& res,
    uint8_t osrs_h,
    uint8_t osrs_t,
    uint8_t mode
)
{
    if (mode == 0 || mode > 3 || osrs_h > 5 || osrs_t > 5) {
        return 1;
    }

    uint8_t result = setConfig(5, 2, 0);

    if (result != 0) {
        return 2;
    }

    // Сначала задаём oversampling влажности.
    // Значение записывается в биты [2:0] регистра 0xF2.
    result = bus.writeReg(data.addr, 0xF2, osrs_h & 0x07);

    if (result != 0) {
        return 3;
    }

    // Настраиваем oversampling температуры и режим
    uint8_t ctrl_meas =
          ((osrs_t & 0x07) << 5)
        | (mode & 0x03);

    result = bus.writeReg(data.addr, 0xF4, ctrl_meas);

    if (result != 0) {
        return 4;
    }

    if (mode != 3) {
        uint32_t startTime = millis();

        while (true) {
            int8_t status = checkStatus('m');

            if (status == -1) {
                break;
            }

            if (status == -2) {
                return 7;
            }

            if (millis() - startTime > 1000) {
                return 5;
            }
        }
    }

    // Регистры влажности: 0xFD–0xFE
    uint8_t b[2];

    result = bus.readRegs(data.addr, 0xFD, b, 2);

    if (result != 0) {
        return 6;
    }

    // Влажность представлена 16-битным числом
    res = ((uint32_t)b[0] << 8) | b[1];

    return 0;
}


// ============================================================
// Чтение давления
// ============================================================
//
// osrs_p — oversampling давления
// osrs_t — oversampling температуры
// mode   — режим измерения
// ============================================================

uint8_t BME280::readPressure(
    uint32_t& res,
    uint8_t osrs_p,
    uint8_t osrs_t,
    uint8_t mode
)
{
    if (mode == 0 || mode > 3 || osrs_p > 5 || osrs_t > 5) {
        return 1;
    }

    uint8_t result = setConfig(5, 2, 0);

    if (result != 0) {
        return 2;
    }

    // Отключаем измерение влажности
    result = bus.writeReg(data.addr, 0xF2, 0);

    if (result != 0) {
        return 3;
    }

    // ctrl_meas:
    // [7:5] osrs_t
    // [4:2] osrs_p
    // [1:0] mode
    uint8_t ctrl_meas =
          ((osrs_t & 0x07) << 5)
        | ((osrs_p & 0x07) << 2)
        | (mode & 0x03);

    result = bus.writeReg(data.addr, 0xF4, ctrl_meas);

    if (result != 0) {
        return 4;
    }

    if (mode != 3) {
        uint32_t startTime = millis();

        while (true) {
            int8_t status = checkStatus('m');

            if (status == -1) {
                break;
            }

            if (status == -2) {
                return 7;
            }

            if (millis() - startTime > 1000) {
                return 5;
            }
        }
    }

    // Регистры давления: 0xF7–0xF9
    uint8_t b[3];

    result = bus.readRegs(data.addr, 0xF7, b, 3);

    if (result != 0) {
        return 6;
    }

    // Давление представлено 20-битным числом
    res =
          ((uint32_t)b[0] << 12)
        | ((uint32_t)b[1] << 4)
        | ((uint32_t)b[2] >> 4);

    return 0;
}


// ============================================================
// Чтение калибровочных коэффициентов
// ============================================================

uint8_t BME280::readCalib()
{
    uint8_t block1[26];
    uint8_t block2[7];

    // Калибровочные данные 0x88–0xA1
    uint8_t result = bus.readRegs(data.addr, 0x88, block1, 26);

    if (result != 0) {
        return 1;
    }

    // Калибровочные данные влажности 0xE1–0xE7
    result = bus.readRegs(data.addr, 0xE1, block2, 7);

    if (result != 0) {
        return 2;
    }

    // --------------------------------------------------------
    // Коэффициенты температуры
    // --------------------------------------------------------

    data.dig_T1 =
          (uint16_t)block1[0]
        | ((uint16_t)block1[1] << 8);

    data.dig_T2 = (int16_t)(
          (uint16_t)block1[2]
        | ((uint16_t)block1[3] << 8)
    );

    data.dig_T3 = (int16_t)(
          (uint16_t)block1[4]
        | ((uint16_t)block1[5] << 8)
    );

    // --------------------------------------------------------
    // Коэффициенты давления
    // --------------------------------------------------------

    data.dig_P1 =
          (uint16_t)block1[6]
        | ((uint16_t)block1[7] << 8);

    data.dig_P2 = (int16_t)(
          (uint16_t)block1[8]
        | ((uint16_t)block1[9] << 8)
    );

    data.dig_P3 = (int16_t)(
          (uint16_t)block1[10]
        | ((uint16_t)block1[11] << 8)
    );

    data.dig_P4 = (int16_t)(
          (uint16_t)block1[12]
        | ((uint16_t)block1[13] << 8)
    );

    data.dig_P5 = (int16_t)(
          (uint16_t)block1[14]
        | ((uint16_t)block1[15] << 8)
    );

    data.dig_P6 = (int16_t)(
          (uint16_t)block1[16]
        | ((uint16_t)block1[17] << 8)
    );

    data.dig_P7 = (int16_t)(
          (uint16_t)block1[18]
        | ((uint16_t)block1[19] << 8)
    );

    data.dig_P8 = (int16_t)(
          (uint16_t)block1[20]
        | ((uint16_t)block1[21] << 8)
    );

    data.dig_P9 = (int16_t)(
          (uint16_t)block1[22]
        | ((uint16_t)block1[23] << 8)
    );

    // --------------------------------------------------------
    // Коэффициенты влажности
    // --------------------------------------------------------

    data.dig_H1 = block1[25];

    data.dig_H2 = (int16_t)(
          (uint16_t)block2[0]
        | ((uint16_t)block2[1] << 8)
    );

    data.dig_H3 = block2[2];

    data.dig_H4 = (int16_t)(
          ((int16_t)(int8_t)block2[3] << 4)
        | (block2[4] & 0x0F)
    );

    data.dig_H5 = (int16_t)(
          ((int16_t)(int8_t)block2[5] << 4)
        | (block2[4] >> 4)
    );

    data.dig_H6 = (int8_t)block2[6];

    return 0;
}


// ============================================================
// Расчёт температуры
// Результат: градусы Цельсия
// ============================================================

double BME280::calculateTemp(uint32_t adc_T)
{
    double v1 =
        (adc_T / 16384.0 - data.dig_T1 / 1024.0)
        * data.dig_T2;

    double v2 =
        adc_T / 131072.0 - data.dig_T1 / 8192.0;

    v2 = v2 * v2 * data.dig_T3;

    data.t_fine = (int32_t)(v1 + v2);

    return (v1 + v2) / 5120.0;
}


// ============================================================
// Расчёт влажности
// Результат: относительная влажность, %
// -1.0 означает ошибку чтения температуры
// ============================================================

double BME280::calculateHum(uint32_t adc_H)
{
    uint32_t rawTemp = 0;

    uint8_t result = readTemp(rawTemp, 1, 1);

    if (result != 0) {
        return -1.0;
    }

    calculateTemp(rawTemp);

    double h = data.t_fine - 76800.0;

    h = (adc_H - (
            data.dig_H4 * 64.0
            + data.dig_H5 / 16384.0 * h
        ))
        * (
            data.dig_H2 / 65536.0
            * (
                1.0
                + data.dig_H6 / 67108864.0 * h
                * (
                    1.0
                    + data.dig_H3 / 67108864.0 * h
                )
            )
        );

    h = h * (
        1.0 - data.dig_H1 * h / 524288.0
    );

    if (h > 100.0) {
        h = 100.0;
    }

    if (h < 0.0) {
        h = 0.0;
    }

    return h;
}


// ============================================================
// Расчёт давления
// Результат: паскали (Pa)
// -1.0 означает ошибку чтения температуры
// ============================================================

double BME280::calculatePressure(uint32_t adc_P)
{
    uint32_t rawTemp = 0;

    uint8_t result = readTemp(rawTemp, 1, 1);

    if (result != 0) {
        return -1.0;
    }

    calculateTemp(rawTemp);

    double v1 = data.t_fine / 2.0 - 64000.0;

    double v2 =
        v1 * v1 * data.dig_P6 / 32768.0;

    v2 = v2 + v1 * data.dig_P5 * 2.0;

    v2 = v2 / 4.0 + data.dig_P4 * 65536.0;

    v1 = (
        data.dig_P3 * v1 * v1 / 524288.0
        + data.dig_P2 * v1
    ) / 524288.0;

    v1 = (1.0 + v1 / 32768.0) * data.dig_P1;

    if (v1 == 0.0) {
        return -1.0;
    }

    double p = 1048576.0 - adc_P;

    p = (p - v2 / 4096.0) * 6250.0 / v1;

    v1 = data.dig_P9 * p * p / 2147483648.0;

    v2 = p * data.dig_P8 / 32768.0;

    p = p + (v1 + v2 + data.dig_P7) / 16.0;

    return p;
}