//Display current system status
void menuSystem()
{
  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: System");

    beginI2C();
    if (online.i2c == false)
      systemPrintln("I2C: Offline - Something is causing bus problems");

    systemPrint("GNSS: ");
    if (online.gnss == true)
    {
      systemPrint("Online - ");

      printZEDInfo();

      printCurrentConditions();
    }
    else
      systemPrintln("Offline");

    systemPrint("Display: ");
    if (online.display == true)
      systemPrintln("Online");
    else
      systemPrintln("Offline");

    if (online.accelerometer == true)
      systemPrintln("Accelerometer: Online");

    systemPrint("Fuel Gauge: ");
    if (online.battery == true)
    {
      systemPrint("Online - ");

      battLevel = lipo.getSOC();
      battVoltage = lipo.getVoltage();

      systemPrintf("Batt (%d%%) / Voltage: %0.02fV", battLevel, battVoltage);
      systemPrintln();
    }
    else
      systemPrintln("Offline");

    systemPrint("microSD: ");
    if (online.microSD == true)
      systemPrintln("Online");
    else
      systemPrintln("Offline");

    if (online.lband == true)
    {
      systemPrint("L-Band: Online - ");

      if (online.lbandCorrections == true)
        systemPrint("Keys Good");
      else
        systemPrint("No Keys");

      systemPrint(" / Corrections Received");
      if (lbandCorrectionsReceived == false)
        systemPrint(" Failed");

      systemPrintf(" / Eb/N0[dB] (>9 is good): %0.2f", lBandEBNO);

      systemPrint(" - ");

      printNEOInfo();
    }

    //Display the Bluetooth status
    bluetoothTest(false);

#ifdef COMPILE_WIFI
    systemPrint("WiFi MAC Address: ");
    systemPrintf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", wifiMACAddress[0],
                 wifiMACAddress[1], wifiMACAddress[2], wifiMACAddress[3],
                 wifiMACAddress[4], wifiMACAddress[5]);
    if (wifiState == WIFI_CONNECTED)
      wifiDisplayIpAddress();
#endif

