#pragma once

#include "Task.h"
#include "MeteostationLibrary.h"

class GetTemp : public Task{
    
    MeteoService meteo;

public:
    GetTemp(const MeteoService& meteo);
    void execute() override;
};