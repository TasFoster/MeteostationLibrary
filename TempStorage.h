#ifndef TEMP_STORAGE_H
#define TEMP_STORAGE_H

#include <Arduino.h>

class TempStorage {
public:
    virtual ~TempStorage() = default;

    virtual uint8_t begin() = 0;

    virtual uint8_t readTemperature(double& temperature) = 0;
    virtual uint8_t readPressure(double& pressure) = 0;
    virtual uint8_t readHumidity(double& humidity) = 0;
};

#endif