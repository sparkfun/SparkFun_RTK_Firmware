//Ethernet

//TCP server
#define ETHERNET_MAX_TCP_CLIENTS 4
#ifdef COMPILE_ETHERNET
static EthernetServer *ethernetTcpServer = nullptr;
static EthernetClient ethernetTcpClient[ETHERNET_MAX_TCP_CLIENTS];
#endif

//Determine if Ethernet is needed. Saves RAM...
bool ethernetIsNeeded()
{
  //Does NTP need Ethernet?
  if (HAS_ETHERNET && systemState >= STATE_NTPSERVER_NOT_STARTED && systemState <= STATE_NTPSERVER_SYNC)
    return true;

  //Does Base mode NTRIP Server need Ethernet?
  if (HAS_ETHERNET
      && settings.enableNtripServer == true
      && (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
      && !settings.ntripServerUseWiFiNotEthernet)
     return true;

  //Does Base mode NTRIP Server need Ethernet?
  if (HAS_ETHERNET
      && settings.enableNtripClient == true
      && (systemState >= STATE_ROVER_NOT_STARTED && systemState <= STATE_ROVER_RTK_FIX)
      && !settings.ntripClientUseWiFiNotEthernet)
     return true;

  //Does TCP need Ethernet?
  if (HAS_ETHERNET
      && (settings.enableTcpServerEthernet || settings.enableTcpClientEthernet))
     return true;

   return false;
}

//Regularly called to update the Ethernet status
void beginEthernet()
{
  if (HAS_ETHERNET == false)
    return;

  // Skip if going into configure-via-ethernet mode
  if (configureViaEthernet)
  {
    log_d("configureViaEthernet: skipping beginEthernet");
    return;
  }

#ifdef COMPILE_ETHERNET

  if (!ethernetIsNeeded())
    return;

  switch (online.ethernetStatus)
  {
    case (ETH_NOT_STARTED):
      Ethernet.init(pin_Ethernet_CS);

      //First we start Ethernet without DHCP to detect if a cable is connected
      //DHCP causes system freeze for ~62 seconds so we avoid it until a cable is conncted
      Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS, settings.ethernetGateway, settings.ethernetSubnet);

      if (Ethernet.hardwareStatus() == EthernetNoHardware)
      {
        log_d("Ethernet hardware not found");
        online.ethernetStatus = ETH_CAN_NOT_BEGIN;
        return;
      }

      online.ethernetStatus = ETH_STARTED_CHECK_CABLE;
      break;

    case (ETH_STARTED_CHECK_CABLE):
      if (millis() - lastEthernetCheck > 1000) //Don't check for cable but once a second
      {
        lastEthernetCheck = millis();

        if (Ethernet.linkStatus() == LinkON)
        {
          log_d("Ethernet cable detected");

          if (settings.ethernetDHCP)
          {
            paintGettingEthernetIP();
            online.ethernetStatus = ETH_STARTED_START_DHCP;
          }
          else
          {
            systemPrintln("Ethernet started with static IP");
            online.ethernetStatus = ETH_CONNECTED;
          }
        }
        else
        {
          //log_d("No cable detected");
        }
      }
      break;

    case (ETH_STARTED_START_DHCP):
      Ethernet.begin(ethernetMACAddress);

      if (Ethernet.hardwareStatus() == EthernetNoHardware)
      {
        log_d("Ethernet hardware not found");
        online.ethernetStatus = ETH_CAN_NOT_BEGIN;
        return;
      }

      log_d("Ethernet started with DHCP");
      online.ethernetStatus = ETH_CONNECTED;
      break;

    case (ETH_CONNECTED):
      if (Ethernet.linkStatus() == LinkOFF)
      {
        log_d("Ethernet cable disconnected!");
        online.ethernetStatus = ETH_STARTED_CHECK_CABLE;
      }
      break;

    case (ETH_CAN_NOT_BEGIN):
      break;

    default:
      log_d("Unknown status");
      break;
  }
#endif ///COMPILE_ETHERNET
}

