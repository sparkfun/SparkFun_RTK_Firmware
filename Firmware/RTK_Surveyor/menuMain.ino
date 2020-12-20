//Display the options
//If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
  while (1)
  {
    Serial.println();
    Serial.printf("SparkFun RTK Surveyor v%d.%d\r\n", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

    Serial.print(F("** Bluetooth broadcasting as: "));
    Serial.print(deviceName);
    Serial.println(F(" **"));

    Serial.println(F("Menu: Main Menu"));

    Serial.println(F("1) Configure GNSS Receiver"));

    Serial.println(F("2) Configure Data Logging"));

    Serial.println(F("3) Configure Base"));

    Serial.println(F("4) Configure Ports"));

    Serial.println(F("r) Reset all settings to default"));

    if(binCount > 0)
      Serial.println(F("f) Firmware upgrade"));

    Serial.println(F("t) Test menu"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      menuGNSS();
    else if (incoming == '2')
      menuLog();
    else if (incoming == '3')
      menuBase();
    else if (incoming == '4')
      menuPorts();
    else if (incoming == 'r')
    {
      Serial.println(F("\r\nResetting to factory defaults. Press 'y' to confirm:"));
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        eepromErase();

        if (sd.exists(settingsFileName))
          sd.remove(settingsFileName);

        myGPS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

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
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  recordSystemSettings(); //Once all menus have exited, record the new settings to EEPROM and config file

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
