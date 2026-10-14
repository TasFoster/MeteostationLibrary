#include <Arduino.h>
#include "TWI.h"


void init_TWI(){
    TWBR = 72;
    TWSR = 0x00;
    TWCR = (1 << TWEN);
}

uint8_t twi_start(){
  TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
  while (!(TWCR & (1 << TWINT)));
  return TWSR & 0xF8;
}

void twi_stop(){
  TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
  while ((TWCR & (1 << TWSTO)));
}

uint8_t twi_write(uint8_t data){
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)));
  return TWSR & 0xF8;
}

uint8_t twi_read_ack(){
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
  while(!(TWCR & (1 << TWINT)));
  return TWDR;
}

uint8_t twi_read_nack(){
  TWCR = (1 << TWINT) | (1 << TWEN);
  while(!(TWCR & (1 << TWINT)));
  
  return TWDR;
}

uint8_t twi_read_reg(uint8_t addr, uint8_t reg){

  TWI_CHECK(twi_start(), 0x08, 1);
  TWI_CHECK(twi_write(addr << 1 | 0), 0x18, 2);
  TWI_CHECK(twi_write(reg), 0x28, 3);
  TWI_CHECK(twi_start(), 0x10, 4);
  TWI_CHECK(twi_write(addr << 1 | 1), 0x40, 5);
  uint8_t res = twi_read_nack();
  twi_stop();
  return res;
}
uint8_t twi_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t status;

    Serial.println("TW: START");

    status = twi_start();
    Serial.print("START status: 0x");
    Serial.println(status, HEX);

    if (status != 0x08)
    {
        twi_stop();
        return 1;
    }

    status = twi_write(addr << 1);

    Serial.print("Address status: 0x");
    Serial.println(status, HEX);

    if (status != 0x18)
    {
        twi_stop();
        return 2;
    }

    status = twi_write(reg);

    Serial.print("Register status: 0x");
    Serial.println(status, HEX);

    if (status != 0x28)
    {
        twi_stop();
        return 3;
    }

    status = twi_write(value);

    Serial.print("Value status: 0x");
    Serial.println(status, HEX);

    if (status != 0x28)
    {
        twi_stop();
        return 4;
    }

    twi_stop();

    Serial.println("TW: success");

    return 0;
}

uint8_t twi_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len){
  if (len <= 0) return -1;
  TWI_CHECK(twi_start(), 0x08, 1);
  TWI_CHECK(twi_write(addr << 1 | 0), 0x18, 2);
  TWI_CHECK(twi_write(reg), 0x28, 3);
  TWI_CHECK(twi_start(), 0x10, 4);
  TWI_CHECK(twi_write(addr << 1 | 1), 0x40, 5);
  for(uint8_t i = 0; i < len - 1; i++){
    buf[i] = twi_read_ack();
  }
  buf[len - 1] = twi_read_nack();
  twi_stop();
  return 0;
}
