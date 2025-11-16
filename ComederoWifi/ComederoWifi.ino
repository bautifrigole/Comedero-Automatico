#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <time.h> // Para el Reloj de Tiempo Real (RTC)

// --- CONFIGURACIÓN DE PINES ---
#define SENSOR_IR_PIN 12 // GPIO12 / D6 (Sensor de Nivel de Comida)
#define MOTOR_PIN 15     // GPIO15 / D8
#define BUTTON_PIN 0     // GPIO0 / D3 (Pulsador)

// --- CONFIGURACIÓN GLOBAL ---
const long IR_CHECK_INTERVAL_MS = 5000; // Intervalo para chequear el sensor IR
const char* ntpServer = "pool.ntp.org"; // Servidor de hora por defecto
unsigned long lastMqttReconnectAttempt = 0; // Para el reintento no bloqueante

// --- ESTRUCTURAS DE CONFIGURACIÓN (EEPROM) ---

// Configuración de Red (manejada por WiFiManager)
struct Config {
  char mqtt_server[40];
  char mqtt_port[6];
  char device_name[32];
  unsigned long lastResetTime;
  int resetCount;
};
Config config;
const int CONFIG_START = 0;
const int RESET_DATA_START = sizeof(Config) - sizeof(unsigned long) - sizeof(int);

// Configuración Operativa (manejada por MQTT y guardada)
struct OpSettings {
  long foodDurationMs;
  char schedule[100]; // "08:00,14:00,20:00"
  long gmtOffset;
};
OpSettings settings;
const int SETTINGS_START = sizeof(Config);

// --- ESTADOS DE OPERACIÓN ---
enum OperationMode {
  MODE_IDLE,
  MODE_DISPENSE_FOOD, // Basado en tiempo
  MODE_MANUAL
};

// --- VARIABLES DE ESTADO GLOBALES ---
OperationMode currentMode = MODE_IDLE;
bool motorRunning = false;
bool foodLevelCheckPending = false;
int foodLevelStatus = HIGH;
unsigned long dispenseStartTime = 0;
unsigned long lastIRCheckTime = 0;
bool shouldSaveConfig = false; // Flag de WiFiManager
bool rtcSyncd = false;         // Flag: ¿Tenemos la hora correcta?
String lastFedTime = "";       // Para evitar doble alimentación "08:00"
long foodDurationMs = 10000;   // Duración por defecto (10s), se carga desde settings
long gmtOffset_sec = -3 * 3600; // Offset por defecto (GMT-3), se carga desde settings

// Horarios (cargados desde settings.schedule)
String feedingSchedule[10]; // "08:00", "14:30", etc.
int scheduleCount = 0;

// --- CLIENTES ---
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// --- 1. FUNCIONES DE GUARDADO Y CARGA (EEPROM) ---

// Cargar config de Red
void loadConfig() {
  EEPROM.begin(sizeof(Config) + sizeof(OpSettings));
  EEPROM.get(CONFIG_START, config);

  if (config.mqtt_server[0] == 0xFF || config.mqtt_server[0] == 0) {
    Serial.println("No hay config de red. Usando valores por defecto.");
    strcpy(config.mqtt_server, "192.168.1.100");
    strcpy(config.mqtt_port, "1883");
    strcpy(config.device_name, "Feeder-01");
    config.lastResetTime = 0;
    config.resetCount = 0;
    EEPROM.put(CONFIG_START, config);
    EEPROM.commit();
  }
  EEPROM.end();
  Serial.println("Config de Red cargada.");
}

// Guardar config de Red (callback de WiFiManager)
void saveConfigCallback() {
  Serial.println("WiFiManager: Guardando nueva config de red...");
  strcpy(config.mqtt_server, "NUEVO_SERVER"); // WiFiManager actualiza esto
  strcpy(config.mqtt_port, "NUEVO_PORT");
  strcpy(config.device_name, "NUEVO_DEVICE");
  EEPROM.put(CONFIG_START, config);
  EEPROM.commit();
  shouldSaveConfig = true; // El flag es para el portal, no para EEPROM
}

