#ifndef METEO_SERVICE_H
#define METEO_SERVICE_H

#include <Arduino.h>
#include "TempStorage.h"
#include "../output/OutputInterface.h"

enum class MeasurementType : char {
    Temperature = 't',
    Pressure = 'p',
    Humidity = 'h'
};

#define METEO_DISPLAY_DECIMAL_PLACES 2

class MeteoService
{
private:
    TempStorage& storage;
    OutputInterface& output;

public:
    MeteoService(TempStorage& storage, OutputInterface& output);
    ~MeteoService();
    void display(MeasurementType type);
};

#endif