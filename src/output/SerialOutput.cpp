#include "SerialOutput.h"

SerialOutput::SerialOutput(uint32_t bitRate) { Serial.begin(bitRate); }

void SerialOutput::print(String data) { Serial.println(data); }