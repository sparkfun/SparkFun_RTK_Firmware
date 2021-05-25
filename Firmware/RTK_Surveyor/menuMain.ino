//Display the options
//If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
  displaySerialConfig(); //Display 'Serial Config' while user is configuring

  while (1)
  {
    Serial.println();
    Serial.printf("SparkFun RTK Surveyor v%d.%d\r\n", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

    Serial.print(F("** Bluetooth broadcasting as: "));
    Serial.print(deviceName);
    Serial.println(F(" **"));

    Serial.println(F("Menu: Main Menu"));

    Serial.println(F("1) Configure GNSS Receiver"));

    Serial.println(F("2) Configure GNSS Messages"));

    Serial.println(F("3) Configure Base"));

    Serial.println(F("4) Configure Ports"));

    if (online.accelerometer == true)
      Serial.println(F("b) Bubble Level"));

    Serial.println(F("d) Configure Debug"));

    Serial.println(F("r) Reset all settings to default"));

    if (binCount > 0)
      Serial.println(F("f) Firmware upgrade"));

    Serial.println(F("t) Test menu"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      menuGNSS();
    else if (incoming == '2')
      menuMessages();
    else if (incoming == '3')
      menuBase();
    else if (incoming == '4')
      menuPorts();
    else if (incoming == 'd')
      menuDebug();
    else if (incoming == 'r')
    {
      Serial.println(F("\r\nResetting to factory defaults. Press 'y' to confirm:"));
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        eepromErase();

        //Assemble settings file name
        char settingsFileName[40]; //SFE_Surveyor_Settings.txt
        strcpy(settingsFileName, platformFilePrefix);
        strcat(settingsFileName, "_Settings.txt");

        if (sd.exists(settingsFileName))
          sd.remove(settingsFileName);

        i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

        Serial.println(F("Settings erased. Please reset RTK Surveyor. Freezing."));
        while (1)
          delay(1); //Prevent CPU freakout
      }
      else
        Serial.println(F("Reset aborted"));
    }
    else if (incoming == 'f' && binCount > 0)
      menuFirmware();
    else if (incoming == 't')
      menuTest();
    else if (incoming == 'b')
    {
      if (online.accelerometer == true) menuBubble();
    }
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
