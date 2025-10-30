#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h> 

// --- CONSTANTES GLOBALES ---
// Tiempo mínimo entre la lectura de dos giros del motor (para rate limiting/debouncing)
const long TURN_PROCESSING_DELAY_MS = 500; 

// Definición de la estructura de configuración para EEPROM
struct Config {
  char mqtt_server[40];
  char mqtt_port[6];
  char device_name[32];
  unsigned long lastResetTime;
  int resetCount;
};

Config config; 

// Dirección de inicio en la EEPROM para la configuración
const int CONFIG_START = 0;
const int RESET_DATA_START = sizeof(Config) - sizeof(unsigned long) - sizeof(int); 

// Flag to save configuration
bool shouldSaveConfig = false;

// --- Pins (ESP8266 D-Pins) ---
#define SWITCH_PIN 5 // GPIO5 / D1 (Pin de interrupción más seguro) 
#define PUMP_PIN 13    // GPIO13 / D7
#define MOTOR_PIN 15 // GPIO15 / D8

// --- Operation Modes ---
enum OperationMode {
  MODE_IDLE,
  MODE_DISPENSE_FOOD,
  MODE_MANUAL,        
  MODE_DISPENSE_WATER, 
  MODE_PUMP_MANUAL     
};

// --- Variables de Estado ---
volatile int motorCounter = 0;
volatile bool switchTriggered = false;
bool motorRunning = false;
bool pumpRunning = false; 
bool initialTestDone = false;
OperationMode currentMode = MODE_IDLE;
int targetTurns = 0; 

// --- Variables de Tiempo ---
unsigned long dispenseStartTime = 0; 
unsigned long lastTurnProcessingTime = 0; 
// Duración de la dispensación de agua en milisegundos (3 segundos)
const long WATER_DISPENSE_DURATION_MS = 3000; 


// --- Clients ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- Function to save configuration (Mismo código) ---
void saveConfigCallback() {
  Serial.println("Configuration saved");
  shouldSaveConfig = true;
}

// --- Interrupt Function (Mínima ISR para estabilidad) ---
void ICACHE_RAM_ATTR onMotorTurn() {
  // La ISR solo incrementa el contador y levanta la bandera.
  motorCounter++;
  switchTriggered = true;
}

// --- MQTT Callback (Mismo código) ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // --- Lógica del Motor (Comida) ---
  if (message == "DISPENSE_FOOD") {
    Serial.println("Order: DISPENSE_FOOD -> start dispensing (10 turns)");
    currentMode = MODE_DISPENSE_FOOD;
    targetTurns = 10;
    motorCounter = 0;
    digitalWrite(MOTOR_PIN, HIGH); // Activate relay
    motorRunning = true;
    client.publish((String(config.device_name) + "/status").c_str(), "DISPENSE_STARTED");
  }
  else if (message == "START_MOTOR") {
    Serial.println("Order: START_MOTOR -> motor in manual mode (infinite turns)");
    currentMode = MODE_MANUAL;
    targetTurns = 0;
    motorCounter = 0;
    digitalWrite(MOTOR_PIN, HIGH); // Activate relay
    motorRunning = true;
    client.publish((String(config.device_name) + "/status").c_str(), "MOTOR_MANUAL_STARTED");
  }
  else if (message == "STOP_MOTOR") {
    Serial.println("Order: STOP_MOTOR -> stop motor");
    currentMode = MODE_IDLE;
    targetTurns = 0;
    motorRunning = false;
    digitalWrite(MOTOR_PIN, LOW); // Deactivate relay
    client.publish((String(config.device_name) + "/status").c_str(), "MOTOR_STOPPED");
  }
  
  // --- Lógica de la Bomba (Agua) ---
  else if (message == "DISPENSE_WATER") {
    Serial.println("Order: DISPENSE_WATER -> start dispensing water (3 seconds)");
    currentMode = MODE_DISPENSE_WATER;
    dispenseStartTime = millis(); // Iniciar temporizador
    digitalWrite(PUMP_PIN, HIGH); // Activar bomba
    pumpRunning = true;
    client.publish((String(config.device_name) + "/status").c_str(), "WATER_DISPENSE_STARTED");
  }
  else if (message == "START_PUMP") {
    Serial.println("Order: START_PUMP -> pump in manual mode (indefinite)");
    currentMode = MODE_PUMP_MANUAL;
    digitalWrite(PUMP_PIN, HIGH); // Activar bomba
    pumpRunning = true;
    client.publish((String(config.device_name) + "/status").c_str(), "PUMP_MANUAL_STARTED");
  }
  else if (message == "STOP_PUMP") {
    Serial.println("Order: STOP_PUMP -> stop pump");
    currentMode = MODE_IDLE;
    pumpRunning = false;
    digitalWrite(PUMP_PIN, LOW); // Desactivar bomba
    client.publish((String(config.device_name) + "/status").c_str(), "PUMP_STOPPED");
  }
}

