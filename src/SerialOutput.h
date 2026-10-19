#ifndef SERIAL_OUTPUT_H
#define SERIAL_OUTPUT_H

#include <Arduino.h>
#include "OutputInterface.h"

class SerialOutput : public OutputInterface
{
public:
    SerialOutput(uint32_t bitRate);


    void print(String data) override;
};

#endif