// Cargar Ajustes Operativos (ración, horarios)
void loadSettings() {
  EEPROM.begin(sizeof(Config) + sizeof(OpSettings));
  EEPROM.get(SETTINGS_START, settings);

  if (settings.foodDurationMs <= 0 || settings.foodDurationMs > 60000) {
    Serial.println("No hay settings operativos. Usando valores por defecto.");
    settings.foodDurationMs = 7000; // 7 segundos
    strcpy(settings.schedule, "08:00,13:00,21:00");
    settings.gmtOffset = -3 * 3600; // GMT-3
    EEPROM.put(SETTINGS_START, settings);
    EEPROM.commit();
  }
  EEPROM.end();
  
  // Cargar settings a variables globales
  foodDurationMs = settings.foodDurationMs;
  gmtOffset_sec = settings.gmtOffset;

  // Parsear el string de horarios
  parseSchedule(String(settings.schedule));
  Serial.println("Settings operativos cargados.");
  Serial.println(settings.schedule);
  Serial.println(settings.foodDurationMs);
}

// Guardar Ajustes Operativos
void saveSettings() {
  EEPROM.begin(sizeof(Config) + sizeof(OpSettings));
  EEPROM.put(SETTINGS_START, settings);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Settings operativos guardados en EEPROM.");
}

// Helper: Parsear "08:00,14:00" al array global
void parseSchedule(String scheduleStr) {
  Serial.println("Parseando horario: " + scheduleStr);
  scheduleCount = 0;
  int str_len = scheduleStr.length() + 1;
  char temp[str_len];
  scheduleStr.toCharArray(temp, str_len);
  char* token = strtok(temp, ",");
  while (token != NULL && scheduleCount < 10) {
    feedingSchedule[scheduleCount] = String(token);
    Serial.println("Horario " + String(scheduleCount) + ": " + feedingSchedule[scheduleCount]);
    scheduleCount++;
    token = strtok(NULL, ",");
  }
}

// --- 2. LÓGICA DE ALIMENTACIÓN ---

// Función centralizada para iniciar la dispensación
void startDispenseFood() {
  if (currentMode != MODE_IDLE) {
    Serial.println("Ignorando petición de dispensar, ya está en funcionamiento.");
    return;
  }

  Serial.println("Iniciando dispensación por " + String(foodDurationMs / 1000) + "s");
  currentMode = MODE_DISPENSE_FOOD;
  dispenseStartTime = millis();
  digitalWrite(MOTOR_PIN, HIGH);
  motorRunning = true;
  foodLevelCheckPending = false;
}

// --- 3. LÓGICA DE TIEMPO (RTC) Y PLANIFICADOR ---

// Obtener hora actual formateada (para logs)
String getFormattedTime() {
  if (!rtcSyncd) {
    return "RTC no sincronizado";
  }
  time_t now = time(nullptr);
  struct tm * timeinfo = localtime(&now);
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%A, %H:%M:%S", timeinfo);
  return String(buffer);
}

// Configurar el RTC (se llama en setup y al recibir offset)
void setupRTC() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("RTC: No hay WiFi, no se puede sincronizar.");
    return;
  }
  
  Serial.print("RTC: Sincronizando con servidor NTP... ");
  Serial.println("Offset: " + String(gmtOffset_sec) + "s");
  
  configTime(gmtOffset_sec, 0, ntpServer);

  struct tm timeinfo;
  timeinfo.tm_year = 0;
  unsigned long startTry = millis();
  
  // Esperar a que la hora sea válida (año > 2000)
  while (timeinfo.tm_year < 100 && millis() - startTry < 10000) {
    time_t now = time(nullptr);
    timeinfo = *localtime(&now);
    Serial.print(".");
    delay(200);
  }

  if (timeinfo.tm_year > 100) {
    rtcSyncd = true;
    Serial.println("\n¡RTC Sincronizado!");
    Serial.println(getFormattedTime());
  } else {
    Serial.println("\nError al sincronizar RTC.");
  }
}