#ifdef COMPILE_ETHERNET
    if (HAS_ETHERNET)
    {
      systemPrint("Ethernet MAC Address: ");
      systemPrintf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", ethernetMACAddress[0],
                   ethernetMACAddress[1], ethernetMACAddress[2], ethernetMACAddress[3],
                   ethernetMACAddress[4], ethernetMACAddress[5]);
      systemPrint("Ethernet IP Address: ");
      systemPrintln(Ethernet.localIP());
      if (!settings.ethernetDHCP)
      {
        systemPrint("Ethernet DNS: ");
        systemPrintf("%s\r\n", settings.ethernetDNS.toString());
        systemPrint("Ethernet Gateway: ");
        systemPrintf("%s\r\n", settings.ethernetGateway.toString());
        systemPrint("Ethernet Subnet Mask: ");
        systemPrintf("%s\r\n", settings.ethernetSubnet.toString());
      }
    }
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

    systemPrint("System Uptime: ");
    systemPrintf("%d %02d:%02d:%02d.%03lld (Resets: %d)\r\n",
                 uptimeDays,
                 uptimeHours,
                 uptimeMinutes,
                 uptimeSeconds,
                 uptimeMilliseconds,
                 settings.resetCount);

    //Display NTRIP Client status and uptime
    if (settings.enableNtripClient == true && (systemState >= STATE_ROVER_NOT_STARTED && systemState <= STATE_ROVER_RTK_FIX))
    {
      systemPrint("NTRIP Client ");
      switch (ntripClientState)
      {
        case NTRIP_CLIENT_OFF:
          systemPrint("Disconnected");
          break;
        case NTRIP_CLIENT_ON:
        case NTRIP_CLIENT_WIFI_ETHERNET_STARTED:
        case NTRIP_CLIENT_WIFI_ETHERNET_CONNECTED:
        case NTRIP_CLIENT_CONNECTING:
          systemPrint("Connecting");
          break;
        case NTRIP_CLIENT_CONNECTED:
          systemPrint("Connected");
          break;
        default:
          systemPrintf("Unknown: %d", ntripClientState);
          break;
      }
      systemPrintf(" - %s/%s:%d", settings.ntripClient_CasterHost, settings.ntripClient_MountPoint, settings.ntripClient_CasterPort);

      uptimeMilliseconds = ntripClientTimer - ntripClientStartTime;

      uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
      uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

      uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
      uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

      uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
      uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

      uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
      uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

      systemPrint(" Uptime: ");
      systemPrintf("%d %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
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
      systemPrint("NTRIP Server ");
      switch (ntripServerState)
      {
        case NTRIP_SERVER_OFF:
          systemPrint("Disconnected");
          break;
        case NTRIP_SERVER_ON:
        case NTRIP_SERVER_WIFI_STARTED:
        case NTRIP_SERVER_WIFI_CONNECTED:
        case NTRIP_SERVER_WAIT_GNSS_DATA:
        case NTRIP_SERVER_CONNECTING:
        case NTRIP_SERVER_AUTHORIZATION:
          systemPrint("Connecting");
          break;
        case NTRIP_SERVER_CASTING:
          systemPrint("Connected");
          break;
        default:
          systemPrintf("Unknown: %d", ntripServerState);
          break;
      }
      systemPrintf(" - %s/%s:%d", settings.ntripServer_CasterHost, settings.ntripServer_MountPoint, settings.ntripServer_CasterPort);

      uptimeMilliseconds = ntripServerTimer - ntripServerStartTime;

      uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
      uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

      uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
      uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

      uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
      uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

      uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
      uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

      systemPrint(" Uptime: ");
      systemPrintf("%d %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
                   uptimeDays,
                   uptimeHours,
                   uptimeMinutes,
                   uptimeSeconds,
                   uptimeMilliseconds,
                   ntripServerConnectionAttemptsTotal);
    }

    if (settings.enableSD == true && online.microSD == true)
    {
      systemPrintln("f) Display microSD Files");
    }

    systemPrint("e) Echo User Input: ");
    if (settings.echoUserInput == true)
      systemPrintln("On");
    else
      systemPrintln("Off");

    systemPrintln("d) Configure Debug");

    systemPrintf("z) Set time zone offset: %02d:%02d:%02d\r\n", settings.timeZoneHours, settings.timeZoneMinutes, settings.timeZoneSeconds);

    systemPrint("b) Set Bluetooth Mode: ");
    if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
      systemPrintln("Classic");
    else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
      systemPrintln("BLE");
    else
      systemPrintln("Off");

    systemPrintln("r) Reset all settings to default");

    //Support mode switching
    systemPrintln("B) Switch to Base mode");
    systemPrintln("R) Switch to Rover mode");
    systemPrintln("W) Switch to WiFi Config mode");
    systemPrintln("S) Shut down");

    systemPrintln("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming == 'd')
      menuDebug();
    else if (incoming == 'z')
    {
      systemPrint("Enter time zone hour offset (-23 <= offset <= 23): ");
      int value = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (value < -23 || value > 23)
          systemPrintln("Error: -24 < hours < 24");
        else
        {
          settings.timeZoneHours = value;

          systemPrint("Enter time zone minute offset (-59 <= offset <= 59): ");
          int value = getNumber(); //Returns EXIT, TIMEOUT, or long
          if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
          {
            if (value < -59 || value > 59)
              systemPrintln("Error: -60 < minutes < 60");
            else
            {
              settings.timeZoneMinutes = value;

              systemPrint("Enter time zone second offset (-59 <= offset <= 59): ");
              int value = getNumber(); //Returns EXIT, TIMEOUT, or long
              if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
              {
                if (value < -59 || value > 59)
                  systemPrintln("Error: -60 < seconds < 60");
                else
                {
                  settings.timeZoneSeconds = value;
                  online.rtc = false;
                  syncRTCInterval = 1000; //Reset syncRTCInterval to 1000ms (tpISR could have set it to 59000)
                  rtcSyncd = false;
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
      //Restart Bluetooth
      bluetoothStop();
      if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
        settings.bluetoothRadioType = BLUETOOTH_RADIO_BLE;
      else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
        settings.bluetoothRadioType = BLUETOOTH_RADIO_OFF;
      else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF)
        settings.bluetoothRadioType = BLUETOOTH_RADIO_SPP;
      bluetoothStart();
    }
    else if (incoming == 'r')
    {
      systemPrintln("\r\nResetting to factory defaults. Press 'y' to confirm:");
      byte bContinue = getCharacterNumber();
      if (bContinue == 'y')
      {
        factoryReset();
      }
      else
        systemPrintln("Reset aborted");
    }
    else if ((incoming == 'f') && (settings.enableSD == true) && (online.microSD == true))
    {
      printFileList();
    }
    //Support mode switching
    else if (incoming == 'B')
    {
      forceSystemStateUpdate = true; //Imediately go to this new state
      changeState(STATE_BASE_NOT_STARTED);
    }
    else if (incoming == 'R')
    {
      forceSystemStateUpdate = true; //Imediately go to this new state
      changeState(STATE_ROVER_NOT_STARTED);
    }
    else if (incoming == 'W')
    {
      forceSystemStateUpdate = true; //Imediately go to this new state
      changeState(STATE_WIFI_CONFIG_NOT_STARTED);
    }
    else if (incoming == 'S')
    {
      systemPrintln("Shutting down...");
      forceDisplayUpdate = true;
      powerDown(true);
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Set WiFi credentials
//Enable TCP connections
void menuWiFi()
{
  bool restartWiFi = false; //Restart WiFi if user changes anything

  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: WiFi Networks");

    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
      systemPrintf("%d) SSID %d: %s\r\n", (x * 2) + 1, x + 1, settings.wifiNetworks[x].ssid);
      systemPrintf("%d) Password %d: %s\r\n", (x * 2) + 2, x + 1, settings.wifiNetworks[x].password);
    }

    systemPrint("a) Configure device via WiFi Access Point or connect to WiFi: ");
    systemPrintf("%s\r\n", settings.wifiConfigOverAP ? "AP" : "WiFi");

    systemPrint("c) WiFi TCP Client (connect to phone): ");
    systemPrintf("%s\r\n", settings.enableTcpClient ? "Enabled" : "Disabled");

    systemPrint("s) WiFi TCP Server: ");
    systemPrintf("%s\r\n", settings.enableTcpServer ? "Enabled" : "Disabled");

    if (settings.enableTcpServer == true || settings.enableTcpClient == true)
      systemPrintf("p) WiFi TCP Port: %ld\r\n", settings.wifiTcpPort);

    systemPrintln("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming >= 1 && incoming <= MAX_WIFI_NETWORKS * 2)
    {
      int arraySlot = ((incoming - 1) / 2); //Adjust incoming to array starting at 0

      if (incoming % 2 == 1)
      {
        systemPrintf("Enter SSID network %d: ", arraySlot + 1);
        getString(settings.wifiNetworks[arraySlot].ssid, sizeof(settings.wifiNetworks[arraySlot].ssid));
        restartWiFi = true; //If we are modifying the SSID table, force restart of WiFi
      }
      else
      {
        systemPrintf("Enter Password for %s: ", settings.wifiNetworks[arraySlot].ssid);
        getString(settings.wifiNetworks[arraySlot].password, sizeof(settings.wifiNetworks[arraySlot].password));
        restartWiFi = true; //If we are modifying the SSID table, force restart of WiFi
      }
    }
    else if (incoming == 'a')
    {
      settings.wifiConfigOverAP ^= 1;
      restartWiFi = true;
    }

    else if (incoming == 'c')
    {
      //Toggle WiFi NEMA client (connect to phone)
      settings.enableTcpClient ^= 1;
      restartWiFi = true;
    }
    else if (incoming == 's')
    {
      //Toggle WiFi NEMA server
      settings.enableTcpServer ^= 1;
      if ((!settings.enableTcpServer) && online.tcpServer)
      {
        //Tell the UART2 tasks that the TCP server is shutting down
        online.tcpServer = false;

        //Wait for the UART2 tasks to close the TCP client connections
        while (wifiTcpServerActive())
          delay(5);
        systemPrintln("TCP Server offline");
      }
      restartWiFi = true;
    }
    else if (incoming == 'p')
    {
      systemPrint("Enter the TCP port to use (0 to 65535): ");
      int wifiTcpPort = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((wifiTcpPort != INPUT_RESPONSE_GETNUMBER_EXIT) && (wifiTcpPort != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (wifiTcpPort < 0 || wifiTcpPort > 65535)
          systemPrintln("Error: TCP Port out of range");
        else
        {
          settings.wifiTcpPort = wifiTcpPort; //Recorded to NVM and file at main menu exit
          restartWiFi = true;
        }
      }
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  //Erase passwords from empty SSID entries
  for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
  {
    if (strlen(settings.wifiNetworks[x].ssid) == 0)
      strcpy(settings.wifiNetworks[x].password, "");
  }

  //Restart WiFi if anything changes
  if (restartWiFi == true)
  {
    //Restart the AP webserver if we are in that state
    if (systemState == STATE_WIFI_CONFIG)
      requestChangeState(STATE_WIFI_CONFIG_NOT_STARTED);
    else
    {
      //Restart WiFi if we are not in AP config mode
      if (wifiIsConnected())
      {
        log_d("Menu caused restarting of WiFi");
        wifiStop();
        wifiStart();
        wifiConnectionAttempts = 0; //Reset the timeout
      }
    }
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: Debug");

    systemPrintf("Filtered by parser: %d NMEA / %d RTCM / %d UBX\r\n",
                 failedParserMessages_NMEA,
                 failedParserMessages_RTCM,
                 failedParserMessages_UBX);

    systemPrint("1) u-blox I2C Debugging Output: ");
    if (settings.enableI2Cdebug == true)
      systemPrintln("Enabled");
    else
      systemPrintln("Disabled");

    systemPrint("2) Heap Reporting: ");
    if (settings.enableHeapReport == true)
      systemPrintln("Enabled");
    else
      systemPrintln("Disabled");

    systemPrint("3) Task Highwater Reporting: ");
    if (settings.enableTaskReports == true)
      systemPrintln("Enabled");
    else
      systemPrintln("Disabled");

    systemPrint("4) Set SPI/SD Interface Frequency: ");
    systemPrint(settings.spiFrequency);
    systemPrintln(" MHz");

    systemPrint("5) Set SPP RX Buffer Size: ");
    systemPrintln(settings.sppRxQueueSize);

    systemPrint("6) Set SPP TX Buffer Size: ");
    systemPrintln(settings.sppTxQueueSize);

    systemPrintf("8) Display Reset Counter: %d - ", settings.resetCount);
    if (settings.enableResetDisplay == true)
      systemPrintln("Enabled");
    else
      systemPrintln("Disabled");

    systemPrint("9) GNSS Serial Timeout: ");
    systemPrintln(settings.serialTimeoutGNSS);

    systemPrint("10) Periodically print WiFi IP Address: ");
    systemPrintf("%s\r\n", settings.enablePrintWifiIpAddress ? "Enabled" : "Disabled");

    systemPrint("11) Periodically print state: ");
    systemPrintf("%s\r\n", settings.enablePrintState ? "Enabled" : "Disabled");

    systemPrint("12) Periodically print WiFi state: ");
    systemPrintf("%s\r\n", settings.enablePrintWifiState ? "Enabled" : "Disabled");

    systemPrint("13) Periodically print NTRIP client state: ");
    systemPrintf("%s\r\n", settings.enablePrintNtripClientState ? "Enabled" : "Disabled");

    systemPrint("14) Periodically print NTRIP server state: ");
    systemPrintf("%s\r\n", settings.enablePrintNtripServerState ? "Enabled" : "Disabled");

    systemPrint("15) Periodically print position: ");
    systemPrintf("%s\r\n", settings.enablePrintPosition ? "Enabled" : "Disabled");

    systemPrint("16) Periodically print CPU idle time: ");
    systemPrintf("%s\r\n", settings.enablePrintIdleTime ? "Enabled" : "Disabled");

    systemPrintln("17) Mirror ZED-F9x's UART1 settings to USB");

    systemPrint("18) Print battery status messages: ");
    systemPrintf("%s\r\n", settings.enablePrintBatteryMessages ? "Enabled" : "Disabled");

    systemPrint("19) Print Rover accuracy messages: ");
    systemPrintf("%s\r\n", settings.enablePrintRoverAccuracy ? "Enabled" : "Disabled");

    systemPrint("20) Print messages with bad checksums or CRCs: ");
    systemPrintf("%s\r\n", settings.enablePrintBadMessages ? "Enabled" : "Disabled");

    systemPrint("21) Print log file messages: ");
    systemPrintf("%s\r\n", settings.enablePrintLogFileMessages ? "Enabled" : "Disabled");

    systemPrint("22) Print log file status: ");
    systemPrintf("%s\r\n", settings.enablePrintLogFileStatus ? "Enabled" : "Disabled");

    systemPrint("23) Print ring buffer offsets: ");
    systemPrintf("%s\r\n", settings.enablePrintRingBufferOffsets ? "Enabled" : "Disabled");

    systemPrint("24) Print GNSS --> NTRIP caster messages: ");
    systemPrintf("%s\r\n", settings.enablePrintNtripServerRtcm ? "Enabled" : "Disabled");

    systemPrint("25) Print NTRIP caster --> GNSS messages: ");
    systemPrintf("%s\r\n", settings.enablePrintNtripClientRtcm ? "Enabled" : "Disabled");

    systemPrint("26) Print states: ");
    systemPrintf("%s\r\n", settings.enablePrintStates ? "Enabled" : "Disabled");

    systemPrint("27) Print duplicate states: ");
    systemPrintf("%s\r\n", settings.enablePrintDuplicateStates ? "Enabled" : "Disabled");

    systemPrint("28) RTCM message checking: ");
    systemPrintf("%s\r\n", settings.enableRtcmMessageChecking ? "Enabled" : "Disabled");

    systemPrint("29) Run Logging Test: ");
    systemPrintf("%s\r\n", settings.runLogTest ? "Enabled" : "Disabled");

    systemPrintln("30) Run Bluetooth Test");

    systemPrint("31) Print TCP status: ");
    systemPrintf("%s\r\n", settings.enablePrintTcpStatus ? "Enabled" : "Disabled");

    systemPrint("32) ESP-Now Broadcast Override: ");
    systemPrintf("%s\r\n", settings.espnowBroadcast ? "Enabled" : "Disabled");

    systemPrint("33) Print buffer overruns: ");
    systemPrintf("%s\r\n", settings.enablePrintBufferOverrun ? "Enabled" : "Disabled");

    systemPrint("34) Set UART Receive Buffer Size: ");
    systemPrintln(settings.uartReceiveBufferSize);

    systemPrint("35) Set GNSS Handler Buffer Size: ");
    systemPrintln(settings.gnssHandlerBufferSize);

    systemPrint("36) Print SD and UART buffer sizes: ");
    systemPrintf("%s\r\n", settings.enablePrintSDBuffers ? "Enabled" : "Disabled");

    systemPrint("37) Print RTC resyncs: ");
    systemPrintf("%s\r\n", settings.enablePrintRtcSync ? "Enabled" : "Disabled");

    systemPrint("38) Print NTP Request diagnostics: ");
    systemPrintf("%s\r\n", settings.enablePrintNTPDiag ? "Enabled" : "Disabled");

    systemPrint("39) Print Ethernet diagnostics: ");
    systemPrintf("%s\r\n", settings.enablePrintEthernetDiag ? "Enabled" : "Disabled");

    systemPrint("40) Set L-Band RTK Fix Timeout (seconds): ");
    systemPrintln(settings.lbandFixTimeout_seconds);

    systemPrintln("t) Enter Test Screen");

    systemPrintln("e) Erase LittleFS");

    systemPrintln("r) Force system reset");

    systemPrintln("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming == 1)
    {
      settings.enableI2Cdebug ^= 1;

      if (settings.enableI2Cdebug)
      {
#if defined(REF_STN_GNSS_DEBUG)
        if (ENABLE_DEVELOPER && productVariant == REFERENCE_STATION)
          theGNSS.enableDebugging(serialGNSS); //Output all debug messages over serialGNSS
        else
#endif
          theGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
      }
      else
        theGNSS.disableDebugging();
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
      systemPrint("Enter SPI frequency in MHz (1 to 16): ");
      int freq = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((freq != INPUT_RESPONSE_GETNUMBER_EXIT) && (freq != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (freq < 1 || freq > 16) //Arbitrary 16MHz limit
          systemPrintln("Error: SPI frequency out of range");
        else
          settings.spiFrequency = freq; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 5)
    {
      systemPrint("Enter SPP RX Queue Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          systemPrintln("Error: Queue size out of range");
        else
          settings.sppRxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 6)
    {
      systemPrint("Enter SPP TX Queue Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          systemPrintln("Error: Queue size out of range");
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
      systemPrint("Enter GNSS Serial Timeout in milliseconds (0 to 1000): ");
      int serialTimeoutGNSS = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_EXIT) && (serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (serialTimeoutGNSS < 0 || serialTimeoutGNSS > 1000) //Arbitrary 1s limit
          systemPrintln("Error: Timeout is out of range");
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
      bool response = setMessagesUSB(MAX_SET_MESSAGES_RETRIES);

      if (response == false)
        systemPrintln(F("Failed to enable USB messages"));
      else
        systemPrintln(F("USB messages successfully enabled"));
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
      settings.enablePrintTcpStatus ^= 1;
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
      systemPrint("Enter UART Receive Buffer Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          systemPrintln("Error: Queue size out of range");
        else
          settings.uartReceiveBufferSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 35)
    {
      systemPrint("Enter GNSS Handler Buffer Size in Bytes (32 to 16384): ");
      int queSize = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
          systemPrintln("Error: Queue size out of range");
        else
          settings.gnssHandlerBufferSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 36)
    {
      settings.enablePrintSDBuffers ^= 1;
    }
    else if (incoming == 37)
    {
      settings.enablePrintRtcSync ^= 1;
    }
    else if (incoming == 38)
    {
      settings.enablePrintNTPDiag ^= 1;
    }
    else if (incoming == 39)
    {
      settings.enablePrintEthernetDiag ^= 1;
    }
    else if (incoming == 40)
    {
      systemPrint("Enter number of seconds in RTK float before hot-start (30 to 1200): ");
      int timeout = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((timeout != INPUT_RESPONSE_GETNUMBER_EXIT) && (timeout != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (timeout < 30 || timeout > 1200) //Arbitrary 20 minute limit
          systemPrintln("Error: Timeout out of range");
        else
          settings.lbandFixTimeout_seconds = timeout; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 'e')
    {
      systemPrintln("Erasing LittleFS and resetting");
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
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
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
    systemPrint("SIV: ");
    systemPrint(numSV);

    systemPrint(", HPA (m): ");
    systemPrint(horizontalAccuracy, 3);

    systemPrint(", Lat: ");
    systemPrint(latitude, haeNumberOfDecimals);
    systemPrint(", Lon: ");
    systemPrint(longitude, haeNumberOfDecimals);

    systemPrint(", Altitude (m): ");
    systemPrint(altitude, 1);

    systemPrintln();
  }
}

void printCurrentConditionsNMEA()
{
  if (online.gnss == true)
  {
    char systemStatus[100];
    snprintf(systemStatus, sizeof(systemStatus), "%02d%02d%02d.%02d,%02d%02d%02d,%0.3f,%d,%0.9f,%0.9f,%0.2f,%d,%d,%d",
             gnssHour, gnssMinute, gnssSecond, mseconds,
             gnssDay, gnssMonth, gnssYear % 2000, //Limit to 2 digits
             horizontalAccuracy, numSV,
             latitude, longitude,
             altitude,
             fixType, carrSoln,
             battLevel);

    char nmeaMessage[100];                                                                       //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, sizeof(nmeaMessage), systemStatus); //textID, buffer, sizeOfBuffer, text
    systemPrintln(nmeaMessage);
  }
  else
  {
    char nmeaMessage[100];                                                                            //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, sizeof(nmeaMessage), (char *)"OFFLINE"); //textID, buffer, sizeOfBuffer, text
    systemPrintln(nmeaMessage);
  }
}

//When called, prints the contents of root folder list of files on SD card
//This allows us to replace the sd.ls() function to point at Serial and BT outputs
void printFileList()
{
  bool sdCardAlreadyMounted = online.microSD;
  if (!online.microSD)
    beginSD();

  //Notify the user if the microSD card is not available
  if (!online.microSD)
    systemPrintln("microSD card not online!");
  else
  {
    //Attempt to gain access to the SD card
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      markSemaphore(FUNCTION_PRINT_FILE_LIST);

      if (USE_SPI_MICROSD)
      {
        SdFile dir;
        dir.open("/"); //Open root
        uint16_t fileCount = 0;

        SdFile tempFile;

        systemPrintln("Files found:");

        while (tempFile.openNext(&dir, O_READ))
        {
          if (tempFile.isFile())
          {
            fileCount++;

            //2017-05-19 187362648 800_0291.MOV

            //Get File Date from sdFat
            uint16_t fileDate;
            uint16_t fileTime;
            tempFile.getCreateDateTime(&fileDate, &fileTime);

            //Convert sdFat file date fromat into YYYY-MM-DD
            char fileDateChar[20];
            snprintf(fileDateChar, sizeof(fileDateChar), "%d-%02d-%02d",
                     ((fileDate >> 9) + 1980),   //Year
                     ((fileDate >> 5) & 0b1111), //Month
                     (fileDate & 0b11111)        //Day
                    );

            char fileSizeChar[20];
            stringHumanReadableSize(tempFile.fileSize()).toCharArray(fileSizeChar, sizeof(fileSizeChar));

            char fileName[50]; //Handle long file names
            tempFile.getName(fileName, sizeof(fileName));

            char fileRecord[100];
            snprintf(fileRecord, sizeof(fileRecord), "%s\t%s\t%s", fileDateChar, fileSizeChar, fileName);

            systemPrintln(fileRecord);
          }
        }

        dir.close();
        tempFile.close();

        if (fileCount == 0)
          systemPrintln("No files found");
      }
#ifdef COMPILE_SD_MMC
      else
      {
        File dir = SD_MMC.open("/"); //Open root
        uint16_t fileCount = 0;

        if (dir && dir.isDirectory())
        {
          systemPrintln("Files found:");

          File tempFile = dir.openNextFile();
          while (tempFile)
          {
            if (!tempFile.isDirectory())
            {
              fileCount++;

              //2017-05-19 187362648 800_0291.MOV

              //Get time of last write
              time_t lastWrite = tempFile.getLastWrite();

              struct tm *timeinfo = localtime(&lastWrite);

              char fileDateChar[20];
              snprintf(fileDateChar, 20, "%.0f-%02.0f-%02.0f",
                       (float)timeinfo->tm_year + 1900, //Year - ESP32 2.0.2 starts the year at 1900...
                       (float)timeinfo->tm_mon + 1,     //Month
                       (float)timeinfo->tm_mday         //Day
                      );

              char fileSizeChar[20];
              stringHumanReadableSize(tempFile.size()).toCharArray(fileSizeChar, sizeof(fileSizeChar));

              char fileName[50]; //Handle long file names
              snprintf(fileName, sizeof(fileName), "%s", tempFile.name());

              char fileRecord[100];
              snprintf(fileRecord, sizeof(fileRecord), "%s\t%s\t%s", fileDateChar, fileSizeChar, fileName);

              systemPrintln(fileRecord);
            }

            tempFile.close();
            tempFile = dir.openNextFile();
          }
        }

        dir.close();

        if (fileCount == 0)
          systemPrintln("No files found");
      }
#endif
    }
    else
    {
      char semaphoreHolder[50];
      getSemaphoreFunction(semaphoreHolder);

      //This is an error because the current settings no longer match the settings
      //on the microSD card, and will not be restored to the expected settings!
      systemPrintf("sdCardSemaphore failed to yield, held by %s, menuSystem.ino line %d\r\n", semaphoreHolder, __LINE__);
    }

    //Release the SD card if not originally mounted
    if (sdCardAlreadyMounted)
      xSemaphoreGive(sdCardSemaphore);
    else
      endSD(true, true);
  }
}
