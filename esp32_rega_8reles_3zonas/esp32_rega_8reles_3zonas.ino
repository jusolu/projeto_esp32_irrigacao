/*
  =============================================================================
  SISTEMA DE IRRIGAÇÃO AUTOMATIZADO - PLACA 8 RELÉS (3 SETORES + 1 BOMBA)
  =============================================================================
  - Hardware:
      * Relé 1 (Bomba d'água Principal) ➔ GPIO 32 (Active-LOW)
      * Relé 2 (Válvula Setor 1)         ➔ GPIO 33 (Active-LOW)
      * Relé 3 (Válvula Setor 2)         ➔ GPIO 25 (Active-LOW)
      * Relé 4 (Válvula Setor 3)         ➔ GPIO 26 (Active-LOW)
      * Relés 5 a 8 (Reservas)           ➔ GPIO 27, 14, 13, 4
      * Módulo RTC DS3231                ➔ I2C (SDA = GPIO 21, SCL = GPIO 22)
      * LED Azul de Status               ➔ GPIO 2
  - Funcionamento Sequencial por Setores (Transição Contínua com Overlap):
      1. Ativa Solenoide 1 (D33).
      2. Aguarda 3s (desobstrução da linha).
      3. Ativa a Bomba d'água Principal (D32).
      4. Irriga o Setor 1 por 15s.
      5. Ativa a Solenoide 2 (D25).
      6. Aguarda 3s (overlap seguro anti-golpe de aríete / sem estrangular a bomba).
      7. Desativa a Solenoide 1 (D33).
      8. Irriga o Setor 2 por 15s.
      9. Ativa a Solenoide 3 (D26).
      10. Aguarda 3s (overlap seguro).
      11. Desativa a Solenoide 2 (D25).
      12. Irriga o Setor 3 por 15s.
      13. Desativa a Bomba d'água Principal (D32).
      14. Aguarda 3s (cessar fluxo e drenar pressão).
      15. Desativa a Solenoide 3 (D26).
  - Grade Oficial (13 Regas Diurnas):
      06:30, 07:30, 08:30, 09:00, 10:00, 11:00, 12:00, 13:00, 14:00, 15:00, 16:00, 17:00, 18:00
  - Duração:
      * 60 segundos (1 minuto) por setor nas regas programadas (Total = 180s)
      * 15 segundos por setor na rega inicial de ativação/boot (Total = 45s)
      * 3 segundos entre ativações e desativações (overlap anti-golpe de aríete)
  - Standby Ativo com Relógio RTC DS3231 (Baixo consumo sem derrubar porta USB)
  =============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =============================================================================
// MAPEAMENTO DOS PINOS DOS RELÉS (PLACA DE 8 RELÉS)
// =============================================================================
#define PIN_BOMBA         32  // Relé 1: Bomba d'água principal
#define PIN_SOLENOIDE_1   33  // Relé 2: Solenoide Setor 1
#define PIN_SOLENOIDE_2   25  // Relé 3: Solenoide Setor 2
#define PIN_SOLENOIDE_3   26  // Relé 4: Solenoide Setor 3

// Relés sobressalentes da placa (já mapeados e mantidos em corte)
#define PIN_RELE_5        27  // Relé 5
#define PIN_RELE_6        14  // Relé 6
#define PIN_RELE_7        13  // Relé 7
#define PIN_RELE_8         4  // Relé 8

#define LED_AZUL           2  // LED Azul de Status

// Pinos I2C do Relógio RTC DS3231
#define I2C_SDA           21
#define I2C_SCL           22
#define RTC_I2C_ADDRESS   0x68

// Lógica dos Relés (Active-HIGH: HIGH = Liga / LOW = Desliga)
const int RELE_LIGADO    = HIGH;
const int RELE_DESLIGADO = LOW;

// Lista com todos os 8 pinos para travamento e inicialização
const int PINOS_TODOS_RELES[] = {
  PIN_BOMBA, PIN_SOLENOIDE_1, PIN_SOLENOIDE_2, PIN_SOLENOIDE_3,
  PIN_RELE_5, PIN_RELE_6, PIN_RELE_7, PIN_RELE_8
};
const int TOTAL_PINOS_RELES = sizeof(PINOS_TODOS_RELES) / sizeof(PINOS_TODOS_RELES[0]);

// =============================================================================
// CONFIGURAÇÕES DE DURAÇÃO DA REGA
// =============================================================================
#define DURACAO_SETOR_PADRAO_SEC 60 // 60s (1 minuto) por setor nas regas da grade
#define DURACAO_SETOR_BOOT_SEC   15 // 15s por setor na rega inicial de ativação/boot
#define TEMPO_TRANSICAO_SEC       3 // 3s entre ativações e desativações

// Configurações Wi-Fi e Vercel
const char* WIFI_SSID       = "AP104-2.4G  "; // Dois espaços no final
const char* WIFI_PASSWORD   = "papagaio";
const char* VERCEL_POST_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";

// Modo Emergência se RTC falhar: 8 Horas (28800 segundos)
const uint64_t SEGUNDOS_EMERGENCIA_8H = 28800ULL;

// Variáveis na memória RTC persistente durante o Deep Sleep
RTC_DATA_ATTR int cicloRega = 1;
RTC_DATA_ATTR uint32_t epochTimePersistente = 0;
RTC_DATA_ATTR uint64_t ultimoSegundosSono = 0;

uint8_t decToBcd(uint8_t val) { return ((val / 10 * 16) + (val % 10)); }
uint8_t bcdToDec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }

void setRTCDateTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day, uint8_t month, uint16_t year) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(sec));
  Wire.write(decToBcd(min));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(1));
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year - 2000));
  Wire.endTransmission();
}

void ajustarRTCDataCompilacao() {
  const char* meses[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char mesStr[4];
  int dia = 1, ano = 2026, hora = 0, min = 0, seg = 0;
  sscanf(__DATE__, "%s %d %d", mesStr, &dia, &ano);
  sscanf(__TIME__, "%d:%d:%d", &hora, &min, &seg);
  
  int mes = 1;
  for (int i = 0; i < 12; i++) {
    if (strncmp(mesStr, meses[i], 3) == 0) {
      mes = i + 1;
      break;
    }
  }
  setRTCDateTime(seg, min, hora, dia, mes, ano);
  Serial.printf("⏰ [RTC] Relógio inicializado com a data de compilação: %02d/%02d/%04d %02d:%02d:%02d\n",
                dia, mes, ano, hora, min, seg);
}

bool lerHoraRTC(int &seg, int &min, int &hora, int &dia, int &mes, int &ano) {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  byte error = Wire.endTransmission();
  if (error != 0) {
    Serial.printf("⚠️ [I2C] Nenhum dispositivo respondeu no endereço 0x%02X (Erro I2C: %d)\n", RTC_I2C_ADDRESS, error);
    return false;
  }

  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  if (Wire.available() >= 7) {
    seg  = bcdToDec(Wire.read() & 0x7F);
    min  = bcdToDec(Wire.read());
    hora = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); // dia da semana
    dia  = bcdToDec(Wire.read());
    mes  = bcdToDec(Wire.read());
    ano  = bcdToDec(Wire.read()) + 2000;
    
    if (ano >= 2026 && ano <= 2035 && mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31) {
      return true;
    } else {
      Serial.printf("⚠️ [RTC] Módulo detectado na I2C sem hora gravada (%02d/%02d/%04d). Calibrando automaticamente...\n", dia, mes, ano);
      ajustarRTCDataCompilacao();
      delay(50);
      Wire.beginTransmission(RTC_I2C_ADDRESS);
      Wire.write(0x00);
      Wire.endTransmission();
      Wire.requestFrom(RTC_I2C_ADDRESS, 7);
      if (Wire.available() >= 7) {
        seg  = bcdToDec(Wire.read() & 0x7F);
        min  = bcdToDec(Wire.read());
        hora = bcdToDec(Wire.read() & 0x3F);
        Wire.read();
        dia  = bcdToDec(Wire.read());
        mes  = bcdToDec(Wire.read());
        ano  = bcdToDec(Wire.read()) + 2000;
        return (ano >= 2026 && ano <= 2035);
      }
      return false;
    }
  }
  return false;
}

bool conectarWiFiRobusto() {
  Serial.println("🌐 Conectando à rede Wi-Fi para telemetria...");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_17dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 60) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
    t++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_AZUL, HIGH);
    Serial.printf("✅ Wi-Fi Conectado com sucesso! (IP: %s)\n", WiFi.localIP().toString().c_str());
    return true;
  }
  
  digitalWrite(LED_AZUL, LOW);
  return false;
}

void reportarRegaParaVercel(int duracaoTotal, String horaFormatada, String motivo) {
  if (!conectarWiFiRobusto()) {
    Serial.println("⚠️ Wi-Fi offline ou fora de alcance. Rega executada localmente pelo RTC!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(12000);
  http.begin(client, VERCEL_POST_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"waterCompleted\":true,\"durationSec\":" + String(duracaoTotal) + 
                   ",\"rtcTime\":\"" + horaFormatada + 
                   "\",\"sectors\":3,\"actuators\":\"Bomba (D32) + 3 Solenoides (D33, D25, D26)\"" +
                   ",\"source\":\"" + motivo + "\"}";
                   
  Serial.printf("📤 Enviando telemetria para Vercel: %s\n", payload.c_str());
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("✅ [Vercel] Enviado com Sucesso! (HTTP %d)\n", httpCode);
  } else {
    Serial.printf("❌ [Vercel] Erro HTTP: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void regarTempoComLed(int segundos) {
  unsigned long startMillis = millis();
  while (millis() - startMillis < ((unsigned long)segundos * 1000)) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
  }
}

// Execução Hidráulica Sequencial dos 3 Setores com Transição Contínua (Overlap)
void executarRegaCompletaTodosSetores(int duracaoPorSetor, String horaStr, String motivo) {
  Serial.println("\n=======================================================");
  Serial.printf("🚿 INICIANDO SEQUÊNCIA HIDRÁULICA (%s)\n", motivo.c_str());
  Serial.printf("   Setores ativos: 3 | Duração por setor: %d s | Intervalo transição: %d s\n", 
                duracaoPorSetor, TEMPO_TRANSICAO_SEC);
  Serial.println("=======================================================");

  // --- SETOR 1 ---
  Serial.println("\n🌱 >>> INICIANDO SETOR 1 (Solenoide 1 no pino D33) <<<");
  Serial.println("1️⃣ [PASSO 1] Ativando Solenoide 1 (D33)...");
  digitalWrite(PIN_SOLENOIDE_1, RELE_LIGADO);
  digitalWrite(LED_AZUL, HIGH);

  Serial.printf("⏳ Aguardando %d segundos para desobstrução e alívio da linha...\n", TEMPO_TRANSICAO_SEC);
  delay(TEMPO_TRANSICAO_SEC * 1000);

  Serial.printf("2️⃣ [PASSO 2] Ativando Bomba Principal (D32) por %d segundos...\n", duracaoPorSetor);
  digitalWrite(PIN_BOMBA, RELE_LIGADO);
  regarTempoComLed(duracaoPorSetor);

  // --- TRANSIÇÃO SETOR 1 -> SETOR 2 ---
  Serial.println("\n🌱 >>> TRANSIÇÃO PARA SETOR 2 (Solenoide 2 no pino D25) <<<");
  Serial.println("3️⃣ [PASSO 3] Ativando Solenoide 2 (D25)...");
  digitalWrite(PIN_SOLENOIDE_2, RELE_LIGADO);

  Serial.printf("⏳ Aguardando %d segundos com ambas abertas (overlap anti-golpe de aríete / bomba desobstruída)...\n", TEMPO_TRANSICAO_SEC);
  delay(TEMPO_TRANSICAO_SEC * 1000);

  Serial.println("4️⃣ [PASSO 4] Desativando Solenoide 1 (D33)...");
  digitalWrite(PIN_SOLENOIDE_1, RELE_DESLIGADO);

  Serial.printf("🚿 Regando Setor 2 por %d segundos...\n", duracaoPorSetor);
  regarTempoComLed(duracaoPorSetor);

  // --- TRANSIÇÃO SETOR 2 -> SETOR 3 ---
  Serial.println("\n🌱 >>> TRANSIÇÃO PARA SETOR 3 (Solenoide 3 no pino D26) <<<");
  Serial.println("5️⃣ [PASSO 5] Ativando Solenoide 3 (D26)...");
  digitalWrite(PIN_SOLENOIDE_3, RELE_LIGADO);

  Serial.printf("⏳ Aguardando %d segundos com ambas abertas (overlap anti-golpe de aríete)...\n", TEMPO_TRANSICAO_SEC);
  delay(TEMPO_TRANSICAO_SEC * 1000);

  Serial.println("6️⃣ [PASSO 6] Desativando Solenoide 2 (D25)...");
  digitalWrite(PIN_SOLENOIDE_2, RELE_DESLIGADO);

  Serial.printf("🚿 Regando Setor 3 por %d segundos...\n", duracaoPorSetor);
  regarTempoComLed(duracaoPorSetor);

  // --- FINALIZAÇÃO DO CICLO ---
  Serial.println("\n⏹️ >>> FINALIZANDO CICLO DE REGA COM SEGURANÇA <<<");
  Serial.println("7️⃣ [PASSO 7] Desativando Bomba Principal (D32)...");
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);

  Serial.printf("⏳ Aguardando %d segundos para cessar o fluxo e drenar pressão da tubulação...\n", TEMPO_TRANSICAO_SEC);
  delay(TEMPO_TRANSICAO_SEC * 1000);

  Serial.println("8️⃣ [PASSO 8] Desativando Solenoide 3 (D26)...");
  digitalWrite(PIN_SOLENOIDE_3, RELE_DESLIGADO);
  digitalWrite(LED_AZUL, LOW);

  // Garante corte de todos os 8 relés da placa
  for (int p : PINOS_TODOS_RELES) {
    digitalWrite(p, RELE_DESLIGADO);
  }

  Serial.println("\n🎉 Todos os 3 setores foram irrigados com máxima pressão e tubulação despressurizada!");

  // Telemetria Wi-Fi desativada na bancada para evitar pico de corrente do rádio na porta USB
  // reportarRegaParaVercel(duracaoPorSetor * 3, horaStr, motivo);
}

void executarJanelaOTA(int segundosLimit) {
  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(LED_AZUL, HIGH);
  
  Serial.println("\n=======================================================");
  Serial.printf("   📡 JANELA OTA DE %d SEGUNDOS ATIVADA NO BOOT\n", segundosLimit);
  Serial.println("=======================================================");
  
  for (int i = 0; i < 20; i++) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(50);
  }
  digitalWrite(LED_AZUL, LOW);
  
  Serial.println("🌐 Conectando à rede Wi-Fi para escutar OTA...");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 40) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
    t++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Conectado com sucesso!");
    Serial.print("   → IP do ESP32 para OTA: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_AZUL, HIGH);
  } else {
    Serial.println("\n⚠️ Wi-Fi não conectou para OTA. Pulando janela...");
    return;
  }

  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("ESP32-Irrigacao-8Reles");

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("\n[OTA] Iniciando gravação sem fio (" + type + ")...");
    for (int p : PINOS_TODOS_RELES) {
      digitalWrite(p, RELE_DESLIGADO);
    }
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Atualização concluída com sucesso! Reiniciando...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progresso: %u%%\r", (progress / (total / 100)));
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\n[OTA] Erro [%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("📡 Serviço OTA pronto. Aguardando gravação...");
  
  unsigned long startMillis = millis();
  unsigned long ultimoPrint = 0;
  unsigned long ultimoPisca = 0;

  while ((millis() - startMillis) < ((unsigned long)segundosLimit * 1000)) {
    ArduinoOTA.handle();
    
    // Suporte a ajuste de horário via serial
    if (Serial.available() > 0) {
      String msg = Serial.readStringUntil('\n');
      msg.trim();
      if (msg.startsWith("SET:")) {
        String dataHora = msg.substring(4);
        int dia_s = dataHora.substring(0, 2).toInt();
        int mes_s = dataHora.substring(3, 5).toInt();
        int ano_s = dataHora.substring(6, 10).toInt();
        int hora_s = dataHora.substring(11, 13).toInt();
        int min_s = dataHora.substring(14, 16).toInt();
        int seg_s = dataHora.substring(17, 19).toInt();
        if (ano_s >= 2026 && mes_s >= 1 && mes_s <= 12 && dia_s >= 1 && dia_s <= 31) {
          setRTCDateTime(seg_s, min_s, hora_s, dia_s, mes_s, ano_s);
          Serial.printf("✅ [RTC] Horário Gravado com Sucesso via Serial: %02d/%02d/%04d %02d:%02d:%02d\n",
                        dia_s, mes_s, ano_s, hora_s, min_s, seg_s);
        }
      }
    }

    if (millis() - ultimoPisca >= 300) {
      ultimoPisca = millis();
      digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    }

    if (millis() - ultimoPrint >= 5000) {
      ultimoPrint = millis();
      int restante = segundosLimit - (int)((millis() - startMillis) / 1000);
      Serial.printf("🛠️ [Janela OTA] Tempo restante: %d segundos...\n", restante);
    }
    delay(10);
  }

  Serial.println("\n⏰ Tempo da janela OTA esgotado. Desligando Wi-Fi e prosseguindo...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  digitalWrite(LED_AZUL, LOW);
  Serial.println("=======================================================");
}

// 13 Horários Oficiais de Rega Diurna (minutos desde a meia-noite):
// 06:30, 07:30, 08:30, 09:00, 10:00, 11:00, 12:00, 13:00, 14:00, 15:00, 16:00, 17:00, 18:00
const int HORARIOS_REGA[] = { 390, 450, 510, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1080 };
const int QTD_HORARIOS = sizeof(HORARIOS_REGA) / sizeof(HORARIOS_REGA[0]);

uint64_t calcularSegundosParaProximaRega(int hora, int min, int seg) {
  int atualMinutos = hora * 60 + min;
  int proximoMinutos = -1;

  for (int i = 0; i < QTD_HORARIOS; i++) {
    if (HORARIOS_REGA[i] > atualMinutos) {
      proximoMinutos = HORARIOS_REGA[i];
      break;
    }
  }

  int minutosAteProximo = 0;
  if (proximoMinutos != -1) {
    minutosAteProximo = proximoMinutos - atualMinutos;
  } else {
    // Passou das 19:30. Próximo é 07:30 da manhã seguinte
    minutosAteProximo = (1440 - atualMinutos) + HORARIOS_REGA[0];
  }

  uint64_t segundos = ((uint64_t)minutosAteProximo * 60ULL) - (uint64_t)seg;
  if (segundos < 60) {
    segundos = 60;
  }
  return segundos;
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  WiFi.mode(WIFI_OFF);
  btStop();

  // Libera pinos de qualquer hold de sleep anterior
  for (int p : PINOS_TODOS_RELES) {
    gpio_hold_dis((gpio_num_t)p);
    pinMode(p, OUTPUT);
    digitalWrite(p, RELE_DESLIGADO);
  }

  Serial.begin(115200);
  delay(300);

  // Inicializa barramento I2C nos pinos SDA=21 e SCL=22
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // Restaura hora persistente via RAM se o RTC oscilar
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER && epochTimePersistente > 0 && ultimoSegundosSono > 0) {
    epochTimePersistente += (uint32_t)ultimoSegundosSono;
    time_t t_calc = (time_t)epochTimePersistente;
    struct tm *tm_calc = localtime(&t_calc);
    if (tm_calc != NULL) {
      setRTCDateTime(tm_calc->tm_sec, tm_calc->tm_min, tm_calc->tm_hour, 
                     tm_calc->tm_mday, tm_calc->tm_mon + 1, tm_calc->tm_year + 1900);
    }
  }

  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(LED_AZUL, HIGH);

  Serial.println("\n=======================================================");
  Serial.println("  SISTEMA DE IRRIGAÇÃO SOLAR - 8 RELÉS (3 SETORES + BOMBA)");
  Serial.println("=======================================================");

  int seg = 0, min = 0, hora = 0, dia = 0, mes = 0, ano = 0;
  char timeBuffer[30] = "Hora Invalida";
  bool rtcValido = lerHoraRTC(seg, min, hora, dia, mes, ano);

  if (rtcValido) {
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d/%02d/%04d %02d:%02d:%02d", dia, mes, ano, hora, min, seg);
    Serial.printf("⏰ RTC DS3231 Lido com Sucesso: %s\n", timeBuffer);
  } else {
    Serial.println("⚠️ Nenhum módulo RTC DS3231 respondeu no circuito.");
  }

  // Duração por setor: 20s no Boot/Reset ou 30s padrão em todas as regas programadas
  int duracaoSetorSec = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) ? DURACAO_SETOR_BOOT_SEC : DURACAO_SETOR_PADRAO_SEC;
  String motivo = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) ? "Rega Inicial 3 Setores (Boot)" : ("Rega Agendada #" + String(cicloRega));

  // Executa os 3 setores sequencialmente
  executarRegaCompletaTodosSetores(duracaoSetorSec, String(timeBuffer), motivo);
  cicloRega++;

  // Janela OTA desativada no boot USB para manter consumo mínimo e não derrubar a porta
  // if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
  //   executarJanelaOTA(300);
  // }

  // Garante que todos os 8 relés fiquem DEFINITIVAMENTE DESLIGADOS (nível HIGH)
  digitalWrite(LED_AZUL, LOW);
  for (int p : PINOS_TODOS_RELES) {
    digitalWrite(p, RELE_DESLIGADO);
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();

  Serial.println("\n=======================================================");
  Serial.println("  💤 CICLO CONCLUÍDO - TODOS OS 8 RELÉS DESLIGADOS (HIGH)");
  Serial.println("  ESP32 em Standby Ativo monitorando o RTC para próximas regas.");
  Serial.println("=======================================================");
}

unsigned long ultimoCheckRTC = 0;
int ultimoMinutoRegado = -1;

void loop() {
  // Garante continuamente que nenhum relé arme por engano
  for (int p : PINOS_TODOS_RELES) {
    digitalWrite(p, RELE_DESLIGADO);
  }

  // Verifica horário a cada 1 segundo
  if (millis() - ultimoCheckRTC >= 1000) {
    ultimoCheckRTC = millis();

    int seg = 0, min = 0, hora = 0, dia = 0, mes = 0, ano = 0;
    if (lerHoraRTC(seg, min, hora, dia, mes, ano)) {
      int atualMinutos = hora * 60 + min;

      // Imprime status a cada 30 segundos
      if (seg % 30 == 0) {
        Serial.printf("⏱️ [RTC] %02d/%02d/%04d %02d:%02d:%02d | Relés: Todos OFF\n", 
                      dia, mes, ano, hora, min, seg);
      }

      // Verifica se o minuto atual coincide com algum horário da grade oficial
      for (int i = 0; i < QTD_HORARIOS; i++) {
        if (HORARIOS_REGA[i] == atualMinutos && atualMinutos != ultimoMinutoRegado) {
          ultimoMinutoRegado = atualMinutos;
          char timeBuffer[30];
          snprintf(timeBuffer, sizeof(timeBuffer), "%02d/%02d/%04d %02d:%02d:%02d", dia, mes, ano, hora, min, seg);
          Serial.printf("\n⏰ Horário da grade atingido (%02d:%02d)! Iniciando rega agendada #%d...\n", hora, min, cicloRega);
          executarRegaCompletaTodosSetores(DURACAO_SETOR_PADRAO_SEC, String(timeBuffer), "Rega Agendada #" + String(cicloRega));
          cicloRega++;
          break;
        }
      }
    }
  }
  delay(100);
}
