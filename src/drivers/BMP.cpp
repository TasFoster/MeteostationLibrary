#include "BMP.h"

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

    if (status != BME280_SUCCESS) {
        return status;
    }

    status = readCalib();

    if (status != BME280_SUCCESS) {
        return status + BME280_CALIBRATION_ERROR_OFFSET;
    }

    return BME280_SUCCESS;
}


// Сброс датчика
uint8_t BME280::reset()
{
    uint8_t status = bus.writeReg(data.addr, BME280_REG_RESET, BME280_RESET_COMMAND);

    if (status != BME280_SUCCESS) {
        return status;
    }

    unsigned long startTime = millis();

    while (true) {
        SensorStatus result = checkStatus(StatusCheck::CalibrationUpdate);

        if (result == SensorStatus::Ready) {
            return BME280_SUCCESS;
        }

        if (result == SensorStatus::Error) {
            return BME280_STATUS_READ_ERROR;
        }

        if (millis() - startTime > BME280_STATUS_TIMEOUT_MS) {
            return BME280_STATUS_TIMEOUT;
        }
    }
}


// Проверка состояния датчика
// CalibrationUpdate — обновление калибровочных данных
// Measurement — выполнение измерения
SensorStatus BME280::checkStatus(StatusCheck target)
{
    uint8_t status = 0;

    if (bus.readReg(data.addr, BME280_REG_STATUS, status) != BME280_SUCCESS) {
        return SensorStatus::Error;
    }

    if (target == StatusCheck::CalibrationUpdate) {
        return (status & BME280_UPDATE_MASK) ? SensorStatus::Busy : SensorStatus::Ready;
    }

    if (target == StatusCheck::Measurement) {
        return (status & BME280_MEASURING_MASK) ? SensorStatus::Busy : SensorStatus::Ready;
    }

    return SensorStatus::Error;
}


// Проверка режима работы
SensorMode BME280::checkMode()
{
    uint8_t ctrl_meas = 0;

    if (bus.readReg(data.addr, BME280_REG_CTRL_MEAS, ctrl_meas) != BME280_SUCCESS) {
        return SensorMode::Unknown;
    }

    SensorMode mode = static_cast<SensorMode>(ctrl_meas & BME280_MODE_MASK);

    if (mode == SensorMode::Sleep) {
        return SensorMode::Sleep; // Sleep
    }

    if (mode == SensorMode::Forced || mode == SensorMode::ForcedAlternate) {
        return SensorMode::Forced; // Forced
    }

    return SensorMode::Normal; // Normal
}


// Настройка регистра CONFIG
uint8_t BME280::setConfig(
    StandbyTime t_sb,
    Filter filter,
    Spi3Wire spi3w_en
)
{
    if (t_sb > StandbyTime::Ms20 || filter > Filter::Code7 || spi3w_en > Spi3Wire::Enabled) {
        return BME280_INVALID_ARGUMENT;
    }

    uint8_t config =
        (static_cast<uint8_t>(t_sb) << 5) |
        (static_cast<uint8_t>(filter) << 2) |
        static_cast<uint8_t>(spi3w_en);

    return bus.writeReg(data.addr, BME280_REG_CONFIG, config);
}


// Чтение температуры
uint8_t BME280::readTemp(
    uint32_t& res,
    Oversampling osrs,
    SensorMode mode
)
{
    if (osrs > Oversampling::X16 || mode < SensorMode::Forced || mode > SensorMode::Normal) {
        return BME280_INVALID_ARGUMENT;
    }

    uint8_t status = setConfig(StandbyTime::Ms1000, Filter::X4, Spi3Wire::Disabled);

    if (status != BME280_SUCCESS) {
        return status;
    }

    // Отключаем измерение влажности
    status = bus.writeReg(data.addr, BME280_REG_CTRL_HUM, static_cast<uint8_t>(Oversampling::Skipped));

    if (status != BME280_SUCCESS) {
        return status;
    }

    // CTRL_MEAS: osrs_t [7:5], osrs_p [4:2], mode [1:0]
    uint8_t ctrl_meas = (static_cast<uint8_t>(osrs) << 5) | static_cast<uint8_t>(mode);

    status = bus.writeReg(data.addr, BME280_REG_CTRL_MEAS, ctrl_meas);

    if (status != BME280_SUCCESS) {
        return status;
    }

    // В forced mode ожидаем завершения измерения
    if (mode != SensorMode::Normal) {
        unsigned long startTime = millis();

        while (checkStatus(StatusCheck::Measurement) == SensorStatus::Busy) {
            if (millis() - startTime > BME280_STATUS_TIMEOUT_MS) {
                return BME280_STATUS_TIMEOUT;
            }
        }

        if (checkStatus(StatusCheck::Measurement) == SensorStatus::Error) {
            return BME280_STATUS_READ_ERROR;
        }
    }

    uint8_t buf[3];

    status = bus.readRegs(data.addr, BME280_REG_TEMPERATURE, buf, 3);

    if (status != BME280_SUCCESS) {
        return status;
    }

    res =
        ((uint32_t)buf[BME280_DATA_MSB] << 12) |
        ((uint32_t)buf[BME280_DATA_LSB] << 4) |
        ((uint32_t)buf[BME280_DATA_XLSB] >> 4);

    return BME280_SUCCESS;
}


