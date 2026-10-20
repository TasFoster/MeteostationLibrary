#ifndef SERIAL_OUTPUT_H
#define SERIAL_OUTPUT_H

#include "OutputInterface.h"
#include <Arduino.h>

class SerialOutput : public OutputInterface {
 public:
  explicit SerialOutput(uint32_t bitRate);

  void print(String data) override;
};

#endif