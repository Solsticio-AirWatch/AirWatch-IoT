#include <WiFi.h>

const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando...");

  WiFi.begin(SSID, PASSWORD, 6);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // vazio por enquanto
  delay(1000);
}
