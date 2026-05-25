void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Teste do potenciometro no pino 34");
}

void loop() {
  int raw = analogRead(34);
  int aqi = map(raw, 0, 4095, 0, 500);
  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print("  AQI: ");
  Serial.println(aqi);
  delay(500);
}
