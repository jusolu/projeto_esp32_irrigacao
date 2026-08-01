/*
  =============================================================================
  TESTE DE DIAGNÓSTICO ESP32: WI-FI + GET NA API VERCEL (ARDUINO IDE)
  =============================================================================
  Este script responde a duas perguntas:
  1. O ESP32 consegue se conectar à internet (Wi-Fi)?
  2. O ESP32 consegue fazer GET na Vercel e receber o JSON esperado?
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char* WIFI_SSID     = "AP104-2.4G";
const char* WIFI_PASSWORD = "papagaio";
const char* VERCEL_URL    = "https://projeto-esp32-irrigacao.vercel.app/api/status";

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("  ESP32 - DIAGNÓSTICO DE WI-FI E REQUISICAO GET VERCEL");
  Serial.println("=======================================================");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // 1. TESTE DE CONEXÃO WI-FI
  Serial.print("\n[TESTE 1/2] Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Pisca LED indicando busca
    tentativas++;
  }
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ [TESTE 1 SUCESSO] ESP32 CONECTADO À INTERNET!");
    Serial.print("   -> Endereço IP Local: ");
    Serial.println(WiFi.localIP());
    Serial.print("   -> Sinal Wi-Fi (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("❌ [TESTE 1 FALHA] Não foi possível conectar ao Wi-Fi!");
    return;
  }

  // 2. TESTE DE REQUISIÇÃO GET NA VERCEL
  Serial.println("\n[TESTE 2/2] Testando HTTP GET na Vercel...");
  Serial.print("   URL: ");
  Serial.println(VERCEL_URL);

  WiFiClientSecure client;
  client.setInsecure(); // Ignora SSL

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(client, VERCEL_URL);

  int httpCode = http.GET();

  Serial.print("   -> Código de Resposta HTTP: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("   -> Resposta JSON Recebida da Vercel:");
    Serial.println("-------------------------------------------------------");
    Serial.println(response);
    Serial.println("-------------------------------------------------------");

    // Valida se o conteúdo retornado contém as chaves esperadas
    if (response.indexOf("waterRequested") != -1) {
      Serial.println("✅ [TESTE 2 SUCESSO] A Vercel respondeu o JSON esperado perfeitamente!");
      
      if (response.indexOf("\"waterRequested\":true") != -1 || response.indexOf("\"waterRequested\": true") != -1) {
        Serial.println("   STATUS ATUAL: Rega SOLICITADA! (waterRequested = true)");
      } else {
        Serial.println("   STATUS ATUAL: Rega DESATIVADA (waterRequested = false)");
      }
    } else {
      Serial.println("⚠️ Resposta recebida, mas o formato JSON veio diferente do esperado.");
    }
  } else {
    Serial.print("❌ [TESTE 2 FALHA] Erro HTTP GET: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
  Serial.println("\n=======================================================");
  Serial.println("  DIAGNÓSTICO CONCLUÍDO - ESP32 EM LOOP");
  Serial.println("=======================================================\n");
}

void loop() {
  // Pisca o LED 2x a cada 5s indicando que o teste foi concluído com sucesso
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(5000);
}
