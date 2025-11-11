#include <Arduino.h> 

// --- Pines ---
#define DATA_PIN 5          // GPIO5 / D1: Pin de SALIDA de datos del receptor IR
#define RECEIVER_VCC_PIN 14 // GPIO14 / D5: Pin para ALIMENTAR el receptor (ON/OFF)
#define EMITTER_VCC_PIN 12  // GPIO12 / D6: Pin para ALIMENTAR el emisor (ON/OFF)

// Tiempo necesario para que el sensor se estabilice después de encenderse
const int STABILIZATION_DELAY_MS = 1; 
// Intervalo entre lecturas (mantiene la cadencia original de 500ms)
const int READ_INTERVAL_MS = 500; 

void setup() {
  Serial.begin(115200);
  Serial.println("Prueba sensor IR Omron break beam iniciada con ciclo de energía.");
  
  // Configurar el pin de datos del sensor como entrada
  pinMode(DATA_PIN, INPUT_PULLUP); 

  // Configurar los pines de alimentación como salida y apagarlos inicialmente
  pinMode(RECEIVER_VCC_PIN, OUTPUT);
  pinMode(EMITTER_VCC_PIN, OUTPUT);
  digitalWrite(RECEIVER_VCC_PIN, HIGH);
  digitalWrite(EMITTER_VCC_PIN, HIGH);
}

void loop() {
  
  int estado = digitalRead(DATA_PIN);
  
  // 4. APAGAR el sensor inmediatamente
  //digitalWrite(RECEIVER_VCC_PIN, LOW);
  //digitalWrite(EMITTER_VCC_PIN, LOW);

  Serial.println(estado);

  // 6. Esperar hasta el próximo ciclo de lectura
  delay(READ_INTERVAL_MS); 
}