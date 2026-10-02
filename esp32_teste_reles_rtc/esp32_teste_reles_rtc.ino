/*
  =============================================================================
  TESTE DE 3 RELÉS (COOLERS / CARGAS) + RTC DS3231 NO ESP32
  =============================================================================
  - Hardware:
      * Relé 1 ➔ GPIO 32
      * Relé 2 ➔ GPIO 33
      * Relé 3 ➔ GPIO 25
      * RTC DS3231 ➔ I2C (SDA = GPIO 21, SCL = GPIO 22)
  - Lógica do Módulo de Relé (Active-LOW):
      * LOW  (0V)  ➔ Relé LIGA (LED acende, contato fecha)
      * HIGH (3.3V)➔ Relé DESLIGA
  =============================================================================
*/

#include <Arduino.h>
#include <Wire.h>

#define PIN_RELE_1 32  // Relé 1
#define PIN_RELE_2 33  // Relé 2
#define PIN_RELE_3 25  // Relé 3

#define I2C_SDA 21
#define I2C_SCL 22
#define RTC_I2C_ADDRESS 0x68

const bool RELE_ATIVO_LOW = true;

void ligarRele(int pino) {
  digitalWrite(pino, RELE_ATIVO_LOW ? LOW : HIGH);
}

void desligarRele(int pino) {
  digitalWrite(pino, RELE_ATIVO_LOW ? HIGH : LOW);
}

uint8_t bcdToDec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }

bool lerHoraRTC(char* buffer, size_t maxLen) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    snprintf(buffer, maxLen, "RTC Offline");
    return false;
  }

  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  if (Wire.available() >= 7) {
    int seg  = bcdToDec(Wire.read() & 0x7F);
    int min  = bcdToDec(Wire.read());
    int hora = bcdToDec(Wire.read() & 0x3F);
    Wire.read();
    int dia  = bcdToDec(Wire.read());
    int mes  = bcdToDec(Wire.read());
    int ano  = bcdToDec(Wire.read()) + 2000;

    snprintf(buffer, maxLen, "%02d/%02d/%04d %02d:%02d:%02d", dia, mes, ano, hora, min, seg);
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("     TESTE DE BANCADA: 3 RELÉS (D32, D33, D25) + RTC   ");
  Serial.println("=======================================================");

  pinMode(PIN_RELE_1, OUTPUT);
  pinMode(PIN_RELE_2, OUTPUT);
  pinMode(PIN_RELE_3, OUTPUT);
  desligarRele(PIN_RELE_1);
  desligarRele(PIN_RELE_2);
  desligarRele(PIN_RELE_3);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  char rtcHora[35];
  if (lerHoraRTC(rtcHora, sizeof(rtcHora))) {
    Serial.printf("⏰ RTC DS3231 Conectado! Horário: %s\n", rtcHora);
  } else {
    Serial.println("⚠️ RTC DS3231 não detectado na I2C.");
  }

  Serial.println("=======================================================");
  Serial.println("Iniciando ciclo dos 3 Relés...\n");
}

void loop() {
  char rtcHora[35];

  // --- PASSO 1: LIGA APENAS RELÉ 1 (D32) ---
  lerHoraRTC(rtcHora, sizeof(rtcHora));
  Serial.printf("[%s] 🟢 LIGANDO RELÉ 1 (D32) por 2 segundos...\n", rtcHora);
  ligarRele(PIN_RELE_1);
  desligarRele(PIN_RELE_2);
  desligarRele(PIN_RELE_3);
  delay(2000);

  // --- PASSO 2: LIGA APENAS RELÉ 2 (D33) ---
  lerHoraRTC(rtcHora, sizeof(rtcHora));
  Serial.printf("[%s] 🔵 LIGANDO RELÉ 2 (D33) por 2 segundos...\n", rtcHora);
  desligarRele(PIN_RELE_1);
  ligarRele(PIN_RELE_2);
  desligarRele(PIN_RELE_3);
  delay(2000);

  // --- PASSO 3: LIGA APENAS RELÉ 3 (D25) ---
  lerHoraRTC(rtcHora, sizeof(rtcHora));
  Serial.printf("[%s] 🟡 LIGANDO RELÉ 3 (D25) por 2 segundos...\n", rtcHora);
  desligarRele(PIN_RELE_1);
  desligarRele(PIN_RELE_2);
  ligarRele(PIN_RELE_3);
  delay(2000);

  // --- PASSO 4: LIGA OS 3 RELÉS JUNTOS ---
  lerHoraRTC(rtcHora, sizeof(rtcHora));
  Serial.printf("[%s] ⚡ LIGANDO OS 3 RELÉS JUNTOS por 2 segundos...\n", rtcHora);
  ligarRele(PIN_RELE_1);
  ligarRele(PIN_RELE_2);
  ligarRele(PIN_RELE_3);
  delay(2000);

  // --- PASSO 5: DESLIGA TUDO (PAUSA) ---
  lerHoraRTC(rtcHora, sizeof(rtcHora));
  Serial.printf("[%s] ⚪ DESLIGANDO TUDO por 2 segundos (pausa)...\n\n", rtcHora);
  desligarRele(PIN_RELE_1);
  desligarRele(PIN_RELE_2);
  desligarRele(PIN_RELE_3);
  delay(2000);
}
