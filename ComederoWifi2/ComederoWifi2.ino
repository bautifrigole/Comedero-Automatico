#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Preferences.h>

// --- MQTT Configuration ---
char mqtt_server[40] = "192.168.247.246";
char mqtt_port[6] = "1883";
char device_name[32] = "Feeder-01";

bool shouldSaveConfig = false;

// --- Pines ---
#define MOTOR_PIN 15   // Relé del motor de comida
#define PUMP_PIN 13    // Relé de la bomba de agua

// --- Estado general ---
bool motorRunning = false;
bool pumpRunning = false;
unsigned long motorStartTime = 0;
unsigned long pumpStartTime = 0;
const unsigned long DISPENSE_DURATION = 10000; // 10 segundos
const unsigned long PUMP_DURATION = 10000;     // 10 segundos

// --- MQTT / WiFi / Storage ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Preferences preferences;

// --- Guardar configuración ---
void saveConfigCallback() {
  shouldSaveConfig = true;
}

// --- Callback MQTT ---
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];

  Serial.println("📩 Mensaje recibido: " + message);

  if (message == "DISPENSE_FOOD") {
    Serial.println("🍖 Dispensing food for 10 seconds...");
    digitalWrite(MOTOR_PIN, HIGH);
    motorRunning = true;
    motorStartTime = millis();
    client.publish((String(device_name) + "/status").c_str(), "DISPENSE_STARTED");
  }
  else if (message == "START_PUMP") {
    Serial.println("💧 Pump ON for 10 seconds...");
    digitalWrite(PUMP_PIN, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    client.publish((String(device_name) + "/status").c_str(), "PUMP_STARTED");
  }
  else if (message == "STOP_ALL") {
    Serial.println("🛑 Stopping all relays");
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);
    motorRunning = false;
    pumpRunning = false;
    client.publish((String(device_name) + "/status").c_str(), "ALL_STOPPED");
  }
}

// --- MQTT reconexión ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Feeder-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected!");
      String topic = String(device_name) + "/orders";
      client.subscribe(topic.c_str());
      Serial.println("Subscribed to: " + topic);
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

// --- Cargar configuración ---
void loadConfig() {
  preferences.begin("comedero", false);
  String saved_server = preferences.getString("mqtt_server", "");
  String saved_port = preferences.getString("mqtt_port", "1883");
  String saved_device = preferences.getString("device_name", "Feeder-01");

  if (saved_server.length() > 0) saved_server.toCharArray(mqtt_server, 40);
  saved_port.toCharArray(mqtt_port, 6);
  saved_device.toCharArray(device_name, 32);

  preferences.end();

  Serial.println("Configuration loaded:");
  Serial.println("MQTT Server: " + String(mqtt_server));
  Serial.println("MQTT Port: " + String(mqtt_port));
  Serial.println("Device Name: " + String(device_name));
}

// --- Guardar configuración ---
void saveConfig() {
  preferences.begin("comedero", false);
  preferences.putString("mqtt_server", mqtt_server);
  preferences.putString("mqtt_port", mqtt_port);
  preferences.putString("device_name", device_name);
  preferences.end();
  Serial.println("Configuration saved");
}

void setup() {
  Serial.begin(115200);
  Serial.println("🐾 Automatic Feeder Starting...");

  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);

  loadConfig();

  // --- WiFiManager ---
  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_device_name("device", "Device Name", device_name, 32);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_device_name);

  wm.setConfigPortalTimeout(180);
  wm.setCustomHeadElement("<style>body{background-color:#f0f8ff;}</style>");

  Serial.println("Connecting to WiFi...");
  if (!wm.autoConnect("Feeder-Setup", "12345678")) {
    Serial.println("Connection failed, restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected!");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(device_name, custom_device_name.getValue());
  if (shouldSaveConfig) saveConfig();

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

  Serial.println("✅ Setup complete.");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  // --- Control de motor ---
  if (motorRunning && (now - motorStartTime >= DISPENSE_DURATION)) {
    digitalWrite(MOTOR_PIN, LOW);
    motorRunning = false;
    Serial.println("🍖 Food dispensing complete.");
    client.publish((String(device_name) + "/status").c_str(), "DISPENSE_COMPLETE");
  }

  // --- Control de bomba ---
  if (pumpRunning && (now - pumpStartTime >= PUMP_DURATION)) {
    digitalWrite(PUMP_PIN, LOW);
    pumpRunning = false;
    Serial.println("💧 Pump cycle complete.");
    client.publish((String(device_name) + "/status").c_str(), "PUMP_COMPLETE");
  }
}
