#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Credenciais pré-configuradas a partir do seu projeto principal
const char* ssid     = "AP104-2.4G  "; // SSID com dois espaços no final
const char* password = "papagaio";

#define LED_AZUL 2 // LED indicador no ESP32

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n==============================================");
  Serial.println("    INICIANDO ESP32 COM SUPORTE A OTA (WI-FI)");
  Serial.println("==============================================");

  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(LED_AZUL, LOW); // Liga o LED durante a conexão para indicar atividade

  // Conectando ao Wi-Fi
  Serial.printf("Conectando a rede: %s...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Aguarda conexão
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL)); // Pisca o LED
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Conectado com sucesso!");
    Serial.print("   → IP do ESP32: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_AZUL, HIGH); // Deixa o LED aceso (estável)
  } else {
    Serial.println("\n❌ Falha ao conectar ao Wi-Fi. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  // --- CONFIGURAÇÃO DO ARDUINO OTA ---

  // Define o nome de rede do seu ESP32. É esse nome que vai aparecer no Arduino IDE!
  ArduinoOTA.setHostname("esp32-rega-ota");

  // Define uma senha de segurança para ninguém atualizar o seu ESP32 sem autorização (opcional)
  // ArduinoOTA.setPassword("senha123");

  // Callbacks do OTA (funções chamadas em eventos específicos)
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch (firmware)";
    } else { // U_SPIFFS
      type = "filesystem (SPIFFS/LittleFS)";
    }
    Serial.println("\n[OTA] Iniciando gravação do " + type);
    
    // Pisca o LED rapidamente para indicar início da gravação
    for(int i = 0; i < 10; i++) {
      digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
      delay(50);
    }
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Gravação concluída com sucesso! Reiniciando...");
    // Pisca 3 vezes bem devagar indicando sucesso
    for(int i = 0; i < 6; i++) {
      digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
      delay(200);
    }
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progresso: %u%%\r", (progress / (total / 100)));
    // Alterna o LED a cada bloco recebido
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\n[OTA] Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Falha na autenticação (Senha incorreta)!");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha ao iniciar a gravação!");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha de conexão!");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Erro ao receber dados!");
    else if (error == OTA_END_ERROR) Serial.println("Erro na finalização da gravação!");
  });

  // Inicializa o serviço OTA
  ArduinoOTA.begin();

  Serial.println("📡 Serviço OTA Ativo e ouvindo na rede local!");
  Serial.println("==============================================\n");
}

void loop() {
  // ESSENCIAL: Mantém o serviço OTA escutando a rede
  ArduinoOTA.handle();

  // Pisca o LED de 2 em 2 segundos
  static unsigned long ultimoPisca = 0;
  if (millis() - ultimoPisca >= 2000) {
    ultimoPisca = millis();
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
  }
  
  static unsigned long ultimoPrint = 0;
  if (millis() - ultimoPrint > 5000) {
    ultimoPrint = millis();
    Serial.println("🚀 ESP32 ATUALIZADO VIA OTA - PISCANDO A CADA 2 SEGUNDOS!");
  }
}
