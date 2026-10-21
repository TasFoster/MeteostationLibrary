#ifndef BMP_H
#define BMP_H

#include "../services/TempStorage.h"
#include "TWI.h"
#include <Arduino.h>

constexpr uint8_t kBme280DefaultAddress = 0x76;
constexpr uint8_t kBme280ResetCommand = 0xB6;
constexpr uint8_t kBme280UpdateMask = 0x01;
constexpr uint8_t kBme280MeasuringMask = 0x08;
constexpr uint8_t kBme280ModeMask = 0x03;
constexpr uint16_t kBme280MeasurementStartDelayMs = 2;
constexpr uint32_t kBme280StatusTimeoutMs = 1000;
constexpr uint8_t kBme280CalibrationErrorOffset = 10;

enum Bme280Register : uint8_t {
  kBme280RegReset = 0xE0,
  kBme280RegCtrlHum = 0xF2,
  kBme280RegStatus = 0xF3,
  kBme280RegCtrlMeas = 0xF4,
  kBme280RegConfig = 0xF5,
  kBme280RegPressure = 0xF7,
  kBme280RegTemperature = 0xFA,
  kBme280RegHumidity = 0xFD,
  kBme280RegCalibrationMain = 0x88,
  kBme280RegCalibrationHumidity = 0xE1
};

enum Bme280Error : uint8_t {
  kBme280Success = 0,
  kBme280InvalidArgument = 1,
  kBme280StatusTimeout = 6,
  kBme280StatusReadError = 7,
  kBme280CompensationError = 10
};

enum class StatusCheck : uint8_t { kCalibrationUpdate = 0, kMeasurement = 1 };

enum class SensorMode : uint8_t {
  kSleep = 0,
  kForced = 1,
  kForcedAlternate = 2,
  kNormal = 3,
  kUnknown = 0xFF
};

enum class SensorStatus : int8_t { kError = -2, kReady = -1, kBusy = 0 };

enum class Oversampling : uint8_t {
  kSkipped = 0,
  kX1 = 1,
  kX2 = 2,
  kX4 = 3,
  kX8 = 4,
  kX16 = 5
};

enum class StandbyTime : uint8_t {
  kMs0_5 = 0,
  kMs62_5 = 1,
  kMs125 = 2,
  kMs250 = 3,
  kMs500 = 4,
  kMs1000 = 5,
  kMs10 = 6,
  kMs20 = 7
};

enum class Filter : uint8_t {
  kOff = 0,
  kX2 = 1,
  kX4 = 2,
  kX8 = 3,
  kX16 = 4,
  kCode5 = 5,
  kCode6 = 6,
  kCode7 = 7
};

enum class Spi3Wire : uint8_t { kDisabled = 0, kEnabled = 1 };

enum MeasurementByte : uint8_t {
  kBme280DataMsb = 0,
  kBme280DataLsb = 1,
  kBme280DataXlsb = 2
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
 public:
  explicit BME280(TWI &twi, uint8_t address = kBme280DefaultAddress);

  // Интерфейс TempStorage
  uint8_t begin() override;

  uint8_t readTemperature(double &temperature) override;
  uint8_t readPressure(double &pressure) override;
  uint8_t readHumidity(double &humidity) override;

  // Управление датчиком
  uint8_t reset();
  SensorMode checkMode();

  uint8_t setConfig(StandbyTime t_sb, Filter filter, Spi3Wire spi3w_en);

  // Необработанные измерения
  uint8_t readTemp(uint32_t &res, Oversampling osrs, SensorMode mode);

  uint8_t readHum(uint32_t &res, Oversampling osrs_h, Oversampling osrs_t,
                  SensorMode mode);

  uint8_t readPressure(uint32_t &res, Oversampling osrs_p, Oversampling osrs_t,
                       SensorMode mode);

  // Компенсация измерений
  double calculateTemp(uint32_t adc_T);
  double calculateHum(uint32_t adc_H);
  double calculatePressure(uint32_t adc_P);

 private:
  uint8_t readCalib();
  SensorStatus checkStatus(StatusCheck target);

  TWI &bus_;
  bme_calib data_;
};

#endif