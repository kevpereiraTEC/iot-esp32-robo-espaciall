# iot-esp32-robo-espaciall

#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

const int pinJoyX = 34;
const int pinBotao = 27;    // botão liga/desliga
const int ledVerde = 26;    // LED ligado
const int ledVermelho = 25; // LED desligado

int valX = 0;              
int angulo = 0; 

bool sistemaLigado = false;
bool ultimoEstadoBotao = HIGH;
unsigned long ultimoDebounce = 0;
const unsigned long debounceDelay = 200;

// --- controle de tempo para envio ---
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 2000; // 2 segundos

void setup() {
  Serial.begin(115200);
  
  pinMode(pinJoyX, INPUT);
  pinMode(pinBotao, INPUT_PULLUP);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVermelho, HIGH);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // ===== Leitura do botão com debounce =====
  bool leituraBotao = digitalRead(pinBotao);

  if (leituraBotao != ultimoEstadoBotao) {
    ultimoDebounce = millis();
    ultimoEstadoBotao = leituraBotao;
  }

  if ((millis() - ultimoDebounce) > debounceDelay) {
    if (leituraBotao == LOW) { // botão pressionado
      sistemaLigado = !sistemaLigado;
      digitalWrite(ledVerde, sistemaLigado ? HIGH : LOW);
      digitalWrite(ledVermelho, sistemaLigado ? LOW : HIGH);
      Serial.println(sistemaLigado ? "Sistema LIGADO" : "Sistema DESLIGADO");
      delay(300); // evita múltiplas trocas rápidas
    }
  }

  // ===== Leitura do joystick =====
  valX = analogRead(pinJoyX);
  angulo = map(valX, 0, 4095, 0, 180);

  // ===== Envia valor via MQTT a cada 2 segundos (somente se ligado) =====
  if (sistemaLigado && (millis() - ultimoEnvio >= intervaloEnvio)) {
    ultimoEnvio = millis(); // atualiza o tempo do último envio
    String msg = String(angulo);
    client.publish("esp32/servo/angulo", msg.c_str()); // <- tópico correto

    Serial.print("Joystick X: ");
    Serial.print(valX);
    Serial.print("  |  Ângulo enviado: ");
    Serial.println(angulo);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    if (client.connect("WokwiClientJoystick")) {
      Serial.println("Conectado");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando de novo em 2s");
      delay(2000);
    }
  }
}
