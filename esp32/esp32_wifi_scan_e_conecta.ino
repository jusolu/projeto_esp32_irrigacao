#include <WiFi.h>

const char* ssid     = "AP104-2.4G";
const char* password = "papagaio";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================================");
  Serial.println("  ESP32: Scan & Conexão Wi-Fi (Fix ScanDelete)");
  Serial.println("=============================================");

  // 1. PASSO DE ESCANEAMENTO DAS REDES EM ALCANCE
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("🔍 1. Escaneando redes Wi-Fi disponíveis ao redor...");
  int n = WiFi.scanNetworks();
  Serial.println("Escaneamento concluído!");

  if (n == 0) {
    Serial.println("❌ Nenhuma rede encontrada.");
  } else {
    Serial.printf("✅ Encontradas %d redes no alcance:\n", n);
    for (int i = 0; i < n; ++i) {
      Serial.printf("  [%d] SSID: %-25s | Sinal: %d dBm | Canal: %d\n", 
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
      delay(10);
    }
  }

  // OBRIGATÓRIO NA DOCUMENTAÇÃO DO ESP32:
  // Apagar o cache do escaneamento e resetar o rádio antes de chamar WiFi.begin()
  WiFi.scanDelete();
  delay(500);

  // 2. PASSO DE CONEXÃO COM A SUA REDE
  Serial.println("\n🌐 2. Tentando conectar a:");
  Serial.printf("   SSID:  '%s'\n", ssid);
  Serial.printf("   Senha: '%s'\n", password);

  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(300);

  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // Garantia explícita do modo estação ativo antes de iniciar a conexão
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("🎉 CONECTADO COM SUCESSO NA REDE WI-FI!");
    Serial.print("   Endereço IP do ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ FALHA AO CONECTAR NA REDE.");
    Serial.printf("   Código de Status Wi-Fi: %d\n", WiFi.status());
  }
}

void loop() {
  // Nada no loop no exemplo de teste
}
