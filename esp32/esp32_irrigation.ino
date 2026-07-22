/*
  =============================================================================
  PROJETO DE IRRIGAÇÃO AUTOMATIZADA ESP32 + SERVIDOR VERCEL
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
#include <ArduinoJson.h> // Instalar biblioteca ArduinoJson via Library Manager
#include <DHT.h>

// Credenciais Wi-Fi configuradas para 2.4 GHz (Compatível com ESP32):
const char* WIFI_SSID     = "AP104-2.4G";
const char* WIFI_PASSWORD = "papagaio";

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
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Conectado com sucesso!");
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Falha ao conectar ao Wi-Fi. Tentando no próximo ciclo.");
  }
}

int lerUmidadeSoloPercentual(int &rawAnalog) {
  rawAnalog = analogRead(PIN_SOIL_ANALOG);
  // Mapeia valor analógico para 0 - 100%
  int pct = map(rawAnalog, ADC_SOLO_SECO, ADC_SOLO_MOLHADO, 0, 100);
  return constrain(pct, 0, 100);
}

float lerTensaoBateria() {
  int rawBat = analogRead(PIN_BATTERY);
  // Leitura ADC de 3.3V com divisor de tensão
  float volts = (rawBat / 4095.0) * 3.3 * 2.0; 
  if (volts < 0.1) volts = 4.12; // Valor simulado caso pino esteja desconectado
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

  if (isnan(temp)) temp = 25.0; // Fallback caso ocorra falha de leitura pontual
  if (isnan(hum))  hum  = 60.0;

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

    // Montar JSON de envio
    StaticJsonDocument<256> docEnvio;
    docEnvio["soilMoisture"]  = umidadeSoloPct;
    docEnvio["rawAnalog"]     = rawSoil;
    docEnvio["temperature"]   = temp;
    docEnvio["humidity"]      = hum;
    docEnvio["batteryVoltage"]= vBat;

    String requestBody;
    serializeJson(docEnvio, requestBody);

    Serial.println("Enviando telemetria para o Vercel...");
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("Resposta do Servidor HTTP %d:\n", httpResponseCode);
      Serial.println(response);

      // Parse da resposta do servidor
      StaticJsonDocument<256> docResposta;
      DeserializationError err = deserializeJson(docResposta, response);

      if (!err) {
        bool waterRequested = docResposta["waterRequested"];
        int durationSec     = docResposta["durationSec"] | 10;

        // Se o painel Web solicitou rega:
        if (waterRequested) {
          acionarBombaDagua(durationSec);

          // Notificar o servidor Vercel que a rega foi concluída
          StaticJsonDocument<128> docConfirmacao;
          docConfirmacao["waterCompleted"] = true;
          String confirmBody;
          serializeJson(docConfirmacao, confirmBody);

          http.POST(confirmBody);
        }
      }
    } else {
      Serial.printf("❌ Erro no envio HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
}
