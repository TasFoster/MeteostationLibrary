#include "MeteoService.h"

MeteoService::MeteoService(TempStorage &storage, OutputInterface &output)
    : storage_(storage), output_(output) {}

MeteoService::~MeteoService() {}

void MeteoService::display(MeasurementType type) {
  double value;

  switch (type) {
  case MeasurementType::Temperature:
    if (storage_.readTemperature(value) == TEMP_STORAGE_SUCCESS) {
      String msg = "Temperature: ";
      msg += String(value, METEO_DISPLAY_DECIMAL_PLACES);
      msg += " C";
      output_.print(msg);
    }
    break;

  case MeasurementType::Pressure:
    if (storage_.readPressure(value) == TEMP_STORAGE_SUCCESS) {
      String msg = "Pres: ";
      msg += String(value, METEO_DISPLAY_DECIMAL_PLACES);
      msg += " Pa";
      output_.print(msg);
    }
    break;

  case MeasurementType::Humidity:
    if (storage_.readHumidity(value) == TEMP_STORAGE_SUCCESS) {
      String msg = "Hum: ";
      msg += String(value, METEO_DISPLAY_DECIMAL_PLACES);
      msg += " %";
      output_.print(msg);
    }
    break;
  }
}