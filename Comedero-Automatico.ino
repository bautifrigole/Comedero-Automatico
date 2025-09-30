//#include "thingProperties.h"

#define SWITCH_PIN 27   // Switch conectado al GPIO27
#define RELAY_PIN 12    // Pin GPIO 12 conectado al IN del relay

volatile int motorCounter = 0;
volatile bool switchTriggered = false;

void IRAM_ATTR onMotorTurn() {
  if (switchTriggered) return;
  motorCounter++;
  switchTriggered = true;
}

void setup() {
  Serial.begin(115200);

  // Inicializar Cloud
  //initProperties();
  //ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Configuración de pines
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relé prendido inicialmente

  // Configurar interrupción en flanco de bajada
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), onMotorTurn, FALLING);

  //setDebugMessageLevel(2);
  //ArduinoCloud.printDebugInfo();
}

void loop() {
  //ArduinoCloud.update();
  
  if (switchTriggered) {
    delay(1000);  
    switchTriggered = false;  // reset bandera
    Serial.println("Vuelta detectada. Contador = " + String(motorCounter));
    
    if (motorCounter >= 10) {
      motorCounter = 0;
      digitalWrite(RELAY_PIN, HIGH); // Desactivar relé
    }
  }
}
