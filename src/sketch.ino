#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

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

float temperatura = 0;
float umidade = 0;
int aqi = 0;
unsigned long ultimaLeitura = 0;
unsigned long inicio = 0;
int totalLeituras = 0;

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
    if (historico[i].aqi == 0 && historico[i].timestamp == 0 && i > 0) {
      continue;
    }
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

  server.on("/", handleRoot);
  server.on("/api/atual", handleApiAtual);
  server.on("/api/historico", handleApiHistorico);
  server.on("/api/status", handleApiStatus);
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  server.handleClient();

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

    if (aqi <= 100) {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_AMARELO, LOW);
      digitalWrite(LED_VERMELHO, LOW);
      noTone(BUZZER);
    } else if (aqi <= 200) {
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_AMARELO, HIGH);
      digitalWrite(LED_VERMELHO, LOW);
      noTone(BUZZER);
    } else {
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_AMARELO, LOW);
      digitalWrite(LED_VERMELHO, HIGH);
      tone(BUZZER, 2000);
    }

    ultimaLeitura = millis();
  }
  delay(10);
}
