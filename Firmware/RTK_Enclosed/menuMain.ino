//Display the options
//If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Main Menu"));

    Serial.println(F("1) Configure Data Logging"));

    if (settings.enableSD && online.microSD)
      Serial.println(F("s) SD Card File Transfer"));

    Serial.println(F("r) Reset all settings to default"));

    Serial.println(F("t) Test menu"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1') {}
    //menuLogRate();
    else if (incoming == '2') {}
    //menuTimeStamp();
    else if (incoming == 'r')
    {
      //      Serial.println(F("\r\nResetting to factory defaults. Press 'y' to confirm:"));
      //      byte bContinue = getByteChoice(menuTimeout);
      //      if (bContinue == 'y')
      //      {
      //        EEPROM.erase();
      //        if (sd.exists("OLA_settings.txt"))
      //          sd.remove("OLA_settings.txt");
      //        if (sd.exists("OLA_deviceSettings.txt"))
      //          sd.remove("OLA_deviceSettings.txt");
      //
      //        Serial.print(F("Settings erased. Please reset OpenLog Artemis and open a terminal at "));
      //        Serial.print((String)settings.serialTerminalBaudRate);
      //        Serial.println(F("bps..."));
      //        while (1);
      //    else
      //      Serial.println(F("Reset aborted"));
    }
    else if (incoming == 't')
      menuTest();
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  //recordSystemSettings(); //Once all menus have exited, record the new settings to EEPROM and config file

  //recordDeviceSettingsToFile(); //Record the current devices settings to device config file

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
