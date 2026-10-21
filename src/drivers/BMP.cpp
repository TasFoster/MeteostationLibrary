#include "BMP.h"

BME280::BME280(TWI &twi, uint8_t address) : bus_(twi) {
  data_ = {};
  data_.addr = address;
}

// Инициализация датчика
uint8_t BME280::begin() {
  uint8_t status = reset();

  if (status != kBme280Success) {
    return status;
  }

  status = readCalib();

  if (status != kBme280Success) {
    return status + kBme280CalibrationErrorOffset;
  }

  return kBme280Success;
}

// Сброс датчика
uint8_t BME280::reset() {
  uint8_t status =
      bus_.writeReg(data_.addr, kBme280RegReset, kBme280ResetCommand);

  if (status != kBme280Success) {
    return status;
  }

  const uint16_t started_at = bus_.timerTicks();

  while (true) {
    SensorStatus result = checkStatus(StatusCheck::kCalibrationUpdate);

    if (result == SensorStatus::kReady) {
      return kBme280Success;
    }

    if (result == SensorStatus::kError) {
      return kBme280StatusReadError;
    }

    if (bus_.mymillis(started_at) > kBme280StatusTimeoutMs) {
      return kBme280StatusTimeout;
    }
  }
}

// Проверка состояния датчика
// CalibrationUpdate — обновление калибровки.
// Measurement — измерение.
SensorStatus BME280::checkStatus(StatusCheck target) {
  uint8_t status = 0;

  if (bus_.readReg(data_.addr, kBme280RegStatus, status) != kBme280Success) {
    return SensorStatus::kError;
  }

  if (target == StatusCheck::kCalibrationUpdate) {
    return (status & kBme280UpdateMask) ? SensorStatus::kBusy
                                         : SensorStatus::kReady;
  }

  if (target == StatusCheck::kMeasurement) {
    return (status & kBme280MeasuringMask) ? SensorStatus::kBusy
                                            : SensorStatus::kReady;
  }

  return SensorStatus::kError;
}

// Проверка режима работы
SensorMode BME280::checkMode() {
  uint8_t ctrl_meas = 0;

  if (bus_.readReg(data_.addr, kBme280RegCtrlMeas, ctrl_meas) !=
      kBme280Success) {
    return SensorMode::kUnknown;
  }

  SensorMode mode = static_cast<SensorMode>(ctrl_meas & kBme280ModeMask);

  if (mode == SensorMode::kSleep) {
    return SensorMode::kSleep;
  }

  if (mode == SensorMode::kForced || mode == SensorMode::kForcedAlternate) {
    return SensorMode::kForced;
  }

  return SensorMode::kNormal;
}

// Настройка регистра CONFIG
uint8_t BME280::setConfig(StandbyTime t_sb, Filter filter, Spi3Wire spi3w_en) {
  if (t_sb > StandbyTime::kMs20 || filter > Filter::kCode7 ||
      spi3w_en > Spi3Wire::kEnabled) {
    return kBme280InvalidArgument;
  }

  uint8_t config = (static_cast<uint8_t>(t_sb) << 5) |
                   (static_cast<uint8_t>(filter) << 2) |
                   static_cast<uint8_t>(spi3w_en);

  return bus_.writeReg(data_.addr, kBme280RegConfig, config);
}

