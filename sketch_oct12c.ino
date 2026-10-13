#include "TWI.h"
void setup() {
  Serial.begin(9600);
  // Serial.println("Start scan");
  // init_TWI();
  // for (uint8_t i = 0x08; i < 0x78; i++){
  //   twi_start();
  //   uint8_t res = twi_write(i << 1);
  //   twi_stop();
  //   if (res == 0x18){
  //     Serial.println(i, HEX);
  //   }
  // }
  // Serial.println("End scan");
  // Serial.println(twi_read_reg(0x76, 0xD0), HEX);
  init_TWI();
  Serial.println("A");
  uint8_t s = twi_write_reg(BMP280_ADDR, 0xE0, 0xB6);
  Serial.println("B");
  delay(5);
  while (twi_read_reg(BMP280_ADDR, 0xF3) & 1);
  Serial.println("C");
  Serial.println(s, HEX);
  Serial.println(twi_read_reg(BMP280_ADDR, 0xD0), HEX);
}

void loop() {

}
  