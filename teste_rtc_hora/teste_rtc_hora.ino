/*
  =============================================================================
  AJUSTE E SINCRONIZAÇÃO DO MÓDULO RTC DS3231/DS1307
  =============================================================================
*/

#include <Wire.h>

#define I2C_SDA 18
#define I2C_SCL 19
#define RTC_I2C_ADDRESS 0x68

uint8_t bcdToDec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
}

uint8_t decToBcd(uint8_t val) {
  return ((val / 10 * 16) + (val % 10));
}

void ajustarHoraRTC(int seg, int min, int hora, int dia, int mes, int ano) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(seg));
  Wire.write(decToBcd(min));
  Wire.write(decToBcd(hora));
  Wire.write(decToBcd(6)); // Sábado (6)
  Wire.write(decToBcd(dia));
  Wire.write(decToBcd(mes));
  Wire.write(decToBcd(ano - 2000));
  Wire.endTransmission();
  Serial.println("\n>>> RELÓGIO ATUALIZADO COM SUCESSO! <<<\n");
}

void lerHoraRTC(int &seg, int &min, int &hora, int &dia, int &mes, int &ano) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  if (Wire.available() >= 7) {
    seg  = bcdToDec(Wire.read() & 0x7F);
    min  = bcdToDec(Wire.read());
    hora = bcdToDec(Wire.read() & 0x3F);
    Wire.read();
    dia  = bcdToDec(Wire.read());
    mes  = bcdToDec(Wire.read());
    ano  = bcdToDec(Wire.read()) + 2000;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("     GRAVANDO HORA CERTA NO MÓDULO RTC DS3231/DS1307");
  Serial.println("=======================================================");

  Wire.begin(I2C_SDA, I2C_SCL);

  // Ajusta para o horário local exato: 15:53:40 do dia 01/08/2026
  ajustarHoraRTC(40, 53, 15, 1, 8, 2026);
}

void loop() {
  int seg = 0, min = 0, hora = 0, dia = 0, mes = 0, ano = 0;

  lerHoraRTC(seg, min, hora, dia, mes, ano);

  Serial.printf("📅 Data no RTC: %02d/%02d/%04d  |  ⏰ Hora no RTC: %02d:%02d:%02d\n",
                dia, mes, ano, hora, min, seg);

  delay(1000);
}
