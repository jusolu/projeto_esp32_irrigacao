#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Nome da rede Wi-Fi que o próprio ESP32 vai criar
const char* ap_ssid     = "ESP32-Rega-AP";
const char* ap_password = "irrigacao123"; // Senha (mínimo de 8 caracteres)

#define LED_AZUL 2 // LED indicador

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n==============================================");
  Serial.println("  INICIANDO ESP32 EM MODO ACCESS POINT (AP) ");
  Serial.println("==============================================");

  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(LED_AZUL, LOW);

  // Configura o ESP32 para gerar sua própria rede Wi-Fi
  WiFi.mode(WIFI_AP);
  
  // Inicializa o Access Point com SSID e Senha
  if (WiFi.softAP(ap_ssid, ap_password)) {
    Serial.println("✅ Rede Wi-Fi criada com sucesso!");
    Serial.printf("   → SSID: %s\n", ap_ssid);
    Serial.printf("   → Senha: %s\n", ap_password);
    
    // O IP padrão do ESP32 em modo AP costuma ser 192.168.4.1
    IPAddress IP = WiFi.softAPIP();
    Serial.print("   → IP do ESP32 (AP): ");
    Serial.println(IP);
    
    digitalWrite(LED_AZUL, HIGH); // LED aceso indica que a rede está ativa
  } else {
    Serial.println("❌ Falha ao criar a rede Wi-Fi AP. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  // --- CONFIGURAÇÃO DO ARDUINO OTA ---
  ArduinoOTA.setHostname("esp32-rega-ap");

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("\n[OTA-AP] Iniciando gravação do " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA-AP] Atualização concluída! Reiniciando...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA-AP] Progresso: %u%%\r", (progress / (total / 100)));
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\n[OTA-AP] Erro [%u]\n", error);
  });

  // Inicializa o serviço OTA
  ArduinoOTA.begin();

  Serial.println("📡 Serviço OTA ativo no Access Point!");
  Serial.println("==============================================\n");
}

void loop() {
  // Mantém o OTA escutando
  ArduinoOTA.handle();

  // Pisca o LED de 2 em 2 segundos (ligado por 2s, desligado por 2s)
  static unsigned long ultimoPisca = 0;
  if (millis() - ultimoPisca >= 2000) {
    ultimoPisca = millis();
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
  }

  static unsigned long ultimoPrint = 0;
  if (millis() - ultimoPrint > 5000) {
    ultimoPrint = millis();
    Serial.printf("🤖 ESP32 AP ativo. Dispositivos conectados: %d\n", WiFi.softAPgetStationNum());
  }
}
