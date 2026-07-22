#include <WiFi.h>

const char* ssid     = "AP104-5G";
const char* password = "papagaio";

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Nada no loop no exemplo mínimo
}
