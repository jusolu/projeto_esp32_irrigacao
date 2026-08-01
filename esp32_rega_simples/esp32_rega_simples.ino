/*
  =============================================================================
  SISTEMA DE IRRIGAÇÃO SOLAR DEFINITIVO ESP32 + RTC DS3231
  =============================================================================
  Cronograma de Operação Solar:
    - Rega a cada 2 HORAS das 06:00 às 20:00.
    - Janela Ativa: 06:00, 08:00, 10:00, 12:00, 14:00, 16:00, 18:00, 20:00.
    - Noite (20:01 às 05:59): Deep Sleep contínuo (Bateria Solar 100% Preservada).

  Hardware:
    - RTC DS3231        ➔ SDA = GPIO 18 | SCL = GPIO 19
    - Relé / Bomba      ➔ GPIO 4
    - LED Indicador Azul➔ GPIO 2
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#define I2C_SDA 18
#define I2C_SCL 19
#define RTC_I2C_ADDRESS 0x68

#define LED_AZUL 2  // LED Azul embutido
#define LED_REGA 4  // Relé da Bomba de Água

const char* WIFI_PASSWORD   = "papagaio";
const char* VERCEL_POST_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";
const char* VERCEL_GET_URL  = "https://projeto-esp32-irrigacao.vercel.app/api/status";

const int HORA_INICIO_DIA   = 6;   // 06:00 AM
const int HORA_FIM_DIA      = 20;  // 20:00 PM (8h da noite)
const int INTERVALO_HORAS   = 2;   // Regar a cada 2 horas
const int DURACAO_REGA_SEC  = 15;  // 15 segundos de bomba ligada

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

void reportarRegaParaVercel(int duracao, String horaFormatada, String motivo) {
  if (!conectarWiFi()) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, VERCEL_POST_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"waterCompleted\":true,\"durationSec\":" + String(duracao) + ",\"rtcTime\":\"" + horaFormatada + "\",\"source\":\"" + motivo + "\"}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("   [Vercel] Relatório de irrigação enviado! (HTTP %d)\n", httpCode);
  }
  http.end();
}

void executarRega(int duracaoSec, String horaStr, String motivo) {
  Serial.printf("💦 LIGANDO BOMBA (GPIO 4) POR %d SEGUNDOS (%s)...\n", duracaoSec, motivo.c_str());
  digitalWrite(LED_REGA, HIGH);
  
  // Pisca o LED azul a cada 250ms enquanto rega
  int totalPiscadas = (duracaoSec * 1000) / 250;
  for (int i = 0; i < totalPiscadas; i++) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
  }

  digitalWrite(LED_REGA, LOW);
  digitalWrite(LED_AZUL, HIGH); // Fixo enquanto reporta
  Serial.println("✅ Irrigação concluída! Bomba desligada.");

  Serial.println("🌐 Conectando Wi-Fi e enviando relatório...");
  reportarRegaParaVercel(duracaoSec, horaStr, motivo);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_REGA, OUTPUT);

  // ESP32 Acordou -> LED Azul acende FIXO
  digitalWrite(LED_AZUL, HIGH);
  digitalWrite(LED_REGA, LOW);

  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("\n=======================================================");
  Serial.println("   SISTEMA DE IRRIGAÇÃO SOLAR DEFINITIVO (ESP32)");
  Serial.println("   Cronograma: A cada 2 horas (das 06:00 às 20:00)");
  Serial.println("=======================================================");

  int seg = 0, min = 0, hora = 0, dia = 0, mes = 0, ano = 0;
  lerHoraRTC(seg, min, hora, dia, mes, ano);

  char timeBuffer[30];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d/%02d/%04d %02d:%02d:%02d", dia, mes, ano, hora, min, seg);
  Serial.printf("⏰ Hora atual no Módulo RTC: %s\n", timeBuffer);

  // 1. Verifica se a hora atual está dentro da janela do dia (06:00 às 20:00) e é hora par
  bool eHoraDeRegar = (hora >= HORA_INICIO_DIA && hora <= HORA_FIM_DIA && (hora % INTERVALO_HORAS == 0) && min < 15);

  if (eHoraDeRegar) {
    Serial.println("🎯 Horário agendado atingido (Hora Par diurna)!");
    executarRega(DURACAO_REGA_SEC, String(timeBuffer), "RTC 2 Horas Diurno");
  } else {
    Serial.println("ℹ️ Fora do horário de rega automática.");
    
    // Checa o painel Vercel caso haja acionamento manual
    if (conectarWiFi()) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      http.begin(client, VERCEL_GET_URL);
      if (http.GET() == 200) {
        String resp = http.getString();
        if (resp.indexOf("\"waterRequested\":true") != -1 || resp.indexOf("\"waterRequested\": true") != -1) {
          Serial.println("👆 Rega manual solicitada pelo Dashboard!");
          executarRega(DURACAO_REGA_SEC, String(timeBuffer), "Manual Dashboard");
        }
      }
      http.end();
    }
  }

  // 2. Calcula o tempo exato de sono até a próxima checagem/hora par
  uint64_t segundosSono = 30 * 60; // Padrão: checar a cada 30 minutos

  // Se passou das 20:00, dorme direto até as 06:00 da manhã do dia seguinte
  if (hora >= HORA_FIM_DIA || hora < HORA_INICIO_DIA) {
    int horasAteManha = (24 - hora + HORA_INICIO_DIA) % 24;
    if (horasAteManha == 0) horasAteManha = 1;
    segundosSono = horasAteManha * 3600;
    Serial.printf("\n🌙 PERÍODO NOTURNO: Dormindo direto por %d horas até as 06:00 AM...\n", horasAteManha);
  } else {
    Serial.println("\n😴 Entrando em Deep Sleep por 30 min (Próxima checagem)...");
  }

  // Desliga todos os LEDs antes de dormir
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_REGA, LOW);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();

  esp_sleep_enable_timer_wakeup(segundosSono * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Deep Sleep ativo
}
