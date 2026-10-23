// Arduino Mega 2560, 16 МГц: BME280 (0x76), LCD I2C (0x27).
// Timer3: одна задача раз в секунду по кругу. Timer4: таймауты.
#include <MeteostationLibrary.h>

constexpr uint8_t kLcdAddress = 0x27;
constexpr uint32_t kSerialBaudRate = 9600;

TWI twi;
BME280 sensor(twi, kBme280DefaultAddress);
TWILCD_output lcd_output(kLcdAddress, twi);
MeteoService meteo(sensor, lcd_output);
GetTemp get_temp(meteo);
GetPress get_press(meteo);
GetHum get_hum(meteo);
Tasker tasker(1, 3);

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

  if (!tasker.AddTask(get_temp) || !tasker.AddTask(get_press) ||
      !tasker.AddTask(get_hum)) {
    Serial.println("Task registration failed");
    while (true) {}
  }

  if (!tasker.start()) {
    Serial.println("Invalid task interval");
    while (true) {}
  }
}

void loop() {}
