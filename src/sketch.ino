// ─── Bibliotecas ─────────────────────────────────────────────────────────────
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
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

// ─── Instâncias dos objetos ──────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient        wifiClient;
PubSubClient      mqtt(wifiClient);
WebServer         server(80);

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

  Serial.printf("[SENSOR] AQI=%d | %s | %s\n",
    leituraAtual.aqi,
    leituraAtual.status.c_str(),
    leituraAtual.timestamp.c_str());
}

// ─── Endpoints HTTP ──────────────────────────────────────────────────────────
void handleLeituraAtual() {
  StaticJsonDocument<128> doc;
  doc["aqi"]       = leituraAtual.aqi;
  doc["status"]    = leituraAtual.status;
  doc["timestamp"] = leituraAtual.timestamp;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleHistorico() {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.createNestedArray("leituras");

  int total = min(totalLeituras, 10);
  for (int i = 0; i < total; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["aqi"]       = historico[i].aqi;
    obj["status"]    = historico[i].status;
    obj["timestamp"] = historico[i].timestamp;
  }
  doc["total"] = totalLeituras;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleStatus() {
  StaticJsonDocument<128> doc;
  doc["dispositivo"] = "AirWatch-ESP32";
  doc["wifi"]        = WiFi.isConnected() ? "conectado" : "desconectado";
  doc["mqtt"]        = mqtt.connected()   ? "conectado" : "desconectado";
  doc["uptime_s"]    = millis() / 1000;
  doc["leituras"]    = totalLeituras;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ─── Dashboard HTML ──────────────────────────────────────────────────────────
void handleDashboard() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="5">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AirWatch</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: Arial, sans-serif;
      background: #0B1320;
      color: #F1F5F9;
      min-height: 100vh;
      padding: 24px 16px;
    }
    header {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      margin-bottom: 32px;
    }
    .logo-title {
      font-size: 2rem;
      font-weight: 700;
      letter-spacing: 2px;
    }
    .logo-title span.air   { color: #F1F5F9; }
    .logo-title span.watch { color: #10B981; }
    .tagline {
      font-size: 0.65rem;
      letter-spacing: 3px;
      color: #22D3EE;
      text-transform: uppercase;
    }
    .divider {
      width: 100%;
      max-width: 480px;
      height: 1px;
      background: linear-gradient(90deg, transparent, #0EA5E9, #10B981, transparent);
      margin: 12px auto 0;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 16px;
      max-width: 480px;
      margin: 0 auto 16px;
    }
    .card {
      background: #0F1F35;
      border: 1px solid #1E3A5F;
      border-radius: 16px;
      padding: 20px 16px;
      position: relative;
      overflow: hidden;
    }
    .card::before {
      content: '';
      position: absolute;
      top: 0; left: 0; right: 0;
      height: 2px;
      background: linear-gradient(90deg, #0EA5E9, #10B981);
    }
    .card.full { grid-column: 1 / -1; }
    .card-label {
      font-size: 0.7rem;
      letter-spacing: 2px;
      text-transform: uppercase;
      color: #22D3EE;
      margin-bottom: 10px;
      font-weight: 600;
    }
    .aqi-value {
      font-size: 3rem;
      font-weight: 700;
      line-height: 1;
      margin-bottom: 8px;
    }
    .aqi-value.BOM       { color: #10B981; }
    .aqi-value.MODERADO  { color: #F59E0B; }
    .aqi-value.RUIM      { color: #EF4444; }
    .aqi-value.AGUARDANDO { color: #475569; }
    .pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 4px 12px;
      border-radius: 999px;
      font-size: 0.72rem;
      font-weight: 700;
      letter-spacing: 1px;
      text-transform: uppercase;
    }
    .pill.BOM       { background: #064E3B; color: #10B981; border: 1px solid #10B981; }
    .pill.MODERADO  { background: #451A03; color: #F59E0B; border: 1px solid #F59E0B; }
    .pill.RUIM      { background: #450A0A; color: #EF4444; border: 1px solid #EF4444; }
    .pill.AGUARDANDO { background: #1E293B; color: #64748B; border: 1px solid #334155; }
    .dot {
      width: 7px; height: 7px;
      border-radius: 50%;
      display: inline-block;
    }
    .dot.BOM       { background: #10B981; box-shadow: 0 0 6px #10B981; }
    .dot.MODERADO  { background: #F59E0B; box-shadow: 0 0 6px #F59E0B; }
    .dot.RUIM      { background: #EF4444; box-shadow: 0 0 6px #EF4444; animation: pulse 1s infinite; }
    .dot.AGUARDANDO { background: #475569; }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50%       { opacity: 0.3; }
    }
    .info-value {
      font-size: 1.1rem;
      font-weight: 600;
      color: #CBD5E1;
      margin-top: 4px;
    }
    .info-sub {
      font-size: 0.72rem;
      color: #475569;
      margin-top: 6px;
    }
    .status-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 8px 0;
      border-bottom: 1px solid #1E3A5F;
      font-size: 0.8rem;
    }
    .status-row:last-child { border-bottom: none; }
    .status-row .key { color: #64748B; }
    .status-row .val { color: #22D3EE; font-weight: 600; }
    footer {
      text-align: center;
      color: #1E3A5F;
      font-size: 0.65rem;
      letter-spacing: 2px;
      text-transform: uppercase;
      margin-top: 24px;
    }
  </style>
</head>
<body>
  <header>
    <div class="logo-title">
      <span class="air">Air</span><span class="watch">Watch</span>
    </div>
    <div class="tagline">Monitoring the Air. Protecting the Future.</div>
    <div class="divider"></div>
  </header>
  <div class="grid">
    <div class="card full">
      <div class="card-label">Índice de Qualidade do Ar (AQI)</div>
      <div class="aqi-value )rawhtml";
  html += leituraAtual.status;
  html += "\">";
  html += leituraAtual.aqi;
  html += R"rawhtml(</div>
      <span class="pill )rawhtml";
  html += leituraAtual.status;
  html += "\"><span class=\"dot ";
  html += leituraAtual.status;
  html += "\"></span>";
  html += leituraAtual.status;
  html += R"rawhtml(</span>
    </div>
    <div class="card">
      <div class="card-label">Última Leitura</div>
      <div class="info-value">)rawhtml";
  html += leituraAtual.timestamp;
  html += R"rawhtml(</div>
      <div class="info-sub">tempo de operação</div>
    </div>
    <div class="card">
      <div class="card-label">Total de Leituras</div>
      <div class="info-value">)rawhtml";
  html += totalLeituras;
  html += R"rawhtml(</div>
      <div class="info-sub">registros na sessão</div>
    </div>
    <div class="card full">
      <div class="card-label">Status do Sistema</div>
      <div class="status-row">
        <span class="key">Dispositivo</span>
        <span class="val">AirWatch-ESP32</span>
      </div>
      <div class="status-row">
        <span class="key">Wi-Fi</span>
        <span class="val">CONECTADO</span>
      </div>
      <div class="status-row">
        <span class="key">MQTT</span>
        <span class="val">CONECTADO</span>
      </div>
    </div>
  </div>
  <footer>Atualiza a cada 5s &nbsp;·&nbsp; Solstício &nbsp;·&nbsp; FIAP 2026</footer>
</body>
</html>
)rawhtml";
  server.send(200, "text/html", html);
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

  server.on("/",              handleDashboard);
  server.on("/leitura/atual", handleLeituraAtual);
  server.on("/historico",     handleHistorico);
  server.on("/status",        handleStatus);
  server.begin();
  Serial.println("[HTTP] Servidor iniciado");

  atualizarLCD();
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();
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