// Función que revisa si es hora de alimentar (se llama en loop)
void checkScheduler() {
  if (!rtcSyncd || scheduleCount == 0 || currentMode != MODE_IDLE) {
    return; // No hacer nada si no hay hora, no hay horarios, o ya estamos ocupados
  }
  
  // Optimización: chequear solo una vez por segundo (aprox)
  static unsigned long lastSchedulerCheck = 0;
  if (millis() - lastSchedulerCheck < 1000) {
    return;
  }
  lastSchedulerCheck = millis();

  // Obtener hora actual "HH:MM"
  time_t now = time(nullptr);
  struct tm * timeinfo = localtime(&now);
  char currentTime[6]; // HH:MM + null
  strftime(currentTime, sizeof(currentTime), "%H:%M", timeinfo);
  String currentTimeStr = String(currentTime);

  // Reiniciar el flag de "lastFedTime" si el minuto ha cambiado
  if (lastFedTime != "" && lastFedTime != currentTimeStr) {
    lastFedTime = "";
  }

  // Si ya alimentamos en este minuto, no hacer nada
  if (lastFedTime == currentTimeStr) {
    return;
  }

  // Comparar con nuestra lista de horarios
  for (int i = 0; i < scheduleCount; i++) {
    if (feedingSchedule[i] == currentTimeStr) {
      // ¡Coincidencia!
      Serial.println("[Scheduler] ¡Hora de alimentar! " + currentTimeStr);
      lastFedTime = currentTimeStr; // Marcar como alimentado para este minuto
      
      startDispenseFood(); // Iniciar dispensación
      
      // Publicar estado si estamos conectados
      if (client.connected()) {
        client.publish((String(config.device_name) + "/status").c_str(), "DISPENSE_STARTED_BY_SCHEDULER");
      }
      break; // Salir del bucle for
    }
  }
}


// --- 4. LÓGICA DE RED (MQTT) ---

// Callback de mensajes MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  String topicStr = String(topic);

  Serial.print("MQTT Recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // --- Lógica de Comandos ---
  if (topicStr.endsWith("/orders")) {
    if (message == "DISPENSE_FOOD") {
      startDispenseFood();
      client.publish((String(config.device_name) + "/status").c_str(), "DISPENSE_STARTED_BY_MQTT");
    } else if (message == "START_MOTOR") {
      currentMode = MODE_MANUAL;
      digitalWrite(MOTOR_PIN, HIGH);
      motorRunning = true;
      client.publish((String(config.device_name) + "/status").c_str(), "MOTOR_MANUAL_STARTED");
    } else if (message == "STOP_MOTOR") {
      currentMode = MODE_IDLE;
      motorRunning = false;
      digitalWrite(MOTOR_PIN, LOW);
      client.publish((String(config.device_name) + "/status").c_str(), "MOTOR_STOPPED");
    }
  }
  // --- Lógica de Configuración ---
  else if (topicStr.endsWith("/config/duration")) {
    
    // CORRECCIÓN: Usar message.toInt()
    long newDurationSec = message.toInt(); 
    
    if (newDurationSec > 0 && newDurationSec <= 60) {
      foodDurationMs = newDurationSec * 1000;
      settings.foodDurationMs = foodDurationMs;
      saveSettings();
      Serial.println("Duración de ración actualizada: " + String(newDurationSec) + "s");
    }
  }
  else if (topicStr.endsWith("/config/gmt_offset")) {
    
    // CORRECCIÓN: Usar message.toInt()
    gmtOffset_sec = message.toInt();
    
    settings.gmtOffset = gmtOffset_sec;
    saveSettings();
    Serial.println("Offset GMT actualizado: " + String(gmtOffset_sec) + "s");
    setupRTC(); // Resincronizar hora con el nuevo offset
  }
  else if (topicStr.endsWith("/config/schedule")) {
    String newSchedule = message;
    parseSchedule(newSchedule); // Actualizar el array global
    newSchedule.toCharArray(settings.schedule, 100);
    saveSettings();
    Serial.println("Horarios actualizados.");
  }
  else if (topicStr.endsWith("/time/sync")) {
    
    // CORRECCIÓN: Usar strtoul() para unsigned long (epoch)
    unsigned long epoch = strtoul(message.c_str(), NULL, 10);
    
    if (epoch > 1000000) {
      struct timeval tv;
      tv.tv_sec = epoch; // Segundos
      tv.tv_usec = 0;    // Microsegundos
      settimeofday(&tv, nullptr); // Setear la hora del sistema
      rtcSyncd = true;
      Serial.println("RTC sincronizado desde Backend (MQTT).");
      Serial.println("Hora actual: " + getFormattedTime());
    }
  }
}

