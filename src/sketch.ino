#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando...");


  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AirWatch IoT");
  lcd.setCursor(0, 1);
  lcd.print("Aguardando...");
  delay(2000);

  WiFi.begin(SSID, PASSWORD, 6);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
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
  delay(1000);
}
