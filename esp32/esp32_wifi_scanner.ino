/*
  =============================================================================
  ESP32 WI-FI SCANNER DE REDES (DIAGNÓSTICO DE SSIDS EM ALCANCE)
  =============================================================================
  Este código escaneia todas as redes Wi-Fi de 2.4 GHz no alcance do ESP32
  e imprime no Monitor Serial o nome exato (SSID), o canal e a força do sinal.
  =============================================================================
*/

#include "WiFi.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configura Wi-Fi no modo Estação e desconecta de redes salvas
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("\n=============================================");
  Serial.println("  ESP32 Wi-Fi Network Scanner Iniciado");
  Serial.println("=============================================");
}

void loop() {
  Serial.println("\n🔍 Escaneando redes Wi-Fi disponíveis...");

  // WiFi.scanNetworks retorna o número de redes encontradas
  int n = WiFi.scanNetworks();
  Serial.println("Escaneamento concluído!");

  if (n == 0) {
    Serial.println("❌ Nenhuma rede Wi-Fi encontrada no alcance.");
  } else {
    Serial.printf("✅ Encontradas %d redes Wi-Fi:\n", n);
    Serial.println("------------------------------------------------------------------");
    Serial.printf("%-4s | %-32s | %-6s | %-8s | %-12s\n", "Nº", "Nome da Rede (SSID)", "Sinal", "Canal", "Criptografia");
    Serial.println("------------------------------------------------------------------");

    for (int i = 0; i < n; ++i) {
      // Imprime cada rede encontrada
      String encType;
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN: encType = "Aberta"; break;
        case WIFI_AUTH_WEP: encType = "WEP"; break;
        case WIFI_AUTH_WPA_PSK: encType = "WPA"; break;
        case WIFI_AUTH_WPA2_PSK: encType = "WPA2"; break;
        case WIFI_AUTH_WPA_WPA2_PSK: encType = "WPA/WPA2"; break;
        case WIFI_AUTH_WPA2_ENTERPRISE: encType = "WPA2-Ent"; break;
        case WIFI_AUTH_WPA3_PSK: encType = "WPA3"; break;
        default: encType = "Outra"; break;
      }

      Serial.printf("%-4d | %-32s | %-4d dBm | Ch %-4d | %-12s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    encType.c_str());
      delay(10);
    }
    Serial.println("------------------------------------------------------------------");
  }

  // Apaga o cache de redes escaneadas
  WiFi.scanDelete();

  // Aguarda 10 segundos antes de escanear novamente
  Serial.println("Aguardando 10 segundos para o próximo escaneamento...");
  delay(10000);
}
