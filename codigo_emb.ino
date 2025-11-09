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
const char* WIFI_SSID = "MotoG";        // Altere para o SSID da sua rede
const char* WIFI_PASSWORD = "12345678";   // Altere para a senha da sua rede

// ===== CONFIGURAÇÕES DO BACKEND =====
const char* API_BASE_URL = "http://150.162.244.124";
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

// Timeouts para requisições HTTP (em milissegundos)
const int HTTP_CONNECT_TIMEOUT = 15000;  // 15 segundos para conectar
const int HTTP_READ_TIMEOUT = 10000;     // 10 segundos para ler resposta
const int HTTP_TOTAL_TIMEOUT = HTTP_CONNECT_TIMEOUT + HTTP_READ_TIMEOUT;  // Timeout total

// Contador de tentativas de envio
int tentativasEnvio = 0;
const int MAX_TENTATIVAS = 3;

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

// Função auxiliar para testar conectividade com o servidor
bool testServerConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi não conectado. Não é possível testar servidor.");
    return false;
  }
  
  Serial.println("Testando conectividade com o servidor...");
  
  WiFiClient client;
  client.setTimeout(5); // 5 segundos timeout
  
  // Extrair host e porta da URL
  String host = String(API_BASE_URL);
  host.replace("http://", "");
  host.replace("https://", "");
  
  int port = 80;
  int colonIndex = host.indexOf(':');
  if (colonIndex > 0) {
    port = host.substring(colonIndex + 1).toInt();
    host = host.substring(0, colonIndex);
  }
  
  Serial.print("Tentando conectar ao servidor: ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
  
  if (client.connect(host.c_str(), port)) {
    Serial.println("✓ Conexão TCP estabelecida com sucesso!");
    client.stop();
    return true;
  } else {
    Serial.println("✗ Falha ao estabelecer conexão TCP");
    Serial.println("  Possíveis causas:");
    Serial.println("  - Servidor não está online");
    Serial.println("  - Firewall bloqueando porta 80");
    Serial.println("  - Servidor não está escutando na interface externa");
    Serial.println("  - Problema de rede/routing");
    client.stop();
    return false;
  }
}

