/*
  Keeps ESP32 running in infinite loop
*/

void setup()
{
  Serial.begin(115200);
  Serial.println("Power on");
}

void loop()
{
  delay(100);
  Serial.print(".");
}
