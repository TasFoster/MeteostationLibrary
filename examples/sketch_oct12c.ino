#include <avr/interrupt.h>
#include <util/atomic.h>

#include "TWI.h"
#include "BMP.h"
#include "SerialOutput.h"
#include "MeteoService.h"
#include "TWILCD_output.h"

TWI twi;
BME280 sensor(twi, 0x76);
TWILCD_output lcd_output(0x27, twi);

MeteoService meteo(sensor, lcd_output);

volatile bool flag_start = false;

ISR(TIMER3_COMPA_vect){
    flag_start = true;
}

void meteoDisplay(){
    meteo.display('h');
}

void initTimer(){
    TCCR3A = 0;
    TCCR3B = 0;
    TCNT3 = 0;
    OCR3A = 62499;
    TIMSK3 = _BV(OCF3A);
    TIFR3 = _BV(OCIE3A);
    TCCR3B = _BV(WGM32) | _BV(CS32);
    interrupts();
}

void setup()
{
    initTimer();
    Serial.begin(9600);
    Serial.println("Program started");

    twi.init();
    Serial.println("TWI initialized");

    if (sensor.begin() != 0)
    {
        Serial.println("BME280 initialization failed!");
        while (true) {}
    }

    Serial.println("BME280 initialized!");
}

void loop()
{
    bool run;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        run = flag_start;
        flag_start = false;
    }


    if (run){
        meteoDisplay();
    }
}