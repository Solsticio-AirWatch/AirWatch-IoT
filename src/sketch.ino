#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <PubSubClient.h>

const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "airwatch/sensor/001";

#define PIN_DHT      4
#define DHTTYPE      DHT22
#define PIN_MQ135    34
#define LED_VERDE    25
#define LED_AMARELO  26
#define LED_VERMELHO 27
#define PIN_BUZZER   13

#define LEDC_CANAL 0
#define LEDC_RES   8

void buzzerBeep(int freq, int durMs) {
  ledcSetup(LEDC_CANAL, freq, LEDC_RES);
  ledcAttachPin(PIN_BUZZER, LEDC_CANAL);
  ledcWrite(LEDC_CANAL, 128);
  delay(durMs);
  ledcWrite(LEDC_CANAL, 0);
  ledcDetachPin(PIN_BUZZER);
}

#define HISTORICO_TAM 10

struct Leitura {
  unsigned long timestamp;
  float temperatura;
  float umidade;
  int   aqi;
};

Leitura historico[HISTORICO_TAM];
int idxHistorico  = 0;
int totalLeituras = 0;

void adicionarHistorico(float temp, float umi, int aqiVal) {
  historico[idxHistorico] = { millis(), temp, umi, aqiVal };
  idxHistorico = (idxHistorico + 1) % HISTORICO_TAM;
  if (totalLeituras < HISTORICO_TAM) totalLeituras++;
}

