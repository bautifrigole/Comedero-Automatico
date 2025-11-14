#include <ESP8266WiFi.h>
#include <time.h>

// --- Configuración WiFi ---
const char* ssid = "FrigoleTMS";
const char* password = "sabami2009";

// --- Configuración de Hora (NTP) ---
const char* ntpServer = "pool.ntp.org";

// Ajusta tu zona horaria aquí (en segundos)
// Ejemplo: Argentina (GMT-3) = -3 * 3600
const long gmtOffset_sec = -3 * 3600; 

// Ajuste para horario de verano (en segundos), 0 si no se usa
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando...");

  // 1. Conectarse a la red WiFi
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡WiFi conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // 2. Configurar el RTC interno usando el servidor NTP
  // Esta es la función clave:
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // 3. Esperar a que el RTC se sincronice
  Serial.print("Sincronizando hora (RTC)...");
  
  // Esperamos hasta que la hora sea válida (tm_year > 100 significa año > 2000)
  struct tm timeinfo;
  timeinfo.tm_year = 0;
  while (timeinfo.tm_year < 100) {
    time_t now = time(nullptr);
    timeinfo = *localtime(&now);
    Serial.print(".");
    delay(200);
  }
  
  Serial.println("\n¡RTC Sincronizado!");
  Serial.println("=================================");
}

void loop() {
  // 1. Obtener la hora actual del RTC
  time_t now = time(nullptr); // 'now' contiene los segundos desde 1970-01-01

  // 2. Convertir 'now' a una estructura de tiempo local
  struct tm * timeinfo;
  timeinfo = localtime(&now);

  // 3. Formatear la hora para mostrarla
  // Usamos strftime para crear un string formateado
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%A, %d de %B de %Y - %H:%M:%S", timeinfo);

  // 4. Imprimir la hora en el Monitor Serie
  Serial.println(buffer);

  // Esperar un segundo antes de volver a imprimir
  delay(1000);
}
