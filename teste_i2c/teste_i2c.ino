#include <Wire.h>

const int gpios[] = {0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const int totalPins = sizeof(gpios) / sizeof(gpios[0]);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== VARREDURA COMPLETA DE TODOS OS PINOS DO ESP32 ===");

  bool encontrouRTC = false;

  for (int i = 0; i < totalPins; i++) {
    for (int j = 0; j < totalPins; j++) {
      if (i == j) continue;
      int sda = gpios[i];
      int scl = gpios[j];

      Wire.end();
      Wire.begin(sda, scl);
      Wire.setTimeOut(20);

      // Testa se o RTC (0x68) ou a EEPROM do RTC (0x57) responde
      Wire.beginTransmission(0x68);
      if (Wire.endTransmission() == 0) {
        Serial.printf("🎉 ENCONTRADO RTC DS3231 (0x68) em SDA=GPIO %d e SCL=GPIO %d!\n", sda, scl);
        encontrouRTC = true;
      }
      Wire.beginTransmission(0x57);
      if (Wire.endTransmission() == 0) {
        Serial.printf("🎉 ENCONTRADA EEPROM (0x57) em SDA=GPIO %d e SCL=GPIO %d!\n", sda, scl);
        encontrouRTC = true;
      }
    }
  }

  if (!encontrouRTC) {
    Serial.println("❌ NENHUM RTC DS3231 (0x68) ou EEPROM (0x57) foi encontrado em NENHUM pino do ESP32.");
  } else {
    Serial.println("✅ Varredura com sucesso!");
  }
}

void loop() {
  delay(5000);
}
