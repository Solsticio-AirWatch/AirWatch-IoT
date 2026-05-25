#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

#define PIN_DHT 4
#define DHTTYPE DHT22
#define PIN_MQ135 34

DHT dht(PIN_DHT, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float temperatura = 0;
float umidade = 0;
int aqi = 0;
unsigned long ultimaLeitura = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AirWatch IoT");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);

  WiFi.begin(SSID, PASSWORD, 6);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();
}

void loop() {
  if (millis() - ultimaLeitura >= 5000) {
    temperatura = dht.readTemperature();
    umidade = dht.readHumidity();
    if (isnan(temperatura)) temperatura = 0;
    if (isnan(umidade)) umidade = 0;

    int raw = analogRead(PIN_MQ135);
    aqi = map(raw, 0, 4095, 0, 500);
    if (aqi < 0) aqi = 0;
    if (aqi > 500) aqi = 500;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperatura, 1);
    lcd.print("C U:");
    lcd.print(umidade, 0);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("AQI:");
    lcd.print(aqi);

    ultimaLeitura = millis();
  }
  delay(10);
}
