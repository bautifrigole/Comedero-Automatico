#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Preferences.h>

// --- Configuración MQTT (configurable) ---
char mqtt_server[40] = "192.168.247.246";
char mqtt_port[6] = "1883";
char device_name[32] = "Comedero-01";

// Flag para guardar configuración
bool shouldSaveConfig = false;

// --- Pines ---
#define SWITCH_PIN 27
#define RELAY_PIN 12
// No usamos RESET_BUTTON_PIN ya que solo tenemos el botón RESET físico

volatile int motorCounter = 0;
volatile bool switchTriggered = false;
bool motorRunning = false;
bool initialTestDone = false; // Flag para controlar la vuelta inicial

// --- Clientes ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Preferences preferences;

// --- Función para guardar configuración ---
void saveConfigCallback() {
  Serial.println("Configuración guardada");
  shouldSaveConfig = true;
}

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
    digitalWrite(RELAY_PIN, LOW); // Activar relé
    motorRunning = true;
  }
  else if (message == "INICIAR_MOTOR") {
    Serial.println("Iniciando motor manualmente...");
    digitalWrite(RELAY_PIN, LOW); // Activar relé
    motorRunning = true;
    motorCounter = 0;
    client.publish((String(device_name) + "/estado").c_str(), "MOTOR_INICIADO");
  }
  else if (message == "DETENER_MOTOR") {
    Serial.println("Deteniendo motor manualmente...");
    digitalWrite(RELAY_PIN, HIGH); // Desactivar relé
    motorRunning = false;
    client.publish((String(device_name) + "/estado").c_str(), "MOTOR_DETENIDO");
  }
}

// --- Reconexión MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT a ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print("...");
    
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado!");
      // Suscribirse al topic de órdenes personalizado
      String topic = String(device_name) + "/ordenes";
      client.subscribe(topic.c_str());
      Serial.println("Suscrito a: " + topic);
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

// --- Función para cargar configuración ---
void loadConfig() {
  preferences.begin("comedero", false);
  
  String saved_server = preferences.getString("mqtt_server", "");
  String saved_port = preferences.getString("mqtt_port", "1883");
  String saved_device = preferences.getString("device_name", "Comedero-01");
  
  if (saved_server.length() > 0) {
    saved_server.toCharArray(mqtt_server, 40);
  }
  saved_port.toCharArray(mqtt_port, 6);
  saved_device.toCharArray(device_name, 32);
  
  preferences.end();
  
  Serial.println("Configuración cargada:");
  Serial.println("MQTT Server: " + String(mqtt_server));
  Serial.println("MQTT Port: " + String(mqtt_port));
  Serial.println("Device Name: " + String(device_name));
}

// --- Función para guardar configuración ---
void saveConfig() {
  preferences.begin("comedero", false);
  preferences.putString("mqtt_server", mqtt_server);
  preferences.putString("mqtt_port", mqtt_port);
  preferences.putString("device_name", device_name);
  preferences.end();
  Serial.println("Configuración guardada en memoria");
}

