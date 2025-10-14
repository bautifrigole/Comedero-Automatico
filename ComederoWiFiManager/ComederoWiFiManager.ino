#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Preferences.h>

// --- MQTT Configuration (configurable) ---
char mqtt_server[40] = "192.168.247.246";
char mqtt_port[6] = "1883";
char device_name[32] = "Feeder-01";

// Flag to save configuration
bool shouldSaveConfig = false;

// --- Pins ---
#define SWITCH_PIN 27
#define RELAY_PIN 12
// We only use the physical RESET button (no BOOT button)

// --- Operation Modes ---
enum OperationMode {
  MODE_IDLE,
  MODE_DISPENSE_FOOD,
  MODE_MANUAL
};

volatile int motorCounter = 0;
volatile bool switchTriggered = false;
bool motorRunning = false;
bool initialTestDone = false; // Flag to control the initial test turn
OperationMode currentMode = MODE_IDLE;
int targetTurns = 0; // Target number of turns for DISPENSE mode

// --- Clients ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Preferences preferences;

// --- Function to save configuration ---
void saveConfigCallback() {
  Serial.println("Configuration saved");
  shouldSaveConfig = true;
}

// --- Interrupt Function ---
void IRAM_ATTR onMotorTurn() {
  if (switchTriggered) return;
  motorCounter++;
  switchTriggered = true;
}

// --- MQTT Callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (message == "DISPENSE_FOOD") {
    Serial.println("Order: DISPENSE_FOOD -> start dispensing (10 turns)");
    currentMode = MODE_DISPENSE_FOOD;
    targetTurns = 10;
    motorCounter = 0;
    digitalWrite(RELAY_PIN, HIGH); // Activate relay
    motorRunning = true;
    client.publish((String(device_name) + "/status").c_str(), "DISPENSE_STARTED");
  }
  else if (message == "START_MOTOR") {
    Serial.println("Order: START_MOTOR -> motor in manual mode (infinite turns)");
    currentMode = MODE_MANUAL;
    targetTurns = 0;
    motorCounter = 0;
    digitalWrite(RELAY_PIN, HIGH); // Activate relay
    motorRunning = true;
    client.publish((String(device_name) + "/status").c_str(), "MOTOR_MANUAL_STARTED");
  }
  else if (message == "STOP_MOTOR") {
    Serial.println("Order: STOP_MOTOR -> stop motor");
    currentMode = MODE_IDLE;
    targetTurns = 0;
    motorRunning = false;
    digitalWrite(RELAY_PIN, LOW); // Deactivate relay
    client.publish((String(device_name) + "/status").c_str(), "MOTOR_STOPPED");
  }
}

