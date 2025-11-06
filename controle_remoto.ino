#include <WiFi.h>
#include <PubSubClient.h>

// --- Configurações de Rede ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

// --- Objetos ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- Mapeamento de Pinos (Controle Remoto) ---
const int pinJoyX = 34;
const int pinBotao = 27;      // botão liga/desliga
const int ledVerde = 26;      // LED ligado
const int ledVermelho = 25;   // LED desligado

// --- Variáveis de Controle ---
int valX = 0;                 
int angulo = 0; 

bool sistemaLigado = false;
bool ultimoEstadoBotao = HIGH;
unsigned long ultimoDebounce = 0;
const unsigned long debounceDelay = 200;

// --- Controle de tempo para envio ---
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 2000; // 2 segundos

void setup() {
  Serial.begin(115200);
  
  pinMode(pinJoyX, INPUT);
  pinMode(pinBotao, INPUT_PULLUP);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  
  // Estado inicial (desligado)
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVermelho, HIGH);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
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

  // Se o estado mudou, reinicia o timer do debounce
  if (leituraBotao != ultimoEstadoBotao) {
    ultimoDebounce = millis();
    ultimoEstadoBotao = leituraBotao;
  }

  // Se o tempo de debounce passou e o botão ainda está pressionado (LOW)
  if ((millis() - ultimoDebounce) > debounceDelay) {
    if (leituraBotao == LOW) { 
      // Inverte o estado do sistema
      sistemaLigado = !sistemaLigado; 
      
      // Atualiza os LEDs com base no novo estado
      digitalWrite(ledVerde, sistemaLigado ? HIGH : LOW);
      digitalWrite(ledVermelho, sistemaLigado ? LOW : HIGH);
      
      Serial.println(sistemaLigado ? "Sistema LIGADO" : "Sistema DESLIGADO");
      
      // Um delay para evitar que um único "pressionar" longo
      // seja lido como múltiplos cliques (complementa o debounce)
      delay(300); 
    }
  }

  // ===== Leitura do joystick =====
  valX = analogRead(pinJoyX);
  // Mapeia o valor do joystick (0-4095) para o ângulo do servo (0-180)
  angulo = map(valX, 0, 4095, 0, 180);

  // ===== Envia valor via MQTT a cada 2 segundos (somente se ligado) =====
  if (sistemaLigado && (millis() - ultimoEnvio >= intervaloEnvio)) {
    ultimoEnvio = millis(); // atualiza o tempo do último envio
    
    String msg = String(angulo);
    
    // Publica o ângulo no tópico MQTT que o robô está escutando
    client.publish("esp32/servo/angulo", msg.c_str()); 

    Serial.print("Joystick X: ");
    Serial.print(valX);
    Serial.print("  |  Ângulo enviado: ");
    Serial.println(angulo);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    // Tenta se conectar com um ID de cliente único
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
