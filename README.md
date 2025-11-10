# 🌟 Sistema IoT de Controle de Iluminação Inteligente

Sistema completo de Internet das Coisas (IoT) para monitoramento e controle automático de iluminação RGB baseado em sensores de luminosidade. O projeto integra hardware embarcado (ESP8266), API REST (Django) e interface web/mobile.

## 📋 Visão Geral

Este projeto implementa um sistema inteligente de controle de iluminação que:

- **Monitora** níveis de luminosidade ambiente usando sensor LDR
- **Controla automaticamente** LEDs RGB WS2812B baseado na luz ambiente
- **Permite controle manual** via interface web/app
- **Armazena dados** históricos em banco de dados
- **Expõe APIs REST** para integração com outros sistemas

### 🏗️ Arquitetura

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   ESP8266       │    │   Django API    │    │   Frontend      │
│   (Sensor)      │◄──►│   (Backend)     │◄──►│   (Interface)   │
│                 │    │                 │    │                 │
│ • Sensor LDR    │    │ • REST API      │    │ • Dashboard     │
│ • LEDs RGB      │    │ • SQLite DB     │    │ • Controle      │
│ • WiFi          │    │ • Nginx Proxy   │    │ • Histórico     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 🚀 Tecnologias Utilizadas

### Hardware
- **ESP8266** - Microcontrolador WiFi
- **Sensor LDR** - Sensor de luminosidade analógico
- **LEDs WS2812B** - Fita de LEDs RGB endereçáveis
- **FastLED** - Biblioteca para controle de LEDs

### Backend
- **Django 5.0** - Framework web Python
- **Django REST Framework** - API REST
- **SQLite** - Banco de dados
- **Gunicorn** - Servidor WSGI
- **Nginx** - Proxy reverso e servidor web

### Frontend
- **React Native** - Desenvolvimento mobile
- **Axios** - Cliente HTTP
- **Expo** - Plataforma de desenvolvimento

## 📁 Estrutura do Projeto

```
embarcados25.1/
├── backend/                    # API Django
│   ├── config/                 # Configurações Django
│   │   ├── settings.py        # Configurações principais
│   │   ├── urls.py            # URLs principais
│   │   └── wsgi.py            # Ponto de entrada WSGI
│   ├── api/                   # Aplicação da API
│   │   ├── models.py          # Modelos de dados
│   │   ├── views.py           # Views da API
│   │   ├── serializers.py     # Serializers REST
│   │   ├── urls.py            # URLs da API
│   │   └── apps.py            # Configuração da app
│   ├── db.sqlite3             # Banco de dados
│   ├── manage.py              # Utilitário Django
│   ├── requirements.txt       # Dependências Python
│   └── venv/                  # Ambiente virtual
├── frontend/                  # App React Native
│   ├── App.js                 # Componente principal
│   ├── index.js               # Ponto de entrada
│   └── package.json           # Dependências Node.js
├── codigo_emb.ino             # Código Arduino ESP8266
└── README.md                  # Esta documentação
```

## 🔧 Instalação e Configuração

### Pré-requisitos

- **Python 3.8+** para o backend
- **Node.js 16+** para o frontend
- **Arduino IDE** para o ESP8266
- **Git** para controle de versão

### 1. Backend (Django API)

```bash
# Clonar o repositório
cd backend/

# Criar ambiente virtual
python -m venv venv
source venv/bin/activate  # Linux/Mac
# ou venv\Scripts\activate  # Windows

# Instalar dependências
pip install -r requirements.txt

# Executar migrações do banco
python manage.py migrate

# Criar superusuário (opcional)
python manage.py createsuperuser

# Executar servidor de desenvolvimento
python manage.py runserver
```

**API estará disponível em:** `http://localhost:8000`

### 2. Servidor de Produção

```bash
# Instalar Gunicorn e Nginx
sudo apt update
sudo apt install nginx gunicorn

# Configurar serviço systemd
sudo cp iot-backend.service /etc/systemd/system/
sudo systemctl enable iot-backend
sudo systemctl start iot-backend

# Configurar Nginx
sudo cp iot-backend.nginx /etc/nginx/sites-available/
sudo ln -s /etc/nginx/sites-available/iot-backend /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

### 3. ESP8266 (Arduino)

1. **Instalar Arduino IDE**
2. **Instalar bibliotecas:**
   - FastLED
   - ESP8266WiFi
   - ESP8266HTTPClient
   - ArduinoJson

3. **Configurar código:**
   ```cpp
   // Em codigo_emb.ino
   const char* WIFI_SSID = "SeuWiFi";
   const char* WIFI_PASSWORD = "SuaSenha";
   const char* API_BASE_URL = "http://seu-servidor.com";
   ```

4. **Upload para ESP8266**

### 4. Frontend (React Native)

```bash
cd frontend/
npm install
npm start
# ou
npx expo start
```

## 📡 APIs Disponíveis

### Status
```http
GET /api/status/
```
**Resposta:** `{"status": "online"}`

### Luminosidade
```http
GET /api/luminosidade/
POST /api/luminosidade/
```

**GET Response:**
```json
{
  "id": 1,
  "valor": 562,
  "modo": "Crepúsculo",
  "timestamp": "2025-11-09T22:41:40.095387Z"
}
```

**POST Body:**
```json
{
  "valor": 562,
  "modo": "Crepúsculo"
}
```

### Controle
```http
GET /api/controle/
POST /api/controle/
```

**GET Response:**
```json
{
  "modo": "auto",
  "cor": {
    "r": 255,
    "g": 0,
    "b": 0
  }
}
```

**POST Body:**
```json
{
  "modo": "manual",
  "cor": {
    "r": 255,
    "g": 0,
    "b": 0
  }
}
```

## 🎮 Como Usar

### 1. Sistema Automático
O ESP8266 monitora a luminosidade e controla os LEDs automaticamente:
- **Escuridão (< 300)**: LED vermelho
- **Crepúsculo (300-600)**: LED azul
- **Luz adequada (600-800)**: LED branco
- **Luz intensa (> 800)**: LEDs desligados

### 2. Controle Manual
Via API ou app, envie comandos para controle manual:
```bash
curl -X POST http://servidor/api/controle/ \
  -H "Content-Type: application/json" \
  -d '{"modo": "manual", "cor": {"r": 255, "g": 0, "b": 0}}'
```

### 3. Monitoramento
```bash
# Ver última leitura
curl http://servidor/api/luminosidade/

# Ver status do sistema
curl http://servidor/api/status/
```

## 🔍 Monitoramento e Debug

### Logs do Servidor
```bash
# Logs do Nginx
sudo tail -f /var/log/nginx/access.log

# Logs do Django/Gunicorn
sudo journalctl -u iot-backend -f
```

### Banco de Dados
```bash
# Acessar SQLite
cd backend/
sqlite3 db.sqlite3

# Ver leituras recentes
SELECT * FROM api_luminosidadereading;

# Ver estado de controle
SELECT * FROM api_controlstate;
```

## 📊 Funcionalidades

- ✅ **Monitoramento em tempo real** de luminosidade
- ✅ **Controle automático** baseado em thresholds
- ✅ **Controle manual** via API
- ✅ **Histórico de dados** em banco
- ✅ **APIs REST** documentadas
- ✅ **Logs detalhados** para debug
- ✅ **Configuração de produção** com Nginx/Gunicorn
- ✅ **Documentação completa** de troubleshooting
