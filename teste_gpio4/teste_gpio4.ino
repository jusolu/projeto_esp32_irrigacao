/*
  TESTE SIMPLES - LED GPIO 4
  Liga o LED no GPIO 4 fixo por 5 segundos, depois pisca infinitamente.
*/
void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);

  Serial.println("=== TESTE LED GPIO 4 ===");
  Serial.println("Ligando LED no GPIO 4...");
  digitalWrite(4, HIGH);
  delay(5000);
  Serial.println("Agora piscando a cada 500ms...");
}

void loop() {
  digitalWrite(4, HIGH);
  Serial.println("GPIO 4 -> HIGH (LED ON)");
  delay(500);
  digitalWrite(4, LOW);
  Serial.println("GPIO 4 -> LOW  (LED OFF)");
  delay(500);
}
