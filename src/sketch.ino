#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <PubSubClient.h>

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "airwatch/sensor/001";

#define PIN_DHT 4
#define DHTTYPE DHT22
#define PIN_MQ135 34
#define LED_VERDE 25
#define LED_AMARELO 26
#define LED_VERMELHO 27
#define BUZZER 13
#define HISTORICO_TAM 5

DHT dht(PIN_DHT, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

float temperatura = 0;
float umidade = 0;
int aqi = 0;
unsigned long ultimaLeitura = 0;
unsigned long inicio = 0;
int totalLeituras = 0;
bool alertaSoando = false;

struct Leitura {
  unsigned long timestamp;
  int aqi;
};
Leitura historico[HISTORICO_TAM];
int idxHistorico = 0;

void adicionarHistorico(int valor) {
  historico[idxHistorico].timestamp = millis();
  historico[idxHistorico].aqi = valor;
  idxHistorico = (idxHistorico + 1) % HISTORICO_TAM;
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (mqttClient.connect("ESP32_AirWatch")) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5'>";
  html += "<title>AirWatch IoT</title>";
  html += "<style>body{font-family:Arial;text-align:center;margin-top:50px;}</style>";
  html += "</head><body>";
  html += "<h1>AirWatch - Qualidade do Ar</h1>";
  html += "<p>Temperatura: " + String(temperatura, 1) + " &deg;C</p>";
  html += "<p>Umidade: " + String(umidade, 0) + " %</p>";
  html += "<p>Índice de qualidade (AQI): " + String(aqi) + "</p>";
  html += "<p><a href='/api/atual'>JSON atual</a> | <a href='/api/historico'>JSON histórico</a> | <a href='/api/status'>JSON status</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleApiAtual() {
  String json = "{";
  json += "\"temperatura\":" + String(temperatura, 1) + ",";
  json += "\"umidade\":" + String(umidade, 0) + ",";
  json += "\"aqi\":" + String(aqi);
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiHistorico() {
  String json = "[";
  for (int i = 0; i < HISTORICO_TAM; i++) {
    if (historico[i].aqi == 0 && historico[i].timestamp == 0 && i > 0) continue;
    if (i > 0 && (historico[i-1].aqi != 0 || historico[i-1].timestamp != 0)) json += ",";
    json += "{\"timestamp\":" + String(historico[i].timestamp) + ",\"aqi\":" + String(historico[i].aqi) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleApiStatus() {
  long rssi = WiFi.RSSI();
  unsigned long uptime = millis() / 1000;
  String json = "{";
  json += "\"status\":\"online\",";
  json += "\"wifi_rssi\":" + String(rssi) + ",";
  json += "\"uptime_segundos\":" + String(uptime) + ",";
  json += "\"total_leituras\":" + String(totalLeituras);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  inicio = millis();
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AirWatch IoT");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  noTone(BUZZER);
  alertaSoando = false;

  WiFi.begin(SSID, PASSWORD, 6);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();

  server.on("/", handleRoot);
  server.on("/api/atual", handleApiAtual);
  server.on("/api/historico", handleApiHistorico);
  server.on("/api/status", handleApiStatus);
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  server.handleClient();
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  if (millis() - ultimaLeitura >= 5000) {
    temperatura = dht.readTemperature();
    umidade = dht.readHumidity();
    if (isnan(temperatura)) temperatura = 0;
    if (isnan(umidade)) umidade = 0;

    int raw = analogRead(PIN_MQ135);
    aqi = map(raw, 0, 4095, 0, 500);
    if (aqi < 0) aqi = 0;
    if (aqi > 500) aqi = 500;

    adicionarHistorico(aqi);
    totalLeituras++;

    String payload = "{";
    payload += "\"temperatura\":" + String(temperatura, 1) + ",";
    payload += "\"umidade\":" + String(umidade, 0) + ",";
    payload += "\"aqi\":" + String(aqi);
    payload += "}";
    mqttClient.publish(MQTT_TOPIC, payload.c_str());
    Serial.print("MQTT publicado: ");
    Serial.println(payload);

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
    lcd.print("    ");

    if (aqi <= 100) {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_AMARELO, LOW);
      digitalWrite(LED_VERMELHO, LOW);
      if (alertaSoando) {
        noTone(BUZZER);
        alertaSoando = false;
      }
    } else if (aqi <= 200) {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_AMARELO, HIGH);
      digitalWrite(LED_VERMELHO, LOW);
      if (alertaSoando) {
        noTone(BUZZER);
        alertaSoando = false;
      }
    } else {
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_AMARELO, LOW);
      digitalWrite(LED_VERMELHO, HIGH);
      if (!alertaSoando) {
        tone(BUZZER, 2000, 200);
        alertaSoando = true;
      }
    }

    ultimaLeitura = millis();
  }
  delay(10);
}