void beginEthernetNTPServer()
{
  if (!HAS_ETHERNET)
    return;

  //Skip if going into configure-via-ethernet mode
  if (configureViaEthernet)
  {
    log_d("configureViaEthernet: skipping beginNTPServer");
    return;
  }

  //Skip if not in NTPSERVER mode
  if (systemState < STATE_NTPSERVER_NOT_STARTED || systemState > STATE_NTPSERVER_SYNC)
    return;

#ifdef COMPILE_ETHERNET
  if ((online.ethernetStatus == ETH_CONNECTED) && (online.ethernetNTPServer == false))
  {
    ethernetNTPServer = new derivedEthernetUDP;
    ethernetNTPServer->begin(settings.ethernetNtpPort);
    ntpSockIndex = ethernetNTPServer->getSockIndex(); //Get the socket index
    w5500ClearSocketInterrupts(); // Clear all interrupts
    w5500EnableSocketInterrupt(ntpSockIndex); // Enable the RECV interrupt for the desired socket index
    online.ethernetNTPServer = true;
  }
#endif
}

void updateEthernet()
{
  //Skip if in configure-via-ethernet mode
  if (configureViaEthernet)
  {
    //log_d("configureViaEthernet: skipping updateEthernet");
    return;
  }

  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET

  if (online.ethernetStatus == ETH_CAN_NOT_BEGIN)
    return;

  beginEthernet(); //This updates the link status

  if (w5500CheckSocketInterrupt(ntpSockIndex))
    w5500ClearSocketInterrupt(ntpSockIndex); //Clear the socket interrupt here

  //Maintain the ethernet connection
  switch (Ethernet.maintain()) {
    case 1:
      //renewed fail
      if (settings.enablePrintEthernetDiag && (!inMainMenu)) systemPrintln("Ethernet: Error: renewed fail");
      break;

    case 2:
      //renewed success
      if (settings.enablePrintEthernetDiag && (!inMainMenu))
      {
        systemPrint("Ethernet: Renewed success. IP address: ");
        systemPrintln(Ethernet.localIP());
      }
      break;

    case 3:
      //rebind fail
      if (settings.enablePrintEthernetDiag && (!inMainMenu)) systemPrintln("Ethernet: Error: rebind fail");
      break;

    case 4:
      //rebind success
      if (settings.enablePrintEthernetDiag && (!inMainMenu))
      {
        systemPrint("Ethernet: Rebind success. IP address: ");
        systemPrintln(Ethernet.localIP());
      }
      break;

    default:
      //nothing happened
      break;
  }
#endif
}

