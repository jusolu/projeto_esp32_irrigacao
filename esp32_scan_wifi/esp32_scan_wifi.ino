/*
  =============================================================================
  SCANNER DE REDES WI-FI 2.4GHz PARA ESP32 (ARDUINO IDE)
  =============================================================================
  Este código escaneia e mostra no Monitor Serial todas as redes Wi-Fi 2.4GHz
  visíveis ao ESP32 com o nome exato (SSID), sinal (RSSI) e tipo de segurança.
  =============================================================================
*/

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("  ESP32 - SCANNER DE REDES WI-FI (2.4GHz)");
  Serial.println("=======================================================");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Escaneando redes Wi-Fi ao redor...\n");

  int n = WiFi.scanNetworks();

  Serial.println("=======================================================");
  if (n == 0) {
    Serial.println("❌ Nenhuma rede Wi-Fi 2.4GHz encontrada!");
  } else {
    Serial.printf("🎉 Encontradas %d redes Wi-Fi 2.4GHz:\n\n", n);
    for (int i = 0; i < n; ++i) {
      Serial.printf(" [%2d] SSID: '%s' | Sinal: %d dBm | Canal: %d | Seguranca: %s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "ABERTA" : "WPA2/WPA3");
      delay(10);
    }
  }
  Serial.println("=======================================================\n");
}

void loop() {
  // Nada no loop
}
