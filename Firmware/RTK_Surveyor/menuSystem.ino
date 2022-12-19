//Display current system status
void menuSystem()
{
  bool sdCardAlreadyMounted;

  while (1)
  {
    Serial.println();
    Serial.println("Menu: System");

    beginI2C();
    if (online.i2c == false)
      Serial.println("I2C: Offline - Something is causing bus problems");

    Serial.print("GNSS: ");
    if (online.gnss == true)
    {
      Serial.print("Online - ");

      printZEDInfo();

      printCurrentConditions();
    }
    else Serial.println("Offline");

    Serial.print("Display: ");
    if (online.display == true) Serial.println("Online");
    else Serial.println("Offline");

    Serial.print("Accelerometer: ");
    if (online.display == true) Serial.println("Online");
    else Serial.println("Offline");

    Serial.print("Fuel Gauge: ");
    if (online.battery == true)
    {
      Serial.print("Online - ");

      battLevel = lipo.getSOC();
      battVoltage = lipo.getVoltage();
      battChangeRate = lipo.getChangeRate();

      Serial.printf("Batt (%d%%) / Voltage: %0.02fV", battLevel, battVoltage);
      Serial.println();
    }
    else Serial.println("Offline");

    Serial.print("microSD: ");
    if (online.microSD == true) Serial.println("Online");
    else Serial.println("Offline");

    if (online.lband == true)
    {
      Serial.print("L-Band: Online - ");

      if (online.lbandCorrections == true) Serial.print("Keys Good");
      else Serial.print("No Keys");

      Serial.print(" / Corrections Received");
      if (lbandCorrectionsReceived == false) Serial.print(" Failed");

      Serial.printf(" / Eb/N0[dB] (>9 is good): %0.2f", lBandEBNO);

      Serial.print(" - ");

      printNEOInfo();
    }

    //Display the Bluetooth status
    bluetoothTest(false);

#ifdef COMPILE_WIFI
    Serial.print("WiFi MAC Address: ");
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", wifiMACAddress[0],
                  wifiMACAddress[1], wifiMACAddress[2], wifiMACAddress[3],
                  wifiMACAddress[4], wifiMACAddress[5]);
    if (wifiState == WIFI_CONNECTED)
      wifiDisplayIpAddress();
#endif

    //Display the uptime
    uint64_t uptimeMilliseconds = millis();
    uint32_t uptimeDays = 0;
    byte uptimeHours = 0;
    byte uptimeMinutes = 0;
    byte uptimeSeconds = 0;

    uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
    uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

    uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
    uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

    uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
    uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

    uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
    uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

    Serial.print("System Uptime: ");
    Serial.printf("%d %02d:%02d:%02d.%03lld (Resets: %d)\r\n",
                  uptimeDays,
                  uptimeHours,
                  uptimeMinutes,
                  uptimeSeconds,
                  uptimeMilliseconds,
                  settings.resetCount);

    //Display NTRIP Client status and uptime
    if (settings.enableNtripClient == true && (systemState >= STATE_ROVER_NOT_STARTED && systemState <= STATE_ROVER_RTK_FIX))
    {
      Serial.print("NTRIP Client ");
      switch (ntripClientState)
      {
        case NTRIP_CLIENT_OFF:
          Serial.print("Disconnected");
          break;
        case NTRIP_CLIENT_ON:
        case NTRIP_CLIENT_WIFI_CONNECTING:
        case NTRIP_CLIENT_WIFI_CONNECTED:
        case NTRIP_CLIENT_CONNECTING:
          Serial.print("Connecting");
          break;
        case NTRIP_CLIENT_CONNECTED:
          Serial.print("Connected");
          break;
        default:
          Serial.printf("Unknown: %d", ntripClientState);
          break;
      }
      Serial.printf(" - %s/%s:%d", settings.ntripClient_CasterHost, settings.ntripClient_MountPoint, settings.ntripClient_CasterPort);

      uptimeMilliseconds = ntripClientTimer - ntripClientStartTime;

      uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
      uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

      uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
      uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

      uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
      uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

      uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
      uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

      Serial.print(" Uptime: ");
      Serial.printf("%d %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
                    uptimeDays,
                    uptimeHours,
                    uptimeMinutes,
                    uptimeSeconds,
                    uptimeMilliseconds,
                    ntripClientConnectionAttemptsTotal);
    }

    //Display NTRIP Server status and uptime
    if (settings.enableNtripServer == true && (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING))
    {
      Serial.print("NTRIP Server ");
      switch (ntripServerState)
      {
        case NTRIP_SERVER_OFF:
          Serial.print("Disconnected");
          break;
        case NTRIP_SERVER_ON:
        case NTRIP_SERVER_WIFI_CONNECTING:
        case NTRIP_SERVER_WIFI_CONNECTED:
        case NTRIP_SERVER_WAIT_GNSS_DATA:
        case NTRIP_SERVER_CONNECTING:
        case NTRIP_SERVER_AUTHORIZATION:
          Serial.print("Connecting");
          break;
        case NTRIP_SERVER_CASTING:
          Serial.print("Connected");
          break;
        default:
          Serial.printf("Unknown: %d", ntripServerState);
          break;
      }
      Serial.printf(" - %s/%s:%d", settings.ntripServer_CasterHost, settings.ntripServer_MountPoint, settings.ntripServer_CasterPort);

      uptimeMilliseconds = ntripServerTimer - ntripServerStartTime;

      uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
      uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

      uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
      uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

      uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
      uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

      uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
      uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

      Serial.print(" Uptime: ");
      Serial.printf("%d %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
                    uptimeDays,
                    uptimeHours,
                    uptimeMinutes,
                    uptimeSeconds,
                    uptimeMilliseconds,
                    ntripServerConnectionAttemptsTotal);
    }

    if (settings.enableSD == true && online.microSD == true)
    {
      Serial.println("f) Display microSD Files");
    }

    Serial.print("e) Echo User Input: ");
    if (settings.echoUserInput == true) Serial.println("On");
    else Serial.println("Off");

    Serial.println("d) Configure Debug");

    Serial.printf("z) Set time zone offset: %02d:%02d:%02d\r\n", settings.timeZoneHours, settings.timeZoneMinutes, settings.timeZoneSeconds);

    Serial.print("b) Set Bluetooth Mode: ");
    if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
      Serial.println("Classic");
    else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
      Serial.println("BLE");
    else
      Serial.println("Off");

    Serial.print("c) Enable/disable WiFi NMEA client (connect to phone): ");
    if (settings.enableNmeaClient == true)
      Serial.println("Enabled");
    else
      Serial.println("Disabled");

    Serial.print("n) Enable/disable WiFi NMEA server: ");
    if (settings.enableNmeaServer == true)
      Serial.println("Enabled");
    else
      Serial.println("Disabled");

    Serial.println("r) Reset all settings to default");

    // Support mode switching
    Serial.println("B) Switch to Base mode");
    Serial.println("R) Switch to Rover mode");
    Serial.println("W) Switch to WiFi Config mode");
    Serial.println("S) Shut down");

    Serial.println("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming == 'd')
      menuDebug();
    else if (incoming == 'z')
    {
      Serial.print("Enter time zone hour offset (-23 <= offset <= 23): ");
      int value = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (value < -23 || value > 23)
          Serial.println("Error: -24 < hours < 24");
        else
        {
          settings.timeZoneHours = value;

          Serial.print("Enter time zone minute offset (-59 <= offset <= 59): ");
          int value = getNumber(); //Returns EXIT, TIMEOUT, or long
          if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
          {
            if (value < -59 || value > 59)
              Serial.println("Error: -60 < minutes < 60");
            else
            {
              settings.timeZoneMinutes = value;

              Serial.print("Enter time zone second offset (-59 <= offset <= 59): ");
              int value = getNumber(); //Returns EXIT, TIMEOUT, or long
              if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
              {
                if (value < -59 || value > 59)
                  Serial.println("Error: -60 < seconds < 60");
                else
                {
                  settings.timeZoneSeconds = value;
                  online.rtc = false;
                  updateRTC();
                } //Succesful seconds
              }
            } //Succesful minute
          }
        } //Succesful hours
      }
    }
    else if (incoming == 'e')
    {
      settings.echoUserInput ^= 1;
    }
    else if (incoming == 'b')
    {
      // Restart Bluetooth
      bluetoothStop();
      if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        settings.bluetoothRadioType = BLUETOOTH_RADIO_BLE;
      else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        settings.bluetoothRadioType = BLUETOOTH_RADIO_OFF;
      else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF)
        settings.bluetoothRadioType = BLUETOOTH_RADIO_SPP;
      bluetoothStart();
    }
    else if (incoming == 'c')
    {
      //Toggle WiFi NEMA client (connect to phone)
      settings.enableNmeaClient ^= 1;
    }
    else if (incoming == 'n')
    {
      //Toggle WiFi NEMA server
      settings.enableNmeaServer ^= 1;
      if ((!settings.enableNmeaServer) && online.nmeaServer)
      {
        //Tell the UART2 tasks that the NMEA server is shutting down
        online.nmeaServer = false;

        //Wait for the UART2 tasks to close the NMEA client connections
        while (wifiNmeaTcpServerActive())
          delay(5);
        Serial.println("NMEA Server offline");
      }
    }
    else if (incoming == 'r')
    {
      Serial.println("\r\nResetting to factory defaults. Press 'y' to confirm:");
      byte bContinue = getCharacterNumber();
      if (bContinue == 'y')
      {
        factoryReset();
      }
      else
        Serial.println("Reset aborted");
    }
    else if ((incoming == 'f') && (settings.enableSD == true) &&  (online.microSD == true))
    {
      sdCardAlreadyMounted = online.microSD;
      if (!online.microSD)
        beginSD();

      //Notify the user if the microSD card is not available
      if (!online.microSD)
        Serial.println("microSD card not online!");
      else
      {
        //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
          markSemaphore(FUNCTION_FILELIST);
          
          Serial.println("Files found (date time size name):\r\n");
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
      Serial.println("Shutting down...");
      forceDisplayUpdate = true;
      powerDown(true);
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_EMPTY)
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Debug");

    Serial.print("1) u-blox I2C Debugging Output: ");
    if (settings.enableI2Cdebug == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("2) Heap Reporting: ");
    if (settings.enableHeapReport == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("3) Task Highwater Reporting: ");
    if (settings.enableTaskReports == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("4) Set SPI/SD Interface Frequency: ");
    Serial.print(settings.spiFrequency);
    Serial.println(" MHz");

    Serial.print("5) Set SPP RX Buffer Size: ");
    Serial.println(settings.sppRxQueueSize);

    Serial.print("6) Set SPP TX Buffer Size: ");
    Serial.println(settings.sppTxQueueSize);

    Serial.printf("8) Display Reset Counter: %d - ", settings.resetCount);
    if (settings.enableResetDisplay == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("9) GNSS Serial Timeout: ");
    Serial.println(settings.serialTimeoutGNSS);

    Serial.print("10) Periodically print WiFi IP Address: ");
    Serial.printf("%s\r\n", settings.enablePrintWifiIpAddress ? "Enabled" : "Disabled");

    Serial.print("11) Periodically print state: ");
    Serial.printf("%s\r\n", settings.enablePrintState ? "Enabled" : "Disabled");

    Serial.print("12) Periodically print WiFi state: ");
    Serial.printf("%s\r\n", settings.enablePrintWifiState ? "Enabled" : "Disabled");

    Serial.print("13) Periodically print NTRIP client state: ");
    Serial.printf("%s\r\n", settings.enablePrintNtripClientState ? "Enabled" : "Disabled");

    Serial.print("14) Periodically print NTRIP server state: ");
    Serial.printf("%s\r\n", settings.enablePrintNtripServerState ? "Enabled" : "Disabled");

    Serial.print("15) Periodically print position: ");
    Serial.printf("%s\r\n", settings.enablePrintPosition ? "Enabled" : "Disabled");

    Serial.print("16) Periodically print CPU idle time: ");
    Serial.printf("%s\r\n", settings.enablePrintIdleTime ? "Enabled" : "Disabled");

    Serial.println("17) Mirror ZED-F9x's UART1 settings to USB");

    Serial.print("18) Print battery status messages: ");
    Serial.printf("%s\r\n", settings.enablePrintBatteryMessages ? "Enabled" : "Disabled");

    Serial.print("19) Print Rover accuracy messages: ");
    Serial.printf("%s\r\n", settings.enablePrintRoverAccuracy ? "Enabled" : "Disabled");

    Serial.print("20) Print messages with bad checksums or CRCs: ");
    Serial.printf("%s\r\n", settings.enablePrintBadMessages ? "Enabled" : "Disabled");

    Serial.print("21) Print log file messages: ");
    Serial.printf("%s\r\n", settings.enablePrintLogFileMessages ? "Enabled" : "Disabled");

    Serial.print("22) Print log file status: ");
    Serial.printf("%s\r\n", settings.enablePrintLogFileStatus ? "Enabled" : "Disabled");

    Serial.print("23) Print ring buffer offsets: ");
    Serial.printf("%s\r\n", settings.enablePrintRingBufferOffsets ? "Enabled" : "Disabled");

    Serial.print("24) Print GNSS --> NTRIP caster messages: ");
    Serial.printf("%s\r\n", settings.enablePrintNtripServerRtcm ? "Enabled" : "Disabled");

    Serial.print("25) Print NTRIP caster --> GNSS messages: ");
    Serial.printf("%s\r\n", settings.enablePrintNtripClientRtcm ? "Enabled" : "Disabled");

    Serial.print("26) Print states: ");
    Serial.printf("%s\r\n", settings.enablePrintStates ? "Enabled" : "Disabled");

    Serial.print("27) Print duplicate states: ");
    Serial.printf("%s\r\n", settings.enablePrintDuplicateStates ? "Enabled" : "Disabled");

    Serial.print("28) RTCM message checking: ");
    Serial.printf("%s\r\n", settings.enableRtcmMessageChecking ? "Enabled" : "Disabled");

    Serial.print("29) Run Logging Test: ");
    Serial.printf("%s\r\n", settings.runLogTest ? "Enabled" : "Disabled");

    Serial.println("30) Run Bluetooth Test");

    Serial.print("31) Print NMEA TCP status: ");
    Serial.printf("%s\r\n", settings.enablePrintNmeaTcpStatus ? "Enabled" : "Disabled");

    Serial.print("32) ESP-Now Broadcast Override: ");
    Serial.printf("%s\r\n", settings.espnowBroadcast ? "Enabled" : "Disabled");

    Serial.print("33) Print buffer overruns: ");
    Serial.printf("%s\r\n", settings.enablePrintBufferOverrun ? "Enabled" : "Disabled");

    Serial.print("34) Set UART Receive Buffer Size: ");
    Serial.println(settings.uartReceiveBufferSize);

    Serial.print("35) Set GNSS Handler Buffer Size: ");
    Serial.println(settings.gnssHandlerBufferSize);

    Serial.print("36) Print SD and UART buffer sizes: ");
    Serial.printf("%s\r\n", settings.enablePrintSDBuffers ? "Enabled" : "Disabled");

    Serial.println("t) Enter Test Screen");

    Serial.println("e) Erase LittleFS");

    Serial.println("r) Force system reset");

    Serial.println("x) Exit");

    byte incoming = getCharacterNumber();

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
      Serial.print("Enter SPI frequency in MHz (1 to 16): ");
      int freq = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((freq != INPUT_RESPONSE_GETNUMBER_EXIT) && (freq != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (freq < 1 || freq > 16) //Arbitrary 16MHz limit
          Serial.println("Error: SPI frequency out of range");
        else
          settings.spiFrequency = freq; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 5)
    {
      Serial.print("Enter SPP RX Queue Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          Serial.println("Error: Queue size out of range");
        else
          settings.sppRxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 6)
    {
      Serial.print("Enter SPP TX Queue Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          Serial.println("Error: Queue size out of range");
        else
          settings.sppTxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
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
      Serial.print("Enter GNSS Serial Timeout in milliseconds (0 to 1000): ");
      int serialTimeoutGNSS = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_EXIT) && (serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (serialTimeoutGNSS < 0 || serialTimeoutGNSS > 1000) //Arbitrary 1s limit
          Serial.println("Error: Timeout is out of range");
        else
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
      settings.enablePrintPosition ^= 1;
    }
    else if (incoming == 16)
    {
      settings.enablePrintIdleTime ^= 1;
    }
    else if (incoming == 17)
    {
      bool response = setMessagesUSB();

      if (response == false)
        Serial.println(F("Failed to enable USB messages"));
      else
        Serial.println(F("USB messages successfully enabled"));
    }
    else if (incoming == 18)
    {
      settings.enablePrintBatteryMessages ^= 1;
    }
    else if (incoming == 19)
    {
      settings.enablePrintRoverAccuracy ^= 1;
    }
    else if (incoming == 20)
    {
      settings.enablePrintBadMessages ^= 1;
    }
    else if (incoming == 21)
    {
      settings.enablePrintLogFileMessages ^= 1;
    }
    else if (incoming == 22)
    {
      settings.enablePrintLogFileStatus ^= 1;
    }
    else if (incoming == 23)
    {
      settings.enablePrintRingBufferOffsets ^= 1;
    }
    else if (incoming == 24)
    {
      settings.enablePrintNtripServerRtcm ^= 1;
    }
    else if (incoming == 25)
    {
      settings.enablePrintNtripClientRtcm ^= 1;
    }
    else if (incoming == 26)
    {
      settings.enablePrintStates ^= 1;
    }
    else if (incoming == 27)
    {
      settings.enablePrintDuplicateStates ^= 1;
    }
    else if (incoming == 28)
    {
      settings.enableRtcmMessageChecking ^= 1;
    }
    else if (incoming == 29)
    {
      settings.runLogTest ^= 1;

      logTestState = LOGTEST_START; //Start test

      //Mark current log file as complete to force test start
      startCurrentLogTime_minutes = systemTime_minutes - settings.maxLogLength_minutes;
    }
    else if (incoming == 30)
    {
      bluetoothTest(true);
    }
    else if (incoming == 31)
    {
      settings.enablePrintNmeaTcpStatus ^= 1;
    }
    else if (incoming == 32)
    {
      settings.espnowBroadcast ^= 1;
    }
    else if (incoming == 33)
    {
      settings.enablePrintBufferOverrun ^= 1;
    }
    else if (incoming == 34)
    {
      Serial.print("Enter UART Receive Buffer Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          Serial.println("Error: Queue size out of range");
        else
          settings.uartReceiveBufferSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 35)
    {
      Serial.print("Enter GNSS Handler Buffer Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          Serial.println("Error: Queue size out of range");
        else
          settings.gnssHandlerBufferSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 36)
    {
      settings.enablePrintSDBuffers ^= 1;
    }
    else if (incoming == 'e')
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
    else if (incoming == INPUT_RESPONSE_EMPTY)
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Print the current long/lat/alt/HPA/SIV
//From Example11_GetHighPrecisionPositionUsingDouble
void printCurrentConditions()
{
  if (online.gnss == true)
  {
    Serial.print("SIV: ");
    Serial.print(numSV);

    Serial.print(", HPA (m): ");
    Serial.print(horizontalAccuracy, 3);

    Serial.print(", Lat: ");
    Serial.print(latitude, haeNumberOfDecimals);
    Serial.print(", Lon: ");
    Serial.print(longitude, haeNumberOfDecimals);

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
