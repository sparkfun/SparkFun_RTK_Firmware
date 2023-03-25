//Ethernet

void beginEthernet()
{
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET
  Ethernet.init(pin_Ethernet_CS);

  switch(settings.ethernetConfig)
  {
    case ETHERNET_FIXED_IP:
      Ethernet.begin(ethernetMACAddress, settings.ethernetIP);
      break;
    case ETHERNET_FIXED_IP_DNS:
      Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS);
      break;
    case ETHERNET_FIXED_IP_DNS_GATEWAY:
      Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS, settings.ethernetGateway);
      break;
    case ETHERNET_FIXED_IP_DNS_GATEWAY_SUBNET:
      Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS, settings.ethernetGateway, settings.ethernetSubnet);
      break;
    case ETHERNET_DHCP:
    default:
      Ethernet.begin(ethernetMACAddress);
      break;
  }

  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    log_d("Ethernet hardware not found");
    online.ethernet = false;
    return;
  }

  if (Ethernet.linkStatus() == LinkOFF)
  {
    log_d("Ethernet cable is not connected");
    online.ethernet = false;
    return;
  }

  online.ethernet = true;
#endif
}

void beginEthernetHTTPServer()
{
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET
  if (online.ethernet)
  {
    ethernetHTTPServer = new EthernetServer(settings.ethernetHttpPort);
    online.ethernetHTTPServer = true;
  }
#endif
}

void beginEthernetNTPServer()
{
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET
  if (online.ethernet)
  {
    ethernetNTPServer = new derivedEthernetUDP;
    ethernetNTPServer->begin(settings.ethernetNtpPort);
    ntpSockIndex = ethernetNTPServer->getSockIndex(); //Get the socket index
    w5500ClearSocketInterrupts(); // Clear all interrupts
    w5500EnableSocketInterrupt(ntpSockIndex); // Enable the RECV interrupt for the desired socket index
    pinMode(pin_Ethernet_Interrupt, INPUT_PULLUP); //Prepare the interrupt pin
    attachInterrupt(pin_Ethernet_Interrupt, ethernetISR, FALLING); //Attach the interrupt
    online.ethernetNTPServer = true;
  }
#endif
}

void updateNTPServer()
{  
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET
  if (online.ethernetNTPServer)
  {    
    char ntpDiag[512]; // Char array to hold diagnostic messages
    
    // Check for new NTP requests - if the time has been sync'd
    bool processed = processOneNTPRequest((lastRTCSync > 0), (const timeval *)&ethernetNtpTv, (const timeval *)&gnssSyncTv, ntpDiag, sizeof(ntpDiag));
  
    if (processed && settings.enablePrintNTPDiag && (!inMainMenu))
      systemPrint(ntpDiag);
  }
#endif
}

void updateEthernet()
{
  if (!HAS_ETHERNET)
    return;

#ifdef COMPILE_ETHERNET
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

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Ethernet (W5500) ISR
// Triggered by the falling edge of the W5500 interrupt signal - indicates the arrival of a packet
// Record the time the packet arrived

#ifdef COMPILE_ETHERNET
void ethernetISR()
{
  if (w5500CheckSocketInterrupt(ntpSockIndex))
  {
    gettimeofday((timeval *)&ethernetNtpTv, NULL); //Record the time of the NTP interrupt
    w5500ClearSocketInterrupt(ntpSockIndex); //Clear interrupt here
  }
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
    switch(settings.ethernetConfig)
    {
      case ETHERNET_FIXED_IP:
        systemPrintln("Fixed IP");
        break;
      case ETHERNET_FIXED_IP_DNS:
        systemPrintln("Fixed IP + DNS");
        break;
      case ETHERNET_FIXED_IP_DNS_GATEWAY:
        systemPrintln("Fixed IP + DNS + Gateway");
        break;
      case ETHERNET_FIXED_IP_DNS_GATEWAY_SUBNET:
        systemPrintln("Fixed IP + DNS + Gateway + Subnet Mask");
        break;
      case ETHERNET_DHCP:
      default:
        systemPrintln("DHCP");
        break;
    }

    if (settings.ethernetConfig >= ETHERNET_FIXED_IP)
    {
      systemPrint("2) Fixed IP Address : ");
      systemPrintln(settings.ethernetIP.toString());
    }

    if (settings.ethernetConfig >= ETHERNET_FIXED_IP_DNS)
    {
      systemPrint("3) DNS              : ");
      systemPrintln(settings.ethernetDNS.toString());
    }

    if (settings.ethernetConfig >= ETHERNET_FIXED_IP_DNS_GATEWAY)
    {
      systemPrint("4) Gateway          : ");
      systemPrintln(settings.ethernetGateway.toString());
    }

    if (settings.ethernetConfig >= ETHERNET_FIXED_IP_DNS_GATEWAY_SUBNET)
    {
      systemPrint("5) Subnet Mask      : ");
      systemPrintln(settings.ethernetSubnet.toString());
    }

    systemPrintln("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming == 1)
    {
      if (settings.ethernetConfig < ETHERNET_FIXED_IP_DNS_GATEWAY_SUBNET)
        settings.ethernetConfig = (ethernetConfigOptions)((int)settings.ethernetConfig + 1);
      else
        settings.ethernetConfig = ETHERNET_DHCP;
      restartEthernet = true;
    }
    else if ((settings.ethernetConfig >= ETHERNET_FIXED_IP) && (incoming == 2))
    {
      systemPrint("Enter new IP Address: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        settings.ethernetIP.fromString(tempStr);
        restartEthernet = true;        
      }
    }
    else if ((settings.ethernetConfig >= ETHERNET_FIXED_IP_DNS) && (incoming == 3))
    {
      systemPrint("Enter new DNS: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        settings.ethernetDNS.fromString(tempStr);
        restartEthernet = true;        
      }
    }
    else if ((settings.ethernetConfig >= ETHERNET_FIXED_IP_DNS_GATEWAY) && (incoming == 4))
    {
      systemPrint("Enter new Gateway: ");
      char tempStr[20];
      if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
      {
        settings.ethernetGateway.fromString(tempStr);
        restartEthernet = true;        
      }
    }
    else if ((settings.ethernetConfig >= ETHERNET_FIXED_IP_DNS_GATEWAY_SUBNET) && (incoming == 5))
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
