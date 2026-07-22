#include <WiFi.h>

// ============================================================
// CÓDIGO DEFINITIVO DE CONEXÃO WI-FI PARA ESP32 (ARDUINO IDE)
// ============================================================

const char* ssid     = "ACERJS 9417";
const char* password = "149oF8@3";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================================");
  Serial.println("  ESP32 - Conexão Wi-Fi Definitiva");
  Serial.println("=============================================");

  // Sequência idêntica ao MicroPython (que conectou com sucesso!)
  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🎉 CONECTADO COM SUCESSO!");
    Serial.print("   Endereço IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("   Sinal RSSI:  ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("❌ Falha na conexão. Status: ");
    Serial.println(WiFi.status());
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🌐 Wi-Fi OK! ESP32 Online.");
    delay(5000);
  } else {
    Serial.println("⚠️ Conexão perdida! Reconectando...");
    WiFi.begin(ssid, password);
    delay(5000);
  }
}
