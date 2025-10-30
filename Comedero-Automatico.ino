#define PUMP_PIN 13    // D7
#define MOTOR_PIN 15   // D8
//#define SWITCH_PIN 0   // D3

void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  //pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Sistema iniciado.");
}

void loop() {
  Serial.println("Encendiendo motor y bomba...");
  digitalWrite(MOTOR_PIN, HIGH);
  digitalWrite(PUMP_PIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);
  delay(10000);

  Serial.println("Apagando motor y bomba...");
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);
}
