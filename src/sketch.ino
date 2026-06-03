// ─── Bibliotecas ─────────────────────────────────────────────────────────────
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ─── Pinagem ─────────────────────────────────────────────────────────────────
#define BTN_BOM       12
#define BTN_MODERADO  13
#define BTN_RUIM      14
#define LED_VERDE     25
#define LED_AMARELO   26
#define LED_VERMELHO  27
#define BUZZER        32

// ─── Credenciais Wi-Fi ───────────────────────────────────────────────────────
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// ─── Configuração MQTT ───────────────────────────────────────────────────────
const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "airwatch/leitura";
const char* MQTT_CLIENT = "airwatch-esp32-solsticio";

// ─── Configuração API ────────────────────────────────────────────────────────
const char* API_URL = "https://airwatch-api.onrender.com/leitura";

// ─── Instâncias dos objetos ──────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient        wifiClient;
PubSubClient      mqtt(wifiClient);

// ─── Estrutura de dados ──────────────────────────────────────────────────────
struct Leitura {
  int    aqi;
  String status;
  String timestamp;
};

// ─── Estado global ───────────────────────────────────────────────────────────
Leitura leituraAtual  = { 0, "AGUARDANDO", "--" };
Leitura historico[10];
int     totalLeituras        = 0;
unsigned long ultimaLeitura      = 0;
unsigned long tempoUltimaLeitura = 0;

// ─── Timestamp ───────────────────────────────────────────────────────────────
String getTimestamp() {
  unsigned long s = millis() / 1000;
  unsigned long h = s / 3600;
  unsigned long m = (s % 3600) / 60;
  unsigned long seg = s % 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, seg);
  return String(buf);
}

// ─── LCD ─────────────────────────────────────────────────────────────────────
void atualizarLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AQI: ");
  lcd.print(leituraAtual.aqi);
  lcd.setCursor(0, 1);
  lcd.print(leituraAtual.status);
}

// ─── LEDs e Buzzer ───────────────────────────────────────────────────────────
void atualizarSaidas() {
  digitalWrite(LED_VERDE,    LOW);
  digitalWrite(LED_AMARELO,  LOW);
  digitalWrite(LED_VERMELHO, LOW);

  if (leituraAtual.aqi <= 50) {
    digitalWrite(LED_VERDE, HIGH);
    noTone(BUZZER);
  } else if (leituraAtual.aqi <= 150) {
    digitalWrite(LED_AMARELO, HIGH);
    noTone(BUZZER);
  } else {
    digitalWrite(LED_VERMELHO, HIGH);
    tone(BUZZER, 500, 1500);
  }
}

// ─── MQTT ────────────────────────────────────────────────────────────────────
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

// ─── API HTTP ────────────────────────────────────────────────────────────────
void enviarParaAPI() {
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  doc["aqi"]       = leituraAtual.aqi;
  doc["status"]    = leituraAtual.status;
  doc["timestamp"] = leituraAtual.timestamp;

  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);
  Serial.print("[API] POST -> HTTP ");
  Serial.println(httpCode);
  http.end();
}

// ─── Registro de leitura ─────────────────────────────────────────────────────
void registrarLeitura(int aqi, const String& status) {
  leituraAtual.aqi       = aqi;
  leituraAtual.status    = status;
  leituraAtual.timestamp = getTimestamp();

  historico[totalLeituras % 10] = leituraAtual;
  totalLeituras++;

  tempoUltimaLeitura = millis();

  atualizarSaidas();
  atualizarLCD();
  publicarMQTT();
  enviarParaAPI();

  Serial.printf("[SENSOR] AQI=%d | %s | %s\n",
    leituraAtual.aqi,
    leituraAtual.status.c_str(),
    leituraAtual.timestamp.c_str());
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(BTN_BOM,      INPUT_PULLUP);
  pinMode(BTN_MODERADO, INPUT_PULLUP);
  pinMode(BTN_RUIM,     INPUT_PULLUP);
  pinMode(LED_VERDE,    OUTPUT);
  pinMode(LED_AMARELO,  OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER,       OUTPUT);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AirWatch IoT");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  Serial.print("[WiFi] Conectando");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Conectado! IP: " + WiFi.localIP().toString());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(1000);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  conectarMQTT();

  atualizarLCD();
  Serial.println("[HTTP] Pronto para enviar para API");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  mqtt.loop();

  if (tempoUltimaLeitura > 0 && millis() - tempoUltimaLeitura > 15000) {
    tempoUltimaLeitura = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("AirWatch IoT");
    lcd.setCursor(0, 1);
    lcd.print("Aguardando...");
  }

  if (millis() - ultimaLeitura < 500) return;

  if (digitalRead(BTN_BOM) == LOW) {
    ultimaLeitura = millis();
    registrarLeitura(30, "BOM");
  } else if (digitalRead(BTN_MODERADO) == LOW) {
    ultimaLeitura = millis();
    registrarLeitura(120, "MODERADO");
  } else if (digitalRead(BTN_RUIM) == LOW) {
    ultimaLeitura = millis();
    registrarLeitura(250, "RUIM");
  }
}
