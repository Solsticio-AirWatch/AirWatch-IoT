# AirWatch - IoT

Módulo de Internet das Coisas do projeto AirWatch.

## Status

**Funcional** – firmware em operação com todas as funcionalidades implementadas e testadas em simulação Wokwi (VS Code + PlatformIO).

## Visão Geral

Este repositório contém o firmware para ESP32, desenvolvido para monitorar qualidade do ar, temperatura e umidade. Os dados são enviados via MQTT, disponibilizados por um servidor web embarcado (dashboard HTML e endpoints JSON) e exibidos em um display LCD com alertas visuais e sonoros.

## Funcionalidades

- Leitura de **temperatura** e **umidade** (sensor DHT22)
- Leitura de **qualidade do ar** (simulada por potenciômetro – equivalente a sensor MQ‑135)
- Exibição em **LCD 16x2 I2C** (atualização a cada 5 segundos)
- Alertas visuais com **LEDs tricolor**:
  - AQI ≤ 100 → verde
  - 101 ≤ AQI ≤ 200 → amarelo
  - AQI > 200 → vermelho
- Alerta sonoro com **buzzer** (beep curto quando AQI > 200)
- **Servidor web** embarcado com:
  - Dashboard HTML (rota `/`) com atualização automática
  - Endpoints JSON:
    - `/api/atual` – última leitura (temp, umid, AQI, faixa)
    - `/api/historico` – últimas 10 leituras (timestamp, temp, umid, AQI)
    - `/api/status` – status do sistema (WiFi RSSI, IP, uptime, total leituras, MQTT conectado)
- **Cliente MQTT** – publica leituras no broker público HiveMQ (tópico `airwatch/sensor/001`)

## Estrutura do Repositório
```
AirWatch-IoT/
├── .gitignore
├── platformio.ini # Configuração do PlatformIO
├── wokwi.toml # Configuração da extensão Wokwi
├── diagram.json # Esquema elétrico para simulação
├── README.md # Este arquivo
└── src/
└── sketch.ino # Código principal do firmware
```

## Como Executar (Simulação Local)

### Pré‑requisitos

- Visual Studio Code
- Extensão **PlatformIO IDE**
- Extensão **Wokwi Simulator**

### Passo a passo

1. Clone o repositório:
   ```bash
   git clone https://github.com/Solsticio-AirWatch/AirWatch-IoT.git
   ```

2. Abra a pasta no VS Code.

3. Aguarde o PlatformIO baixar as dependências (primeira vez pode demorar alguns minutos).

4. Compile o firmware:

    - Clique no ícone do PlatformIO na barra lateral.
    - Em PROJECT TASKS → wokwi → Build (ícone ✓).

5. Inicie a simulação:

    - Clique no ícone do Wokwi (chip com fio).
    - Selecione "Start Simulation".

6. O Serial Monitor será aberto automaticamente (baud rate 115200). O IP do ESP32 simulado aparecerá (ex: http://localhost:8080).

7. Acesse o IP no navegador para visualizar o dashboard.

## Testando as funcionalidades

| Funcionalidade       | Como testar                                                                                           |
|----------------------|--------------------------------------------------------------------------------------------------------|
| LCD                  | Exibe `T:XX.XC H:XX%` e `AQI:XXX BOM/MOD/RUIM`.                                                       |
| Potenciômetro (AQI)  | Na janela do Wokwi, arraste a bolinha do potenciômetro. O valor do AQI muda.                           |
| LEDs                 | Gire o potenciômetro: verde (<100), amarelo (101‑200), vermelho (>200).                                |
| Buzzer               | Quando AQI > 200, um beep curto (200ms) é emitido.                                                    |
| Dashboard            | `http://localhost:8080` – mostra valores atualizados a cada 5 segundos.                               |
| Endpoint `/api/atual` | Abra no navegador – retorna JSON com a última leitura.                                                |
| Endpoint `/api/historico` | Retorna as últimas 10 leituras.                                                                      |
| Endpoint `/api/status`  | Retorna status do sistema (WiFi, IP, uptime, total leituras, MQTT).                                   |
| MQTT                 | Assine o tópico `airwatch/sensor/001` no [HiveMQ Web Client](http://www.hivemq.com/demos/websocket-client/) e veja as mensagens. |

## Documentação da API (Endpoints)

Todos os endpoints retornam application/json.

`GET /api/atual`

Exemplo de resposta:
```bash
{
  "temperatura": 25.3,
  "umidade": 61.0,
  "aqi": 120,
  "faixa": "moderada"
}
```

---

`GET /api/historico`

Exemplo (últimas 2 leituras):
```bash
[
  {"timestamp":123456,"temperatura":25.3,"umidade":61.0,"aqi":120},
  {"timestamp":128456,"temperatura":25.4,"umidade":60.5,"aqi":118}
]
```

---

`GET /api/status`

Exemplo:
```bash
{
  "status": "online",
  "wifi_rssi": -45,
  "ip": "10.0.0.2",
  "uptime_segundos": 120,
  "total_leituras": 24,
  "mqtt_conectado": true
}
```

---

## Hardware (Simulado)

- ESP32 DevKit V4
- DHT22 (temperatura/umidade)
- Potenciômetro (simula MQ‑135)
- LCD 16x2 I2C (endereço 0x27)
- 3 LEDs (verde, amarelo, vermelho) com resistores de 220Ω
- Buzzer passivo

## Equipe

| Nome                         | RM         |
|------------------------------|------------|
| Felipe Kirschner Modesto     | RM561810   |
| Enrico Delesporte            | RM565760   |
| Vitor Dias dos Santos        | RM565422   |

### Licença

Uso acadêmico – todos os direitos reservados à equipe Solstício.

### Vídeo de Demonstração

🔗 Link do vídeo será adicionado após gravação.

---

*Projeto desenvolvido para a Global Solution 2026/1 – FIAP*