// Чтение температуры
uint8_t BME280::readTemp(uint32_t &res, Oversampling osrs, SensorMode mode) {
  if (osrs > Oversampling::kX16 || mode < SensorMode::kForced ||
      mode > SensorMode::kNormal) {
    return kBme280InvalidArgument;
  }

  uint8_t status =
      setConfig(StandbyTime::kMs1000, Filter::kX4, Spi3Wire::kDisabled);

  if (status != kBme280Success) {
    return status;
  }

  // Влажность отключена.
  status = bus_.writeReg(data_.addr, kBme280RegCtrlHum,
                        static_cast<uint8_t>(Oversampling::kSkipped));

  if (status != kBme280Success) {
    return status;
  }

  // CTRL_MEAS: osrs_t [7:5], osrs_p [4:2], mode [1:0]
  uint8_t ctrl_meas =
      (static_cast<uint8_t>(osrs) << 5) | static_cast<uint8_t>(mode);

  status = bus_.writeReg(data_.addr, kBme280RegCtrlMeas, ctrl_meas);

  if (status != kBme280Success) {
    return status;
  }

  // Forced: ожидание измерения.
  if (mode != SensorMode::kNormal) {
    const uint16_t started_at = bus_.timerTicks();

    while (checkStatus(StatusCheck::kMeasurement) == SensorStatus::kBusy) {
      if (bus_.mymillis(started_at) > kBme280StatusTimeoutMs) {
        return kBme280StatusTimeout;
      }
    }

    if (checkStatus(StatusCheck::kMeasurement) == SensorStatus::kError) {
      return kBme280StatusReadError;
    }
  }

  uint8_t buf[3];

  status = bus_.readRegs(data_.addr, kBme280RegTemperature, buf, 3);

  if (status != kBme280Success) {
    return status;
  }

  res = (static_cast<uint32_t>(buf[kBme280DataMsb]) << 12) |
        (static_cast<uint32_t>(buf[kBme280DataLsb]) << 4) |
        (static_cast<uint32_t>(buf[kBme280DataXlsb]) >> 4);

  return kBme280Success;
}

// Чтение влажности
uint8_t BME280::readHum(uint32_t &res, Oversampling osrs_h, Oversampling osrs_t,
                        SensorMode mode) {
  if (osrs_h > Oversampling::kX16 || osrs_t > Oversampling::kX16 ||
      mode < SensorMode::kForced || mode > SensorMode::kNormal) {
    return kBme280InvalidArgument;
  }

  uint8_t status =
      setConfig(StandbyTime::kMs1000, Filter::kX4, Spi3Wire::kDisabled);

  if (status != kBme280Success) {
    return status;
  }

  // Порядок записи: CTRL_HUM → CTRL_MEAS.
  status = bus_.writeReg(data_.addr, kBme280RegCtrlHum,
                        static_cast<uint8_t>(osrs_h));

  if (status != kBme280Success) {
    return status;
  }

  uint8_t ctrl_meas =
      (static_cast<uint8_t>(osrs_t) << 5) | static_cast<uint8_t>(mode);

  status = bus_.writeReg(data_.addr, kBme280RegCtrlMeas, ctrl_meas);
  mydelay(kBme280MeasurementStartDelayMs);
  if (status != kBme280Success) {
    return status;
  }

  if (mode != SensorMode::kNormal) {
    const uint16_t started_at = bus_.timerTicks();

    while (checkStatus(StatusCheck::kMeasurement) == SensorStatus::kBusy) {
      if (bus_.mymillis(started_at) > kBme280StatusTimeoutMs) {
        return kBme280StatusTimeout;
      }
    }

    if (checkStatus(StatusCheck::kMeasurement) == SensorStatus::kError) {
      return kBme280StatusReadError;
    }
  }

  uint8_t buf[2];

  status = bus_.readRegs(data_.addr, kBme280RegHumidity, buf, 2);

  if (status != kBme280Success) {
    return status;
  }

  res = (static_cast<uint32_t>(buf[kBme280DataMsb]) << 8) |
      buf[kBme280DataLsb];

  return kBme280Success;
}

// Чтение давления
uint8_t BME280::readPressure(uint32_t &res, Oversampling osrs_p,
                             Oversampling osrs_t, SensorMode mode) {
  if (osrs_p > Oversampling::kX16 || osrs_t > Oversampling::kX16 ||
      mode < SensorMode::kForced || mode > SensorMode::kNormal) {
    return kBme280InvalidArgument;
  }

  uint8_t status =
      setConfig(StandbyTime::kMs1000, Filter::kX4, Spi3Wire::kDisabled);

  if (status != kBme280Success) {
    return status;
  }

  // Влажность отключена.
  status = bus_.writeReg(data_.addr, kBme280RegCtrlHum,
                        static_cast<uint8_t>(Oversampling::kSkipped));

  if (status != kBme280Success) {
    return status;
  }

  uint8_t ctrl_meas = (static_cast<uint8_t>(osrs_t) << 5) |
                      (static_cast<uint8_t>(osrs_p) << 2) |
                      static_cast<uint8_t>(mode);

  status = bus_.writeReg(data_.addr, kBme280RegCtrlMeas, ctrl_meas);

  if (status != kBme280Success) {
    return status;
  }

  if (mode != SensorMode::kNormal) {
    const uint16_t started_at = bus_.timerTicks();

    while (checkStatus(StatusCheck::kMeasurement) == SensorStatus::kBusy) {
      if (bus_.mymillis(started_at) > kBme280StatusTimeoutMs) {
        return kBme280StatusTimeout;
      }
    }

    if (checkStatus(StatusCheck::kMeasurement) == SensorStatus::kError) {
      return kBme280StatusReadError;
    }
  }

  uint8_t buf[3];

  status = bus_.readRegs(data_.addr, kBme280RegPressure, buf, 3);

  if (status != kBme280Success) {
    return status;
  }

  res = (static_cast<uint32_t>(buf[kBme280DataMsb]) << 12) |
        (static_cast<uint32_t>(buf[kBme280DataLsb]) << 4) |
        (static_cast<uint32_t>(buf[kBme280DataXlsb]) >> 4);

  return kBme280Success;
}

