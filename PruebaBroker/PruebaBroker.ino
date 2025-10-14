#include <WiFi.h>
#include <PubSubClient.h>

// --- Configuración WiFi y MQTT ---
const char* ssid = "buti"; // Cambia esto por el SSID de tu red
const char* password = "vladimir";
// IP de la computadora donde corre el backend. ¡Cámbiala por la tuya!
const char* mqtt_server = "192.168.247.246"; 
const int mqtt_port = 1883; // Puerto TCP para MQTT

// --- Pines ---
#define SWITCH_PIN 27
#define RELAY_PIN 12

volatile int motorCounter = 0;
volatile bool switchTriggered = false;
bool motorRunning = false; // Estado del motor

// --- Clientes ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- Función de Interrupción ---
void IRAM_ATTR onMotorTurn() {
  if (switchTriggered) return;
  motorCounter++;
  switchTriggered = true;
}

// --- Callback de MQTT ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (message == "DISPENSAR_COMIDA") {
    Serial.println("Activando el relé para dispensar comida...");
    digitalWrite(RELAY_PIN, HIGH); // Activar relé
    motorRunning = true;
  }
  else if (message == "INICIAR_MOTOR") {
    Serial.println("Iniciando motor manualmente...");
    digitalWrite(RELAY_PIN, HIGH); // Activar relé
    motorRunning = true;
    motorCounter = 0; // Resetear contador
    client.publish("comedero/01/estado", "MOTOR_INICIADO");
  }
  else if (message == "DETENER_MOTOR") {
    Serial.println("Deteniendo motor manualmente...");
    digitalWrite(RELAY_PIN, LOW); // Desactivar relé
    motorRunning = false;
    client.publish("comedero/01/estado", "MOTOR_DETENIDO");
  }
}

// --- Reconexión MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado!");
      // Suscribirse al topic de órdenes
      client.subscribe("comedero/01/ordenes");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // --- Configuración de Pines ---
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relé apagado inicialmente

  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), onMotorTurn, FALLING);

  // --- Conexión WiFi ---
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // --- Conexión MQTT ---
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (switchTriggered) {
    delay(500); // Anti-rebote
    switchTriggered = false;
    Serial.println("Vuelta detectada. Contador = " + String(motorCounter));
    
    // Publicar estado al backend
    client.publish("comedero/01/estado", ("Vuelta: " + String(motorCounter)).c_str());

    // Solo detener automáticamente si estaba en modo dispensar comida (no en modo manual)
    if (motorRunning && motorCounter >= 10) {
      motorCounter = 0;
      digitalWrite(RELAY_PIN, HIGH); // Desactivar relé
      motorRunning = false;
      Serial.println("Dispensador detenido. Ración completa.");
      client.publish("comedero/01/estado", "RACION_COMPLETA");
    }
  }
}