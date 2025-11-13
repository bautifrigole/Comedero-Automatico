#define SENSOR_PIN 12  // D6

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  int estado = digitalRead(SENSOR_PIN);

  if (estado == LOW) {
    Serial.println("⚫ Objeto detectado");
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    Serial.println("⚪ Camino libre");
    digitalWrite(LED_BUILTIN, HIGH);
  }

  delay(500);
}