// --- Función para detectar resets rápidos consecutivos ---
bool checkForConfigReset() {
  preferences.begin("comedero", false);
  
  unsigned long currentTime = millis();
  unsigned long lastResetTime = preferences.getULong("lastReset", 0);
  int resetCount = preferences.getInt("resetCount", 0);
  
  // Si han pasado más de 10 segundos, reiniciar contador
  if (currentTime > 10000 && (currentTime - lastResetTime) > 10000) {
    resetCount = 0;
  }
  
  resetCount++;
  preferences.putULong("lastReset", currentTime);
  preferences.putInt("resetCount", resetCount);
  
  Serial.println("🔄 Reset #" + String(resetCount) + " detectado");
  
  bool shouldReset = false;
  if (resetCount >= 3) {
    Serial.println("🚨 3 resets detectados - Activando modo configuración");
    preferences.putInt("resetCount", 0); // Limpiar contador
    shouldReset = true;
  } else {
    Serial.println("💡 Presiona RESET " + String(3 - resetCount) + " veces más en 10seg para configurar WiFi");
  }
  
  preferences.end();
  return shouldReset;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== COMEDERO AUTOMÁTICO ===");
  Serial.println("Iniciando...");

  // --- Configuración de Pines ---
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relé apagado inicialmente

  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), onMotorTurn, FALLING);

  // Cargar configuración guardada
  loadConfig();

  // --- Verificar resets rápidos para forzar configuración ---
  bool forceConfig = checkForConfigReset();

  // --- Configuración WiFiManager ---
  WiFiManager wm;
  
  // Callback para guardar configuración
  wm.setSaveConfigCallback(saveConfigCallback);

  // Parámetros personalizados para el portal web
  WiFiManagerParameter custom_mqtt_server("server", "Servidor MQTT", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "Puerto MQTT", mqtt_port, 6);
  WiFiManagerParameter custom_device_name("device", "Nombre del dispositivo", device_name, 32);

  // Agregar parámetros al WiFiManager
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_device_name);

  // Configurar timeout para el portal (3 minutos)
  wm.setConfigPortalTimeout(180);

  // Texto personalizado para el portal
  wm.setCustomHeadElement("<style>body{background-color:#f0f8ff;}</style>");
  
  Serial.println("Intentando conectar a WiFi...");
  
  // Si se presionó el botón BOOT, resetear configuración
  if (forceConfig) {
    Serial.println("🗑️  Borrando configuración WiFi guardada...");
    wm.resetSettings();
    preferences.begin("comedero", false);
    preferences.clear();
    preferences.end();
    Serial.println("✅ Configuración borrada. Iniciando portal...");
  }
  
  // Intentar conectar o abrir portal de configuración
  if (!wm.autoConnect("Comedero-Setup", "12345678")) {
    Serial.println("Falló la conexión y se agotó el tiempo");
    delay(3000);
    ESP.restart();
  }

  // Si llegamos aquí, WiFi está conectado
  Serial.println("");
  Serial.println("¡WiFi conectado exitosamente!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Copiar valores de los parámetros personalizados
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(device_name, custom_device_name.getValue());

  // Guardar configuración si cambió
  if (shouldSaveConfig) {
    saveConfig();
  }

  // --- Conexión MQTT ---
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  
  Serial.println("=== CONFIGURACIÓN COMPLETA ===");
  Serial.println("🌐 WiFi: " + WiFi.SSID());
  Serial.println("📡 IP: " + WiFi.localIP().toString());
  Serial.println("🔧 MQTT: " + String(mqtt_server) + ":" + String(mqtt_port));
  Serial.println("📱 Dispositivo: " + String(device_name));
  Serial.println("");
  Serial.println("💡 Para reconfigurar WiFi:");
  Serial.println("   1. Presiona el botón RESET 3 veces rápidas");
  Serial.println("   2. En menos de 10 segundos entre resets");
  Serial.println("   3. Aparecerá la red 'Comedero-Setup'");
  Serial.println("");
  
  // --- VUELTA INICIAL DE PRUEBA ---
  Serial.println("🔄 Iniciando vuelta de prueba...");
  digitalWrite(RELAY_PIN, LOW); // Activar motor
  motorRunning = true;
  motorCounter = 0;
  Serial.println("⚙️  Motor activado para vuelta inicial");
}

void loop() {
  // Conexión MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Lógica del motor
  if (switchTriggered) {
    delay(500); // Anti-rebote
    switchTriggered = false;
    Serial.println("Vuelta detectada. Contador = " + String(motorCounter));
    
    // Publicar estado al backend
    String statusTopic = String(device_name) + "/estado";
    client.publish(statusTopic.c_str(), ("Vuelta: " + String(motorCounter)).c_str());

    // Lógica para vuelta inicial de prueba
    if (!initialTestDone && motorCounter >= 1) {
      digitalWrite(RELAY_PIN, HIGH); // Desactivar relé
      motorRunning = false;
      initialTestDone = true;
      Serial.println("✅ Vuelta inicial completada - Sistema listo");
      client.publish(statusTopic.c_str(), "VUELTA_INICIAL_COMPLETADA");
      return; // Salir temprano para no ejecutar la lógica normal
    }

    // Solo detener automáticamente si estaba en modo dispensar comida (10 vueltas)
    if (motorRunning && motorCounter >= 10) {
      motorCounter = 0;
      digitalWrite(RELAY_PIN, HIGH); // Desactivar relé
      motorRunning = false;
      Serial.println("Dispensador detenido. Ración completa.");
      client.publish(statusTopic.c_str(), "RACION_COMPLETA");
    }
  }
}