// --- MQTT Reconnection ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print("...");
    
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected!");
      // Subscribe to custom orders topic
      String topic = String(device_name) + "/orders";
      client.subscribe(topic.c_str());
      Serial.println("Subscribed to: " + topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

// --- Function to load configuration ---
void loadConfig() {
  preferences.begin("comedero", false);
  
  String saved_server = preferences.getString("mqtt_server", "");
  String saved_port = preferences.getString("mqtt_port", "1883");
  String saved_device = preferences.getString("device_name", "Feeder-01");
  
  if (saved_server.length() > 0) {
    saved_server.toCharArray(mqtt_server, 40);
  }
  saved_port.toCharArray(mqtt_port, 6);
  saved_device.toCharArray(device_name, 32);
  
  preferences.end();
  
  Serial.println("Configuration loaded:");
  Serial.println("MQTT Server: " + String(mqtt_server));
  Serial.println("MQTT Port: " + String(mqtt_port));
  Serial.println("Device Name: " + String(device_name));
}

// --- Function to save configuration ---
void saveConfig() {
  preferences.begin("comedero", false);
  preferences.putString("mqtt_server", mqtt_server);
  preferences.putString("mqtt_port", mqtt_port);
  preferences.putString("device_name", device_name);
  preferences.end();
  Serial.println("Configuration saved to memory");
}

// --- Function to detect rapid consecutive resets ---
bool checkForConfigReset() {
  preferences.begin("comedero", false);
  
  unsigned long currentTime = millis();
  unsigned long lastResetTime = preferences.getULong("lastReset", 0);
  int resetCount = preferences.getInt("resetCount", 0);
  
  // If more than 10 seconds have passed, restart counter
  if (currentTime > 10000 && (currentTime - lastResetTime) > 10000) {
    resetCount = 0;
  }
  
  resetCount++;
  preferences.putULong("lastReset", currentTime);
  preferences.putInt("resetCount", resetCount);
  
  Serial.println("🔄 Reset #" + String(resetCount) + " detected");
  
  bool shouldReset = false;
  if (resetCount >= 3) {
    Serial.println("🚨 3 resets detected - Activating configuration mode");
    preferences.putInt("resetCount", 0); // Clear counter
    shouldReset = true;
  } else {
    Serial.println("💡 Press RESET " + String(3 - resetCount) + " more time(s) in 10sec to configure WiFi");
  }
  
  preferences.end();
  return shouldReset;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== AUTOMATIC PET FEEDER ===");
  Serial.println("Starting...");

  // --- Pin Configuration ---
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relay off initially

  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), onMotorTurn, FALLING);

  // Load saved configuration
  loadConfig();

  // --- Check for rapid resets to force configuration ---
  bool forceConfig = checkForConfigReset();

  // --- WiFiManager Configuration ---
  WiFiManager wm;
  
  // Callback to save configuration
  wm.setSaveConfigCallback(saveConfigCallback);

  // Custom parameters for web portal
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_device_name("device", "Device Name", device_name, 32);

  // Add parameters to WiFiManager
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_device_name);

  // Configure portal timeout (3 minutes)
  wm.setConfigPortalTimeout(180);

  // Custom text for portal
  wm.setCustomHeadElement("<style>body{background-color:#f0f8ff;}</style>");
  
  Serial.println("Attempting WiFi connection...");
  
  // If BOOT button was pressed, reset configuration
  if (forceConfig) {
    Serial.println("🗑️  Clearing saved WiFi configuration...");
    wm.resetSettings();
    preferences.begin("comedero", false);
    preferences.clear();
    preferences.end();
    Serial.println("✅ Configuration cleared. Starting portal...");
  }
  
  // Try to connect or open configuration portal
  if (!wm.autoConnect("Feeder-Setup", "12345678")) {
    Serial.println("Connection failed and timeout reached");
    delay(3000);
    ESP.restart();
  }

  // If we get here, WiFi is connected
  Serial.println("");
  Serial.println("WiFi connected successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Copy custom parameter values
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(device_name, custom_device_name.getValue());

  // Save configuration if changed
  if (shouldSaveConfig) {
    saveConfig();
  }

  // --- MQTT Connection ---
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  
  Serial.println("=== CONFIGURATION COMPLETE ===");
  Serial.println("🌐 WiFi: " + WiFi.SSID());
  Serial.println("📡 IP: " + WiFi.localIP().toString());
  Serial.println("🔧 MQTT: " + String(mqtt_server) + ":" + String(mqtt_port));
  Serial.println("📱 Device: " + String(device_name));
  Serial.println("");
  Serial.println("💡 To reconfigure WiFi:");
  Serial.println("   1. Press the RESET button 3 times quickly");
  Serial.println("   2. Within 10 seconds between resets");
  Serial.println("   3. The 'Feeder-Setup' network will appear");
  Serial.println("");
  Serial.println("=== STARTING INITIAL TEST TURN ===");
  Serial.println("🔄 Starting test turn...");
  digitalWrite(RELAY_PIN, HIGH); // Activate motor
  motorRunning = true;
  motorCounter = 0;
  Serial.println("⚙️  Motor activated for initial turn");
}

void loop() {
  // MQTT Connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Motor logic
  if (switchTriggered) {
    delay(500); // Debounce
    switchTriggered = false;
    Serial.println("Turn detected. Counter = " + String(motorCounter));

    // Publish state to backend
    String statusTopic = String(device_name) + "/status";
    client.publish(statusTopic.c_str(), ("Turn: " + String(motorCounter)).c_str());

    // Initial test turn logic: only if not done yet
    if (!initialTestDone) {
      digitalWrite(RELAY_PIN, LOW); // Deactivate relay
      motorRunning = false;
      initialTestDone = true;
      Serial.println("✅ Initial turn completed - System ready");
      client.publish(statusTopic.c_str(), "INITIAL_TURN_COMPLETED");
      // If mode was set to DISPENSE/MANUAL while test was running, continue
      if (currentMode == MODE_IDLE) {
        return; // Don't continue with dispensing logic
      }
    }

    // If in MODE_DISPENSE_FOOD, check target
    if (currentMode == MODE_DISPENSE_FOOD) {
      if (motorCounter >= targetTurns) {
        motorCounter = 0;
        digitalWrite(RELAY_PIN, LOW); // Stop motor
        motorRunning = false;
        currentMode = MODE_IDLE;
        Serial.println("Dispenser stopped. Ration complete.");
        client.publish(statusTopic.c_str(), "RATION_COMPLETE");
      }
    }

    // In MODE_MANUAL: motor runs until receiving DETENER_MOTOR
    // Nothing to do here; stop is done from callback when receiving DETENER_MOTOR
  }
}