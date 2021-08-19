//Display the options
//If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
  displaySerialConfig(); //Display 'Serial Config' while user is configuring

  while (1)
  {
    Serial.println();
    Serial.printf("SparkFun RTK %s v%d.%d-%s\r\n", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

    Serial.print(F("** Bluetooth broadcasting as: "));
    Serial.print(deviceName);
    Serial.println(F(" **"));

    Serial.println(F("Menu: Main Menu"));

    Serial.println(F("1) Configure GNSS Receiver"));

    Serial.println(F("2) Configure GNSS Messages"));

    if (zedModuleType == PLATFORM_F9P)
      Serial.println(F("3) Configure Base"));

    Serial.println(F("4) Configure Ports"));

    Serial.println(F("5) Configure Logging"));

    if (settings.enableSD == true && online.microSD == true)
    {
      Serial.println(F("6) Display microSD contents"));
    }

    Serial.println(F("d) Configure Debug"));

    Serial.println(F("r) Reset all settings to default"));

    if (binCount > 0)
      Serial.println(F("f) Firmware upgrade"));

    //Serial.println(F("t) Test menu"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      menuGNSS();
    else if (incoming == '2')
      menuMessages();
    else if (incoming == '3' && zedModuleType == PLATFORM_F9P)
      menuBase();
    else if (incoming == '4')
      menuPorts();
    else if (incoming == '5')
      menuLog();
    else if (incoming == '6' && settings.enableSD == true && online.microSD == true)
    {
      //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
      if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
      {
        Serial.println(F("Files found (date time size name):\n\r"));
        sd.ls(LS_R | LS_DATE | LS_SIZE);

        xSemaphoreGive(xFATSemaphore);
      }
    }
    else if (incoming == 'd')
      menuDebug();
    else if (incoming == 'r')
    {
      Serial.println(F("\r\nResetting to factory defaults. Press 'y' to confirm:"));
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        factoryReset();
      }
      else
        Serial.println(F("Reset aborted"));
    }
    else if (incoming == 'f' && binCount > 0)
      menuFirmware();
    else if (incoming == 't')
      menuTest();
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  recordSystemSettings(); //Once all menus have exited, record the new settings to EEPROM and config file

  i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR on the ZED-F9P

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Erase all settings. Upon restart, unit will use defaults
void factoryReset()
{
  eepromErase();

  //Assemble settings file name
  char settingsFileName[40]; //SFE_Surveyor_Settings.txt
  strcpy(settingsFileName, platformFilePrefix);
  strcat(settingsFileName, "_Settings.txt");

  //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
  if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
  {
    if (sd.exists(settingsFileName))
      sd.remove(settingsFileName);
    xSemaphoreGive(xFATSemaphore);
  } //End xFATSemaphore

  i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

  displaySytemReset(); //Display friendly message on OLED

  Serial.println(F("Settings erased successfully. Rebooting. Good bye!"));
  delay(2000);
  ESP.restart();
}
