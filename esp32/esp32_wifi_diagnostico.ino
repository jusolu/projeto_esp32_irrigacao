#include <WiFi.h>

// ============================================================
// DIAGNÓSTICO COMPLETO DE CONEXÃO WI-FI ESP32
// Testa 3 abordagens diferentes para identificar o problema
// ============================================================

const char* ssid     = "AP104-2.4G";
const char* password = "papagaio";

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=============================================");
  Serial.println("  ESP32 Wi-Fi - Diagnóstico Completo");
  Serial.println("=============================================");

  // -------------------------------------------------------
  // TESTE 1: Abordagem com waitForConnectResult()
  // (Mais confiável que loop manual de status)
  // -------------------------------------------------------
  Serial.println("\n[TESTE 1] Usando WiFi.waitForConnectResult()...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK); // Aceita WPA e WPA2
  WiFi.begin(ssid, password);

  Serial.print("Aguardando resultado...");
  long resultado = WiFi.waitForConnectResult(20000); // timeout de 20 segundos

  if (resultado == WL_CONNECTED) {
    Serial.println("\n✅ TESTE 1 PASSOU: Conectado!");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
    return;
  } else {
    Serial.printf("\n❌ TESTE 1 FALHOU: Resultado = %ld\n", resultado);
  }

  // -------------------------------------------------------
  // TESTE 2: Limpar NVS (memória flash do ESP32) e tentar
  // (Resolve problema de credenciais corrompidas em memória)
  // -------------------------------------------------------
  Serial.println("\n[TESTE 2] Limpando Flash (NVS) e tentando novamente...");
  WiFi.disconnect(true, true); // true, true = desconecta E apaga credenciais salvas na flash
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
  WiFi.begin(ssid, password);

  resultado = WiFi.waitForConnectResult(25000);
  if (resultado == WL_CONNECTED) {
    Serial.println("\n✅ TESTE 2 PASSOU: Conectado após limpar NVS!");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
    return;
  } else {
    Serial.printf("\n❌ TESTE 2 FALHOU: Resultado = %ld\n", resultado);
  }

  // -------------------------------------------------------
  // TESTE 3: Reiniciar o ESP32 completamente e tentar
  // (Resolve problema de estado corrompido na RAM)
  // -------------------------------------------------------
  Serial.println("\n[TESTE 3] Reiniciando ESP32 e tentando...");
  delay(1000);

  // Salva as credenciais e manda reiniciar o chip
  WiFi.persistent(true); // DESTA VEZ salva na flash antes de reiniciar
  WiFi.begin(ssid, password);
  delay(500);

  Serial.println("Reiniciando ESP32 em 3 segundos...");
  delay(3000);
  ESP.restart();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ ESP32 Conectado após reinicialização!");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
    delay(5000);
  }
}
