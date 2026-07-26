#include <WiFi.h>

// ============================================================
// CÓDIGO DEFINITIVO DE CONEXÃO WI-FI PARA ESP32 DevKit V1 (ARDUINO IDE)
// ============================================================

const char* ssid        = "AP104-2.4G";
const char* password    = "papagaio";
const int   LED_BUILTIN = 2; // LED Azul Nativo do ESP32 DevKit V1

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configuração do LED Nativo (GPIO 2)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Inicia DESLIGADO

  Serial.println("\n=============================================");
  Serial.println("  ESP32 DevKit V1 - Conexão Wi-Fi Definitiva");
  Serial.println("=============================================");

  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    // Pisca curto o LED azul enquanto tenta conectar ao Wi-Fi
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    tentativas++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    // Liga o LED Azul fixo por 3 segundos para confirmar que conectou ao Wi-Fi!
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("🎉 CONECTADO COM SUCESSO!");
    Serial.print("   Endereço IP: ");
    Serial.println(WiFi.localIP());
    delay(3000);
    digitalWrite(LED_BUILTIN, LOW); // Desliga após confirmar
  } else {
    digitalWrite(LED_BUILTIN, LOW);
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
