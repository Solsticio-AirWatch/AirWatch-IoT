#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BTN_BOM  18
#define BTN_RUIM 19
#define LED_VERDE 25
#define LED_VERMELHO 27

LiquidCrystal_I2C lcd(0x27, 16, 2);

int aqi = 50;
String qualidade = "BOA";

void setup() {
  Serial.begin(115200);
  pinMode(BTN_BOM, INPUT_PULLUP);
  pinMode(BTN_RUIM, INPUT_PULLUP);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  lcd.init();
  lcd.backlight();
  lcd.print("AirWatch IoT");
  lcd.setCursor(0, 1);
  lcd.print("Use botoes");
  delay(2000);
  lcd.clear();
}

void loop() {
  if (digitalRead(BTN_BOM) == LOW) {
    delay(300);
    aqi = 50;
    qualidade = "BOA";
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
    lcd.clear();
    lcd.print("AQI: 50 - BOA");
    lcd.setCursor(0, 1);
    lcd.print("Ar Saudavel");
  }
  if (digitalRead(BTN_RUIM) == LOW) {
    delay(300);
    aqi = 250;
    qualidade = "RUIM";
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
    lcd.clear();
    lcd.print("AQI: 250 - RUIM");
    lcd.setCursor(0, 1);
    lcd.print("Evite atividades");
  }
  delay(100);
}
