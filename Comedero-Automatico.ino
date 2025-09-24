#include "thingProperties.h"

#define SWITCH_PIN 27   // Switch conectado al GPIO27
#define RELAY_PIN 12    // Pin GPIO 12 conectado al IN del relay

bool lastSwitchState = HIGH;  // estado anterior del switch

void setup() {
  Serial.begin(115200);

  // Inicializar Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Configuración de pines
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relé apagado inicialmente

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  motorCounter = 0;
}

void loop() {
  ArduinoCloud.update();

  // Leer el estado del switch
  int currentState = digitalRead(SWITCH_PIN);

  // Actualizar variable en Arduino Cloud
  switchState = (currentState == LOW);

  // Detectar flanco de HIGH -> LOW (cuando se presiona)
  if (lastSwitchState == HIGH && currentState == LOW) {
    motorCounter++;
    Serial.println("Vuelta detectada. Contador = " + String(motorCounter));
    delay(1000);
  }

  lastSwitchState = currentState;

  if (motorCounter >= 10 ) {
    
    while (switchState){
      int currentState = digitalRead(SWITCH_PIN);
      switchState = (currentState == LOW);
    }

    motorCounter = 0;
    digitalWrite(RELAY_PIN, HIGH);
  }
}
