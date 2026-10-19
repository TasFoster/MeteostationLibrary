#ifndef METEO_SERVICE_H
#define METEO_SERVICE_H

#include <Arduino.h>
#include "TempStorage.h"
#include "OutputInterface.h"

class MeteoService
{
private:
    TempStorage& storage;
    OutputInterface& output;

public:
    MeteoService(TempStorage& storage, OutputInterface& output);
    ~MeteoService();
    void display(char type);
};

#endif