#include "MeteoService.h"

MeteoService::MeteoService(TempStorage &storage, OutputInterface &output)
    : storage_(storage), output_(output) {}

MeteoService::~MeteoService() {}

void MeteoService::display(MeasurementType type) {
  double value;

  switch (type) {
  case MeasurementType::kTemperature:
    if (storage_.readTemperature(value) == kTempStorageSuccess) {
      String msg = "Temperature: ";
      msg += String(value, kMeteoDisplayDecimalPlaces);
      msg += " C";
      output_.print(msg);
    }
    break;

  case MeasurementType::kPressure:
    if (storage_.readPressure(value) == kTempStorageSuccess) {
      String msg = "Pres: ";
      msg += String(value, kMeteoDisplayDecimalPlaces);
      msg += " Pa";
      output_.print(msg);
    }
    break;

  case MeasurementType::kHumidity:
    if (storage_.readHumidity(value) == kTempStorageSuccess) {
      String msg = "Hum: ";
      msg += String(value, kMeteoDisplayDecimalPlaces);
      msg += " %";
      output_.print(msg);
    }
    break;
  }
}