#include "GetTemp.h"

GetTemp::GetTemp(const MeteoService& meteo)
    : meteo(meteo) {}

void GetTemp::execute() {
    meteo.display(MeasurementType::kTemperature);
}