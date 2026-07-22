/*
  =============================================================================
  PROJETO DE IRRIGAÇÃO AUTOMATIZADA ESP32 + SERVIDOR VERCEL (SEM DEPENDÊNCIAS)
  =============================================================================
  Hardware Utilizado:
  - ESP32 NodeMCU
  - Módulo Relé (1 Canal) -> Aciona Mini Bomba D'água
  - Sensor de Umidade do Solo Capacitivo v1.2
  - Sensor DHT22 (ou DHT11) -> Temp & Umidade do Ar
  - Suporte 4x Baterias 18650 + Painel Solar
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// =============================================================================
// 1. CONFIGURAÇÕES DE REDE E SERVIDOR
// =============================================================================
const char* WIFI_SSID     = "ACERJS 9417";
const char* WIFI_PASSWORD = "149oF8@3";

// URL de produção do seu servidor Vercel:
const char* VERCEL_API_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";

// =============================================================================
// 2. PINAGEM DO HARDWARE
// =============================================================================
#define PIN_RELE         26  // Pino Digital ligado ao IN do Relé (Bomba d'água)
#define PIN_SOIL_ANALOG  34  // Pino Analógico ADC ligado ao Sensor Capacitivo v1.2
#define PIN_DHT          4   // Pino Digital ligado ao Data do DHT22/DHT11
#define PIN_BATTERY      35  // Pino Analógico ADC ligado ao medidor de tensão da bateria

#define DHTTYPE          DHT22 // Altere para DHT11 se estiver usando o sensor azul

// Intervalo entre leituras no modo contínuo (em milissegundos)
const unsigned long INTERVALO_LEITURA_MS = 15000; // 15 segundos

// Calibração do Sensor Capacitivo v1.2 (Ajustar conforme medições reais no ar e na água)
const int ADC_SOLO_SECO    = 3000; // Leitura do sensor totalmente no AR (0%)
const int ADC_SOLO_MOLHADO = 1200; // Leitura do sensor imerso na ÁGUA (100%)

// =============================================================================
// 3. OBJETOS E VARIÁVEIS GLOBAIS
// =============================================================================
DHT dht(PIN_DHT, DHTTYPE);

unsigned long ultimaChecagem = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=============================================");
  Serial.println("  Iniciando Irrigador Automatizado ESP32");
  Serial.println("=============================================");

  // Configuração SEGURA do Pino do Relé (Garantir que a bomba inicie DESLIGADA)
  // A maioria dos módulos relé aciona em Nível LÓGICO BAIXO (Active Low).
  // Portanto, colocar HIGH mantém a bomba desligada na inicialização.
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, HIGH);

  // Inicializa sensores
  dht.begin();

  // Conexão Wi-Fi
  conectarWiFi();
}

void loop() {
  // Mantém a rede conectada
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  unsigned long agora = millis();
  if (agora - ultimaChecagem >= INTERVALO_LEITURA_MS || ultimaChecagem == 0) {
    ultimaChecagem = agora;
    executarCicloTelemetriaEControle();
  }
}

// =============================================================================
// FUNÇÕES AUXILIARES
// =============================================================================

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
    Serial.print("❌ Falha na conexão Wi-Fi. Status: ");
    Serial.println(WiFi.status());
  }
}

int lerUmidadeSoloPercentual(int &rawAnalog) {
  rawAnalog = analogRead(PIN_SOIL_ANALOG);
  if (rawAnalog <= 0) return 0; // Sensor desconectado (ADC 0)
  
  // Mapeia valor analógico para 0 - 100%
  int pct = map(rawAnalog, ADC_SOLO_SECO, ADC_SOLO_MOLHADO, 0, 100);
  return constrain(pct, 0, 100);
}

float lerTensaoBateria() {
  int rawBat = analogRead(PIN_BATTERY);
  if (rawBat <= 0) return 0.0; // Pinos desconectados retornam 0.0V
  
  // Leitura ADC de 3.3V com divisor de tensão
  float volts = (rawBat / 4095.0) * 3.3 * 2.0; 
  return volts;
}

void acionarBombaDagua(int duracaoSegundos) {
  Serial.print("🚰 LIGANDO MINI BOMBA D'ÁGUA POR ");
  Serial.print(duracaoSegundos);
  Serial.println(" SEGUNDOS...");

  // Ligar o relé (Geralmente acionado com LOW)
  digitalWrite(PIN_RELE, LOW);

  // Aguarda o tempo de rega configurado
  delay(duracaoSegundos * 1000);

  // Desligar o relé (HIGH para desligar)
  digitalWrite(PIN_RELE, HIGH);
  Serial.println("🛑 MINI BOMBA D'ÁGUA DESLIGADA!");
}

void executarCicloTelemetriaEControle() {
  int rawSoil = 0;
  int umidadeSoloPct = lerUmidadeSoloPercentual(rawSoil);
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  float vBat = lerTensaoBateria();

  if (isnan(temp)) temp = 0.0; // Se o sensor DHT não estiver ligado, retorna 0.0
  if (isnan(hum))  hum  = 0.0;

  Serial.println("\n--- [Leitura dos Sensores] ---");
  Serial.printf("Umidade do Solo: %d%% (RAW ADC: %d)\n", umidadeSoloPct, rawSoil);
  Serial.printf("Temperatura: %.1f °C | Umidade Ar: %.1f %%\n", temp, hum);
  Serial.printf("Tensão Bateria: %.2f V\n", vBat);

  // Enviar dados para a API no Vercel
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Ignora validação de certificado SSL no ESP32 para simplificar

    HTTPClient http;
    http.begin(client, VERCEL_API_URL);
    http.addHeader("Content-Type", "application/json");

    // Montar JSON nativo (Sem depender de biblioteca externa ArduinoJson)
    char requestBody[256];
    snprintf(requestBody, sizeof(requestBody),
      "{\"soilMoisture\":%d,\"rawAnalog\":%d,\"temperature\":%.1f,\"humidity\":%.1f,\"batteryVoltage\":%.2f}",
      umidadeSoloPct, rawSoil, temp, hum, vBat);

    Serial.println("Enviando telemetria para o Vercel...");
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("Resposta do Servidor HTTP %d:\n", httpResponseCode);
      Serial.println(response);

      // Verifica se o servidor Vercel solicitou rega
      if (response.indexOf("\"waterRequested\":true") != -1 || response.indexOf("\"waterRequested\": true") != -1) {
        int durationSec = 10;
        int idx = response.indexOf("\"durationSec\":");
        if (idx != -1) {
          int d = response.substring(idx + 14).toInt();
          if (d > 0) durationSec = d;
        }

        acionarBombaDagua(durationSec);

        // Notificar o servidor Vercel que a rega foi concluída
        http.POST("{\"waterCompleted\":true}");
      }
    } else {
      Serial.printf("❌ Erro no envio HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
}
