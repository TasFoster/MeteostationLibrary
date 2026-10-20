#ifndef METEO_SERVICE_H
#define METEO_SERVICE_H

#include "../output/OutputInterface.h"
#include "TempStorage.h"
#include <Arduino.h>

enum class MeasurementType : char {
  Temperature = 't',
  Pressure = 'p',
  Humidity = 'h'
};

#define METEO_DISPLAY_DECIMAL_PLACES 2

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