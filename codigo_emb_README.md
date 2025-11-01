# Configuração do Código Arduino (ESP8266)

## Bibliotecas Necessárias

Instale as seguintes bibliotecas via Arduino Library Manager:

1. **FastLED** - Para controle dos LEDs WS2812
2. **ArduinoJson** (versão 6.x) - Para parsing de JSON nas requisições HTTP

As bibliotecas WiFi (`ESP8266WiFi.h`, `ESP8266HTTPClient.h`) já vêm incluídas com o board ESP8266.

## Configurações Necessárias

Antes de fazer upload do código, edite as seguintes constantes em `codigo_emb.ino`:

### 1. Credenciais WiFi
```cpp
const char* WIFI_SSID = "SEU_WIFI_SSID";        // Altere para o SSID da sua rede
const char* WIFI_PASSWORD = "SUA_SENHA_WIFI";   // Altere para a senha da sua rede
```

### 2. URL do Backend
```cpp
const char* API_BASE_URL = "http://vps.ufsc.br";  // URL do backend sem porta (Nginx cuida disso)
```

Para desenvolvimento local, use o IP da sua máquina:
```cpp
const char* API_BASE_URL = "http://192.168.1.100";  // Substitua pelo IP da sua máquina
```

## Funcionalidades Implementadas

### Modo Automático (Padrão)
- O sistema inicia em modo automático
- Lê o sensor LDR continuamente
- Controla os LEDs baseado na luminosidade:
  - **0-300**: Vermelho (Escuridão)
  - **301-600**: Azul (Crepúsculo)
  - **601-800**: Branco suave (Luz adequada)
  - **801-1023**: Desligado (Luz intensa)

### Modo Manual
- Ativado via comando do aplicativo móvel
- Permite definir cores RGB específicas
- Timeout de 30 segundos: retorna ao automático se não houver novos comandos

### Comunicação com Backend
- **POST /api/luminosidade/**: Envia leituras a cada 5 segundos
- **GET /api/controle/**: Verifica comandos manuais a cada 2 segundos
- Reconexão WiFi automática em caso de desconexão

## Hardware

- **Microcontrolador**: Wemos D1 R2 (ESP8266)
- **Pino LED**: D7 (conectado ao Din da fita WS2812)
- **Pino Sensor**: A0 (conectado ao LDR via divisor de tensão com resistor 10kΩ)
- **Numero de LEDs**: 8 (configurável via `NUM_LEDS`)

## Debug

O código envia informações detalhadas via Serial Monitor (115200 baud):
- Status de conexão WiFi
- Leituras de luminosidade
- Modo de operação atual
- Comandos recebidos do backend
- Erros de comunicação HTTP

