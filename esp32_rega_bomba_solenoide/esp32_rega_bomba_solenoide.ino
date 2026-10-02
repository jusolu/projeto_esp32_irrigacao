/*
  =============================================================================
  SISTEMA DE IRRIGAÇÃO AUTOMATIZADO - ARQUITETURA 2 RELÉS (BOMBA + SOLENOIDE)
  =============================================================================
  - Hardware:
      * Relé 1 (Bomba d'água)           ➔ GPIO 32 (Active-LOW)
      * Relé 2 (Válvula Solenoide)      ➔ GPIO 33 (Active-LOW)
      * Módulo RTC DS3231               ➔ I2C (SDA = GPIO 21, SCL = GPIO 22)
      * LED Azul de Status              ➔ GPIO 2
  - Sequência Hidráulica de Proteção (Anti-Golpe de Aríete / Anti-Sobrecarga):
      1. LIGAR:
         - Primeiro abre a Válvula Solenoide (Relé 2 / D33).
         - Aguarda 1.5s para desobstrução e alívio da pressão da tubulação.
         - Em seguida, liga a Bomba d'água (Relé 1 / D32).
      2. DESLIGAR:
         - Primeiro desliga a Bomba d'água (Relé 1 / D32).
         - Aguarda 1.5s para cessar o fluxo e drenar o golpe de aríete.
         - Em seguida, fecha a Válvula Solenoide (Relé 2 / D33).
  - Grade Oficial (12 Regas Diurnas):
      07:30, 09:00, 10:00, 11:00, 12:00, 13:00, 14:00, 15:00, 16:00, 17:00, 18:00, 19:30
  - Duração:
      * 45 segundos padrão em todas as regas programadas
      * 60 segundos na rega de teste no Reset/Boot
  - Modo Emergência (Falha no RTC): Rega a cada 8 horas (28.800s)
  - Economia de Energia: Deep Sleep ultra-econômico entre as regas
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

// Pinos dos Relés
#define PIN_BOMBA       32  // Relé 1: Bomba d'água
#define PIN_VALVULA     33  // Relé 2: Válvula Solenoide
#define LED_AZUL         2  // LED Azul de Status

// Pinos I2C do Relógio RTC DS3231
#define I2C_SDA         21
#define I2C_SCL         22
#define RTC_I2C_ADDRESS 0x68

// Lógica dos Relés (Active-LOW: LOW Liga / HIGH Desliga)
const int RELE_LIGADO    = LOW;
const int RELE_DESLIGADO = HIGH;

// Configurações de Duração da Rega
#define DURACAO_REGA_PADRAO_SEC 45 // 45 segundos nas regas diurnas programadas
#define DURACAO_REGA_BOOT_SEC   60 // 60 segundos (1 minuto) no reset/boot

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

void reportarRegaParaVercel(int duracao, String horaFormatada, String motivo) {
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

  String payload = "{\"waterCompleted\":true,\"durationSec\":" + String(duracao) + 
                   ",\"rtcTime\":\"" + horaFormatada + 
                   "\",\"source\":\"" + motivo + "\"}";
                   
  Serial.printf("📤 Enviando telemetria para Vercel: %s\n", payload.c_str());
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("✅ [Vercel] Enviado com Sucesso! (HTTP %d)\n", httpCode);
  } else {
    Serial.printf("❌ [Vercel] Erro HTTP: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Execução Hidráulica Segura: Abre Solenoide -> Liga Bomba -> Desliga Bomba -> Fecha Solenoide
void executarRegaSequencial(int duracaoSec, String horaStr, String motivo) {
  Serial.println("\n=======================================================");
  Serial.printf("🚿 INICIANDO SEQUÊNCIA HIDRÁULICA (%s)\n", motivo.c_str());
  Serial.printf("   Tempo total de bombeamento: %d segundos\n", duracaoSec);
  Serial.println("=======================================================");

  // 1. Abre a Válvula Solenoide primeiro (D33)
  Serial.println("1️⃣ [PASSO 1] Abrindo Válvula Solenoide (Relé 2 / D33)...");
  digitalWrite(PIN_VALVULA, RELE_LIGADO);
  digitalWrite(LED_AZUL, HIGH);
  delay(1500); // 1.5s para desobstrução e alívio da linha

  // 2. Liga a Bomba d'água (D32)
  Serial.printf("2️⃣ [PASSO 2] Ligando Bomba d'água (Relé 1 / D32) por %d segundos...\n", duracaoSec);
  digitalWrite(PIN_BOMBA, RELE_LIGADO);

  // 3. Mantém ambos ativos e pisca o LED indicador
  unsigned long startMillis = millis();
  while (millis() - startMillis < ((unsigned long)duracaoSec * 1000)) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
  }

  // 4. Desliga a Bomba d'água primeiro (D32)
  Serial.println("3️⃣ [PASSO 3] Desligando Bomba d'água (Relé 1 / D32)...");
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);
  digitalWrite(LED_AZUL, HIGH);
  delay(1500); // 1.5s para cessar o fluxo e drenar a pressão

  // 5. Fecha a Válvula Solenoide (D33)
  Serial.println("4️⃣ [PASSO 4] Fechando Válvula Solenoide (Relé 2 / D33)...");
  digitalWrite(PIN_VALVULA, RELE_DESLIGADO);
  digitalWrite(LED_AZUL, LOW);
  Serial.println("✅ Irrigação concluída com sucesso e tubulação despressurizada!");

  // 6. Conecta Wi-Fi e envia telemetria para a Vercel
  reportarRegaParaVercel(duracaoSec, horaStr, motivo);
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
  ArduinoOTA.setHostname("ESP32-Irrigacao-Valvula");

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("\n[OTA] Iniciando gravação sem fio (" + type + ")...");
    digitalWrite(PIN_BOMBA, RELE_DESLIGADO);
    digitalWrite(PIN_VALVULA, RELE_DESLIGADO);
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
    
    // Suporte a comando de ajuste de hora via serial durante o boot
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

// 12 Horários de Rega Diurnos (minutos desde a meia-noite):
// 07:30, 09:00, 10:00, 11:00, 12:00, 13:00, 14:00, 15:00, 16:00, 17:00, 18:00, 19:30
const int HORARIOS_REGA[] = { 450, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1080, 1170 };
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
    // Passou das 19:30. O próximo é 07:30 da manhã seguinte
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
  gpio_hold_dis((gpio_num_t)PIN_BOMBA);
  gpio_hold_dis((gpio_num_t)PIN_VALVULA);

  // Configura pinos dos relés imediatamente como saída desligada (HIGH)
  pinMode(PIN_BOMBA, OUTPUT);
  pinMode(PIN_VALVULA, OUTPUT);
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);
  digitalWrite(PIN_VALVULA, RELE_DESLIGADO);

  Serial.begin(115200);
  delay(300);

  // Inicializa barramento I2C nos pinos SDA=21 e SCL=22
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // Restaura hora persistente via RAM se o RTC tiver oscilado
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
  Serial.println("  SISTEMA DE IRRIGAÇÃO SOLAR - 2 RELÉS (BOMBA + SOLENOIDE)");
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

  // Duração da rega fixa: 60s no Boot/Reset ou 45s padrão em todas as regas normais
  int duracaoRegaSec = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) ? DURACAO_REGA_BOOT_SEC : DURACAO_REGA_PADRAO_SEC;
  String motivo = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) ? "Rega Inicial (Boot)" : ("Rega Agendada #" + String(cicloRega));

  // Executa a rega hidráulica sequencial (Abre Válvula -> Liga Bomba -> Desliga Bomba -> Fecha Válvula)
  executarRegaSequencial(duracaoRegaSec, String(timeBuffer), motivo);
  cicloRega++;

  // Janela OTA de 5 minutos no boot frio/reset
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    executarJanelaOTA(300);
  }

  // Calcula o sono exato até a próxima rega agendada
  uint64_t segundosSono = SEGUNDOS_EMERGENCIA_8H;
  if (rtcValido) {
    segundosSono = calcularSegundosParaProximaRega(hora, min, seg);
    int horasSono = segundosSono / 3600;
    int minutosRestantes = (segundosSono % 3600) / 60;
    Serial.printf("🌿 [AGENDAMENTO NORMAL] Próxima rega em %02d:%02d (%llu seg)...\n", 
                  horasSono, minutosRestantes, segundosSono);

    struct tm tm_sleep;
    tm_sleep.tm_sec = seg;
    tm_sleep.tm_min = min;
    tm_sleep.tm_hour = hora;
    tm_sleep.tm_mday = dia;
    tm_sleep.tm_mon = mes - 1;
    tm_sleep.tm_year = ano - 1900;
    tm_sleep.tm_isdst = -1;
    time_t t_sleep = mktime(&tm_sleep);
    if (t_sleep != -1) {
      epochTimePersistente = (uint32_t)t_sleep;
    }
  } else {
    segundosSono = SEGUNDOS_EMERGENCIA_8H; // 8 HORAS de Emergência
    Serial.printf("⚠️ [MODO DE EMERGÊNCIA] Relógio sem hora. Próxima rega em 8 HORAS (%llu seg)!\n", segundosSono);
  }

  ultimoSegundosSono = segundosSono;

  // Garante relés desligados antes de dormir
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);
  digitalWrite(PIN_VALVULA, RELE_DESLIGADO);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();

  // Trava os pinos dos relés em nível desligado durante o Deep Sleep
  gpio_hold_en((gpio_num_t)PIN_BOMBA);
  gpio_hold_en((gpio_num_t)PIN_VALVULA);
  gpio_deep_sleep_hold_en();

  esp_sleep_enable_timer_wakeup(segundosSono * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Deep Sleep
}
