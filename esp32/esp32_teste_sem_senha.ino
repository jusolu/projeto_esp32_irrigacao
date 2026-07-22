#include <WiFi.h>
#include "nvs_flash.h"

// ============================================================
// TESTE SEM SENHA - Para isolar se o problema é na autenticação
// No Windows Hotspot: Editar -> apague a senha -> Salvar
// ============================================================
const char* ssid = "ACERJS 9417"; // Mesmo nome, SEM senha

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n======================================");
  Serial.println("  TESTE DE REDE ABERTA (SEM SENHA)");
  Serial.println("======================================");
  Serial.println("  Certifique-se que o Hotspot do Windows");
  Serial.println("  está configurado SEM senha antes de testar.");
  Serial.println("======================================\n");

  // Limpar NVS para garantir estado limpo
  nvs_flash_erase();
  nvs_flash_init();

  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  Serial.printf("Conectando em '%s' (sem senha)...\n", ssid);
  WiFi.begin(ssid); // Sem password = rede aberta

  long resultado = WiFi.waitForConnectResult(20000);

  if (resultado == WL_CONNECTED) {
    Serial.println("\n✅ CONECTOU EM REDE ABERTA!");
    Serial.println("   → O problema ERA a autenticação WPA2.");
    Serial.println("   → Tente mudar a segurança do Hotspot para WPA2 e testar novamente.");
    Serial.print("   IP:  "); Serial.println(WiFi.localIP());
    Serial.print("   MAC: "); Serial.println(WiFi.macAddress());
  } else {
    Serial.printf("\n❌ FALHOU mesmo sem senha! Status = %ld\n", resultado);
    Serial.println("   → O problema NÃO é a senha.");
    Serial.println("   → Possíveis causas:");
    Serial.println("     a) Hotspot do Windows bloqueando por MAC");
    Serial.println("     b) Firewall do Windows bloqueando");
    Serial.println("     c) Limite de dispositivos no Hotspot atingido");
    Serial.println("     d) Bug no driver Wi-Fi do ESP32 com este Hotspot");
    Serial.println("\n   → SOLUÇÃO: Tente conectar usando o celular como Hotspot");
    Serial.println("     em vez do notebook Windows.");
    Serial.print("\n   MAC do ESP32: "); Serial.println(WiFi.macAddress());
  }
}

void loop() {}
