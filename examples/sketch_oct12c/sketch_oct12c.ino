#include <MeteostationLibrary.h>

TWI twi;
BME280 sensor(twi, kBme280DefaultAddress);
TWILCD_output lcd(0x27, twi);
MeteoService meteo(sensor, lcd);

GetTemp temperature(meteo);
GetPress pressure(meteo);
GetHum humidity(meteo);

Tasker tasker(1, 3);

void setup() {
  twi.init();
  sensor.begin();
  lcd.init(kLcdCursorSteady);

  tasker.AddTask(temperature);
  tasker.AddTask(pressure);
  tasker.AddTask(humidity);
  tasker.start();
}

void loop() {}