// --- MQTT Reconnection (Mismo código) ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(config.mqtt_server);
    Serial.print(":");
    Serial.print(config.mqtt_port);
    Serial.print("...");
    
    String clientId = "ESP8266Client-"; 
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected!");
      // Subscribe to custom orders topic
      String topic = String(config.device_name) + "/orders";
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

// --- Function to load configuration (Mismo código) ---
void loadConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.get(CONFIG_START, config);
  
  if (config.mqtt_server[0] == 0xFF || config.mqtt_server[0] == 0) {
    Serial.println("No saved configuration found. Using default values.");
    strcpy(config.mqtt_server, "192.168.247.246");
    strcpy(config.mqtt_port, "1883");
    strcpy(config.device_name, "Feeder-01");
    config.lastResetTime = 0;
    config.resetCount = 0;
    saveConfig(); 
  }
  
  Serial.println("Configuration loaded:");
  Serial.println("MQTT Server: " + String(config.mqtt_server));
  Serial.println("MQTT Port: " + String(config.mqtt_port));
  Serial.println("Device Name: " + String(config.device_name));
  EEPROM.end();
}

// --- Function to save configuration (Mismo código) ---
void saveConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.put(CONFIG_START, config);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Configuration saved to memory");
}

// --- Function to detect rapid consecutive resets (Mismo código) ---
bool checkForConfigReset() {
  EEPROM.begin(sizeof(Config));
  EEPROM.get(RESET_DATA_START, config.lastResetTime);
  EEPROM.get(RESET_DATA_START + sizeof(unsigned long), config.resetCount);
  
  unsigned long currentTime = millis();
  
  if (currentTime > 10000 && (currentTime - config.lastResetTime) > 10000) {
    config.resetCount = 0;
  }
  
  config.resetCount++;
  config.lastResetTime = currentTime;
  
  EEPROM.put(RESET_DATA_START, config.lastResetTime);
  EEPROM.put(RESET_DATA_START + sizeof(unsigned long), config.resetCount);
  EEPROM.commit();
  
  Serial.println("🔄 Reset #" + String(config.resetCount) + " detected");
  
  bool shouldReset = false;
  if (config.resetCount >= 3) {
    Serial.println("🚨 3 resets detected - Activating configuration mode");
    config.resetCount = 0; 
    EEPROM.put(RESET_DATA_START + sizeof(unsigned long), config.resetCount);
    EEPROM.commit();
    shouldReset = true;
  } else {
    Serial.println("💡 Press RESET " + String(3 - config.resetCount) + " more time(s) in 10sec to configure WiFi");
  }
  
  EEPROM.end();
  return shouldReset;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== AUTOMATIC PET FEEDER (ESP8266) ==="); 
  Serial.println("Starting...");

  // --- Pin Configuration ---
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT); 
  digitalWrite(MOTOR_PIN, LOW); 
  digitalWrite(PUMP_PIN, LOW);  

  // Load saved configuration
  loadConfig(); 

  // --- Check for rapid resets to force configuration ---
  bool forceConfig = checkForConfigReset(); 

  // Activar la interrupción (¡ISR mínima!)
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), onMotorTurn, FALLING);
  
  // --- WiFiManager Configuration (Mismo código) ---
  WiFiManager wm;
  
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", config.mqtt_port, 6);
  WiFiManagerParameter custom_device_name("device", "Device Name", config.device_name, 32);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_device_name);

  wm.setConfigPortalTimeout(180);
  wm.setCustomHeadElement("<style>body{background-color:#f0f8ff;}</style>");
  
  Serial.println("Attempting WiFi connection...");
  
  if (forceConfig) {
    Serial.println("🗑️  Clearing saved WiFi configuration and EEPROM...");
    wm.resetSettings();
    
    EEPROM.begin(sizeof(Config));
    for (int i = 0; i < sizeof(Config); i++) {
        EEPROM.write(i, 0xFF); 
    }
    EEPROM.commit();
    EEPROM.end();
    
    loadConfig(); 
    Serial.println("✅ Configuration cleared. Starting portal...");
  }
  
  if (!wm.autoConnect("Feeder-Setup", "12345678")) {
    Serial.println("Connection failed and timeout reached");
    delay(3000);
    ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  strcpy(config.mqtt_server, custom_mqtt_server.getValue());
  strcpy(config.mqtt_port, custom_mqtt_port.getValue());
  strcpy(config.device_name, custom_device_name.getValue());

  if (shouldSaveConfig) {
    saveConfig(); 
  }

  // --- MQTT Connection ---
  client.setServer(config.mqtt_server, atoi(config.mqtt_port));
  client.setCallback(callback);
  
  Serial.println("=== CONFIGURATION COMPLETE ===");
  Serial.println("🌐 WiFi: " + WiFi.SSID());
  Serial.println("📡 IP: " + WiFi.localIP().toString());
  Serial.println("🔧 MQTT: " + String(config.mqtt_server) + ":" + String(config.mqtt_port));
  Serial.println("📱 Device: " + String(config.device_name));
  Serial.println("");
  Serial.println("💡 To reconfigure WiFi:");
  Serial.println("   1. Press the RESET button 3 times quickly");
  Serial.println("   2. Within 10 seconds between resets");
  Serial.println("   3. The 'Feeder-Setup' network will appear");
  Serial.println("");
  
  // --- Prueba Inicial ---
  Serial.println("=== STARTING INITIAL TEST TURN ===");
  Serial.println("🔄 Starting test turn...");
  digitalWrite(MOTOR_PIN, HIGH); // Activate motor
  motorRunning = true;
  motorCounter = 0;
  Serial.println("⚙️  Motor activated for initial turn");
}

