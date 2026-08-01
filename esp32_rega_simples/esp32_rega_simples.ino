/*
  =============================================================================
  SISTEMA DE IRRIGAÇÃO SOLAR DEFINITIVO ESP32 + RTC DS3231 + MONITOR DE BATERIA
  =============================================================================
  Monitoramento de Bateria 18650 (Divisor de Tensão 100kΩ / 100kΩ):
    - Pino de Leitura: GPIO 34 (ADC1_CH6)
    - Fator Multiplicador: 2.0x (Entrada reduzida de 4.2V para máx 2.1V no pino)
    - Proteção: Se a voltagem for < 3.3V, cancela a rega para preservar as baterias!

  Hardware:
    - RTC DS3231        ➔ SDA = GPIO 18 | SCL = GPIO 19
    - Relé / Bomba      ➔ GPIO 4 (Inicia em Repouso/OFF)
    - LED Indicador Azul➔ GPIO 2
    - Divisor Bateria   ➔ GPIO 34
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#define I2C_SDA 18
#define I2C_SCL 19
#define RTC_I2C_ADDRESS 0x68

#define LED_AZUL     2  // LED Azul embutido
#define LED_REGA     4  // Relé da Bomba de Água
#define PIN_BATERIA 34  // Pino do Divisor de Tensão (GPIO 34)

const char* WIFI_PASSWORD   = "papagaio";
const char* VERCEL_POST_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";

const int HORA_INICIO_DIA   = 6;   // 06:00 AM
const int HORA_FIM_DIA      = 20;  // 20:00 PM (8h da noite)
const int INTERVALO_HORAS   = 2;   // Regar a cada 2 horas
const int DURACAO_REGA_SEC  = 15;  // Duração da rega

RTC_DATA_ATTR int ultimaHoraRegada = -1; // Guarda na memória do RTC a última hora em que regou

uint8_t bcdToDec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }

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

// Leitura da voltagem real do banco de baterias 18650 (Divisor 100k / 100k)
float lerVoltagemBateria(int &pctBateria) {
  int leituraADC = analogRead(PIN_BATERIA);
  float tensaoPino = (leituraADC / 4095.0) * 3.3;
  float voltagemBateria = tensaoPino * 2.0; // Multiplica por 2 revertendo a divisão

  // Mapeia 3.2V (0%) até 4.2V (100%)
  pctBateria = map((int)(voltagemBateria * 100), 320, 420, 0, 100);
  if (pctBateria < 0) pctBateria = 0;
  if (pctBateria > 100) pctBateria = 100;

  return voltagemBateria;
}

String obterSSIDECanal(int &canalOut) {
  int n = WiFi.scanNetworks(false, true); 
  for (int i = 0; i < n; i++) {
    String foundSSID = WiFi.SSID(i);
    if (foundSSID.indexOf("AP104") != -1 && foundSSID.indexOf("plus") == -1) {
      canalOut = WiFi.channel(i);
      WiFi.scanDelete();
      return foundSSID;
    }
  }
  WiFi.scanDelete();
  canalOut = 0;
  return "";
}

bool conectarWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  int canal = 0;
  String ssidExato = obterSSIDECanal(canal);

  if (ssidExato.length() > 0) {
    WiFi.begin(ssidExato.c_str(), WIFI_PASSWORD, canal);
  } else {
    WiFi.begin("AP104-2.4G  ", WIFI_PASSWORD);
  }

  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 20) {
    delay(300);
    t++;
  }

  return (WiFi.status() == WL_CONNECTED);
}

void reportarRegaParaVercel(int duracao, String horaFormatada, String motivo, float volts, int pct) {
  if (!conectarWiFi()) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, VERCEL_POST_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"waterCompleted\":true,\"durationSec\":" + String(duracao) + 
                   ",\"rtcTime\":\"" + horaFormatada + 
                   "\",\"source\":\"" + motivo + 
                   "\",\"batteryVoltage\":" + String(volts, 2) + 
                   ",\"batteryPct\":" + String(pct) + "}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("   [Vercel] Relatório enviado com sucesso! (HTTP %d)\n", httpCode);
  }
  http.end();
}

void executarRega(int duracaoSec, String horaStr, String motivo, float volts, int pct) {
  Serial.printf("💦 LIGANDO BOMBA (GPIO 4) POR %d SEGUNDOS (%s)...\n", duracaoSec, motivo.c_str());
  digitalWrite(LED_REGA, HIGH);
  
  int totalPiscadas = (duracaoSec * 1000) / 250;
  for (int i = 0; i < totalPiscadas; i++) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
  }

  digitalWrite(LED_REGA, LOW);
  digitalWrite(LED_AZUL, HIGH);
  Serial.println("✅ Irrigação concluída! Bomba desligada.");

  Serial.println("🌐 Conectando Wi-Fi e enviando relatório...");
  reportarRegaParaVercel(duracaoSec, horaStr, motivo, volts, pct);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // GARANTIA DE HARDWARE: Iniciar relé e LED em LOW (Repouso)
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_REGA, OUTPUT);
  pinMode(PIN_BATERIA, INPUT);

  digitalWrite(LED_AZUL, HIGH);
  digitalWrite(LED_REGA, LOW);

  Wire.begin(I2C_SDA, I2C_SCL);

  esp_sleep_wakeup_cause_t motivoAcordo = esp_sleep_get_wakeup_cause();

  Serial.println("\n=======================================================");
  Serial.println("   SISTEMA DE IRRIGAÇÃO SOLAR DEFINITIVO (ESP32)");
  Serial.println("   Monitor de Bateria 18650 no GPIO 34 (Divisor 100k/100k)");
  Serial.println("=======================================================");

  // 1. Leitura do Banco de Baterias 18650
  int pctBateria = 0;
  float voltagemBateria = lerVoltagemBateria(pctBateria);
  Serial.printf("🔋 Banco de Baterias 18650: %.2fV (%d%% de carga)\n", voltagemBateria, pctBateria);

  // 2. Lê hora no RTC
  int seg = 0, min = 0, hora = 0, dia = 0, mes = 0, ano = 0;
  lerHoraRTC(seg, min, hora, dia, mes, ano);

  char timeBuffer[30];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d/%02d/%04d %02d:%02d:%02d", dia, mes, ano, hora, min, seg);
  Serial.printf("⏰ Hora atual no Módulo RTC: %s\n", timeBuffer);

  // Proteção: Só irriga se a voltagem for segura (> 3.3V)
  if (voltagemBateria < 3.3 && voltagemBateria > 0.5) {
    Serial.printf("⚠️ ALERTA: Bateria baixa (%.2fV)! Irrigação cancelada para proteção.\n", voltagemBateria);
  } else if (motivoAcordo == ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.println("🔌 Energizado (POWERON_RESET). Inicializando sem regar imediatamente.");
  } else {
    bool eHoraDeRegar = (hora >= HORA_INICIO_DIA && hora <= HORA_FIM_DIA && (hora % INTERVALO_HORAS == 0) && hora != ultimaHoraRegada);

    if (eHoraDeRegar) {
      ultimaHoraRegada = hora;
      Serial.println("🎯 Horário agendado no RTC atingido!");
      executarRega(DURACAO_REGA_SEC, String(timeBuffer), "RTC 2 Horas Diurno", voltagemBateria, pctBateria);
    } else {
      Serial.println("ℹ️ Fora do horário exato de rega.");
    }
  }

  // CALCULA SONO DEEP SLEEP
  uint64_t segundosSono = 30 * 60; // 30 minutos

  if (hora >= HORA_FIM_DIA || hora < HORA_INICIO_DIA) {
    int horasAteManha = (24 - hora + HORA_INICIO_DIA) % 24;
    if (horasAteManha == 0) horasAteManha = 1;
    segundosSono = horasAteManha * 3600;
    Serial.printf("🌙 PERÍODO NOTURNO: Dormindo por %d horas até as 06:00 AM...\n", horasAteManha);
  } else {
    Serial.println("😴 Entrando em Deep Sleep (Próxima checagem em 30 min)...");
  }

  // DESLIGA LEDS E WI-FI
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_REGA, LOW);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();

  esp_sleep_enable_timer_wakeup(segundosSono * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Deep Sleep
}
