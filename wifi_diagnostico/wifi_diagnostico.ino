/*
  DIAGNÓSTICO WI-FI COMPLETO - VER MOTIVO EXATO DA FALHA
  Mostra os eventos internos do rádio Wi-Fi do ESP32
*/
#include <WiFi.h>

const char* WIFI_SSID     = "AP104-2.4G";
const char* WIFI_PASSWORD = "papagaio";

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("[EVENTO] STA iniciado");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("[EVENTO] Conectado ao AP!");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("[EVENTO] IP obtido: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.print("[EVENTO] DESCONECTADO! Motivo: ");
      Serial.println(info.wifi_sta_disconnected.reason);
      // Códigos comuns:
      // 2  = AUTH_EXPIRE (senha errada ou WPA3 rejeitado)
      // 3  = AUTH_LEAVE
      // 15 = ASSOC_LEAVE
      // 201= NO_AP_FOUND (rede não encontrada)
      // 202= AUTH_FAIL (falha de autenticação - SENHA ERRADA)
      // 203= ASSOC_FAIL
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== DIAGNÓSTICO WI-FI ESP32 ===");
  Serial.printf("SSID: %s\n", WIFI_SSID);
  Serial.printf("Senha: %s\n", WIFI_PASSWORD);
  Serial.println("================================\n");

  WiFi.onEvent(WiFiEvent);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Aguardando conexao (30s)...");
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 60) {
    delay(500);
    Serial.print(".");
    t++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n>>> CONECTADO COM SUCESSO! <<<");
  } else {
    Serial.println("\n>>> FALHA NA CONEXAO! Ver motivo acima. <<<");
  }
}

void loop() {}
