# AirWatch - IoT
Módulo de Internet das Coisas do projeto AirWatch.

## Sobre o Projeto

O AirWatch IoT é uma estação de monitoramento de qualidade do ar simulada no Wokwi, utilizando um ESP32 como microcontrolador central. O sistema permite injetar leituras de qualidade do ar via botões físicos, exibindo o índice AQI em tempo real no LCD e nos LEDs indicadores — além de publicar os dados via MQTT e enviá-los para uma API REST hospedada no Render.

## Solução Implementada

A implementação inicial com os sensores DHT22 e potenciômetro (simulando MQ-135) apresentou instabilidades na simulação Wokwi, com leituras inconsistentes e dificuldades de validação dos componentes.

Diante disso, a equipe optou por uma **readequação da solução**, mantendo o mesmo propósito (monitoramento da qualidade do ar) com uma abordagem mais confiável para simulação: **3 botões representando os níveis BOM, MODERADO e RUIM**, que injetam leituras AQI diretamente no sistema.

## Hardware (Wokwi)

| Componente     | Pino ESP32     | Função                            |
|----------------|----------------|-----------------------------------|
| Botão Verde    | 12             | Injeta leitura AQI 30 — BOM       |
| Botão Amarelo  | 13             | Injeta leitura AQI 120 — MODERADO |
| Botão Vermelho | 14             | Injeta leitura AQI 250 — RUIM     |
| LED Verde      | 25             | Indicador visual — ar bom         |
| LED Amarelo    | 26             | Indicador visual — ar moderado    |
| LED Vermelho   | 27             | Indicador visual — ar ruim        |
| Buzzer         | 32             | Alerta sonoro — ar ruim           |
| LCD 16x2 I2C   | SDA=21, SCL=22 | Exibe AQI e status em tempo real  |

## Funcionalidades

- Leitura de qualidade do ar via botões (BOM / MODERADO / RUIM)
- Indicação visual com LEDs tricolor
- Alerta sonoro via buzzer quando qualidade está ruim (500Hz, 1.5s)
- Display LCD 16x2 exibindo AQI e status — volta para tela de espera após 15s
- Conexão Wi-Fi e publicação de dados via MQTT (HiveMQ)
- Envio de dados via HTTP POST para API REST no Render
- Dashboard HTML com identidade visual AirWatch
- Histórico das últimas 10 leituras em memória

## API REST

**Base URL:** `https://airwatch-iot.onrender.com`

| Método | Rota             | Descrição                              |
|--------|------------------|----------------------------------------|
| GET    | `/`              | Dashboard HTML com auto-refresh (5s)   |
| POST   | `/leitura`       | Recebe leitura do ESP32 em JSON        |
| GET    | `/leitura/atual` | Leitura mais recente em JSON           |
| GET    | `/historico`     | Últimas 10 leituras em JSON            |
| GET    | `/status`        | Status da API                          |

**Exemplo de payload enviado pelo ESP32:**
```json
{
  "aqi": 250,
  "status": "RUIM",
  "timestamp": "00:12:34"
}
```

## Estrutura do Repositório
```
AirWatch-IoT/
├── api/
│   ├── app.py               # API Flask hospedada no Render
│   ├── requirements.txt     # Dependências Python
│   └── Procfile             # Configuração de deploy
├── src/
│   └── sketch.ino           # Firmware do ESP32
├── diagram.json             # Esquema elétrico para simulação
├── platformio.ini           # Configuração do PlatformIO
├── wokwi.toml               # Configuração da extensão Wokwi
└── README.md
```

## Equipe

| Nome                     | RM       |
|--------------------------|----------|
| Felipe Kirschner Modesto | RM561810 |
| Enrico Delesporte        | RM565760 |
| Vitor Dias dos Santos    | RM565422 |

## Licença
Uso acadêmico – todos os direitos reservados à equipe Solstício.

## Vídeo de Demonstração
🔗 Link do vídeo será adicionado após gravação.

---
*Projeto desenvolvido para a Global Solution 2026/1 – FIAP*
