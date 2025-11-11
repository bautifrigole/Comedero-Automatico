#define SENSOR_PIN 5  // D1: Pin de salida del receptor IR

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP); // Usa resistencia pull-up interna
  Serial.println("Prueba sensor IR Omron break beam iniciada...");
}

void loop() {
  int estado = digitalRead(SENSOR_PIN);

  if (estado == LOW) {
    Serial.println("⚫ Objeto detectado");
  } else {
    Serial.println("⚪ Camino libre");
  }

  delay(500); // Actualiza cada medio segundo
}
