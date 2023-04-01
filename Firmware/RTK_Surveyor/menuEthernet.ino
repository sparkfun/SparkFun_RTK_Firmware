//Ethernet

void beginEthernet()
{
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET

  if (online.ethernetStatus == ETH_CAN_NOT_BEGIN)
    return;

  if (online.ethernetStatus == ETH_NOT_BEGUN)
  {
    Ethernet.init(pin_Ethernet_CS);
  
    if (settings.ethernetDHCP)
      Ethernet.begin(ethernetMACAddress);
    else
      Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS, settings.ethernetGateway, settings.ethernetSubnet);

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      log_d("Ethernet hardware not found");
      online.ethernetStatus = ETH_CAN_NOT_BEGIN;
      return;
    }

    online.ethernetStatus = ETH_BEGUN_NO_LINK;
  }

  if (online.ethernetStatus == ETH_BEGUN_NO_LINK)
  {
    if (Ethernet.linkStatus() == LinkON)
    {
      online.ethernetStatus = ETH_LINK;
      return;
    }
  }

  //online.ethernetStatus == ETH_LINK
  if (Ethernet.linkStatus() == LinkOFF)
    online.ethernetStatus = ETH_BEGUN_NO_LINK;

#endif ///COMPILE_ETHERNET
}

void beginEthernetNTPServer()
{
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET
  if ((online.ethernetStatus == ETH_LINK) && (online.ethernetNTPServer == false))
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

    systemPrint("1) Ethernet Config  : ");
    if (settings.ethernetDHCP)
      systemPrintln("DHCP");
    else
      systemPrintln("Fixed IP");

    if (!settings.ethernetDHCP)
    {
      systemPrint("2) Fixed IP Address : ");
      systemPrintln(settings.ethernetIP.toString());
      systemPrint("3) DNS              : ");
      systemPrintln(settings.ethernetDNS.toString());
      systemPrint("4) Gateway          : ");
      systemPrintln(settings.ethernetGateway.toString());
      systemPrint("5) Subnet Mask      : ");
      systemPrintln(settings.ethernetSubnet.toString());
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
        settings.ethernetIP.fromString(tempStr);
        restartEthernet = true;        
      }
    }
    else if ((!settings.ethernetDHCP) && (incoming == 3))
    {
      systemPrint("Enter new DNS: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        settings.ethernetDNS.fromString(tempStr);
        restartEthernet = true;        
      }
    }
    else if ((!settings.ethernetDHCP) && (incoming == 4))
    {
      systemPrint("Enter new Gateway: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        settings.ethernetGateway.fromString(tempStr);
        restartEthernet = true;        
      }
    }
    else if ((!settings.ethernetDHCP) && (incoming == 5))
    {
      systemPrint("Enter new Subnet Mask: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        settings.ethernetSubnet.fromString(tempStr);
        restartEthernet = true;        
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

  clearBuffer(); //Empty buffer of any newline chars

  if (restartEthernet) //Restart the ESP to use the new ethernet settings
  {
    recordSystemSettings();
    ESP.restart();
  }

#endif  //COMPILE_ETHERNET
}