// Калибровочные коэффициенты
uint8_t BME280::readCalib() {
  uint8_t buf1[26];
  uint8_t buf2[7];

  uint8_t status =
      bus_.readRegs(data_.addr, kBme280RegCalibrationMain, buf1, 26);

  if (status != kBme280Success) {
    return status;
  }

  status = bus_.readRegs(data_.addr, kBme280RegCalibrationHumidity, buf2, 7);

  if (status != kBme280Success) {
    return status;
  }

  // Температура
  data_.dig_T1 = static_cast<uint16_t>(buf1[1]) << 8 | buf1[0];
  data_.dig_T2 = static_cast<int16_t>(static_cast<uint16_t>(buf1[3]) << 8 |
      buf1[2]);
  data_.dig_T3 = static_cast<int16_t>(static_cast<uint16_t>(buf1[5]) << 8 |
      buf1[4]);

  // Давление
  data_.dig_P1 = static_cast<uint16_t>(buf1[7]) << 8 | buf1[6];
  data_.dig_P2 = static_cast<int16_t>(static_cast<uint16_t>(buf1[9]) << 8 |
      buf1[8]);
  data_.dig_P3 = static_cast<int16_t>(static_cast<uint16_t>(buf1[11]) << 8 |
      buf1[10]);
  data_.dig_P4 = static_cast<int16_t>(static_cast<uint16_t>(buf1[13]) << 8 |
      buf1[12]);
  data_.dig_P5 = static_cast<int16_t>(static_cast<uint16_t>(buf1[15]) << 8 |
      buf1[14]);
  data_.dig_P6 = static_cast<int16_t>(static_cast<uint16_t>(buf1[17]) << 8 |
      buf1[16]);
  data_.dig_P7 = static_cast<int16_t>(static_cast<uint16_t>(buf1[19]) << 8 |
      buf1[18]);
  data_.dig_P8 = static_cast<int16_t>(static_cast<uint16_t>(buf1[21]) << 8 |
      buf1[20]);
  data_.dig_P9 = static_cast<int16_t>(static_cast<uint16_t>(buf1[23]) << 8 |
      buf1[22]);

  // Влажность
  data_.dig_H1 = buf1[25];

  data_.dig_H2 = static_cast<int16_t>(static_cast<uint16_t>(buf2[1]) << 8 |
      buf2[0]);
  data_.dig_H3 = buf2[2];

  // dig_H4/dig_H5: знаковые 12 бит.
  int16_t h4 =
      static_cast<int16_t>(static_cast<int16_t>(static_cast<int8_t>(buf2[3])) *
      16 + (buf2[4] & 0x0F));

  int16_t h5 =
      static_cast<int16_t>(static_cast<int16_t>(static_cast<int8_t>(buf2[5])) *
      16 + (buf2[4] >> 4));

  data_.dig_H4 = h4;
  data_.dig_H5 = h5;
  data_.dig_H6 = static_cast<int8_t>(buf2[6]);

  return kBme280Success;
}

