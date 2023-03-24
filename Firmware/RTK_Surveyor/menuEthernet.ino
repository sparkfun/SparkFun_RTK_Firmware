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
    attachInterrupt(pin_Ethernet_Interrupt, ethernetISR, FALLING); //Attach the interrupt
    online.ethernetNTPServer = true;
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
    w5500ClearSocketInterrupt(ntpSockIndex); //Not sure if it is best to clear the interrupt(s) here - or in the loop?
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
#endif  //COMPILE_ETHERNET
}