void updateEthernetNTPServer()
{
  // Skip if in configure-via-ethernet mode
  if (configureViaEthernet)
  {
    //log_d("configureViaEthernet: skipping updateEthernetNTPServer");
    return;
  }

  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET

  if (online.ethernetNTPServer == false)
    beginEthernetNTPServer();

  if (online.ethernetNTPServer == false)
    return;

  char ntpDiag[512]; //Char array to hold diagnostic messages

  //Check for new NTP requests - if the time has been sync'd
  bool processed = processOneNTPRequest(systemState == STATE_NTPSERVER_SYNC, (const timeval *)&ethernetNtpTv, (const timeval *)&gnssSyncTv, ntpDiag, sizeof(ntpDiag));

  if (processed)
  {
    //Print the diagnostics - if enabled
    if (settings.enablePrintNTPDiag && (!inMainMenu))
      systemPrint(ntpDiag);

    //Log the NTP request to file - if enabled
    if (settings.enableNTPFile)
    {
      //Gain access to the SPI controller for the microSD card
      if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
      {
        markSemaphore(FUNCTION_NTPEVENT);

        //Get the marks file name
        char fileName[32];
        bool fileOpen = false;
        bool sdCardWasOnline;
        int year;
        int month;
        int day;

        //Get the date
        year = rtc.getYear();
        month = rtc.getMonth() + 1;
        day = rtc.getDay();

        //Build the file name
        snprintf(fileName, sizeof(fileName), "/NTP_Requests_%04d_%02d_%02d.txt", year, month, day);

        //Try to gain access the SD card
        sdCardWasOnline = online.microSD;
        if (online.microSD != true)
          beginSD();

        if (online.microSD == true)
        {
          //Check if the NTP file already exists
          bool ntpFileExists = false;
          if (USE_SPI_MICROSD)
          {
            ntpFileExists = sd->exists(fileName);
          }
#ifdef COMPILE_SD_MMC
          else
          {
            ntpFileExists = SD_MMC.exists(fileName);
          }
#endif

          //Open the NTP file
          FileSdFatMMC ntpFile;

          if (ntpFileExists)
          {
            if (ntpFile && ntpFile.open(fileName, O_APPEND | O_WRITE))
            {
              fileOpen = true;
              ntpFile.updateFileCreateTimestamp();
            }
          }
          else
          {
            if (ntpFile && ntpFile.open(fileName, O_CREAT | O_WRITE))
            {
              fileOpen = true;
              ntpFile.updateFileAccessTimestamp();

              //If you want to add a file header, do it here
            }
          }

          if (fileOpen)
          {
            //Write the NTP request to the file
            ntpFile.write((const uint8_t *)ntpDiag, strlen(ntpDiag));

            //Update the file to create time & date
            ntpFile.updateFileCreateTimestamp();

            //Close the mark file
            ntpFile.close();
          }

          //Dismount the SD card
          if (!sdCardWasOnline)
            endSD(true, false);
        }

        //Done with the SPI controller
        xSemaphoreGive(sdCardSemaphore);

        lastLoggedNTPRequest = millis();
        ntpLogIncreasing = true;
      } //End sdCardSemaphore
    }

  }

  if (millis() > (lastLoggedNTPRequest + 5000))
    ntpLogIncreasing = false;

#endif
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Send data to the Ethernet TCP clients
void ethernetSendTcpData(uint8_t * data, uint16_t length)
{
#ifdef COMPILE_ETHERNET
  static IPAddress ipAddressEthernet[ETHERNET_MAX_TCP_CLIENTS];
  int index = 0;
  static uint32_t lastTcpConnectAttempt;

  if (online.tcpClientEthernet)
  {
    //Start the TCP client if enabled
    if (((!ethernetTcpClient[0]) || (!ethernetTcpClient[0].connected()))
        && ((millis() - lastTcpConnectAttempt) >= 1000))
    {
      lastTcpConnectAttempt = millis();

      //Remove any http:// or https:// prefix from host name
      char hostname[51];
      strncpy(hostname, settings.hostForTCPClient, sizeof(hostname) - 1); //strtok modifies string to be parsed so we create a copy
      char *token = strtok(hostname, "//");
      if (token != nullptr)
      {
        token = strtok(nullptr, "//"); //Advance to data after //
        if (token != nullptr)
          strcpy(settings.hostForTCPClient, token);
      }
          
      if (settings.enablePrintTcpStatus)
      {
        systemPrint("Trying to connect Ethernet TCP client to ");
        systemPrintln(settings.hostForTCPClient);
      }
      if (ethernetTcpClient[0].connect(settings.hostForTCPClient, settings.ethernetTcpPort))
      {
        online.tcpClientEthernet = true;
        systemPrint("Ethernet TCP client connected to ");
        systemPrintln(settings.hostForTCPClient);
        ethernetTcpConnected |= 1 << index;
      }
      else
      {
        //Release any allocated resources
        //if (ethernetTcpClient[0])
        ethernetTcpClient[0].stop();
      }
    }
  }

  if (online.tcpServerEthernet)
  {
    //Check for another client
    for (index = 0; index < ETHERNET_MAX_TCP_CLIENTS; index++)
      if (!(ethernetTcpConnected & (1 << index)))
      {
        if ((!ethernetTcpClient[index]) || (!ethernetTcpClient[index].connected()))
        {
          ethernetTcpClient[index] = ethernetTcpServer->available();
          if (!ethernetTcpClient[index])
            break;
          ipAddressEthernet[index] = ethernetTcpClient[index].remoteIP();
          systemPrintf("Connected Ethernet TCP client %d to ", index);
          systemPrintln(ipAddressEthernet[index]);
          ethernetTcpConnected |= 1 << index;
        }
      }
  }

  //Walk the list of TCP clients
  for (index = 0; index < ETHERNET_MAX_TCP_CLIENTS; index++)
  {
    if (ethernetTcpConnected & (1 << index))
    {
      //Check for a broken connection
      if ((!ethernetTcpClient[index]) || (!ethernetTcpClient[index].connected()))
        systemPrintf("Disconnected TCP client %d from ", index);

      //Send the data to the connected clients
      else if (((settings.enableTcpServerEthernet && online.tcpServerEthernet)
                || (settings.enableTcpClientEthernet && online.tcpClientEthernet))
               && ((!length) || (ethernetTcpClient[index].write(data, length) == length)))
      {
        if (settings.enablePrintTcpStatus && length)
          systemPrintf("%d bytes written over Ethernet TCP\r\n", length);
        continue;
      }

      //Failed to write the data
      else
        systemPrintf("Breaking Ethernet TCP client %d connection to ", index);

      //Done with this client connection
      systemPrintln(ipAddressEthernet[index]);
      ethernetTcpClient[index].stop();
      ethernetTcpConnected &= ~(1 << index);

      //Shutdown the TCP server if necessary
      if (settings.enableTcpServerEthernet || online.tcpServerEthernet)
        ethernetTcpServerActive();
    }
  }
#endif  //COMPILE_ETHERNET
}

//Check for TCP server active
bool ethernetTcpServerActive()
{
#ifdef COMPILE_ETHERNET
  if ((settings.enableTcpServerEthernet && online.tcpServerEthernet) || ethernetTcpConnected)
    return true;

  log_d("Stopping Ethernet TCP Server");

  //Shutdown the TCP server
  online.tcpServerEthernet = false;

  //Stop the TCP server
  //ethernetTcpServer->stop();

  if (ethernetTcpServer != nullptr)
    delete ethernetTcpServer;
#endif  //COMPILE_ETHERNET
  return false;
}

void tcpUpdateEthernet()
{
  // Skip if in configure-via-ethernet mode
  if (configureViaEthernet)
  {
    //log_d("configureViaEthernet: skipping tcpUpdateEthernet");
    return;
  }

  if (!HAS_ETHERNET)
    return;
    
#ifdef COMPILE_ETHERNET

  if (settings.enableTcpClientEthernet == false && settings.enableTcpServerEthernet == false) return; //Nothing to do

  if (settings.enableTcpClientEthernet == true || settings.enableTcpServerEthernet == true)
  {
    //Verify ETHERNET_MAX_TCP_CLIENTS
    if ((sizeof(ethernetTcpConnected) * 8) < ETHERNET_MAX_TCP_CLIENTS)
    {
      systemPrintf("Please set ETHERNET_MAX_TCP_CLIENTS <= %d or increase size of ethernetTcpConnected\r\n", sizeof(ethernetTcpConnected) * 8);
      while (true) ; //Freeze
    }
  }

  //Start the TCP client if enabled
  if (settings.enableTcpClientEthernet
      && (!online.tcpClientEthernet)
      && (!settings.enableTcpServerEthernet)
      && (online.ethernetStatus >= ETH_STARTED_CHECK_CABLE) && (online.ethernetStatus <= ETH_CONNECTED) //TODO: Maybe just online.ethernetStatus == ETH_CONNECTED ?
     )
  {
    online.tcpClientEthernet = true;
    systemPrint("Ethernet TCP client online, local IP ");
    systemPrint(Ethernet.localIP());
    systemPrint(", Host ");
    systemPrintln(settings.hostForTCPClient);
  }

  //Start the TCP server if enabled
  if (settings.enableTcpServerEthernet
      && (ethernetTcpServer == nullptr)
      && (!settings.enableTcpClientEthernet)
      && (online.ethernetStatus >= ETH_STARTED_CHECK_CABLE) && (online.ethernetStatus <= ETH_CONNECTED) //TODO: Maybe just online.ethernetStatus == ETH_CONNECTED ?
     )
  {
    ethernetTcpServer = new EthernetServer((uint16_t)settings.ethernetTcpPort);

    ethernetTcpServer->begin();
    online.tcpServerEthernet = true;
    systemPrint("Ethernet TCP Server online, IP Address ");
    systemPrintln(Ethernet.localIP());
  }
#endif
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//Start Ethernet WebServer ESP32 W5500 - needs exclusive access to WiFi, SPI and Interrupts
void startEthernerWebServerESP32W5500()
{
#ifdef COMPILE_ETHERNET
  //Configure the W5500
  //To be called before ETH.begin()
  ESP32_W5500_onEvent();

  //start the ethernet connection and the server:
  //Use DHCP dynamic IP
  //bool begin(int POCI_GPIO, int PICO_GPIO, int SCLK_GPIO, int CS_GPIO, int INT_GPIO, int SPI_CLOCK_MHZ,
  //           int SPI_HOST, uint8_t *W5500_Mac = W5500_Default_Mac, bool installIsrService = true);
  ETH.begin( pin_POCI, pin_PICO, pin_SCK, pin_Ethernet_CS, pin_Ethernet_Interrupt, 25, SPI3_HOST, ethernetMACAddress );

  if (!settings.ethernetDHCP)
    ETH.config( settings.ethernetIP, settings.ethernetGateway, settings.ethernetSubnet, settings.ethernetDNS );

  if (ETH.linkUp())
    ESP32_W5500_waitForConnect();
#endif
}

void endEthernerWebServerESP32W5500()
{
#ifdef COMPILE_ETHERNET
  ETH.end(); //This is _really_ important. It undoes the low-level changes to SPI and interrupts
#endif
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Ethernet (W5500) ISR
// Triggered by the falling edge of the W5500 interrupt signal - indicates the arrival of a packet
// Record the time the packet arrived

#ifdef COMPILE_ETHERNET
void ethernetISR()
{
  //Don't check or clear the interrupt here -
  //it may clash with a GNSS SPI transaction and cause a wdt timeout.
  //Do it in updateEthernet
  gettimeofday((timeval *)&ethernetNtpTv, NULL); //Record the time of the NTP interrupt
}
#endif

void menuEthernet()
{
#ifdef COMPILE_ETHERNET
  if (!HAS_ETHERNET)
  {
    clearBuffer(); //Empty buffer of any newline chars
    return;
  }

  bool restartEthernet = false;

  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: Ethernet");
    systemPrintln();

    systemPrint("1) Ethernet Config: ");
    if (settings.ethernetDHCP)
      systemPrintln("DHCP");
    else
      systemPrintln("Fixed IP");

    if (!settings.ethernetDHCP)
    {
      systemPrint("2) Fixed IP Address: ");
      systemPrintln(settings.ethernetIP.toString().c_str());
      systemPrint("3) DNS: ");
      systemPrintln(settings.ethernetDNS.toString().c_str());
      systemPrint("4) Gateway: ");
      systemPrintln(settings.ethernetGateway.toString().c_str());
      systemPrint("5) Subnet Mask: ");
      systemPrintln(settings.ethernetSubnet.toString().c_str());
    }

    systemPrint("c) Ethernet TCP Client: ");
    systemPrintf("%s\r\n", settings.enableTcpClientEthernet ? "Enabled" : "Disabled");

    systemPrint("s) Ethernet TCP Server: ");
    systemPrintf("%s\r\n", settings.enableTcpServerEthernet ? "Enabled" : "Disabled");

    if (settings.enableTcpServerEthernet == true || settings.enableTcpClientEthernet == true)
      systemPrintf("p) Ethernet TCP Port: %ld\r\n", settings.ethernetTcpPort);

    if (settings.enableTcpClientEthernet == true)
    {
     systemPrint("h) Host for Ethernet TCP: ");
     systemPrintln(settings.hostForTCPClient);
    }

    systemPrintln("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming == 1)
    {
      settings.ethernetDHCP ^= 1;
      restartEthernet = true;
    }
    else if ((!settings.ethernetDHCP) && (incoming == 2))
    {
      systemPrint("Enter new IP Address: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        String tempString = String(tempStr);
        settings.ethernetIP.fromString(tempString);
        restartEthernet = true;
      }
      else
        systemPrint("Error: invalid IP Address");
    }
    else if ((!settings.ethernetDHCP) && (incoming == 3))
    {
      systemPrint("Enter new DNS: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        String tempString = String(tempStr);
        settings.ethernetDNS.fromString(tempString);
        restartEthernet = true;
      }
      else
        systemPrint("Error: invalid DNS");
    }
    else if ((!settings.ethernetDHCP) && (incoming == 4))
    {
      systemPrint("Enter new Gateway: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        String tempString = String(tempStr);
        settings.ethernetGateway.fromString(tempString);
        restartEthernet = true;
      }
      else
        systemPrint("Error: invalid Gateway");
    }
    else if ((!settings.ethernetDHCP) && (incoming == 5))
    {
      systemPrint("Enter new Subnet Mask: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        String tempString = String(tempStr);
        settings.ethernetSubnet.fromString(tempString);
        restartEthernet = true;
      }
      else
        systemPrint("Error: invalid Subnet Mask");
    }
    else if (incoming == 'c')
    {
      //Toggle TCP client
      settings.enableTcpClientEthernet ^= 1;
      restartEthernet = true;
    }
    else if (incoming == 's')
    {
      //Toggle TCP server
      settings.enableTcpServerEthernet ^= 1;
      if ((!settings.enableTcpServerEthernet) && online.tcpServerEthernet)
      {
        //Tell the UART2 tasks that the TCP server is shutting down
        online.tcpServerEthernet = false;

        //Wait for the UART2 tasks to close the TCP client connections
        while (ethernetTcpServerActive())
          delay(5);
        systemPrintln("TCP Server offline");
      }
      restartEthernet = true;
    }
    else if ((incoming == 'p') && (settings.enableTcpServerEthernet == true || settings.enableTcpClientEthernet == true))
    {
      systemPrint("Enter the TCP port to use (0 to 65535): ");
      int ethernetTcpPort = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((ethernetTcpPort != INPUT_RESPONSE_GETNUMBER_EXIT) && (ethernetTcpPort != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (ethernetTcpPort < 0 || ethernetTcpPort > 65535)
          systemPrintln("Error: TCP Port out of range");
        else
        {
          settings.ethernetTcpPort = ethernetTcpPort; //Recorded to NVM and file at main menu exit
          restartEthernet = true;
        }
      }
    }
    else if ((incoming == 'h') && (settings.enableTcpClientEthernet == true))
    {
      systemPrint("Enter new host name / address: ");
      getString(settings.hostForTCPClient, sizeof(settings.hostForTCPClient));
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

  if (restartEthernet) //Restart the ESP to use the new ethernet settings
  {
    recordSystemSettings();
    ESP.restart();
  }

#endif  //COMPILE_ETHERNET
}
