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
