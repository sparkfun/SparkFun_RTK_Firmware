//Update Battery level LEDs every 5s
void updateBattLEDs()
{
  if (millis() - lastBattUpdate > 5000)
  {
    lastBattUpdate = millis();

    checkBatteryLevels();
  }
}

//When called, checks level of battery and updates the LED brightnesses
//And outputs a serial message to USB
void checkBatteryLevels()
{
  battLevel = lipo.getSOC();
  battVoltage = lipo.getVoltage();
  battChangeRate = lipo.getChangeRate();

  Serial.printf("Batt (%d%%): Voltage: %0.02fV", battLevel, battVoltage);

  char tempStr[25];
  if (battChangeRate > 0)
    sprintf(tempStr, "C");
  else
    sprintf(tempStr, "Disc");
  Serial.printf(" %sharging: %0.02f%%/hr ", tempStr, battChangeRate);

  if (battLevel < 10)
    sprintf(tempStr, "Red");
  else if (battLevel < 50)
    sprintf(tempStr, "Yellow");
  else if (battLevel >= 50)
    sprintf(tempStr, "Green");
  else
    sprintf(tempStr, "No batt");

  Serial.printf("%s\n\r", tempStr);

//  if (productVariant == RTK_SURVEYOR)
//  {
//    if (battLevel < 10)
//    {
//      ledcWrite(ledRedChannel, 255);
//      ledcWrite(ledGreenChannel, 0);
//    }
//    else if (battLevel < 50)
//    {
//      ledcWrite(ledRedChannel, 128);
//      ledcWrite(ledGreenChannel, 128);
//    }
//    else if (battLevel >= 50)
//    {
//      ledcWrite(ledRedChannel, 0);
//      ledcWrite(ledGreenChannel, 255);
//    }
//    else
//    {
//      ledcWrite(ledRedChannel, 10);
//      ledcWrite(ledGreenChannel, 0);
//    }
//  }
}
