#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "DHT.h"
#include <PubSubClient.h>
#include <ESP32Servo.h>

// --- DEFINIÇÕES DE PINOS SENSORES ---
#define DHTPIN 2
#define DHTTYPE DHT11
#define LDR_PIN 35
#define PIR_PIN 19
#define LED_VERDE 26
#define LED_VERMELHO 27

DHT dht(DHTPIN, DHTTYPE);

// --- CONFIGURAÇÕES DE REDE ---
const char* ssid = "artur";
const char* password = "123456789";

const char* backendURL = "http://10.127.2.191:5000/leituras";

// --- CONFIGURAÇÕES DO CALLMEBOT ---
String phoneNumber = "557191601162";  // Seu número (sem o +)
String apiKey = "4165803";            // Sua API Key do Callmebot

// --- CONFIGURAÇÕES DO MQTT ---
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

// --- CONFIGURAÇÃO DO SERVO ---
Servo servo;
const int pinServo = 4;
int anguloAtual = 90;

// --- FUNÇÃO PARA ENVIAR MENSAGEM WHATSAPP ---
void sendMessage(String message) {
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber +
               "&apikey=" + apiKey + "&text=" + urlEncode(message);

  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    Serial.println("✅ Mensagem enviada com sucesso!");
  } else {
    Serial.print("❌ Erro ao enviar mensagem. Código HTTP: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void enviarParaBackend(float temperatura, float umidade, int luz, int presenca, float prob) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(backendURL);
    http.addHeader("Content-Type", "application/json");

    // --- Criar JSON --- 
    String json = "{";
    json += "\"temperatura_c\":" + String(temperatura, 2) + ",";
    json += "\"umidade_pct\":" + String(umidade, 2) + ",";
    json += "\"luminosidade\":" + String(luz) + ",";
    json += "\"presenca\":" + String(presenca) + ",";
    json += "\"probabilidade_vida\":" + String(prob, 2);
    json += "}";

    int httpResponseCode = http.POST(json);

    if (httpResponseCode == 201) {
      Serial.println("✅ Dados enviados para o backend com sucesso!");
    } else {
      Serial.print("❌ Falha ao enviar para backend. Código HTTP: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("❌ WiFi desconectado. Não foi possível enviar os dados.");
  }
}


// --- FUNÇÃO PARA CONEXÃO WI-FI ---
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// --- CALLBACK MQTT PARA RECEBER ÂNGULO ---
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  int novoAngulo = message.toInt();
  novoAngulo = constrain(novoAngulo, 0, 180);

  servo.write(novoAngulo);
  anguloAtual = novoAngulo;

  Serial.print("Recebido: ");
  Serial.print(message);
  Serial.print(" -> Servo para: ");
  Serial.println(anguloAtual);
}

// --- RECONEXÃO MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    if (client.connect("ESP32_Servo")) {
      Serial.println("Conectado!");
      client.subscribe("esp32/servo/angulo");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 2s");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // --- Configuração sensores ---
  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  dht.begin();

  // --- Conexão Wi-Fi ---
  setup_wifi();

  // --- Configuração servo ---
  servo.attach(pinServo);
  servo.write(anguloAtual);

  // --- Configuração MQTT ---
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // --- Loop MQTT ---
  if (!client.connected()) reconnect();
  client.loop();

  // --- Leitura dos sensores ---
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  int luz = analogRead(LDR_PIN);
  int presenca = digitalRead(PIR_PIN);

  Serial.println("======================");
  Serial.println("Leituras dos sensores:");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
  Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
  Serial.print("Luz (LDR): "); Serial.println(luz);
  Serial.print("Presença: "); Serial.println(presenca == HIGH ? "Detectada" : "Ausente");

  // --- Cálculo da probabilidade de vida ---
  float prob = 0;
  if (temperatura >= 15 && temperatura <= 30) prob += 25;
  if (umidade >= 40 && umidade <= 70) prob += 25;
  if (luz > 2000) prob += 20;
  if (presenca == HIGH) prob += 30;
  if (prob > 100) prob = 100;

  Serial.print("Probabilidade de vida: ");
  Serial.print(prob);
  Serial.println("%");

  enviarParaBackend(temperatura, umidade, luz, presenca, prob);

  // --- Decisão ---
  if (prob <= 75) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
    Serial.println("💚 Exploração normal. Nenhum indício relevante detectado.");
  } else {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
    Serial.println("🚨 ALERTA! Alta probabilidade de vida detectada!");
    sendMessage("🚨 Alerta! Alta probabilidade de vida detectada no planeta!");
  }

  Serial.println("======================\n");
  delay(2000);  
}