DHT               dht(PIN_DHT, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer         server(80);
WiFiClient        wifiClient;
PubSubClient      mqttClient(wifiClient);

float temperatura = 25.0;
float umidade     = 60.0;
int   aqi         = 0;

unsigned long ultimaLeitura = 0;
unsigned long ultimaMQTT    = 0;

const unsigned long INTERVALO_LEITURA = 5000;
const unsigned long INTERVALO_MQTT    = 3000;

void tentarReconectarMQTT() {
  if (mqttClient.connected()) return;
  if (millis() - ultimaMQTT < INTERVALO_MQTT) return;
  ultimaMQTT = millis();

  Serial.print("Tentando conectar ao MQTT... ");
  if (mqttClient.connect("ESP32_AirWatch_001")) {
    Serial.println("conectado.");
  } else {
    Serial.print("falhou. rc=");
    Serial.println(mqttClient.state());
  }
}

void handleRoot() {
  String faixaTexto, faixaCor;
  if      (aqi <= 100) { faixaTexto = "Boa";     faixaCor = "#27ae60"; }
  else if (aqi <= 200) { faixaTexto = "Moderada"; faixaCor = "#f39c12"; }
  else                 { faixaTexto = "Ruim";      faixaCor = "#e74c3c"; }

  String html = "<!DOCTYPE html><html lang='pt-BR'><head>";
  html += "<meta charset='UTF-8'><meta http-equiv='refresh' content='5'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>AirWatch IoT</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;text-align:center;padding:20px;margin:0}";
  html += "h1{color:#00d4ff;margin-bottom:30px}";
  html += ".card{background:#16213e;border-radius:12px;padding:20px;margin:10px auto;max-width:400px}";
  html += ".valor{font-size:2.5em;font-weight:bold;margin:10px 0}";
  html += ".label{font-size:0.9em;color:#aaa;text-transform:uppercase;letter-spacing:1px}";
  html += ".faixa{padding:8px 20px;border-radius:20px;font-weight:bold;display:inline-block;margin-top:10px}";
  html += ".links{margin-top:20px}.links a{color:#00d4ff;margin:0 10px;text-decoration:none}";
  html += "</style></head><body>";
  html += "<h1>AirWatch IoT</h1>";
  html += "<div class='card'><div class='label'>Temperatura</div>";
  html += "<div class='valor'>" + String(temperatura, 1) + " &deg;C</div></div>";
  html += "<div class='card'><div class='label'>Umidade</div>";
  html += "<div class='valor'>" + String(umidade, 0) + " %</div></div>";
  html += "<div class='card'><div class='label'>Indice de Qualidade do Ar (AQI)</div>";
  html += "<div class='valor'>" + String(aqi) + "</div>";
  html += "<div class='faixa' style='background:" + faixaCor + "'>" + faixaTexto + "</div></div>";
  html += "<div class='links'>";
  html += "<a href='/api/atual'>JSON atual</a> | ";
  html += "<a href='/api/historico'>Historico</a> | ";
  html += "<a href='/api/status'>Status</a>";
  html += "</div></body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleApiAtual() {
  String faixa;
  if      (aqi <= 100) faixa = "boa";
  else if (aqi <= 200) faixa = "moderada";
  else                 faixa = "ruim";

  String json = "{";
  json += "\"temperatura\":" + String(temperatura, 1) + ",";
  json += "\"umidade\":"     + String(umidade, 1)     + ",";
  json += "\"aqi\":"         + String(aqi)             + ",";
  json += "\"faixa\":\""     + faixa                  + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiHistorico() {
  String json = "[";
  int leituras = (totalLeituras < HISTORICO_TAM) ? totalLeituras : HISTORICO_TAM;
  int inicio   = (totalLeituras >= HISTORICO_TAM) ? idxHistorico : 0;

  for (int i = 0; i < leituras; i++) {
    int idx = (inicio + i) % HISTORICO_TAM;
    if (i > 0) json += ",";
    json += "{";
    json += "\"timestamp\":"   + String(historico[idx].timestamp)      + ",";
    json += "\"temperatura\":" + String(historico[idx].temperatura, 1) + ",";
    json += "\"umidade\":"     + String(historico[idx].umidade, 1)     + ",";
    json += "\"aqi\":"         + String(historico[idx].aqi);
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleApiStatus() {
  String json = "{";
  json += "\"status\":\"online\",";
  json += "\"wifi_rssi\":"       + String(WiFi.RSSI())              + ",";
  json += "\"ip\":\""            + WiFi.localIP().toString()         + "\",";
  json += "\"uptime_segundos\":" + String(millis() / 1000)           + ",";
  json += "\"total_leituras\":"  + String(totalLeituras)             + ",";
  json += "\"mqtt_conectado\":"  + String(mqttClient.connected() ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_DHT, INPUT_PULLUP);
  delay(2000);
  dht.begin();
  delay(2000);
  dht.readTemperature();
  dht.readHumidity();

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("AirWatch IoT");
  lcd.setCursor(0, 1); lcd.print("Iniciando...");
  delay(1000);

  pinMode(LED_VERDE,    OUTPUT);
  pinMode(LED_AMARELO,  OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER,   OUTPUT);
  digitalWrite(LED_VERDE,    LOW);
  digitalWrite(LED_AMARELO,  LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(PIN_BUZZER,   LOW);

  lcd.setCursor(0, 1); lcd.print("Conectando WiFi ");
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Conectando WiFi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK. IP: " + WiFi.localIP().toString());
    lcd.setCursor(0, 0); lcd.print("WiFi OK");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP().toString());
  } else {
    Serial.println("\nSem WiFi - modo offline.");
    lcd.setCursor(0, 0); lcd.print("WiFi: FALHOU");
    lcd.setCursor(0, 1); lcd.print("Modo offline");
  }
  delay(2000);

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setKeepAlive(30);

  server.on("/",              handleRoot);
  server.on("/api/atual",     handleApiAtual);
  server.on("/api/historico", handleApiHistorico);
  server.on("/api/status",    handleApiStatus);
  server.begin();
  Serial.println("Servidor web iniciado.");

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("AirWatch pronto");
  lcd.setCursor(0, 1); lcd.print("Aguardando...");
}

void loop() {
  server.handleClient();
  tentarReconectarMQTT();
  mqttClient.loop();

  if (millis() - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) umidade     = h;

    int raw = 0;
    for (int i = 0; i < 10; i++) {
      raw += analogRead(PIN_MQ135);
      delay(5);
    }
    raw /= 10;
    Serial.printf("RAW ADC: %d\n", raw);
    aqi = map(raw, 0, 4095, 0, 500);
    aqi = constrain(aqi, 0, 500);

    adicionarHistorico(temperatura, umidade, aqi);

    if (mqttClient.connected()) {
      String payload = "{";
      payload += "\"temperatura\":" + String(temperatura, 1) + ",";
      payload += "\"umidade\":"     + String(umidade, 1)     + ",";
      payload += "\"aqi\":"         + String(aqi);
      payload += "}";
      mqttClient.publish(MQTT_TOPIC, payload.c_str());
      Serial.println("MQTT: " + payload);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperatura, 1);
    lcd.print("C H:");
    lcd.print(umidade, 0);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("AQI:");
    lcd.print(aqi);
    if      (aqi <= 100) lcd.print(" BOM  ");
    else if (aqi <= 200) lcd.print(" MOD  ");
    else                 lcd.print(" RUIM ");

    if (aqi <= 100) {
      digitalWrite(LED_VERDE,    HIGH);
      digitalWrite(LED_AMARELO,  LOW);
      digitalWrite(LED_VERMELHO, LOW);
    } else if (aqi <= 200) {
      digitalWrite(LED_VERDE,    LOW);
      digitalWrite(LED_AMARELO,  HIGH);
      digitalWrite(LED_VERMELHO, LOW);
    } else {
      digitalWrite(LED_VERDE,    LOW);
      digitalWrite(LED_AMARELO,  LOW);
      digitalWrite(LED_VERMELHO, HIGH);
      buzzerBeep(2000, 200);
    }

    Serial.printf("[%lums] T=%.1fC H=%.0f%% AQI=%d\n",
                  millis(), temperatura, umidade, aqi);
  }
}
