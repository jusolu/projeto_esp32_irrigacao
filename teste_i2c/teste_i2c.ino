/*
  =============================================================================
  SCANNER I2C PARA ESP32 (SDA = GPIO 18 | SCL = GPIO 19)
  =============================================================================
*/

#include <Wire.h>

#define I2C_SDA 18
#define I2C_SCL 19

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("         ESP32 - SCANNER DE DISPOSITIVOS I2C");
  Serial.println("=======================================================");
  Serial.printf("Configuração dos Pinos I2C: SDA = GPIO %d | SCL = GPIO %d\n", I2C_SDA, I2C_SCL);
  Serial.println("-------------------------------------------------------\n");

  Wire.begin(I2C_SDA, I2C_SCL);
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Escaneando barramento I2C...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  -> Dispositivo I2C encontrado no endereço 0x%02X", address);
      if (address == 0x68) {
        Serial.print(" (Módulo RTC DS3231 / DS1307)");
      } else if (address == 0x57) {
        Serial.print(" (Memória EEPROM AT24C32 do RTC)");
      }
      Serial.println();
      nDevices++;
    } else if (error == 4) {
      Serial.printf("  -> Erro desconhecido no endereço 0x%02X\n", address);
    }
  }

  if (nDevices == 0) {
    Serial.println("Nenhum dispositivo I2C encontrado. Verifique os fios SDA (GPIO 18) e SCL (GPIO 19).\n");
  } else {
    Serial.printf("Busca concluída. %d dispositivo(s) encontrado(s).\n\n", nDevices);
  }

  delay(5000);
}
