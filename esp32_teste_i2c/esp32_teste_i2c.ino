#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================================");
  Serial.println("  VARREDURA TOTAL DE PINOS I2C NO ESP32");
  Serial.println("=================================================");

  int candidatePins[] = {21, 22, 18, 19, 16, 17, 25, 26, 27, 32, 33, 13, 14, 15, 4, 5};
  int numPins = sizeof(candidatePins) / sizeof(candidatePins[0]);
  bool foundAny = false;

  for (int sda_idx = 0; sda_idx < numPins; sda_idx++) {
    for (int scl_idx = 0; scl_idx < numPins; scl_idx++) {
      if (sda_idx == scl_idx) continue;

      int sda = candidatePins[sda_idx];
      int scl = candidatePins[scl_idx];

      Wire.begin(sda, scl);
      delay(20);

      // Testa se o DS3231 (0x68) ou AT24C32 (0x57) responde
      Wire.beginTransmission(0x68);
      byte err68 = Wire.endTransmission();

      Wire.beginTransmission(0x57);
      byte err57 = Wire.endTransmission();

      if (err68 == 0 || err57 == 0) {
        Serial.printf("🎉 ACHOU! SDA = GPIO %d | SCL = GPIO %d (0x68=%s, 0x57=%s)\n",
                      sda, scl, (err68 == 0 ? "OK" : "NAO"), (err57 == 0 ? "OK" : "NAO"));
        foundAny = true;
      }
    }
  }

  if (!foundAny) {
    Serial.println("❌ Nenhum dispositivo I2C respondeu nos pinos testados.");
    Serial.println("👉 Verifique:");
    Serial.println("   1. O fio VCC do RTC está ligado no 3.3V ou 5V do ESP32?");
    Serial.println("   2. O fio GND do RTC está ligado no GND do ESP32?");
    Serial.println("   3. Os fios SDA e SCL estão bem encaixados?");
  }
}

void loop() {
  delay(2000);
}