// Чтение влажности
uint8_t BME280::readHum(
    uint32_t& res,
    Oversampling osrs_h,
    Oversampling osrs_t,
    SensorMode mode
)
{
    if (osrs_h > Oversampling::X16 || osrs_t > Oversampling::X16 || mode < SensorMode::Forced || mode > SensorMode::Normal) {
        return BME280_INVALID_ARGUMENT;
    }

    uint8_t status = setConfig(StandbyTime::Ms1000, Filter::X4, Spi3Wire::Disabled);

    if (status != BME280_SUCCESS) {
        return status;
    }

    // CTRL_HUM необходимо записывать перед CTRL_MEAS
    status = bus.writeReg(data.addr, BME280_REG_CTRL_HUM, static_cast<uint8_t>(osrs_h));

    if (status != BME280_SUCCESS) {
        return status;
    }

    uint8_t ctrl_meas = (static_cast<uint8_t>(osrs_t) << 5) | static_cast<uint8_t>(mode);

    status = bus.writeReg(data.addr, BME280_REG_CTRL_MEAS, ctrl_meas);
    mydelay(BME280_MEASUREMENT_START_mydelay_MS);
    if (status != BME280_SUCCESS) {
        return status;
    }

    if (mode != SensorMode::Normal) {
        unsigned long startTime = millis();

        while (checkStatus(StatusCheck::Measurement) == SensorStatus::Busy) {
            if (millis() - startTime > BME280_STATUS_TIMEOUT_MS) {
                return BME280_STATUS_TIMEOUT;
            }
        }

        if (checkStatus(StatusCheck::Measurement) == SensorStatus::Error) {
            return BME280_STATUS_READ_ERROR;
        }
    }

    uint8_t buf[2];

    status = bus.readRegs(data.addr, BME280_REG_HUMIDITY, buf, 2);

    if (status != BME280_SUCCESS) {
        return status;
    }

    res =
        ((uint32_t)buf[BME280_DATA_MSB] << 8) |
        buf[BME280_DATA_LSB];

    return BME280_SUCCESS;
}


// Чтение давления
uint8_t BME280::readPressure(
    uint32_t& res,
    Oversampling osrs_p,
    Oversampling osrs_t,
    SensorMode mode
)
{
    if (osrs_p > Oversampling::X16 || osrs_t > Oversampling::X16 || mode < SensorMode::Forced || mode > SensorMode::Normal) {
        return BME280_INVALID_ARGUMENT;
    }

    uint8_t status = setConfig(StandbyTime::Ms1000, Filter::X4, Spi3Wire::Disabled);

    if (status != BME280_SUCCESS) {
        return status;
    }

    // Отключаем измерение влажности
    status = bus.writeReg(data.addr, BME280_REG_CTRL_HUM, static_cast<uint8_t>(Oversampling::Skipped));

    if (status != BME280_SUCCESS) {
        return status;
    }

    uint8_t ctrl_meas =
        (static_cast<uint8_t>(osrs_t) << 5) |
        (static_cast<uint8_t>(osrs_p) << 2) |
        static_cast<uint8_t>(mode);

    status = bus.writeReg(data.addr, BME280_REG_CTRL_MEAS, ctrl_meas);

    if (status != BME280_SUCCESS) {
        return status;
    }

    if (mode != SensorMode::Normal) {
        unsigned long startTime = millis();

        while (checkStatus(StatusCheck::Measurement) == SensorStatus::Busy) {
            if (millis() - startTime > BME280_STATUS_TIMEOUT_MS) {
                return BME280_STATUS_TIMEOUT;
            }
        }

        if (checkStatus(StatusCheck::Measurement) == SensorStatus::Error) {
            return BME280_STATUS_READ_ERROR;
        }
    }

    uint8_t buf[3];

    status = bus.readRegs(data.addr, BME280_REG_PRESSURE, buf, 3);

    if (status != BME280_SUCCESS) {
        return status;
    }

    res =
        ((uint32_t)buf[BME280_DATA_MSB] << 12) |
        ((uint32_t)buf[BME280_DATA_LSB] << 4) |
        ((uint32_t)buf[BME280_DATA_XLSB] >> 4);

    return BME280_SUCCESS;
}


// Чтение калибровочных коэффициентов
uint8_t BME280::readCalib()
{
    uint8_t buf1[26];
    uint8_t buf2[7];

    uint8_t status = bus.readRegs(data.addr, BME280_REG_CALIBRATION_MAIN, buf1, 26);

    if (status != BME280_SUCCESS) {
        return status;
    }

    status = bus.readRegs(data.addr, BME280_REG_CALIBRATION_HUMIDITY, buf2, 7);

    if (status != BME280_SUCCESS) {
        return status;
    }

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

    return BME280_SUCCESS;
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

    if (readTemp(rawTemp, Oversampling::X1, SensorMode::Forced) != BME280_SUCCESS) {
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

    if (readTemp(rawTemp, Oversampling::X1, SensorMode::Forced) != BME280_SUCCESS) {
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

    uint8_t status = readTemp(rawTemperature, Oversampling::X1, SensorMode::Forced);

    if (status != BME280_SUCCESS) {
        return status;
    }

    temperature = calculateTemp(rawTemperature);

    return BME280_SUCCESS;
}


// Получить давление в паскалях
uint8_t BME280::readPressure(double& pressure)
{
    uint32_t rawPressure = 0;

    uint8_t status = readPressure(rawPressure, Oversampling::X1, Oversampling::X1, SensorMode::Forced);

    if (status != BME280_SUCCESS) {
        return status;
    }

    pressure = calculatePressure(rawPressure);

    if (pressure < 0) {
        return BME280_COMPENSATION_ERROR;
    }

    return BME280_SUCCESS;
}


// Получить относительную влажность в процентах
uint8_t BME280::readHumidity(double& humidity)
{
    uint32_t rawHumidity = 0;

    uint8_t status = readHum(rawHumidity, Oversampling::X1, Oversampling::X1, SensorMode::Forced);

    if (status != BME280_SUCCESS) {
        return status;
    }

    humidity = calculateHum(rawHumidity);

    if (humidity < 0) {
        return BME280_COMPENSATION_ERROR;
    }

    return BME280_SUCCESS;
}