// --- 4. LÓGICA DE RED (MQTT) ---
void checkMqttConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (client.connected()) {
    client.loop();
    return;
  }
  
  // Reintentar solo una vez cada 5 segundos (no bloquear)
  if (millis() - lastMqttReconnectAttempt < 5000) {
    return; // Aún no es tiempo de reintentar, salir.
  }

  lastMqttReconnectAttempt = millis(); // Marcar el tiempo de este intento
  
  Serial.print("[MQTT] Intentando conexión...");
  String clientId = "ESP8266Client-" + String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("¡Conectado!");
    
    // Suscribirse a todos los tópicos
    String deviceName = String(config.device_name);
    client.subscribe((deviceName + "/orders").c_str());
    client.subscribe((deviceName + "/config/duration").c_str());
    client.subscribe((deviceName + "/config/gmt_offset").c_str());
    client.subscribe((deviceName + "/config/schedule").c_str());
    client.subscribe((deviceName + "/time/sync").c_str());
    Serial.println("[MQTT] Suscrito a tópicos de control y config.");
    
    // Publicar estado actual al conectarse
    client.publish((deviceName + "/status").c_str(), "ONLINE");
    
  } else {
    Serial.print("falló, rc=");
    Serial.print(client.state());
    Serial.println(" reintentando en 5 segundos...");
  }
}

// --- 5. FUNCIONES DE HARDWARE (Botón, IR) ---

// Chequear el pulsador (se llama en loop)
void checkButton() {
  if (digitalRead(BUTTON_PIN) == LOW && currentMode == MODE_IDLE) {
    Serial.println("🔘 Pulsador presionado - dispensando comida");
    
    startDispenseFood(); // Iniciar dispensación
    
    // Publicar al broker (solo si estamos conectados)
    if (client.connected()) {
      client.publish((String(config.device_name) + "/status").c_str(), "DISPENSE_STARTED_BY_BUTTON");
    }
    
    delay(500); // Pequeño delay para evitar rebotes
  }
}

// Chequear el sensor IR (se llama en loop)
void checkIRSensor() {
  if (foodLevelCheckPending || (currentMode == MODE_IDLE && millis() - lastIRCheckTime >= IR_CHECK_INTERVAL_MS)) {
    
    if (!foodLevelCheckPending && currentMode != MODE_IDLE) {
      lastIRCheckTime = millis();
      return;
    }
    
    int currentFoodLevel = digitalRead(SENSOR_IR_PIN);
    
    if (currentFoodLevel != foodLevelStatus || foodLevelCheckPending) {
      foodLevelStatus = currentFoodLevel;
      String levelTopic = String(config.device_name) + "/level";
      String levelMsg = "";

      if (foodLevelStatus == LOW) {
        levelMsg = "FOOD_LEVEL_OK";
        Serial.println("🟢 Nivel de comida: OK");
      } else {
        levelMsg = "FOOD_LEVEL_LOW";
        Serial.println("🔴 Nivel de comida: BAJO");
      }
      
      if (client.connected()) {
        client.publish(levelTopic.c_str(), levelMsg.c_str());
      }
      
      foodLevelCheckPending = false;
    }
    lastIRCheckTime = millis();
  }
}

// Chequear si el motor debe detenerse (se llama en loop)
void checkMotorStop() {
  if (currentMode == MODE_DISPENSE_FOOD) {
    if (millis() - dispenseStartTime >= foodDurationMs) {
      digitalWrite(MOTOR_PIN, LOW);
      motorRunning = false;
      currentMode = MODE_IDLE;
      foodLevelCheckPending = true; // Activar chequeo IR
      Serial.println("Ración completada.");
      
      if (client.connected()) {
        client.publish((String(config.device_name) + "/status").c_str(), "RATION_COMPLETE");
      }
    }
  }
}

