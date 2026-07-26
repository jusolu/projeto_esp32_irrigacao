/*
  =============================================================================
  PROJETO DE IRRIGAÇÃO AUTOMATIZADA ESP32 + SERVIDOR VERCEL (ARDUINO IDE)
  =============================================================================
  Placa: ESP32 DevKit V1
  LED Nativo (Azul): GPIO 2 (const int LED_BUILTIN = 2;)
  Módulo Relé (Bomba d'água): GPIO 26
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// =============================================================================
// 1. CONFIGURAÇÕES DE REDE E SERVIDOR
// =============================================================================
const char* WIFI_SSID     = "AP104-2.4G";
const char* WIFI_PASSWORD = "papagaio";

// URL de produção do seu servidor Vercel:
const char* VERCEL_API_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";

// =============================================================================
// 2. PINAGEM DO HARDWARE (ESP32 DevKit V1)
// =============================================================================
const int LED_BUILTIN_PIN = 2;   // LED Nativo Azul do ESP32 DevKit V1
#define PIN_RELE            26  // Pino do Módulo Relé (Bomba d'água)
#define PIN_SOIL_ANALOG     34  // Sensor Umidade Solo Capacitivo v1.2 (ADC)
#define PIN_DHT             4   // Sensor DHT22 / DHT11
#define PIN_BATTERY         35  // Medidor Tensão Bateria (ADC)

#define DHTTYPE             DHT22 // Altere para DHT11 se necessário

const unsigned long INTERVALO_LEITURA_MS = 5000; // 5 segundos

DHT dht(PIN_DHT, DHTTYPE);
unsigned long ultimaChecagem = 0;

void conectarWiFi();
void executarCiclo();
void acionarRegaELED(int duracaoSegundos);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================================");
  Serial.println("  ESP32 DevKit V1 - Irrigador Automatizado");
  Serial.println("=============================================");

  // Configuração dos Pinos de Saída
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, LOW); // Inicia com o LED azul DESLIGADO

  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, HIGH); // Relé inicia DESLIGADO (Active LOW)

  dht.begin();
  conectarWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  unsigned long agora = millis();
  if (agora - ultimaChecagem >= INTERVALO_LEITURA_MS || ultimaChecagem == 0) {
    ultimaChecagem = agora;
    executarCiclo();
  }
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🎉 CONECTADO COM SUCESSO!");
    Serial.print("   IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.print("❌ Falha no Wi-Fi. Status: ");
    Serial.println(WiFi.status());
  }
}

void acionarRegaELED(int duracaoSegundos) {
  Serial.print("💧 🚨 REGAR SOLICITADO! LIGANDO LED AZUL NATIVO (GPIO 2) POR ");
  Serial.print(duracaoSegundos);
  Serial.println(" SEGUNDOS...");

  // ACENDE O LED AZUL NATIVO (GPIO 2) FIXO E LIGA O RELÉ (GPIO 26)
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  digitalWrite(PIN_RELE, LOW);

  delay(duracaoSegundos * 1000);

  // DESLIGA O LED AZUL NATIVO E O RELÉ
  digitalWrite(LED_BUILTIN_PIN, LOW);
  digitalWrite(PIN_RELE, HIGH);

  Serial.println("✅ REGA CONCLUÍDA! LED AZUL DESLIGADO.");
}

void executarCiclo() {
  int rawSoil = analogRead(PIN_SOIL_ANALOG);
  int umidadeSolo = (rawSoil <= 0) ? 50 : map(constrain(rawSoil, 1200, 3000), 3000, 1200, 0, 100);
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp)) temp = 25.0;
  if (isnan(hum))  hum  = 60.0;

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, VERCEL_API_URL);
    http.addHeader("Content-Type", "application/json");

    char requestBody[200];
    snprintf(requestBody, sizeof(requestBody),
      "{\"soilMoisture\":%d,\"temperature\":%.1f,\"humidity\":%.1f,\"batteryVoltage\":4.1}",
      umidadeSolo, temp, hum);

    int httpCode = http.POST(requestBody);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.printf("HTTP %d -> Resposta Vercel: %s\n", httpCode, payload.c_str());

      if (payload.indexOf("\"waterRequested\":true") != -1 || payload.indexOf("\"waterRequested\": true") != -1) {
        int durationSec = 10;
        int idx = payload.indexOf("\"durationSec\":");
        if (idx != -1) {
          int d = payload.substring(idx + 14).toInt();
          if (d > 0) durationSec = d;
        }

        // Aciona o LED azul nativo (GPIO 2) e o Relé
        acionarRegaELED(durationSec);

        // Notifica Vercel que concluiu
        http.POST("{\"waterCompleted\":true}");
      }
    } else {
      Serial.printf("❌ Erro HTTP: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}
