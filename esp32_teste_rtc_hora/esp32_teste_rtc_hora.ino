#include <Arduino.h>
#include <Wire.h>

#define I2C_SDA 21
#define I2C_SCL 22
#define RTC_I2C_ADDRESS 0x68

uint8_t decToBcd(uint8_t val) { return ((val / 10 * 16) + (val % 10)); }
uint8_t bcdToDec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }

void setRTCDateTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day, uint8_t month, uint16_t year) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(sec));
  Wire.write(decToBcd(min));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(1)); // Dia da semana
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year - 2000));
  Wire.endTransmission();
}

bool lerHoraRTC(int &seg, int &min, int &hora, int &dia, int &mes, int &ano) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  if (Wire.available() >= 7) {
    seg  = bcdToDec(Wire.read() & 0x7F);
    min  = bcdToDec(Wire.read());
    hora = bcdToDec(Wire.read() & 0x3F);
    Wire.read();
    dia  = bcdToDec(Wire.read());
    mes  = bcdToDec(Wire.read());
    ano  = bcdToDec(Wire.read()) + 2000;
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  Serial.println("\n=======================================================");
  Serial.println("  GRAVADOR DIRETO DE HORÁRIO LOCAL NO RTC DS3231");
  Serial.println("=======================================================");
  Serial.println("Envie a hora no formato: SET:DD/MM/AAAA HH:MM:SS");
  Serial.println("Exemplo: SET:24/08/2026 18:34:00");
}

void loop() {
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg.startsWith("SET:")) {
      String dataHora = msg.substring(4);
      // Formato esperado: DD/MM/AAAA HH:MM:SS
      int dia = dataHora.substring(0, 2).toInt();
      int mes = dataHora.substring(3, 5).toInt();
      int ano = dataHora.substring(6, 10).toInt();
      int hora = dataHora.substring(11, 13).toInt();
      int min = dataHora.substring(14, 16).toInt();
      int seg = dataHora.substring(17, 19).toInt();

      setRTCDateTime(seg, min, hora, dia, mes, ano);
      Serial.printf("✅ SUCESSO! HORA GRAVADA NO CHIP RTC: %02d/%02d/%04d %02d:%02d:%02d\n",
                    dia, mes, ano, hora, min, seg);
    }
  }

  int s, m, h, d, me, a;
  if (lerHoraRTC(s, m, h, d, me, a)) {
    Serial.printf("🕰️ Horário Atual no RTC DS3231: %02d/%02d/%04d %02d:%02d:%02d\n", d, me, a, h, m, s);
  } else {
    Serial.println("❌ Falha na leitura I2C do RTC DS3231.");
  }
  delay(1500);
}
