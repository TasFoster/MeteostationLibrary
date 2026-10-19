#ifndef OUTPUT_INTERFACE_H
#define OUTPUT_INTERFACE_H
#include <Arduino.h> 


class OutputInterface
{
public:
    virtual void print(String data) = 0;
};

#endif