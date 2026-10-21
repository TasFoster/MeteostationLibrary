#ifndef METEO_SERVICE_H
#define METEO_SERVICE_H

#include "../output/OutputInterface.h"
#include "TempStorage.h"
#include <Arduino.h>

enum class MeasurementType : char {
  kTemperature = 't',
  kPressure = 'p',
  kHumidity = 'h'
};

constexpr uint8_t kMeteoDisplayDecimalPlaces = 2;

class MeteoService {
 public:
  MeteoService(TempStorage &storage, OutputInterface &output);
  ~MeteoService();
  void display(MeasurementType type);

 private:
  TempStorage &storage_;
  OutputInterface &output_;
};

#endif