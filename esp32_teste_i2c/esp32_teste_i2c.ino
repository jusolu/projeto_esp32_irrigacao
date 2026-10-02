#include <Arduino.h>
#include <Wire.h>

void testPair(int sda, int scl, const char* name) {
  pinMode(sda, INPUT_PULLUP);
  pinMode(scl, INPUT_PULLUP);
  delay(10);
  int v_sda = digitalRead(sda);
  int v_scl = digitalRead(scl);
  
  Serial.printf("\n🔍 Testando [%s]: SDA(GPIO %d)=%d, SCL(GPIO %d)=%d\n", 
                name, sda, v_sda, scl, v_scl);

  Wire.end();
  delay(20);
  Wire.begin(sda, scl);
  Wire.setClock(50000);
  Wire.setTimeOut(50);
  delay(20);

  Wire.beginTransmission(0x68);
  byte err = Wire.endTransmission();
  
  Wire.beginTransmission(0x57);
  byte errEE = Wire.endTransmission();

  if (err == 0 || errEE == 0) {
    Serial.printf("🎉🎉 SUCESSO! RTC DS3231 RESPONDENDO COM SUCESSO EM SDA=%d e SCL=%d! (0x68: %d, 0x57: %d)\n", 
                  sda, scl, err, errEE);
  } else {
    Serial.printf("   ❌ Falha (Erro I2C no 0x68: %d, 0x57: %d)\n", err, errEE);
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("\n=======================================================");
  Serial.println("  DIAGNÓSTICO COMPLETO DO RELÓGIO DS3231");
  Serial.println("=======================================================");

  // 1. Padrão: SDA=21, SCL=22
  testPair(21, 22, "D21 como SDA, D22 como SCL (Normal)");

  // 2. Invertido: SDA=22, SCL=21
  testPair(22, 21, "D22 como SDA, D21 como SCL (Invertido)");

  // 3. Outros pares comuns:
  testPair(19, 18, "D19 como SDA, D18 como SCL");
  testPair(23, 22, "D23 como SDA, D22 como SCL");
  testPair(25, 26, "D25 como SDA, D26 como SCL");
  testPair(32, 33, "D32 como SDA, D33 como SCL");
}

void loop() {
  delay(4000);
}
