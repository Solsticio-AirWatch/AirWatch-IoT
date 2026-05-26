# AirWatch - IoT

Módulo de Internet das Coisas do projeto AirWatch.

## Status do Desenvolvimento

**Nota sobre a abordagem atual (26/05/2026):**  
A implementação inicial com os sensores DHT22 e potenciômetro (simulando MQ-135) apresentou instabilidades na simulação Wokwi, com leituras inconsistentes e dificuldades de validação dos componentes.  

Diante disso, a equipe optou por uma **readequação da solução**, mantendo o mesmo propósito (monitoramento da qualidade do ar) porém utilizando componentes mais confiáveis na simulação. Ainda está sendo discutido qual será a abordagem.

A lógica de negócio (índice AQI, alertas, dashboard, endpoints e MQTT) será mantida integralmente, apenas substituindo as entradas por componentes mais simples, e assim seguindo algo que seja plenamente aceito pelos critérios da disciplina.  

A nova versão está em fase de implementação e será finalizada com todas as funcionalidades exigidas (WiFi, MQTT, 3 endpoints JSON, dashboard HTML). O cronograma não será afetado.

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
