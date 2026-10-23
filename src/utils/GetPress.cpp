#include "GetPress.h"

GetPress::GetPress(const MeteoService& meteo) : meteo_(meteo) {}

void GetPress::execute() {
  meteo_.display(MeasurementType::kPressure);
}
