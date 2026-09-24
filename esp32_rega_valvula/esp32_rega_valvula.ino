/*
  =============================================================================
  SISTEMA DE IRRIGAÇÃO SOLAR AUTOMATIZADO - V2 (BOMBA 12V + VÁLVULA SOLENOIDE NF)
  =============================================================================
  - Hardware:
      * Módulo Relé 1 (Bomba d'água 12V)          ➔ GPIO 4
      * Módulo Relé 2 (Válvula Solenoide 12V NF)  ➔ GPIO 16
      * Módulo RTC DS3231 (Relógio de Precisão)   ➔ I2C (SDA=GPIO 21, SCL=GPIO 22)
      * LED Azul de Status                        ➔ GPIO 2
  - Sequência Hidráulica de Operação (Proteção Anti-Golpe de Aríete):
      1. Abre Válvula Solenoide (GPIO 16)
      2. Espera 1 segundo para desobstrução e alívio de pressão na linha
      3. Liga a Bomba d'água (GPIO 4)
      4. Mantém ambos ativos pelo tempo de rega (60 segundos)
      5. Desliga a Bomba d'água primeiro
      6. Espera 1 segundo para drenagem e alívio do fluxo
      7. Fecha a Válvula Solenoide (NF)
  - Grade Diurna Oficial (13 Regas):
      06:30, 08:00, 09:30, 10:30, 11:30, 12:00, 13:00, 14:00, 15:00, 16:00, 17:00, 18:00, 19:30
  - Duração: 60 SEGUNDOS (1 minuto) em todas as regas e no Boot inicial
  - Modo Sono: Deep Sleep Ultra-Econômico (< 15mA) travando pinos 4 e 16 em LOW
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
#include "driver/rtc_io.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Pinos I2C do RTC DS3231 (SDA D21, SCL D22)
#define I2C_SDA 21
#define I2C_SCL 22
#define RTC_I2C_ADDRESS 0x68

#define LED_AZUL        2   // LED Azul embutido
#define PIN_BOMBA       4   // Relé 1: Bomba d'água 12V (GPIO 4)
#define PIN_VALVULA    16   // Relé 2: Válvula Solenoide 12V Normalmente Fechada (GPIO 16)

// Configuração do Nível Lógico dos Módulos Relé:
// - Se o seu relé for Active-HIGH (ou tiver jumper na posição 'H'): RELE_LIGADO = HIGH, RELE_DESLIGADO = LOW
// - Se o seu relé for Active-LOW (ou tiver jumper na posição 'L'): RELE_LIGADO = LOW, RELE_DESLIGADO = HIGH
const int RELE_LIGADO    = HIGH;
const int RELE_DESLIGADO = LOW;

// Configurações Fixas de Tempo de Rega
#define DURACAO_REGA_PADRAO_SEC 60 // 60 segundos (1 minuto) em todas as regas normais
#define DURACAO_REGA_BOOT_SEC   60 // 60 segundos (1 minuto) na rega inicial no reset/boot

const char* WIFI_SSID       = "AP104-2.4G  "; // Dois espaços no final
const char* WIFI_PASSWORD   = "papagaio";
const char* VERCEL_POST_URL = "https://projeto-esp32-irrigacao.vercel.app/api/esp32";

// Configurações de NTP (Sincronização de Hora Oficial via Internet)
// Fuso Horário: GMT-4 (Hora do Brasil Central / Manaus / Cuiabá)
const long GMT_OFFSET_SEC        = -4 * 3600;
const int  DAYLIGHT_OFFSET_SEC   = 0;
const char* NTP_SERVER_1         = "a.st1.ntp.br";
const char* NTP_SERVER_2         = "pool.ntp.org";
const char* NTP_SERVER_3         = "time.google.com";

// Tempo de sono do Modo Emergência caso o RTC/NTP falhem: 8 Horas (28800 segundos)
const uint64_t SEGUNDOS_EMERGENCIA_8H = 28800ULL;

// Variáveis persistentes na memória RTC do ESP32 durante o Deep Sleep
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

bool sincronizarRTCComNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  Serial.println("🌐 [NTP] Consultando horário oficial na internet...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

  struct tm timeinfo;
  int tentativas = 0;
  while (!getLocalTime(&timeinfo) && tentativas < 25) {
    delay(150);
    tentativas++;
  }

  if (getLocalTime(&timeinfo)) {
    int ano = timeinfo.tm_year + 1900;
    if (ano >= 2026 && ano <= 2035) {
      setRTCDateTime(timeinfo.tm_sec, timeinfo.tm_min, timeinfo.tm_hour,
                     timeinfo.tm_mday, timeinfo.tm_mon + 1, ano);
      Serial.printf("⏰ [NTP ➔ RTC] Sucesso! Relógio calibrado: %02d/%02d/%04d %02d:%02d:%02d\n",
                    timeinfo.tm_mday, timeinfo.tm_mon + 1, ano,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      return true;
    }
  }
  Serial.println("⚠️ [NTP] Servidor NTP não respondeu a tempo.");
  return false;
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
  Serial.printf("⏰ [RTC] Relógio calibrado com data/hora de compilação: %02d/%02d/%04d %02d:%02d:%02d\n",
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
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
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
    sincronizarRTCComNTP();
    return true;
  }
  
  digitalWrite(LED_AZUL, LOW);
  return false;
}

void reportarRegaParaVercel(int duracao, String horaFormatada, String motivo) {
  if (!conectarWiFiRobusto()) {
    Serial.println("⚠️ Wi-Fi offline ou fora de alcance. Rega executada localmente!");
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
                   "\",\"source\":\"" + motivo + 
                   "\",\"actuators\":\"Bomba (GPIO 4) + Valvula (GPIO 16)\"}";
                   
  Serial.printf("📤 Enviando telemetria para Vercel: %s\n", payload.c_str());
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("✅ [Vercel] Enviado com Sucesso! (HTTP %d)\n", httpCode);
  } else {
    Serial.printf("❌ [Vercel] Erro HTTP: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Sequência Hidráulica Segura: Válvula Solenoide NF + Bomba d'água 12V
void executarRegaCompleta(int duracaoSec, String horaStr, String motivo) {
  Serial.println("\n=======================================================");
  Serial.printf("💦 INICIANDO REGA: BOMBA + VÁLVULA SOLENOIDE (%d seg)\n", duracaoSec);
  Serial.println("=======================================================");

  // 1. Abre a Válvula Solenoide 12V (NF -> Aberta)
  Serial.println("🚰 [1/4] Abrindo Válvula Solenoide (GPIO 16)...");
  digitalWrite(PIN_VALVULA, RELE_LIGADO);
  delay(1000); // 1 segundo de folga para aliviar contrapressão na tubulação

  // 2. Liga a Bomba d'água 12V
  Serial.println("⚡ [2/4] Ligando Bomba d'água (GPIO 4)...");
  digitalWrite(PIN_BOMBA, RELE_LIGADO);
  Serial.println("🌿 Bomba e Válvula operando em conjunto!");

  // 3. Mantém ambos ativos durante a rega, piscando o LED Azul
  int totalPiscadas = (duracaoSec * 1000) / 250;
  for (int i = 0; i < totalPiscadas; i++) {
    digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
    delay(250);
  }

  // 4. Desliga a Bomba primeiro (evita sobrepressão hidráulica e golpe de aríete)
  Serial.println("🛑 [3/4] Desligando Bomba d'água primeiro...");
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);
  delay(1000); // 1 segundo para vazão residual drenar antes do bloqueio da válvula

  // 5. Fecha a Válvula Solenoide (NF -> Fechada)
  Serial.println("🔒 [4/4] Fechando Válvula Solenoide (GPIO 16)...");
  digitalWrite(PIN_VALVULA, RELE_DESLIGADO);
  digitalWrite(LED_AZUL, HIGH);
  Serial.println("✅ Irrigação concluída! Bomba e Válvula totalmente desativadas.");

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
    Serial.println("\n❌ Falha ao conectar ao Wi-Fi. Cancelando janela OTA...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(LED_AZUL, LOW);
    return;
  }

  ArduinoOTA.setHostname("esp32-rega-valvula");

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("\n[OTA] Recebendo gravação sem fio: " + type);
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_AZUL, !digitalRead(LED_AZUL));
      delay(50);
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

// 13 Horários Oficiais de Rega (minutos desde a meia-noite):
// 06:30, 08:00, 09:30, 10:30, 11:30, 12:00, 13:00, 14:00, 15:00, 16:00, 17:00, 18:00, 19:30
const int HORARIOS_REGA[] = { 390, 480, 570, 630, 690, 720, 780, 840, 900, 960, 1020, 1080, 1170 };
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
    // Passou das 19:30. O próximo é 06:30 da manhã seguinte
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

  // Libera pinos de retenção de Deep Sleep
  gpio_hold_dis(GPIO_NUM_4);
  gpio_hold_dis(GPIO_NUM_16);

  Serial.begin(115200);
  delay(300);

  // Inicializa barramento I2C nos pinos SDA=21 e SCL=22
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // Se for Boot Frio (Reset manual ou ligar na fonte), conecta imediatamente ao Wi-Fi para sincronizar o RTC via NTP
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.println("🚀 [BOOT FRIO] Conectando ao Wi-Fi para sincronizar relógio via NTP...");
    conectarWiFiRobusto();
  }

  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(LED_AZUL, HIGH);

  // Inicializa os pinos da Bomba e da Válvula desativados
  pinMode(PIN_BOMBA, OUTPUT);
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);

  pinMode(PIN_VALVULA, OUTPUT);
  digitalWrite(PIN_VALVULA, RELE_DESLIGADO);

  Serial.println("\n=======================================================");
  Serial.println("   SISTEMA DE IRRIGAÇÃO V2 (BOMBA 12V + VÁLVULA NF)");
  Serial.println("=======================================================");

  int seg = 0, min = 0, hora = 0, dia = 0, mes = 0, ano = 0;
  char timeBuffer[30] = "Hora Invalida";
  bool rtcValido = lerHoraRTC(seg, min, hora, dia, mes, ano);

  // Fallback 1: se o RTC físico não respondeu (erro I2C), usa a hora oficial já sincronizada via NTP
  if (!rtcValido) {
    struct tm ti;
    if (getLocalTime(&ti)) {
      int ano_ntp = ti.tm_year + 1900;
      if (ano_ntp >= 2026 && ano_ntp <= 2035) {
        seg  = ti.tm_sec;
        min  = ti.tm_min;
        hora = ti.tm_hour;
        dia  = ti.tm_mday;
        mes  = ti.tm_mon + 1;
        ano  = ano_ntp;
        rtcValido = true;
        Serial.printf("🌐 [NTP ADOTADO] Relógio sincronizado com sucesso: %02d/%02d/%04d %02d:%02d:%02d\n",
                      dia, mes, ano, hora, min, seg);
        setRTCDateTime(seg, min, hora, dia, mes, ano);
      }
    }
  }

  // Fallback 2: se o RTC falhou mesmo após NTP e temos hora na RAM de sono anterior
  if (!rtcValido && epochTimePersistente > 0 && ultimoSegundosSono > 0) {
    Serial.println("⚠️ [RTC FALLBACK] Recuperando hora pela memória RAM...");
    epochTimePersistente += (uint32_t)ultimoSegundosSono;
    time_t t_calc = (time_t)epochTimePersistente;
    struct tm *tm_calc = localtime(&t_calc);
    if (tm_calc != NULL) {
      seg  = tm_calc->tm_sec;
      min  = tm_calc->tm_min;
      hora = tm_calc->tm_hour;
      dia  = tm_calc->tm_mday;
      mes  = tm_calc->tm_mon + 1;
      ano  = tm_calc->tm_year + 1900;
      rtcValido = true;
      setRTCDateTime(seg, min, hora, dia, mes, ano);
    }
  }

  if (rtcValido) {
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d/%02d/%04d %02d:%02d:%02d", dia, mes, ano, hora, min, seg);
    Serial.printf("⏰ Horário Atual Validado: %s\n", timeBuffer);
  } else {
    Serial.println("⚠️ Nenhum módulo RTC DS3231 ou NTP respondeu no circuito.");
  }

  // Duração da rega fixa: 60s em todas as regas e no Boot inicial
  int duracaoRegaSec = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) ? DURACAO_REGA_BOOT_SEC : DURACAO_REGA_PADRAO_SEC;
  String motivo = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) ? "Rega Inicial (Boot)" : ("Rega Agendada #" + String(cicloRega));

  // Executa a sequência completa (Válvula -> Bomba -> Bomba Desliga -> Válvula Desliga) e envia para a Vercel
  executarRegaCompleta(duracaoRegaSec, String(timeBuffer), motivo);
  cicloRega++;

  // Janela OTA rápida de 15 segundos no boot frio/reset para permitir testes pontuais imediatos
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    executarJanelaOTA(15);
  }

  // Recalcula hora fresca para agendamento de sono com máxima precisão
  if (!lerHoraRTC(seg, min, hora, dia, mes, ano)) {
    struct tm ti;
    if (getLocalTime(&ti)) {
      seg  = ti.tm_sec;
      min  = ti.tm_min;
      hora = ti.tm_hour;
      dia  = ti.tm_mday;
      mes  = ti.tm_mon + 1;
      ano  = ti.tm_year + 1900;
      rtcValido = true;
    }
  } else {
    rtcValido = true;
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

  digitalWrite(LED_AZUL, LOW);
  pinMode(PIN_BOMBA, OUTPUT);
  digitalWrite(PIN_BOMBA, RELE_DESLIGADO);
  pinMode(PIN_VALVULA, OUTPUT);
  digitalWrite(PIN_VALVULA, RELE_DESLIGADO);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();

  // Trava com segurança os pinos da Bomba e da Válvula desligados durante o Deep Sleep
  gpio_hold_en(GPIO_NUM_4);
  gpio_hold_en(GPIO_NUM_16);
  gpio_deep_sleep_hold_en();

  esp_sleep_enable_timer_wakeup(segundosSono * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Deep Sleep
}
