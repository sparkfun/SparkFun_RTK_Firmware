
const int batteryLevelLED_Red = 32;
const int batteryLevelLED_Green = 33;

const int freq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int resolution = 8;

void setup() {
  ledcSetup(ledRedChannel, freq, resolution);
  ledcSetup(ledGreenChannel, freq, resolution);

  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);

  ledcWrite(ledRedChannel, 128);
  ledcWrite(ledGreenChannel, 128);
}

// the loop function runs over and over again forever
void loop() {
//  ledcWrite(batteryLevelLED_Red, 128);
  //ledcWrite(batteryLevelLED_Green, 128);
  delay(1000);                       // wait for a second
  //ledcWrite(batteryLevelLED_Red, 128);
  //ledcWrite(batteryLevelLED_Green, 128);
  delay(1000);                       // wait for a second
}
