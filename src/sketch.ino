#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define BTN_BOM       12
#define BTN_MODERADO  13
#define BTN_RUIM      14
#define LED_VERDE     25
#define LED_AMARELO   26
#define LED_VERMELHO  27
#define BUZZER        32

const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "airwatch/leitura";
const char* MQTT_CLIENT = "airwatch-esp32-solsticio";

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient        wifiClient;
PubSubClient      mqtt(wifiClient);
WebServer         server(80);

struct Leitura {
  int    aqi;
  String status;
  String timestamp;
};

Leitura leituraAtual  = { 0, "AGUARDANDO", "--" };
Leitura historico[10];
int     totalLeituras = 0;
unsigned long ultimaLeitura = 0;

String getTimestamp() {
  unsigned long s = millis() / 1000;
  unsigned long h = s / 3600;
  unsigned long m = (s % 3600) / 60;
  unsigned long seg = s % 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, seg);
  return String(buf);
}

void atualizarLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AQI: ");
  lcd.print(leituraAtual.aqi);
  lcd.setCursor(0, 1);
  lcd.print(leituraAtual.status);
}

void atualizarSaidas() {
  digitalWrite(LED_VERDE,    LOW);
  digitalWrite(LED_AMARELO,  LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(BUZZER,       LOW);

  if (leituraAtual.aqi <= 50) {
    digitalWrite(LED_VERDE, HIGH);
  } else if (leituraAtual.aqi <= 150) {
    digitalWrite(LED_AMARELO, HIGH);
  } else {
    digitalWrite(LED_VERMELHO, HIGH);
    digitalWrite(BUZZER, HIGH);
  }
}

void conectarMQTT() {
  if (mqtt.connected()) return;
  Serial.print("[MQTT] Conectando...");
  if (mqtt.connect(MQTT_CLIENT)) {
    Serial.println(" OK");
  } else {
    Serial.print(" Falhou, rc=");
    Serial.println(mqtt.state());
  }
}

void publicarMQTT() {
  if (!mqtt.connected()) conectarMQTT();

  StaticJsonDocument<128> doc;
  doc["aqi"]       = leituraAtual.aqi;
  doc["status"]    = leituraAtual.status;
  doc["timestamp"] = leituraAtual.timestamp;

  char payload[128];
  serializeJson(doc, payload);
  mqtt.publish(MQTT_TOPIC, payload);
  Serial.print("[MQTT] Publicado: ");
  Serial.println(payload);
}

void registrarLeitura(int aqi, const String& status) {
  leituraAtual.aqi       = aqi;
  leituraAtual.status    = status;
  leituraAtual.timestamp = getTimestamp();

  historico[totalLeituras % 10] = leituraAtual;
  totalLeituras++;

  atualizarSaidas();
  atualizarLCD();
  publicarMQTT();

  Serial.printf("[SENSOR] AQI=%d | %s | %s\n",
    leituraAtual.aqi,
    leituraAtual.status.c_str(),
    leituraAtual.timestamp.c_str());
}
