/*
  =============================================================================
  SISTEMA DE REGA ESP32 (ARDUINO IDE) - VERSÃO SIMPLIFICADA
  =============================================================================
  - Conecta à rede Wi-Fi "AP104-2.4G"
  - Consulta o servidor Vercel a cada 2 segundos
  - LED Nativo (LED_BUILTIN): 
      - Fica DESLIGADO no estado normal
      - LIGA FIXO por 10s (ou tempo configurado) quando você clica REGAR AGORA
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// 1. CONFIGURAÇÕES DA REDE E DA API
const char* WIFI_SSID     = "AP104-2.4G";
const char* WIFI_PASSWORD = "papagaio";

// Endpoint da sua API na Vercel:
const char* VERCEL_API_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";

// Pino do LED Nativo da Placa
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

// Intervalo de consulta ao servidor (2 segundos)
const unsigned long INTERVALO_MS = 2000;
unsigned long ultimaChecagem = 0;

void conectarWiFi();
void checarVercelEAcionarRega();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================================");
  Serial.println("  ESP32 - Rega Simplificada (Vercel + Wi-Fi)");
  Serial.println("=============================================");

  // Configura o LED nativo como SAÍDA e inicia DESLIGADO
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Conecta ao Wi-Fi
  conectarWiFi();
}

void loop() {
  // Garante reconexão automática caso o Wi-Fi caia
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Executa a checagem a cada 2 segundos
  unsigned long agora = millis();
  if (agora - ultimaChecagem >= INTERVALO_MS) {
    ultimaChecagem = agora;
    checarVercelEAcionarRega();
  }
}

// -----------------------------------------------------------------------------
// Função para Conectar ao Wi-Fi
// -----------------------------------------------------------------------------
void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Conectando ao Wi-Fi ");
  Serial.print(WIFI_SSID);

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
    Serial.print(" Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ Falha na conexão Wi-Fi!");
  }
}

// -----------------------------------------------------------------------------
// Função para consultar a Vercel e Acender o LED Azul se rega for solicitada
// -----------------------------------------------------------------------------
void checarVercelEAcionarRega() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure(); // Ignora validação SSL no ESP32 para alta velocidade

  HTTPClient http;
  http.begin(client, VERCEL_API_URL);
  http.addHeader("Content-Type", "application/json");

  // Envia payload JSON simples
  String payload = "{\"soilMoisture\":50}";
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    
    // Procura se a Vercel respondeu waterRequested: true
    if (response.indexOf("\"waterRequested\":true") != -1 || response.indexOf("\"waterRequested\": true") != -1) {
      
      // Descobre a duração configurada (padrão 10 segundos)
      int durationSec = 10;
      int idx = response.indexOf("\"durationSec\":");
      if (idx != -1) {
        int d = response.substring(idx + 14).toInt();
        if (d > 0) durationSec = d;
      }

      Serial.println("\n💧 🚨 REGAR AGORA CLICADO NO DASHBOARD!");
      Serial.print("   --> ACENDENDO LED AZUL (LED_BUILTIN) POR ");
      Serial.print(durationSec);
      Serial.println(" SEGUNDOS...");

      // LIGA O LED AZUL FIXO
      digitalWrite(LED_BUILTIN, HIGH);

      // Aguarda o tempo da rega
      delay(durationSec * 1000);

      // DESLIGA O LED AZUL
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("   --> REGA CONCLUÍDA! Desligando LED azul e notificando Vercel...\n");

      // Avisa o servidor Vercel que a rega terminou para resetar o botão no dashboard
      http.POST("{\"waterCompleted\":true}");
    } else {
      Serial.print(".");
    }
  } else {
    Serial.printf("\n❌ Erro na consulta HTTP: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
