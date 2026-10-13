#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#define BMP280_ADDR 0x76

void init_TWI();

uint8_t twi_start();

void twi_stop();

uint8_t twi_write(uint8_t data);

uint8_t twi_read_ack();

uint8_t twi_read_nack();

uint8_t twi_read_reg(uint8_t addr, uint8_t reg);

uint8_t twi_write_reg(uint8_t addr, uint8_t reg, uint8_t value);

uint8_t twi_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);