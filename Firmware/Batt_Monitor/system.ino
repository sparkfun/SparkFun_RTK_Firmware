//Update Battery level LEDs
void batteryLEDTask(void *e)
{
  while (1)
  {
    int battLevel = battMonitor.percent();

    Serial.print("Batt (");
    Serial.print(battLevel);
    Serial.print("%): ");

    if (battLevel < 10)
    {
      Serial.print("RED uh oh!");
      ledcWrite(batteryLevelLED_Red, 255);
      ledcWrite(batteryLevelLED_Green, 0);
    }
    else if (battLevel < 50)
    {
      Serial.print("Yellow ok");
      ledcWrite(batteryLevelLED_Red, 128);
      ledcWrite(batteryLevelLED_Green, 128);
    }
    else if (battLevel >= 50)
    {
      Serial.print("Green all good");
      ledcWrite(batteryLevelLED_Red, 0);
      ledcWrite(batteryLevelLED_Green, 255);
    }
    else
    {
      Serial.print("No batt");
      ledcWrite(batteryLevelLED_Red, 0);
      ledcWrite(batteryLevelLED_Green, 0);
    }
    Serial.println();

    delay(5000);
    taskYIELD();
  }
}
