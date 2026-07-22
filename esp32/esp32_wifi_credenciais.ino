#include <WiFi.h>
#include "nvs_flash.h"

// ============================================================
// Credenciais do Hotspot do Notebook
// ============================================================
const char* ssid     = "ACERJS 9417";
const char* password = "149oF8@3";

// Canais 1-13 = 2.4GHz | Canais 36+ = 5GHz
// O ESP32 SÓ SUPORTA 2.4GHz!
String getBand(int32_t canal) {
  if (canal >= 1 && canal <= 14)  return "2.4GHz ✅";
  if (canal >= 36)                return "5GHz   ❌ ESP32 NAO SUPORTA!";
  return "Desconhecida";
}

void imprimirBytes(const char* label, const char* str) {
  Serial.printf("%s [%s] len=%d | bytes: ", label, str, strlen(str));
  for (int i = 0; str[i] != '\0'; i++) {
    Serial.printf("%02X ", (unsigned char)str[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n============================================");
  Serial.println("  ESP32 Wi-Fi - SCAN + DIAGNÓSTICO COMPLETO");
  Serial.println("============================================");

  // -----------------------------------------------------------
  // PASSO 1: VERIFICAR BYTES DAS CREDENCIAIS
  // -----------------------------------------------------------
  Serial.println("\n[PASSO 1] Verificando credenciais digitadas:");
  imprimirBytes("   SSID: ", ssid);
  imprimirBytes("   PASS: ", password);

  // -----------------------------------------------------------
  // PASSO 2: ESCANEAR REDES E MOSTRAR SE SÃO 2.4GHz OU 5GHz
  // -----------------------------------------------------------
  Serial.println("\n[PASSO 2] Escaneando redes Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
  Serial.printf("   Encontradas %d redes:\n\n", n);

  bool ssidEncontrado   = false;
  bool ssidEh24GHz      = false;

  for (int i = 0; i < n; i++) {
    String nome   = WiFi.SSID(i);
    int32_t canal = WiFi.channel(i);
    int32_t rssi  = WiFi.RSSI(i);
    String banda  = getBand(canal);

    Serial.printf("   [%02d] %-24s | Ch %2d | %4d dBm | %s\n",
                  i + 1,
                  nome.c_str(),
                  canal,
                  rssi,
                  banda.c_str());

    if (nome == ssid) {
      ssidEncontrado = true;
      ssidEh24GHz    = (canal >= 1 && canal <= 14);
    }
  }

  // -----------------------------------------------------------
  // PASSO 3: DIAGNÓSTICO DO SSID ALVO
  // -----------------------------------------------------------
  Serial.println("\n[PASSO 3] Diagnóstico do SSID alvo:");
  if (!ssidEncontrado) {
    Serial.printf("   ❌ A rede '%s' NAO FOI ENCONTRADA!\n", ssid);
    Serial.println("      → Verifique se o Hotspot está ligado.");
    Serial.println("      → O SSID tem algum espaço extra? (veja bytes acima)");
  } else if (!ssidEh24GHz) {
    Serial.printf("   ⚠️  REDE '%s' ENCONTRADA, MAS ESTÁ EM 5GHz!\n", ssid);
    Serial.println("      → O ESP32 SÓ FUNCIONA EM 2.4GHz!");
    Serial.println("      → No Hotspot do Windows: vá em 'Editar'");
    Serial.println("        e mude a BANDA para '2,4 GHz'.");
    Serial.println("        (Configurações > Rede > Hotspot Móvel > Editar)");
  } else {
    Serial.printf("   ✅ Rede '%s' encontrada em 2.4GHz! Tentando conectar...\n", ssid);
  }

  if (!ssidEncontrado) return; // Não tenta conectar se não achou a rede

  // -----------------------------------------------------------
  // PASSO 4: LIMPAR NVS E CONECTAR
  // -----------------------------------------------------------
  Serial.println("\n[PASSO 4] Limpando NVS e conectando...");
  nvs_flash_erase();
  nvs_flash_init();

  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
  WiFi.begin(ssid, password);

  long resultado = WiFi.waitForConnectResult(25000);

  Serial.println("\n============================================");
  if (resultado == WL_CONNECTED) {
    Serial.println("  ✅ CONECTADO COM SUCESSO!");
    Serial.print("     IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("     MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("     RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.printf("  ❌ FALHOU: Status = %ld\n", resultado);
    Serial.println("  Codigos de status:");
    Serial.println("  0=IDLE  1=NO SSID  3=CONNECTED");
    Serial.println("  4=CONNECT FAILED (senha errada!)");
    Serial.println("  6=DISCONNECTED  255=NO SHIELD");
    Serial.print("\n  MAC do ESP32: ");
    Serial.println(WiFi.macAddress());
    Serial.println("\n  Possíveis causas:");
    Serial.println("  - Senha errada (status 4 = certeza de senha errada)");
    Serial.println("  - Roteador com filtro de MAC ativo");
    Serial.println("  - Rede em 5GHz (veja PASSO 2 acima)");
  }
  Serial.println("============================================");
}

void loop() {}