// --- 6. SETUP Y LOOP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== COMEDERO AUTOMÁTICO ===");

  // --- LÓGICA DE RESET (CORREGIDA) ---
  // Configurar el botón PRIMERO
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Iniciando en 3 segundos...");
  Serial.println("MANTÉN PRESIONADO EL BOTÓN (D3) PARA BORRAR LA CONFIGURACIÓN.");
  delay(3000); // 3 segundos para que presiones el botón

  bool forceConfig = false;
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("¡Botón presionado! Forzando borrado de fábrica...");
    forceConfig = true;
  }
  // --- FIN LÓGICA DE RESET ---

  // Configurar el resto de pines
  pinMode(SENSOR_IR_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // Cargar configuraciones de EEPROM
  loadConfig();
  loadSettings();

  // --- WiFiManager ---
  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", config.mqtt_port, 6);
  WiFiManagerParameter custom_device_name("device", "Device Name", config.device_name, 32);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_device_name);
  wm.setConfigPortalTimeout(180);

  // --- ESTE BLOQUE AHORA SÍ FUNCIONARÁ ---
  if (forceConfig) {
    Serial.println("Borrando configuración de WiFi y EEPROM...");
    wm.resetSettings(); // Borra credenciales WiFi
    
    EEPROM.begin(sizeof(Config) + sizeof(OpSettings)); // Abrir EEPROM
    
    // Borrar settings operativos
    for (int i = 0; i < sizeof(OpSettings); i++) {
        EEPROM.write(SETTINGS_START + i, 0xFF); 
    }
    // Borrar config de red
    for (int i = 0; i < sizeof(Config); i++) {
        EEPROM.write(CONFIG_START + i, 0xFF); 
    }
    EEPROM.commit(); // Guardar el borrado
    EEPROM.end();    // Cerrar EEPROM
    
    loadConfig();   // Recargar valores por defecto
    loadSettings(); // Recargar valores por defecto
  }

  // --- MODO NO BLOQUEANTE ---
  if (!wm.autoConnect("Feeder-Setup", "12345678")) {
    Serial.println("Portal falló o expiró. Iniciando en MODO OFFLINE.");
  }

  // Si nos conectamos (o ya estábamos conectados)
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Guardar si hubo cambios en el portal
    if (shouldSaveConfig) {
      strcpy(config.mqtt_server, custom_mqtt_server.getValue());
      strcpy(config.mqtt_port, custom_mqtt_port.getValue());
      strcpy(config.device_name, custom_device_name.getValue());
      
      EEPROM.begin(sizeof(Config) + sizeof(OpSettings));
      EEPROM.put(CONFIG_START, config);
      EEPROM.commit();
      EEPROM.end();
      
      Serial.println("Config de red guardada desde el portal.");
    }
    
    // Configurar cliente MQTT
    client.setServer(config.mqtt_server, atoi(config.mqtt_port));
    client.setCallback(callback);

    // Sincronizar el RTC
    setupRTC();
    
  } else {
    Serial.println("No se pudo conectar a WiFi. El modo planificador NO funcionará.");
    Serial.println("El modo manual (pulsador) sigue activo.");
  }
  
  Serial.println("=== Arranque Completo ===");
}

void loop() {
  // 1. Lógica de Hardware (Siempre activa)
  checkButton();     // Chequear pulsador
  checkMotorStop();  // Chequear si hay que parar el motor
  checkIRSensor();   // Chequear nivel de comida

  // 2. Lógica de Planificador (Requiere RTC)
  checkScheduler();  // Chequear si es hora de alimentar

  // 3. Lógica de Red
  checkMqttConnection();

  // 4. Lógica de Sincronización de RTC
  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long lastRTCSyncAttempt = 0;
    
    if (!rtcSyncd && (millis() - lastRTCSyncAttempt > 60000)) { 
      Serial.println("[Loop] El RTC no está sincronizado. Reintentando...");
      lastRTCSyncAttempt = millis();
      setupRTC();
    }
  }
}
