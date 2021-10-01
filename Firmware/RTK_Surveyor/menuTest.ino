//Production testing
//Allow operator to output NMEA on radio port for connector testing
//Scan for display
void menuTest()
{
  //Enable RTCM 1230. This is the GLONASS bias sentence and is transmitted
  //even if there is no GPS fix. We use it to test serial output.
  i2cGNSS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 1); //Enable message every second

  //Go direct into test display
  if (online.display)
    changeState(STATE_TEST);

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Test Menu"));

    Serial.print(F("Bluetooth broadcasting as: "));
    Serial.println(deviceName);

    Serial.println(F("Radio Port is now outputting RTCM"));

    if (settings.enableSD && online.microSD)
    {
      Serial.print(F("microSD card detected"));
      if (createTestFile() == false)
      {
        Serial.print(F(" Failed to create test file. Format SD card with 'SD Card Formatter'."));
      }
      Serial.println();
    }

    //0x3D is default on Qwiic board
    if (isConnected(0x3D) == true || isConnected(0x3C) == true)
      Serial.println(F("Qwiic Good. Display detected."));
    else
      Serial.println(F("Qwiic port failed! No display detected."));

    Serial.println(F("Any character received over Blueooth connection will be displayed here"));

    Serial.println(F("1) Display microSD contents"));
    Serial.println(F("2) Duplicate UART messages to USB"));
    Serial.println(F("3) Enter Test Screen"));

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
    {
      if (settings.enableSD && online.microSD)
      {
        //Attempt to access file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
        if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
          Serial.println(F("Files found (date time size name):\n\r"));
          sd.ls(LS_R | LS_DATE | LS_SIZE);

          xSemaphoreGive(xFATSemaphore);
        }
      }
    }
    else if (incoming == 2)
    {
      //Send the current settings to USB
      bool response = configureGNSSMessageRates(COM_PORT_USB, settings.ubxMessages); //Make sure the appropriate messages are enabled
      if (response == false)
        Serial.println(F("menuTest: Failed to enable USB messages"));
      else
        Serial.println(F("USB now matches UART messages"));
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  //Disable RTCM sentences
  i2cGNSS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 0);

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
