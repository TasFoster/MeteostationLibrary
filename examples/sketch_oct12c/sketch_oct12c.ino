// Arduino Mega 2560, 16 МГц: BME280 (0x76), LCD I2C (0x27).
// Timer3: вывод раз в секунду. Timer4: таймауты.
#include <MeteostationLibrary.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

constexpr uint8_t kLcdAddress = 0x27;
constexpr uint32_t kSerialBaudRate = 9600;
constexpr uint16_t kTimer3Compare = 62499;  // 1 с: 16 МГц / 256.

TWI twi;
BME280 sensor(twi, kBme280DefaultAddress);
TWILCD_output lcd_output(kLcdAddress, twi);
MeteoService meteo(sensor, lcd_output);
//Task<MeteoService, &MeteoService::display, MeasurementType> task(meteo, &MeteoService::display, MeasurementType::kTemperature);

ISR(TIMER3_COMPA_vect) {
 // task.execute()
}
void InitDisplayTimer() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR3B = 0;
    TIMSK3 = 0;
    TCCR3A = 0;
    TCCR3C = 0;
    TCNT3 = 0;
    OCR3A = kTimer3Compare;
    TIFR3 = _BV(OCF3A);  // Сброс флага сравнения.
    TIMSK3 = _BV(OCIE3A);
    TCCR3B = _BV(WGM32) | _BV(CS32);  // CTC, делитель 256.
  }
}

void setup() {
  Serial.begin(kSerialBaudRate);
  twi.init();

  if (sensor.begin() != kBme280Success) {
    Serial.println("BME280 initialization failed");
    while (true) {}
  }

  if (lcd_output.init(kLcdCursorSteady) != kLcdSuccess) {
    Serial.println("LCD initialization failed");
    while (true) {}
  }

  // Запуск Timer3 после инициализации устройств.
  InitDisplayTimer();
}

void loop() {}
