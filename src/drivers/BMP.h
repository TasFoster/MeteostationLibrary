#ifndef BMP_H
#define BMP_H

#include "../services/TempStorage.h"
#include "TWI.h"
#include <Arduino.h>

#define BME280_DEFAULT_ADDRESS 0x76
#define BME280_RESET_COMMAND 0xB6
#define BME280_UPDATE_MASK 0x01
#define BME280_MEASURING_MASK 0x08
#define BME280_MODE_MASK 0x03
#define BME280_MEASUREMENT_START_mydelay_MS 2
#define BME280_STATUS_TIMEOUT_MS 1000
#define BME280_CALIBRATION_ERROR_OFFSET 10

enum Bme280Register : uint8_t {
  BME280_REG_RESET = 0xE0,
  BME280_REG_CTRL_HUM = 0xF2,
  BME280_REG_STATUS = 0xF3,
  BME280_REG_CTRL_MEAS = 0xF4,
  BME280_REG_CONFIG = 0xF5,
  BME280_REG_PRESSURE = 0xF7,
  BME280_REG_TEMPERATURE = 0xFA,
  BME280_REG_HUMIDITY = 0xFD,
  BME280_REG_CALIBRATION_MAIN = 0x88,
  BME280_REG_CALIBRATION_HUMIDITY = 0xE1
};

enum Bme280Error : uint8_t {
  BME280_SUCCESS = 0,
  BME280_INVALID_ARGUMENT = 1,
  BME280_STATUS_TIMEOUT = 6,
  BME280_STATUS_READ_ERROR = 7,
  BME280_COMPENSATION_ERROR = 10
};

enum class StatusCheck : uint8_t { CalibrationUpdate = 0, Measurement = 1 };

enum class SensorMode : uint8_t {
  Sleep = 0,
  Forced = 1,
  ForcedAlternate = 2,
  Normal = 3,
  Unknown = 0xFF
};

enum class SensorStatus : int8_t { Error = -2, Ready = -1, Busy = 0 };

enum class Oversampling : uint8_t {
  Skipped = 0,
  X1 = 1,
  X2 = 2,
  X4 = 3,
  X8 = 4,
  X16 = 5
};

enum class StandbyTime : uint8_t {
  Ms0_5 = 0,
  Ms62_5 = 1,
  Ms125 = 2,
  Ms250 = 3,
  Ms500 = 4,
  Ms1000 = 5,
  Ms10 = 6,
  Ms20 = 7
};

enum class Filter : uint8_t {
  Off = 0,
  X2 = 1,
  X4 = 2,
  X8 = 3,
  X16 = 4,
  Code5 = 5,
  Code6 = 6,
  Code7 = 7
};

enum class Spi3Wire : uint8_t { Disabled = 0, Enabled = 1 };

enum MeasurementByte : uint8_t {
  BME280_DATA_MSB = 0,
  BME280_DATA_LSB = 1,
  BME280_DATA_XLSB = 2
};

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

class BME280 : public TempStorage {
private:
  TWI &bus;
  bme_calib data;

  uint8_t readCalib();
  SensorStatus checkStatus(StatusCheck target);

public:
  explicit BME280(TWI &twi, uint8_t address = BME280_DEFAULT_ADDRESS);

  // Реализация интерфейса TempStorage
  uint8_t begin() override;

  uint8_t readTemperature(double &temperature) override;
  uint8_t readPressure(double &pressure) override;
  uint8_t readHumidity(double &humidity) override;

  // Методы управления датчиком
  uint8_t reset();
  SensorMode checkMode();

  uint8_t setConfig(StandbyTime t_sb, Filter filter, Spi3Wire spi3w_en);

  // Чтение необработанных значений
  uint8_t readTemp(uint32_t &res, Oversampling osrs, SensorMode mode);

  uint8_t readHum(uint32_t &res, Oversampling osrs_h, Oversampling osrs_t,
                  SensorMode mode);

  uint8_t readPressure(uint32_t &res, Oversampling osrs_p, Oversampling osrs_t,
                       SensorMode mode);

  // Компенсация измерений
  double calculateTemp(uint32_t adc_T);
  double calculateHum(uint32_t adc_H);
  double calculatePressure(uint32_t adc_P);
};

#endif