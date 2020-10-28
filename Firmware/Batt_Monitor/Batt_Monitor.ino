/*
  Red = 34
  Green = 35
  Blue = Used for Charging

  Battery level LED goes from Green (50-100%) to Yellow (10-50%) to Red (<10%)
*/

#include "MAX17048.h" //Click here to get the library: http://librarymanager/All#MAX17048
MAX17048 battMonitor;

const int batteryLevelLED_Red = 34;
const int batteryLevelLED_Green = 35;

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  battMonitor.attatch(Wire);

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(batteryLevelLED_Red, ledChannel);
  ledcAttachPin(batteryLevelLED_Green, ledChannel);

  xTaskCreate(batteryLEDTask, "batteryLEDs", 1000, NULL, 0, NULL); //1000 stack, 0 = Low priority
}

void loop()
{
  Serial.println(".");
  delay(1000);
}