void loop() {
  // MQTT Connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Esta función ayuda a mantener el WDT y la conexión MQTT

  // --- LÓGICA DE LA BOMBA (NO BLOQUEANTE) ---
  if (currentMode == MODE_DISPENSE_WATER) {
    if (millis() - dispenseStartTime >= WATER_DISPENSE_DURATION_MS) {
      digitalWrite(PUMP_PIN, LOW); // Detener bomba
      pumpRunning = false;
      currentMode = MODE_IDLE;
      Serial.println("Water dispenser stopped. Ration complete.");
      String statusTopic = String(config.device_name) + "/status";
      client.publish(statusTopic.c_str(), "WATER_RATION_COMPLETE");
    }
  }

  // --- Lógica del Motor (Comida) - Implementación de Rate Limiting No Bloqueante ---
  // CORRECCIÓN: Usamos noInterrupts() para acceder a variables volátiles
  if (switchTriggered && (millis() - lastTurnProcessingTime >= TURN_PROCESSING_DELAY_MS)) {
    
    // Proteger el acceso a la bandera volátil y asegurar la operación atómica
    noInterrupts();
    bool shouldProcess = switchTriggered;
    switchTriggered = false; // Bajar la bandera AHORA
    interrupts();

    if (shouldProcess) { // Procesar si la bandera estaba levantada
        lastTurnProcessingTime = millis(); // Registrar el tiempo de procesamiento

        // NOTA: motorCounter se mantiene para el conteo total.
        // Si el motor gira más rápido que 500ms, solo se procesa un evento cada 500ms.
        Serial.println("Turn detected. Counter = " + String(motorCounter));

        // Publish state to backend (estas operaciones pueden ser lentas)
        String statusTopic = String(config.device_name) + "/status";
        client.publish(statusTopic.c_str(), ("Turn: " + String(motorCounter)).c_str());

        // Initial test turn logic: only if not done yet
        if (!initialTestDone) {
          digitalWrite(MOTOR_PIN, LOW); // Deactivate relay
          motorRunning = false;
          initialTestDone = true;
          Serial.println("✅ Initial turn completed - System ready");
          client.publish(statusTopic.c_str(), "INITIAL_TURN_COMPLETED");
          if (currentMode == MODE_IDLE) {
            return; 
          }
        }

        // If in MODE_DISPENSE_FOOD, check target
        if (currentMode == MODE_DISPENSE_FOOD) {
          if (motorCounter >= targetTurns) {
            motorCounter = 0;
            digitalWrite(MOTOR_PIN, LOW); // Stop motor
            motorRunning = false;
            currentMode = MODE_IDLE;
            Serial.println("Dispenser stopped. Ration complete.");
            client.publish(statusTopic.c_str(), "RATION_COMPLETE");
          }
        }
    }
  }
}