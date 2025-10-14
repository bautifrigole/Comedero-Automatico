void setup()
{
  Serial.begin(115200);
}

void loop()
{
  int val=analogRead(25);
  Serial.println(val);
  delay(999);
}
