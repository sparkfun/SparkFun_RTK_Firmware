//Display current system status
void menuSystem()
{
  bool sdCardAlreadyMounted;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: System Menu"));

    Serial.print(F("GNSS: "));
    if (online.gnss == true)
    {
      Serial.print(F("Online - "));

      printZEDInfo();

      printCurrentConditions();
    }
    else Serial.println(F("Offline"));

    Serial.print(F("Display: "));
    if (online.display == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    Serial.print(F("Accelerometer: "));
    if (online.display == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    Serial.print(F("Fuel Gauge: "));
    if (online.battery == true)
    {
      Serial.print(F("Online - "));

      battLevel = lipo.getSOC();
      battVoltage = lipo.getVoltage();
      battChangeRate = lipo.getChangeRate();

      Serial.printf("Batt (%d%%) / Voltage: %0.02fV", battLevel, battVoltage);
      Serial.println();
    }
    else Serial.println(F("Offline"));

    Serial.print(F("microSD: "));
    if (online.microSD == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    if (online.lband == true)
    {
      Serial.print(F("L-Band: Online - "));

      if (online.lbandCorrections == true) Serial.print(F("Keys Good"));
      else Serial.print(F("No Keys"));

      Serial.print(F(" / Corrections Received"));
      if (lbandCorrectionsReceived == false) Serial.print(F(" Failed"));

      Serial.printf(" / Eb/N0[dB] (>9 is good): %0.2f", lBandEBNO);

      Serial.print(F(" - "));

      printNEOInfo();
    }

    //Display MAC address
    char macAddress[5];
    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
    Serial.print(F("Bluetooth ("));
    Serial.print(macAddress);
    Serial.print(F("): "));

    //Verify the ESP UART2 can communicate TX/RX to ZED UART1
    if (online.gnss == true)
    {
      if (zedUartPassed == false)
      {
        stopUART2Tasks(); //Stop absoring ZED serial via task

        i2cGNSS.setSerialRate(460800, COM_PORT_UART1); //Defaults to 460800 to maximize message output support
        serialGNSS.begin(460800); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

        SFE_UBLOX_GNSS myGNSS;
        if (myGNSS.begin(serialGNSS) == true) //begin() attempts 3 connections
        {
          zedUartPassed = true;
          Serial.print(F("Online"));
        }
        else
          Serial.print(F("Offline"));

        i2cGNSS.setSerialRate(settings.dataPortBaud, COM_PORT_UART1); //Defaults to 460800 to maximize message output support
        serialGNSS.begin(settings.dataPortBaud); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

        startUART2Tasks(); //Return to normal operation
      }
      else
        Serial.print(F("Online"));
    }
    else
    {
      Serial.print("GNSS Offline");
    }
    Serial.println();

    //Display the uptime
    uint64_t uptimeMilliseconds = uptime;
    uint32_t uptimeDays = 0;
    while (uptimeMilliseconds >= MILLISECONDS_IN_A_DAY) {
      uptimeMilliseconds -= MILLISECONDS_IN_A_DAY;
      uptimeDays += 1;
    }
    byte uptimeHours = 0;
    while (uptimeMilliseconds >= MILLISECONDS_IN_AN_HOUR) {
      uptimeMilliseconds -= MILLISECONDS_IN_AN_HOUR;
      uptimeHours += 1;
    }
    byte uptimeMinutes = 0;
    while (uptimeMilliseconds >= MILLISECONDS_IN_A_MINUTE) {
      uptimeMilliseconds -= MILLISECONDS_IN_A_MINUTE;
      uptimeMinutes += 1;
    }
    byte uptimeSeconds = 0;
    while (uptimeMilliseconds >= MILLISECONDS_IN_A_SECOND) {
      uptimeMilliseconds -= MILLISECONDS_IN_A_SECOND;
      uptimeSeconds += 1;
    }
    Serial.print(F("Uptime: "));
    Serial.printf("%d %02d:%02d:%02d.%03ld\r\n",
                  uptimeDays,
                  uptimeHours,
                  uptimeMinutes,
                  uptimeSeconds,
                  uptimeMilliseconds);

    if (settings.enableSD == true)
    {
      Serial.println(F("f) Display microSD Files"));
    }

    Serial.println(F("d) Configure Debug"));
    Serial.printf("h) Set time zone hours: %d\r\n", settings.timeZoneHours);
    Serial.printf("m) Set time zone minutes: %d\r\n", settings.timeZoneMinutes);
    Serial.printf("s) Set time zone seconds: %d\r\n", settings.timeZoneSeconds);

    Serial.println(F("r) Reset all settings to default"));

    // Support mode switching
    Serial.println(F("B) Switch to Base mode"));
    Serial.println(F("R) Switch to Rover mode"));
    Serial.println(F("W) Switch to WiFi Config mode"));
    Serial.println(F("S) Shut down"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == 'd')
      menuDebug();
    else if (incoming == 'h')
    {
      Serial.print(F("Enter time zone hour offset (-23 <= offset <= 23): "));
      int64_t value = getNumber(menuTimeout);
      if (value < -23 || value > 23)
        Serial.println(F("Error: -24 < hours < 24"));
      else
      {
        settings.timeZoneHours = value;
        online.rtc = false;
        updateRTC();
      }
    }
    else if (incoming == 'm')
    {
      Serial.print(F("Enter time zone minute offset (-59 <= offset <= 59): "));
      int64_t value = getNumber(menuTimeout);
      if (value < -59 || value > 59)
        Serial.println(F("Error: -60 < minutes < 60"));
      else
      {
        settings.timeZoneMinutes = value;
        online.rtc = false;
        updateRTC();
      }
    }
    else if (incoming == 's')
    {
      Serial.print(F("Enter time zone second offset (-59 <= offset <= 59): "));
      int64_t value = getNumber(menuTimeout);
      if (value < -59 || value > 59)
        Serial.println(F("Error: -60 < seconds < 60"));
      else
      {
        settings.timeZoneSeconds = value;
        online.rtc = false;
        updateRTC();
      }
    }
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
    else if ((incoming == 'f') && (settings.enableSD == true))
    {
      sdCardAlreadyMounted = online.microSD;
      if (!online.microSD)
        beginSD();

      //Notify the user if the microSD card is not available
      if (!online.microSD)
        Serial.println(F("microSD card not online!"));
      else
      {
        //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
          Serial.println(F("Files found (date time size name):\n\r"));
          sd->ls(LS_R | LS_DATE | LS_SIZE);
        }
        else
        {
          //Error failed to list the contents of the microSD card
          Serial.printf("sdCardSemaphore failed to yield, menuSystem.ino line %d\r\n", __LINE__);
        }

        //Release the SD card if not originally mounted
        if (sdCardAlreadyMounted)
          xSemaphoreGive(sdCardSemaphore);
        else
          endSD(true, true);
      }
    }
    // Support mode switching
    else if (incoming == 'B') {
      forceSystemStateUpdate = true; //Imediately go to this new state
      changeState(STATE_BASE_NOT_STARTED);
    }
    else if (incoming == 'R') {
      forceSystemStateUpdate = true; //Imediately go to this new state
      changeState(STATE_ROVER_NOT_STARTED);
    }
    else if (incoming == 'W') {
      forceSystemStateUpdate = true; //Imediately go to this new state
      changeState(STATE_WIFI_CONFIG_NOT_STARTED);
    }
    else if (incoming == 'S') {
      Serial.println(F("Shutting down..."));
      forceDisplayUpdate = true;
      powerDown(true);
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

//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Debug Menu"));

    Serial.print(F("1) I2C Debugging Output: "));
    if (settings.enableI2Cdebug == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Heap Reporting: "));
    if (settings.enableHeapReport == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Task Highwater Reporting: "));
    if (settings.enableTaskReports == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Set SPI/SD Interface Frequency: "));
    Serial.print(settings.spiFrequency);
    Serial.println(" MHz");

    Serial.print(F("5) Set SPP RX Buffer Size: "));
    Serial.println(settings.sppRxQueueSize);

    Serial.print(F("6) Set SPP TX Buffer Size: "));
    Serial.println(settings.sppTxQueueSize);

    Serial.print(F("7) Throttle BT Transmissions During SPP Congestion: "));
    if (settings.throttleDuringSPPCongestion == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("8) Display Reset Counter: "));
    if (settings.enableResetDisplay == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("9) GNSS Serial Timeout: "));
    Serial.println(settings.serialTimeoutGNSS);

    Serial.print(F("10) Periodically print Wifi IP Address: "));
    Serial.printf("%s\r\n", settings.enablePrintWifiIpAddress ? "Enabled" : "Disabled");

    Serial.print(F("11) Periodically print state: "));
    Serial.printf("%s\r\n", settings.enablePrintState ? "Enabled" : "Disabled");

    Serial.print(F("12) Periodically print Wifi state: "));
    Serial.printf("%s\r\n", settings.enablePrintWifiState ? "Enabled" : "Disabled");

    Serial.print(F("13) Periodically print NTRIP client state: "));
    Serial.printf("%s\r\n", settings.enablePrintNtripClientState ? "Enabled" : "Disabled");

    Serial.print(F("14) Periodically print NTRIP server state: "));
    Serial.printf("%s\r\n", settings.enablePrintNtripServerState ? "Enabled" : "Disabled");

    Serial.print(F("15) Print GNSS --> NTRIP caster messages: "));
    Serial.printf("%s\r\n", settings.enablePrintNtripServerRtcm ? "Enabled" : "Disabled");

    Serial.print(F("16) Periodically print position: "));
    Serial.printf("%s\r\n", settings.enablePrintPosition ? "Enabled" : "Disabled");

    Serial.print(F("17) Periodically print CPU idle time: "));
    Serial.printf("%s\r\n", settings.enablePrintIdleTime ? "Enabled" : "Disabled");

    Serial.println(F("t) Enter Test Screen"));

    Serial.println(F("e) Erase LittleFS"));

    Serial.println(F("r) Force system reset"));

    Serial.println(F("x) Exit"));

    int incoming;
    int digits = getMenuChoice(&incoming, menuTimeout); //Timeout after x seconds

    //Handle input timeout
    if (digits == GMCS_TIMEOUT)
      break;

    //Handle numeric input
    if (digits > 0)
    {
      if (incoming == 1)
      {
        settings.enableI2Cdebug ^= 1;

        if (settings.enableI2Cdebug)
          i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
        else
          i2cGNSS.disableDebugging();
      }
      else if (incoming == 2)
      {
        settings.enableHeapReport ^= 1;
      }
      else if (incoming == 3)
      {
        settings.enableTaskReports ^= 1;
      }
      else if (incoming == 4)
      {
        Serial.print(F("Enter SPI frequency in MHz (1 to 16): "));
        int freq = getNumber(menuTimeout); //Timeout after x seconds
        if (freq < 1 || freq > 16) //Arbitrary 16MHz limit
        {
          Serial.println(F("Error: SPI frequency out of range"));
        }
        else
        {
          settings.spiFrequency = freq; //Recorded to NVM and file at main menu exit
        }
      }
      else if (incoming == 5)
      {
        Serial.print(F("Enter SPP RX Queue Size in Bytes (32 to 16384): "));
        uint16_t queSize = getNumber(menuTimeout); //Timeout after x seconds
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
        {
          Serial.println(F("Error: Queue size out of range"));
        }
        else
        {
          settings.sppRxQueueSize = queSize; //Recorded to NVM and file at main menu exit
        }
      }
      else if (incoming == 6)
      {
        Serial.print(F("Enter SPP TX Queue Size in Bytes (32 to 16384): "));
        uint16_t queSize = getNumber(menuTimeout); //Timeout after x seconds
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
        {
          Serial.println(F("Error: Queue size out of range"));
        }
        else
        {
          settings.sppTxQueueSize = queSize; //Recorded to NVM and file at main menu exit
        }
      }
      else if (incoming == 7)
      {
        settings.throttleDuringSPPCongestion ^= 1;
      }
      else if (incoming == 8)
      {
        settings.enableResetDisplay ^= 1;
        if (settings.enableResetDisplay == true)
        {
          settings.resetCount = 0;
          recordSystemSettings(); //Record to NVM
        }
      }
      else if (incoming == 9)
      {
        Serial.print(F("Enter GNSS Serial Timeout in milliseconds (0 to 1000): "));
        uint16_t serialTimeoutGNSS = getNumber(menuTimeout); //Timeout after x seconds
        if (serialTimeoutGNSS < 0 || serialTimeoutGNSS > 1000) //Arbitrary 1s limit
        {
          Serial.println(F("Error: Timeout is out of range"));
        }
        else
        {
          settings.serialTimeoutGNSS = serialTimeoutGNSS; //Recorded to NVM and file at main menu exit
        }
      }
      else if (incoming == 10)
      {
        settings.enablePrintWifiIpAddress ^= 1;
      }
      else if (incoming == 11)
      {
        settings.enablePrintState ^= 1;
      }
      else if (incoming == 12)
      {
        settings.enablePrintWifiState ^= 1;
      }
      else if (incoming == 13)
      {
        settings.enablePrintNtripClientState ^= 1;
      }
      else if (incoming == 14)
      {
        settings.enablePrintNtripServerState ^= 1;
      }
      else if (incoming == 15)
      {
        settings.enablePrintNtripServerRtcm ^= 1;
      }
      else if (incoming == 16)
      {
        settings.enablePrintPosition ^= 1;
      }
      else if (incoming == 17)
      {
        settings.enablePrintIdleTime ^= 1;
      }
      else
        printUnknown(incoming);
    }

    //Handle character input
    else if (digits == GMCS_CHARACTER)
    {
      if (incoming == 'e')
      {
        Serial.println("Erasing LittleFS and resetting");
        LittleFS.format();
        ESP.restart();
      }
      else if (incoming == 'r')
      {
        recordSystemSettings();

        ESP.restart();
      }
      else if (incoming == 't')
      {
        requestChangeState(STATE_TEST); //We'll enter test mode once exiting all serial menus
      }
      else if (incoming == 'x')
        break;
      else
        printUnknown(((uint8_t)incoming));
    }
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Print the current long/lat/alt/HPA/SIV
//From Example11_GetHighPrecisionPositionUsingDouble
void printCurrentConditions()
{
  if (online.gnss == true)
  {
    Serial.print(F("SIV: "));
    Serial.print(numSV);

    Serial.print(", HPA (m): ");
    Serial.print(horizontalAccuracy, 3);

    Serial.print(", Lat: ");
    Serial.print(latitude, 9);
    Serial.print(", Lon: ");
    Serial.print(longitude, 9);

    Serial.print(", Altitude (m): ");
    Serial.print(altitude, 1);

    Serial.println();
  }
}

void printCurrentConditionsNMEA()
{
  if (online.gnss == true)
  {
    char systemStatus[100];
    sprintf(systemStatus, "%02d%02d%02d.%02d,%02d%02d%02d,%0.3f,%d,%0.9f,%0.9f,%0.2f,%d,%d,%d",
            gnssHour, gnssMinute, gnssSecond, mseconds,
            gnssDay, gnssMonth, gnssYear % 2000, //Limit to 2 digits
            horizontalAccuracy, numSV,
            latitude, longitude,
            altitude,
            fixType, carrSoln,
            battLevel
           );

    char nmeaMessage[100]; //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, systemStatus); //textID, buffer, text
    Serial.println(nmeaMessage);
  }
  else
  {
    char nmeaMessage[100]; //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, (char *)"OFFLINE"); //textID, buffer, text
    Serial.println(nmeaMessage);
  }
}
