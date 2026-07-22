#include <WiFi.h>
#include "nvs_flash.h" // Para limpar a NVS completamente

// COLOQUE AQUI a senha EXATA da sua rede AP104-2.4G
// Confirme com alguem que tenha acesso ao roteador
const char* ssid     = "ACERJS 9417";
const char* password = "149oF8@3";

void imprimirBytes(const char* label, const char* str) {
  Serial.print(label);
  Serial.print(" [");
  Serial.print(str);
  Serial.print("] len=");
  Serial.print(strlen(str));
  Serial.print(" bytes: ");
  for (int i = 0; str[i] != '\0'; i++) {
    Serial.printf("%02X ", (unsigned char)str[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n============================================");
  Serial.println("  DIAGNÓSTICO DE CREDENCIAIS WI-FI ESP32");
  Serial.println("============================================");

  // 1. VERIFICAR BYTES EXATOS DO SSID E SENHA
  Serial.println("\n[1] Verificando bytes exatos das credenciais:");
  imprimirBytes("   SSID:  ", ssid);
  imprimirBytes("   SENHA: ", password);
  
  // '149oF8@3' correto deve ser:
  // 1=31 4=34 9=39 o=6F F=46 8=38 @=40 3=33
  Serial.println("   SENHA esperada para '149oF8@3':");
  Serial.println("   31 34 39 6F 46 38 40 33");

  // 2. APAGAR COMPLETAMENTE A NVS DO ESP32 (memória de configuração interna)
  Serial.println("\n[2] Apagando NVS Flash completa do ESP32...");
  esp_err_t err = nvs_flash_erase();
  if (err == ESP_OK) {
    Serial.println("   ✅ NVS apagada com sucesso!");
  } else {
    Serial.printf("   ⚠️  Erro ao apagar NVS: %d\n", err);
  }
  nvs_flash_init();

  // 3. TENTAR CONEXÃO COM VERBOSE DEBUG ATIVADO
  Serial.println("\n[3] Tentando conexão após limpeza total...");
  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
  WiFi.begin(ssid, password);

  long resultado = WiFi.waitForConnectResult(25000);

  if (resultado == WL_CONNECTED) {
    Serial.println("\n✅ CONECTADO COM SUCESSO!");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.printf("\n❌ FALHOU: Status = %ld\n", resultado);
    Serial.println("\n=========================================");
    Serial.println("  POR FAVOR VERIFIQUE NO SEU ROTEADOR:");
    Serial.println("=========================================");
    Serial.println("  1. A rede AP104-2.4G tem a mesma senha");
    Serial.println("     que a AP104-5G? Pode ser diferente!");
    Serial.println("  2. O roteador tem filtro de MAC ativo?");
    Serial.print("     MAC do seu ESP32: ");
    Serial.println(WiFi.macAddress());
    Serial.println("  3. O roteador está em modo WPA3? O ESP32");
    Serial.println("     pode ter problemas com WPA3.");
    Serial.println("=========================================");
  }
}

void loop() {}