void sendLuminosidadeToAPI(int valor, String modo) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi não conectado. Não é possível enviar dados.");
    return;
  }
  
  String url = String(API_BASE_URL) + "/api/luminosidade/";
  
  Serial.print("Enviando para: ");
  Serial.println(url);
  
  // Criar JSON uma única vez (reduzido para economizar memória)
  DynamicJsonDocument doc(256);
  doc["valor"] = valor;
  doc["modo"] = modo;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.print("JSON enviado: ");
  Serial.println(jsonString);
  
  // Testar conectividade básica antes de tentar enviar (apenas na primeira vez após falhas)
  static unsigned long lastConnectionTest = 0;
  static bool lastConnectionTestResult = false;
  const unsigned long CONNECTION_TEST_INTERVAL = 30000; // Testar a cada 30 segundos
  
  unsigned long now = millis();
  if (tentativasEnvio > 0 && (now - lastConnectionTest) > CONNECTION_TEST_INTERVAL) {
    Serial.println("\n[DIAGNÓSTICO] Testando conectividade com servidor...");
    lastConnectionTestResult = testServerConnection();
    lastConnectionTest = now;
    
    if (!lastConnectionTestResult && tentativasEnvio >= 3) {
      Serial.println("⚠ Servidor inacessível. Verifique:");
      Serial.println("  1. Servidor está online?");
      Serial.println("  2. Nginx está rodando? (sudo systemctl status nginx)");
      Serial.println("  3. Porta 80 está aberta? (sudo ss -tlnp | grep :80)");
      Serial.println("  4. Firewall está bloqueando? (sudo ufw status)");
      Serial.println("  5. Teste manual: curl http://150.162.244.124/api/status/");
    }
  }
  
  // Tentar enviar com retry - cada tentativa cria novos objetos
  int httpResponseCode = -1;
  int tentativa = 0;
  bool sucesso = false;
  
  while (tentativa < MAX_TENTATIVAS && !sucesso) {
    tentativa++;
    Serial.print("Tentativa ");
    Serial.print(tentativa);
    Serial.print("/");
    Serial.print(MAX_TENTATIVAS);
    Serial.println("...");
    
    // Verificar WiFi antes de cada tentativa
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("✗ WiFi desconectado durante envio. Abortando...");
      break;
    }
    
    // Criar novos objetos para cada tentativa (evita problemas de estado)
    WiFiClient client;
    client.setTimeout((HTTP_READ_TIMEOUT / 1000) + 2); // setTimeout recebe segundos
    
    HTTPClient http;
    
    // Configurar HTTPClient com timeouts corretos
    // http.setTimeout() recebe milissegundos e é o timeout TOTAL (conexão + leitura)
    if (!http.begin(client, url)) {
      Serial.println("✗ Erro ao iniciar conexão HTTP");
      http.end();
      client.stop();
      delay(500);
      continue;
    }
    
    http.setTimeout(HTTP_TOTAL_TIMEOUT);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "close");
    http.setReuse(false); // Não reutilizar conexão
    
    // Enviar POST
    httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      Serial.print("✓ Resposta recebida: HTTP ");
      Serial.println(httpResponseCode);
      
      // Verificar se foi sucesso
      if (httpResponseCode == 200 || httpResponseCode == 201) {
        // Para sucesso, a leitura da resposta é opcional
        // Tentar ler apenas se houver dados disponíveis rapidamente
        // Para evitar timeout, usar uma leitura não bloqueante
        WiFiClient* stream = http.getStreamPtr();
        if (stream && stream->available()) {
          // Ler resposta com limite de tempo e tamanho
          String response = "";
          unsigned long readStart = millis();
          const int MAX_READ_TIME = 2000; // Máximo 2 segundos para ler
          const int MAX_RESPONSE_LENGTH = 256;
          
          while (stream->available() && response.length() < MAX_RESPONSE_LENGTH && 
                 (millis() - readStart) < MAX_READ_TIME) {
            int c = stream->read();
            if (c >= 0) {
              response += (char)c;
            } else {
              break;
            }
          }
          
          if (response.length() > 0) {
            Serial.print("Resposta: ");
            Serial.println(response);
          }
        } else {
          // Se não houver stream disponível, não é problema
          // O importante é que o código HTTP foi 200/201
          Serial.println("(Resposta vazia ou não disponível - OK)");
        }
        
        sucesso = true;
        tentativasEnvio = 0; // Reset contador de falhas
        Serial.println("✓ Leitura enviada com sucesso!");
      } else {
        // Erro HTTP do servidor (400, 404, 500, etc.)
        Serial.print("✗ Erro do servidor: HTTP ");
        Serial.println(httpResponseCode);
        
        // Tentar ler resposta de erro (com timeout curto)
        WiFiClient* stream = http.getStreamPtr();
        if (stream && stream->available()) {
          String response = "";
          unsigned long errorStartTime = millis();
          const int MAX_ERROR_READ_TIME = 2000;
          const int MAX_ERROR_LENGTH = 200;
          
          while (stream->available() && response.length() < MAX_ERROR_LENGTH && 
                 (millis() - errorStartTime) < MAX_ERROR_READ_TIME) {
            int c = stream->read();
            if (c >= 0) {
              response += (char)c;
            } else {
              break;
            }
          }
          
          if (response.length() > 0) {
            Serial.print("Detalhes do erro: ");
            Serial.println(response);
          }
        }
        
        // Para erros 4xx, não faz sentido tentar novamente imediatamente
        // (problema nos dados enviados)
        if (httpResponseCode >= 400 && httpResponseCode < 500) {
          Serial.println("⚠ Erro do cliente (4xx). Verifique os dados enviados.");
          break; // Não tenta novamente para erros 4xx
        }
      }
    } else {
      // Erro de conexão (código negativo)
      Serial.print("✗ Erro de conexão: ");
      Serial.print(httpResponseCode);
      
      // Códigos de erro comuns do HTTPClient (valores negativos)
      // -1: Connection refused
      // -2: Send header failed
      // -3: Send payload failed
      // -4: Not connected
      // -5: Connection lost
      // -6: No stream
      // -7: No HTTP server
      // -11: Read timeout
      // -12: Payload read failed
      if (httpResponseCode == -1 || httpResponseCode == -4) {
        Serial.println(" → Servidor recusou conexão ou não conectado");
        Serial.println("  DIAGNÓSTICO:");
        Serial.println("  1. Verifique se o servidor está online");
        Serial.println("  2. Verifique se Nginx está rodando: sudo systemctl status nginx");
        Serial.println("  3. Verifique se a porta 80 está escutando: sudo ss -tlnp | grep :80");
        Serial.println("  4. Verifique firewall: sudo ufw status");
        Serial.println("  5. Teste manual no servidor: curl http://localhost/api/status/");
        Serial.println("  6. Teste externo: curl http://150.162.244.124/api/status/");
        Serial.print("  URL testada: ");
        Serial.println(url);
      } else if (httpResponseCode == -5) {
        Serial.println(" → Conexão perdida durante requisição");
      } else if (httpResponseCode == -7) {
        Serial.println(" → Servidor HTTP não encontrado (URL incorreta?)");
        Serial.print("  URL testada: ");
        Serial.println(url);
      } else if (httpResponseCode == -11) {
        Serial.println(" → Timeout na leitura da resposta");
        Serial.println("  Servidor pode estar lento ou sobrecarregado");
      } else if (httpResponseCode == -2 || httpResponseCode == -3) {
        Serial.println(" → Erro ao enviar dados (header ou payload)");
        Serial.println("  Verifique conexão WiFi e tamanho dos dados");
      } else {
        Serial.println(" → Erro desconhecido de conexão");
        Serial.print("  Código: ");
        Serial.println(httpResponseCode);
        Serial.println("  Verifique WiFi, servidor e URL");
      }
    }
    
    // Fechar conexões após cada tentativa
    http.end();
    client.stop();
    
    // Se não teve sucesso e ainda há tentativas, aguardar antes de tentar novamente
    if (!sucesso && tentativa < MAX_TENTATIVAS) {
      Serial.println("Aguardando 1 segundo antes de tentar novamente...");
      delay(1000);
    }
  }
  
  if (!sucesso) {
    tentativasEnvio++;
    Serial.print("✗ Falha após ");
    Serial.print(MAX_TENTATIVAS);
    Serial.println(" tentativas");
    
    // Se houver muitas falhas consecutivas, verificar WiFi
    if (tentativasEnvio >= 5) {
      Serial.println("⚠ Muitas falhas consecutivas. Verificando WiFi...");
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado! Tentando reconectar...");
        connectWiFi();
      }
      tentativasEnvio = 0;
    }
  }
}

void checkManualCommand() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  // Criar WiFiClient com timeouts
  WiFiClient client;
  client.setTimeout(HTTP_READ_TIMEOUT / 1000);
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/controle/";
  
  // Configurar timeouts
  http.begin(client, url);
  http.setTimeout(HTTP_CONNECT_TIMEOUT);
  http.addHeader("Connection", "close");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    
    // Parse JSON da resposta
    DynamicJsonDocument doc(512);
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
    } else {
      Serial.print("✗ Erro ao parsear JSON do controle: ");
      Serial.println(error.c_str());
    }
  } else if (httpResponseCode > 0) {
    // Erro HTTP do servidor (não é crítico para esta função)
    // Não imprimir para não poluir o Serial Monitor
  } else {
    // Erro de conexão (código negativo) - apenas em modo debug
    // Não imprimir para não poluir o Serial Monitor
  }
  
  http.end();
  client.stop();
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
  FastLED.show(); // Atualiza a fita com as novas cores
}
