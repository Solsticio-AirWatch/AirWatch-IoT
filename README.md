# AirWatch - IoT
Módulo de Internet das Coisas do projeto AirWatch.

## Sobre o Projeto

O AirWatch IoT é uma estação de monitoramento de qualidade do ar simulada no Wokwi, utilizando um ESP32 como microcontrolador central. O sistema permite injetar leituras de qualidade do ar via botões físicos, exibindo o índice AQI em tempo real no LCD, nos LEDs indicadores e no dashboard web — além de publicar os dados via MQTT e expor endpoints JSON para integração com outros módulos do projeto.

## Solução Implementada

A implementação inicial com os sensores DHT22 e potenciômetro (simulando MQ-135) apresentou instabilidades na simulação Wokwi, com leituras inconsistentes e dificuldades de validação dos componentes.

Diante disso, a equipe optou por uma **readequação da solução**, mantendo o mesmo propósito (monitoramento da qualidade do ar) com uma abordagem mais confiável para simulação: **3 botões representando os níveis BOM, MODERADO e RUIM**, que injetam leituras AQI diretamente no sistema.

Toda a lógica de negócio foi mantida integralmente: índice AQI, alertas sonoros e visuais, dashboard HTML com identidade visual AirWatch, 3 endpoints JSON e publicação MQTT.

## Hardware (Wokwi)

| Componente         | Pino ESP32 | Função                              |
|--------------------|------------|-------------------------------------|
| Botão Verde        | 12         | Injeta leitura AQI 30 — BOM         |
| Botão Amarelo      | 13         | Injeta leitura AQI 120 — MODERADO   |
| Botão Vermelho     | 14         | Injeta leitura AQI 250 — RUIM       |
| LED Verde          | 25         | Indicador visual — ar bom           |
| LED Amarelo        | 26         | Indicador visual — ar moderado      |
| LED Vermelho       | 27         | Indicador visual — ar ruim          |
| Buzzer             | 32         | Alerta sonoro — ar ruim             |
| LCD 16x2 I2C       | SDA=21, SCL=22 | Exibe AQI e status em tempo real |

## Funcionalidades

- Leitura de qualidade do ar via botões (BOM / MODERADO / RUIM)
- Indicação visual com LEDs tricolor
- Alerta sonoro via buzzer quando qualidade está ruim
- Display LCD 16x2 exibindo AQI e status atual
- Conexão Wi-Fi e publicação de dados via MQTT (HiveMQ)
- Dashboard HTML servido pelo próprio ESP32 com identidade visual AirWatch
- Histórico circular das últimas 10 leituras em memória

## Endpoints HTTP

| Método | Rota             | Descrição                              |
|--------|------------------|----------------------------------------|
| GET    | `/`              | Dashboard HTML com auto-refresh (5s)   |
| GET    | `/leitura/atual` | Leitura mais recente em JSON           |
| GET    | `/historico`     | Últimas 10 leituras em JSON            |
| GET    | `/status`        | Status do dispositivo (Wi-Fi, MQTT...) |

## Estrutura do Repositório
```
AirWatch-IoT/
├── .gitignore
├── platformio.ini       # Configuração do PlatformIO
├── wokwi.toml           # Configuração da extensão Wokwi
├── diagram.json         # Esquema elétrico para simulação
├── README.md            # Este arquivo
└── src/
└── sketch.ino       # Código principal do firmware
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
