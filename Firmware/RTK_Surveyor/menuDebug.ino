//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  int maxWait = 2000;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Debug Menu"));

    //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
    if (i2cGNSS.getModuleInfo(maxWait) == true) // Try to get the module info
    {
      Serial.print(F("ZED-F9P firmware: "));
      Serial.println(i2cGNSS.minfo.extension[1]);
    }

    Serial.print(F("1) Toggle I2C Debugging Output: "));
    if (settings.enableI2Cdebug == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Toggle Heap Reporting: "));
    if (settings.enableHeapReport == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enableI2Cdebug ^= 1;

      if (settings.enableI2Cdebug)
        i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
      else
        i2cGNSS.disableDebugging();
    }
    else if (incoming == '2')
    {
      settings.enableHeapReport ^= 1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Globals but only used for menuBubble
double averagedRoll = 0.0;
double averagedPitch = 0.0;

//A bubble level
void menuBubble()
{
  Serial.println();
  Serial.println(F("Menu: Bubble Level"));

  Serial.print(F("Press any key to exit"));

  delay(10);
  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars

  while (1)
  {
    if (Serial.available()) break;

    getAngles();

    if (online.display == true)
    {
      oled.clear(PAGE); // Clear the display's internal memory

      //Draw dot in middle
      oled.pixel(LCDWIDTH / 2, LCDHEIGHT / 2);
      oled.pixel(LCDWIDTH / 2 + 1, LCDHEIGHT / 2);
      oled.pixel(LCDWIDTH / 2, LCDHEIGHT / 2 + 1);
      oled.pixel(LCDWIDTH / 2 + 1, LCDHEIGHT / 2 + 1);

      //Draw circle relative to dot
      const int radiusLarge = 10;
      const int radiusSmall = 4;

      oled.circle(LCDWIDTH / 2 - averagedPitch, LCDHEIGHT / 2 + averagedRoll, radiusLarge);
      oled.circle(LCDWIDTH / 2 - averagedPitch, LCDHEIGHT / 2 + averagedRoll, radiusSmall);

      oled.display();
    }
  }

  displaySerialConfig(); //Display 'Serial Config' while user is configuring

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

void getAngles()
{
  if (online.accelerometer == true)
  {
    averagedRoll = 0.0;
    averagedPitch = 0.0;
    const int avgAmount = 16;

    //Take an average readings
    for (int reading = 0 ; reading < avgAmount ; reading++)
    {
      while (accel.available() == false) delay(1);

      float accelX = accel.getX();
      float accelZ = accel.getY();
      float accelY = accel.getZ();
      accelZ *= -1.0;
      accelX *= -1.0;

      double roll = atan2(accelY , accelZ) * 57.3;
      double pitch = atan2((-accelX) , sqrt(accelY * accelY + accelZ * accelZ)) * 57.3;

      averagedRoll += roll;
      averagedPitch += pitch;
    }

    averagedRoll /= (float)avgAmount;
    averagedPitch /= (float)avgAmount;

    //Avoid -0 since we're not printing the decimal portion
    if (averagedRoll < 0.5 && averagedRoll > -0.5) averagedRoll = 0;
    if (averagedPitch < 0.5 && averagedPitch > -0.5) averagedPitch = 0;
  }
}
