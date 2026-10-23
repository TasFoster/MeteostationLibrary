#include "GetHum.h"

GetHum::GetHum(const MeteoService& meteo) : meteo_(meteo) {}

void GetHum::execute() {
  meteo_.display(MeasurementType::kHumidity);
}
