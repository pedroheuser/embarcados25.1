#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// ===== CONFIGURAÇÕES DOS LEDs =====
#define LED_PIN D7        // Pino de dados conectado ao Din da fita
#define NUM_LEDS 8        // Número de LEDs na fita
#define BRIGHTNESS 255    // Brilho máximo (0-255)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// ===== CONFIGURAÇÕES DO SENSOR =====
#define LDR_PIN A0        // Pino analógico do sensor de luminosidade

// ===== CONFIGURAÇÕES WiFi =====
const char* WIFI_SSID = "iPhone de Pedro Heuser";        // Altere para o SSID da sua rede
const char* WIFI_PASSWORD = "12345679";   // Altere para a senha da sua rede

// ===== CONFIGURAÇÕES DO BACKEND =====
const char* API_BASE_URL = "http://embarcados.pedro.heuser.vms.ufsc.br";
// ===== CONSTANTES DE MODOS =====
#define MODO_AUTO 0
#define MODO_MANUAL 1

// ===== VARIÁVEIS GLOBAIS =====
int ldrValue = 0;
int modoOperacao = MODO_AUTO;  // Inicia em modo automático

// Variáveis para comando manual
int manualR = 0;
int manualG = 0;
int manualB = 0;
unsigned long ultimoComandoManual = 0;
const unsigned long TIMEOUT_COMANDO_MANUAL = 30000;  // 30 segundos

// Timing para requisições HTTP
unsigned long ultimaLeituraEnviada = 0;
unsigned long ultimoComandoVerificado = 0;
const unsigned long INTERVALO_ENVIAR_LEITURA = 5000;   // 5 segundos
const unsigned long INTERVALO_VERIFICAR_COMANDO = 2000; // 2 segundos

// Objeto WiFiClient para requisições HTTP
WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n=== Sistema de Controle de Iluminação ===");
  
  // Inicialização dos LEDs com FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  
  Serial.println("✓ LEDs inicializados com FastLED");
  
  // Conectar ao WiFi
  connectWiFi();
  
  Serial.println("=== Sistema pronto! ===\n");
}

void loop() {
  // Verificar e reconectar WiFi se necessário
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Tentando reconectar...");
    connectWiFi();
  }
  
  // Leitura do sensor de luminosidade (0-1023)
  ldrValue = analogRead(LDR_PIN);
  
  unsigned long tempoAtual = millis();
  
  // Verificar timeout do comando manual (30 segundos sem novo comando)
  if (modoOperacao == MODO_MANUAL && 
      (tempoAtual - ultimoComandoManual) > TIMEOUT_COMANDO_MANUAL) {
    Serial.println("Timeout: retornando ao modo automático");
    modoOperacao = MODO_AUTO;
  }
  
  // Controlar LEDs baseado no modo
  if (modoOperacao == MODO_AUTO) {
    // Modo automático: controlar baseado na luminosidade
    controleLuminosidade(ldrValue);
  } else {
    // Modo manual: usar cor definida pelo backend
    setColor(manualR, manualG, manualB);
  }
  
  // Enviar leitura para backend a cada INTERVALO_ENVIAR_LEITURA
  if (WiFi.status() == WL_CONNECTED && 
      (tempoAtual - ultimaLeituraEnviada) >= INTERVALO_ENVIAR_LEITURA) {
    String modoAtual = getModoString(ldrValue);
    sendLuminosidadeToAPI(ldrValue, modoAtual);
    ultimaLeituraEnviada = tempoAtual;
  }
  
  // Verificar comandos manuais do backend a cada INTERVALO_VERIFICAR_COMANDO
  if (WiFi.status() == WL_CONNECTED && 
      (tempoAtual - ultimoComandoVerificado) >= INTERVALO_VERIFICAR_COMANDO) {
    checkManualCommand();
    ultimoComandoVerificado = tempoAtual;
  }
  
  // Debug via Serial Monitor
  Serial.print("Luminosidade: ");
  Serial.print(ldrValue);
  Serial.print(" | Modo: ");
  Serial.print(getModoString(ldrValue));
  Serial.print(" | Operacao: ");
  Serial.print(modoOperacao == MODO_AUTO ? "AUTO" : "MANUAL");
  if (modoOperacao == MODO_MANUAL) {
    Serial.print(" (R:");
    Serial.print(manualR);
    Serial.print(" G:");
    Serial.print(manualG);
    Serial.print(" B:");
    Serial.print(manualB);
    Serial.print(")");
  }
  Serial.println();
  
  delay(100); // Delay para estabilidade
}

// ===== FUNÇÕES WiFi =====

void connectWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("✗ Falha ao conectar WiFi");
  }
}

// ===== FUNÇÕES DE COMUNICAÇÃO COM BACKEND =====

void sendLuminosidadeToAPI(int valor, String modo) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/luminosidade/";
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  // Criar JSON
  DynamicJsonDocument doc(1024);
  doc["valor"] = valor;
  doc["modo"] = modo;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    Serial.print("✓ Leitura enviada: HTTP ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("✗ Erro ao enviar leitura: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void checkManualCommand() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/controle/";
  
  http.begin(client, url);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == HTTP_CODE_OK) {
    String response = http.getString();
    
    // Parse JSON da resposta
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      // Verificar se há comando manual ou comando para voltar ao automático
      if (doc.containsKey("modo")) {
        String modo = doc["modo"].as<String>();
        
        if (modo == "manual" && doc.containsKey("cor")) {
          JsonObject cor = doc["cor"];
          if (cor.containsKey("r") && cor.containsKey("g") && cor.containsKey("b")) {
            manualR = cor["r"];
            manualG = cor["g"];
            manualB = cor["b"];
            
            modoOperacao = MODO_MANUAL;
            ultimoComandoManual = millis();
            
            Serial.println("✓ Comando manual recebido!");
          }
        } else if (modo == "auto") {
          // Retornar ao modo automático imediatamente
          if (modoOperacao == MODO_MANUAL) {
            modoOperacao = MODO_AUTO;
            ultimoComandoManual = 0;
            Serial.println("✓ Retornando ao modo automático");
          }
        }
      }
    }
  } else {
    // Se não houver comando ou erro, manter modo atual
    // (não faz nada, permite timeout natural)
  }
  
  http.end();
}

// ===== FUNÇÕES DE CONTROLE DE LEDs =====

String getModoString(int valor) {
  if (valor <= 300) {
    return "Escuridão";
  } else if (valor <= 600) {
    return "Crepúsculo";
  } else if (valor <= 800) {
    return "Luz adequada";
  } else {
    return "Luz intensa";
  }
}

void controleLuminosidade(int valor) {
  // Define cores baseadas no nível de luminosidade
  
  if (valor <= 300) {
    // Escuridão total - LED Vermelho forte
    setColor(255, 0, 0);
  }
  else if (valor <= 600) {
    // Ambiente com pouca luz - LED Azul
    setColor(0, 0, 255);
  }
  else if (valor <= 800) {
    // Luz adequada - LED Branco suave
    setColor(100, 100, 100);
  }
  else {
    // Muita luz - LEDs desligados (economiza energia)
    setColor(0, 0, 0);
  }
}

void setColor(int r, int g, int b) {
  // Define a mesma cor RGB para todos os LEDs da fita usando FastLED
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED.show(); // Atualiza a fita com as novas cores
}