// Компенсация температуры
double BME280::calculateTemp(uint32_t adc_T) {
  double var1;
  double var2;

  var1 = (static_cast<double>(adc_T) / 16384.0 -
      static_cast<double>(data_.dig_T1) / 1024.0) *
         static_cast<double>(data_.dig_T2);

  var2 = (static_cast<double>(adc_T) / 131072.0 -
      static_cast<double>(data_.dig_T1) / 8192.0);

  var2 = var2 * var2 * static_cast<double>(data_.dig_T3);

  data_.t_fine = static_cast<int32_t>(var1 + var2);

  return (var1 + var2) / 5120.0;
}

// Компенсация влажности
double BME280::calculateHum(uint32_t adc_H) {
  uint32_t rawTemp = 0;

  if (readTemp(rawTemp, Oversampling::kX1, SensorMode::kForced) !=
      kBme280Success) {
    return -1.0;
  }

  calculateTemp(rawTemp);

  double v = static_cast<double>(data_.t_fine) - 76800.0;

  v = (static_cast<double>(adc_H) -
       (static_cast<double>(data_.dig_H4) * 64.0 +
           static_cast<double>(data_.dig_H5) / 16384.0 * v)) *
      (static_cast<double>(data_.dig_H2) / 65536.0 *
       (1.0 + static_cast<double>(data_.dig_H6) / 67108864.0 * v *
                  (1.0 + static_cast<double>(data_.dig_H3) / 67108864.0 * v)));

  v *= 1.0 - static_cast<double>(data_.dig_H1) * v / 524288.0;

  if (v > 100.0) {
    v = 100.0;
  } else if (v < 0.0) {
    v = 0.0;
  }

  return v;
}

// Компенсация давления
double BME280::calculatePressure(uint32_t adc_P) {
  uint32_t rawTemp = 0;

  if (readTemp(rawTemp, Oversampling::kX1, SensorMode::kForced) !=
      kBme280Success) {
    return -1.0;
  }

  calculateTemp(rawTemp);

  double var1 = static_cast<double>(data_.t_fine) / 2.0 - 64000.0;

  double var2 = var1 * var1 * static_cast<double>(data_.dig_P6) / 32768.0;

  var2 += var1 * static_cast<double>(data_.dig_P5) * 2.0;

  var2 = var2 / 4.0 + static_cast<double>(data_.dig_P4) * 65536.0;

  var1 = (static_cast<double>(data_.dig_P3) * var1 * var1 / 524288.0 +
          static_cast<double>(data_.dig_P2) * var1) /
         524288.0;

  var1 = (1.0 + var1 / 32768.0) * static_cast<double>(data_.dig_P1);

  if (var1 == 0.0) {
    return -1.0;
  }

  double pressure = 1048576.0 - static_cast<double>(adc_P);

  pressure = (pressure - var2 / 4096.0) * 6250.0 / var1;

  var1 = static_cast<double>(data_.dig_P9) * pressure * pressure / 2147483648.0;

  var2 = pressure * static_cast<double>(data_.dig_P8) / 32768.0;

  pressure += (var1 + var2 + static_cast<double>(data_.dig_P7)) / 16.0;

  return pressure;
}

// Интерфейс TempStorage

// Температура, °C.
uint8_t BME280::readTemperature(double &temperature) {
  uint32_t rawTemperature = 0;

  uint8_t status =
      readTemp(rawTemperature, Oversampling::kX1, SensorMode::kForced);

  if (status != kBme280Success) {
    return status;
  }

  temperature = calculateTemp(rawTemperature);

  return kBme280Success;
}

// Давление, Па.
uint8_t BME280::readPressure(double &pressure) {
  uint32_t rawPressure = 0;

  uint8_t status =
      readPressure(rawPressure, Oversampling::kX1, Oversampling::kX1,
                   SensorMode::kForced);

  if (status != kBme280Success) {
    return status;
  }

  pressure = calculatePressure(rawPressure);

  if (pressure < 0) {
    return kBme280CompensationError;
  }

  return kBme280Success;
}

// Относительная влажность, %.
uint8_t BME280::readHumidity(double &humidity) {
  uint32_t rawHumidity = 0;

  uint8_t status = readHum(rawHumidity, Oversampling::kX1, Oversampling::kX1,
                           SensorMode::kForced);

  if (status != kBme280Success) {
    return status;
  }

  humidity = calculateHum(rawHumidity);

  if (humidity < 0) {
    return kBme280CompensationError;
  }

  return kBme280Success;
}
