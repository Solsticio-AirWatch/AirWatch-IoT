// ─── Bibliotecas ─────────────────────────────────────────────────────────────
#include <Wire.h>              // Comunicação I2C (LCD)
#include <LiquidCrystal_I2C.h> // Controle do LCD 16x2 via I2C
#include <WiFi.h>              // Conexão Wi-Fi do ESP32
#include <PubSubClient.h>      // Cliente MQTT
#include <WebServer.h>         // Servidor HTTP embutido no ESP32
#include <ArduinoJson.h>       // Serialização e deserialização JSON

// ─── Pinagem ─────────────────────────────────────────────────────────────────
#define BTN_BOM       12  // Botão verde — injeta leitura "Ar Bom"
#define BTN_MODERADO  13  // Botão amarelo — injeta leitura "Ar Moderado"
#define BTN_RUIM      14  // Botão vermelho — injeta leitura "Ar Ruim"
#define LED_VERDE     25  // LED verde — indica qualidade boa
#define LED_AMARELO   26  // LED amarelo — indica qualidade moderada
#define LED_VERMELHO  27  // LED vermelho — indica qualidade ruim
#define BUZZER        32  // Buzzer — alarme sonoro quando ar está ruim

// ─── Credenciais Wi-Fi ───────────────────────────────────────────────────────
const char* WIFI_SSID     = "Wokwi-GUEST"; // SSID padrão do Wokwi (sem senha)
const char* WIFI_PASSWORD = "";

// ─── Configuração MQTT ───────────────────────────────────────────────────────
const char* MQTT_BROKER = "broker.hivemq.com"; // Broker público HiveMQ
const int   MQTT_PORT   = 1883;                // Porta padrão MQTT
const char* MQTT_TOPIC  = "airwatch/leitura";  // Tópico de publicação
const char* MQTT_CLIENT = "airwatch-esp32-solsticio"; // ID único do cliente

// ─── Instâncias dos objetos ──────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD no endereço I2C 0x27, 16 colunas, 2 linhas
WiFiClient        wifiClient;        // Cliente TCP para o MQTT
PubSubClient      mqtt(wifiClient);  // Cliente MQTT usando o WiFiClient
WebServer         server(80);        // Servidor HTTP na porta 80

// ─── Estrutura de dados ──────────────────────────────────────────────────────
struct Leitura {
  int    aqi;       // Índice de Qualidade do Ar (AQI)
  String status;    // Status textual: BOM, MODERADO ou RUIM
  String timestamp; // Horário da leitura no formato HH:MM:SS
};

// ─── Estado global ───────────────────────────────────────────────────────────
Leitura leituraAtual  = { 0, "AGUARDANDO", "--" }; // Leitura mais recente
Leitura historico[10];      // Histórico circular com as últimas 10 leituras
int     totalLeituras = 0;  // Contador total de leituras realizadas
unsigned long ultimaLeitura = 0; // Timestamp do último botão pressionado (debounce)

// ─── Timestamp ───────────────────────────────────────────────────────────────
// Gera um horário no formato HH:MM:SS baseado no tempo de execução do ESP32
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
// Atualiza o display com o AQI atual e o status na segunda linha
void atualizarLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AQI: ");
  lcd.print(leituraAtual.aqi);
  lcd.setCursor(0, 1);
  lcd.print(leituraAtual.status);
}

// ─── LEDs e Buzzer ───────────────────────────────────────────────────────────
// Apaga todos os LEDs e o buzzer, depois acende o LED correspondente ao nível
// de qualidade do ar. Se ruim, ativa o buzzer também.
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
    digitalWrite(BUZZER, HIGH); // Alerta sonoro apenas quando ar está ruim
  }
}

// ─── MQTT ────────────────────────────────────────────────────────────────────
// Tenta conectar ao broker MQTT caso ainda não esteja conectado
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

// Serializa a leitura atual em JSON e publica no tópico MQTT
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
// Atualiza o estado global, salva no histórico circular,
// aciona saídas físicas, atualiza LCD e publica via MQTT
void registrarLeitura(int aqi, const String& status) {
  leituraAtual.aqi       = aqi;
  leituraAtual.status    = status;
  leituraAtual.timestamp = getTimestamp();

  historico[totalLeituras % 10] = leituraAtual; // Histórico circular (máx 10)
  totalLeituras++;

  atualizarSaidas();
  atualizarLCD();
  publicarMQTT();

  Serial.printf("[SENSOR] AQI=%d | %s | %s\n",
    leituraAtual.aqi,
    leituraAtual.status.c_str(),
    leituraAtual.timestamp.c_str());
}

// ─── Endpoint: GET /leitura/atual ────────────────────────────────────────────
// Retorna a leitura mais recente em formato JSON
void handleLeituraAtual() {
  StaticJsonDocument<128> doc;
  doc["aqi"]       = leituraAtual.aqi;
  doc["status"]    = leituraAtual.status;
  doc["timestamp"] = leituraAtual.timestamp;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ─── Endpoint: GET /historico ────────────────────────────────────────────────
// Retorna as últimas 10 leituras e o total acumulado em formato JSON
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

// ─── Endpoint: GET /status ───────────────────────────────────────────────────
// Retorna informações de saúde do dispositivo: Wi-Fi, MQTT, uptime e leituras
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

// ─── Endpoint: GET / — Dashboard HTML ────────────────────────────────────────
// Serve uma página HTML com identidade visual AirWatch que exibe
// o AQI atual, status, última leitura e status do sistema.
// A página faz auto-refresh a cada 5 segundos.
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
// Inicializa pinos, LCD, Wi-Fi, MQTT e servidor HTTP
void setup() {
  Serial.begin(115200);

  // Botões com pull-up interno (LOW = pressionado)
  pinMode(BTN_BOM,      INPUT_PULLUP);
  pinMode(BTN_MODERADO, INPUT_PULLUP);
  pinMode(BTN_RUIM,     INPUT_PULLUP);

  // Saídas digitais
  pinMode(LED_VERDE,    OUTPUT);
  pinMode(LED_AMARELO,  OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER,       OUTPUT);

  // Inicializa LCD via I2C nos pinos SDA=21, SCL=22
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AirWatch v1.0");
  lcd.setCursor(0, 1);
  lcd.print("Conectando...");

  // Conecta ao Wi-Fi e aguarda conexão
  Serial.print("[WiFi] Conectando");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Conectado! IP: " + WiFi.localIP().toString());

  // Exibe IP no LCD por 2 segundos
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(2000);

  // Configura e conecta ao broker MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  conectarMQTT();

  // Registra os endpoints HTTP
  server.on("/",              handleDashboard);
  server.on("/leitura/atual", handleLeituraAtual);
  server.on("/historico",     handleHistorico);
  server.on("/status",        handleStatus);
  server.begin();
  Serial.println("[HTTP] Servidor iniciado");

  // Exibe estado inicial no LCD
  atualizarLCD();
}

// ─── Loop ────────────────────────────────────────────────────────────────────
// Mantém o servidor HTTP e o MQTT ativos, e monitora os botões
// com debounce de 500ms para evitar leituras duplicadas
void loop() {
  server.handleClient(); // Processa requisições HTTP
  mqtt.loop();           // Mantém conexão MQTT ativa

  // Debounce: ignora acionamentos dentro de 500ms
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
