#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>

// ============================================================
// TESTE DE VERSÃO + DIAGNÓSTICO COMPLETO DE HARDWARE
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n========================================");
  Serial.println("  ESP32 - INFO DE VERSÃO E HARDWARE");
  Serial.println("========================================");

  // 1. Versões do SDK e Core
  Serial.println("\n[1] Versões do firmware:");
  Serial.printf("   ESP-IDF (SDK) version: %s\n", esp_get_idf_version());
  Serial.printf("   Arduino Core: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  Serial.printf("   Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("   CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("   Flash Size: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
  Serial.printf("   RAM Free: %d bytes\n", ESP.getFreeHeap());

  // 2. Informações de Wi-Fi
  Serial.println("\n[2] Info de Wi-Fi:");
  WiFi.mode(WIFI_STA);
  delay(100);
  Serial.printf("   MAC Address: %s\n", WiFi.macAddress().c_str());

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("   MAC (esp_wifi): %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // 3. Scan para verificar rádio
  Serial.println("\n[3] Scan rápido para confirmar rádio OK:");
  int n = WiFi.scanNetworks(false, true); // false=blocking, true=show hidden
  Serial.printf("   Redes encontradas: %d\n", n);
  for (int i = 0; i < min(n, 5); i++) {
    Serial.printf("   [%d] %-20s Ch%2d %4d dBm\n",
                  i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
  }

  // 4. Teste de associação com log detalhado
  Serial.println("\n[4] Tentativa de conexão com logs detalhados:");
  Serial.println("   (Veja se aparece alguma mensagem de erro do sistema)");
  
  WiFi.disconnect(true, true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); // Desativa auto-reconexão que pode causar conflito
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Força potência máxima de transmissão

  // Tenta com hotspot do notebook
  const char* ssid = "ACERJS 9417";
  const char* pass = "149oF8@3";

  Serial.printf("   Conectando em '%s'...\n", ssid);
  WiFi.begin(ssid, pass);

  // Monitora o status a cada 500ms por 20 segundos
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    tentativas++;
    wl_status_t status = WiFi.status();
    Serial.printf("   t=%ds status=%d", tentativas/2, status);
    switch(status) {
      case WL_IDLE_STATUS:     Serial.println(" (IDLE)"); break;
      case WL_NO_SSID_AVAIL:   Serial.println(" (NO SSID - rede sumiu!)"); break;
      case WL_CONNECT_FAILED:  Serial.println(" (CONNECT_FAILED - SENHA ERRADA!)"); break;
      case WL_CONNECTION_LOST: Serial.println(" (CONNECTION_LOST)"); break;
      case WL_DISCONNECTED:    Serial.println(" (DISCONNECTED)"); break;
      default:                 Serial.println(" (outro)"); break;
    }
    if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED) break;
  }

  Serial.println("\n========================================");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("  ✅ CONECTADO!");
    Serial.print("  IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.printf("  ❌ Status final: %d\n", WiFi.status());
    Serial.println("\n  PRÓXIMOS PASSOS:");
    Serial.println("  1. Copie TODO o output acima e compartilhe");
    Serial.println("  2. No Arduino IDE: Ferramentas > Core Debug Level > Verbose");
    Serial.println("     Isso mostra erros internos do Wi-Fi driver");
    Serial.println("  3. Atualize o pacote ESP32 no Boards Manager para 2.0.17");
    Serial.println("     (versão mais estável para Wi-Fi)");
  }
  Serial.println("========================================");
}

void loop() {}
