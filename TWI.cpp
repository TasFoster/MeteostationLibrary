#include "TWI.h"
#define TWI_CHECK(expr, msg, err)\
   do{                           \
      if(expr != msg){           \  
        twi_stop();              \
        return err;              \  
      }                          \
   }while(0)                     \

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
uint8_t twi_write_reg(uint8_t addr, uint8_t reg, uint8_t value){
  TWI_CHECK(twi_start(), 0x08, 1);
  TWI_CHECK(twi_write(addr << 1 | 0), 0x18, 2);
  TWI_CHECK(twi_write(reg), 0x18, 3);
  TWI_CHECK(twi_write(value), 0x28, 4);
  twi_stop();
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
