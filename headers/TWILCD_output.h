#ifndef TWILCD_OUTPUT_H
#define TWILCD_OUTPUT_H

#include "OutputInterface.h"
#include <Arduino.h>

class TWILCD_output : public OutputInterface
{
private:
    /* data */
public:
    TWILCD_output(/* args */);
    ~TWILCD_output();

    void print(String data) override;

};

#endif