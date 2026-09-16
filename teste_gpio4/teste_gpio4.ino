#include <Arduino.h>

#define PIN_MOSFET 4
#define LED_AZUL 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================================");
  Serial.println("  TESTE DE CHAVEAMENTO DO MOSFET NO GPIO 4");
  Serial.println("=================================================");

  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  digitalWrite(PIN_MOSFET, LOW);
  digitalWrite(LED_AZUL, LOW);
}

void loop() {
  // 1. DESLIGADO (LOW = 0V)
  Serial.println("🔴 MOSFET DESLIGADO (LOW / 0V) -> Bomba deve estar TOTALMENTE PARADA por 6 segundos...");
  digitalWrite(PIN_MOSFET, LOW);
  digitalWrite(LED_AZUL, LOW);
  delay(6000);

  // 2. LIGADO (HIGH = 3.3V)
  Serial.println("🟢 MOSFET LIGADO (HIGH / 3.3V) -> Bomba deve LIGAR por 3 segundos...");
  digitalWrite(PIN_MOSFET, HIGH);
  digitalWrite(LED_AZUL, HIGH);
  delay(3000);
}
