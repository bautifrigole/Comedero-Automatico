#define IN_PIN 34
#define OUT_PIN 33

void setup() {
  Serial.begin(115200);

  pinMode(IN_PIN, INPUT);
  pinMode(OUT_PIN, OUTPUT);
}

void loop() {
  int thereIsNoWater;
  
  delay(2000);

  digitalWrite(OUT_PIN, HIGH);
  thereIsNoWater = digitalRead(IN_PIN);

  if (thereIsNoWater == 1){
    Serial.println("No hay agua");
  } else  if (thereIsNoWater == 0) {
    Serial.println("Hay agua");
  }
  
}
