/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  WiFi Status Values:
    WL_CONNECTED: assigned when connected to a WiFi network
    WL_CONNECTION_LOST: assigned when the connection is lost
    WL_CONNECT_FAILED: assigned when the connection fails for all the attempts
    WL_DISCONNECTED: assigned when disconnected from a network
    WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and
                    remains active until the number of attempts expires (resulting in
                    WL_CONNECT_FAILED) or a connection is established (resulting in
                    WL_CONNECTED)
    WL_NO_SHIELD: assigned when no WiFi shield is present
    WL_NO_SSID_AVAIL: assigned when no SSID are available
    WL_SCAN_COMPLETED: assigned when the scan networks is completed

  WiFi Station States:

                                  WIFI_OFF
                                    |   ^
                           Use WiFi |   |
                                    |   | WL_CONNECT_FAILED (Bad password)
                                    |   | WL_NO_SSID_AVAIL (Out of range)
                                    v   |
                                   WIFI_ON
                                    |   ^
                         WiFi.begin |   | WiFi.stop
                                    |   | WL_CONNECTION_LOST
                                    |   | WL_CONNECT_FAILED (Bad password)
                                    |   | WL_DISCONNECTED
                                    |   | WL_NO_SSID_AVAIL (Out of range)
                                    v   |
                                  WIFI_NOT_CONNECTED
                                    |   ^
                       WL_CONNECTED |   | WiFi.disconnect
                                    |   | WL_CONNECTION_LOST
                                    |   | WL_CONNECT_FAILED (Bad password)
                                    |   | WL_DISCONNECTED
                                    |   | WL_NO_SSID_AVAIL (Out of range)
                                    v   |
                                  WIFI_CONNECTED

  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Constants
//----------------------------------------

//If we cannot connect to local wifi, give up/go to Rover
static const int WIFI_CONNECTION_TIMEOUT = 10 * 1000;  //Milliseconds

//Interval to use when displaying the IP address
static const int WIFI_IP_ADDRESS_DISPLAY_INTERVAL = 12 * 1000;  //Milliseconds

#define WIFI_MAX_TCP_CLIENTS     4

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#ifdef COMPILE_WIFI

//WiFi Timer usage:
//  * Measure connection time to access point
//  * Measure interval to display IP address
static unsigned long wifiTimer = 0;

//Last time the WiFi state was displayed
static uint32_t lastWifiState = 0;

//TCP server
static WiFiServer *wifiTcpServer = NULL;
static WiFiClient wifiTcpClient[WIFI_MAX_TCP_CLIENTS];

//----------------------------------------
// WiFi Routines - compiled out
//----------------------------------------

void wifiDisplayIpAddress()
{
  systemPrint("WiFi IP address: ");
  systemPrint(WiFi.localIP());
  systemPrintf(" RSSI: %d\r\n", WiFi.RSSI());

  wifiTimer = millis();
}

IPAddress wifiGetIpAddress()
{
  return WiFi.localIP();
}

byte wifiGetStatus()
{
  return WiFi.status();
}

bool wifiIsConnected()
{
  bool isConnected;

  isConnected = (wifiGetStatus() == WL_CONNECTED);
  if (isConnected)
  {
    wifiSetState(WIFI_CONNECTED);
    wifiDisplayIpAddress();
  }
  return isConnected;
}

void wifiPeriodicallyDisplayIpAddress()
{
  if (settings.enablePrintWifiIpAddress && (wifiGetStatus() == WL_CONNECTED))
    if ((millis() - wifiTimer) >= WIFI_IP_ADDRESS_DISPLAY_INTERVAL)
      wifiDisplayIpAddress();
}

//Update the state of the WiFi state machine
void wifiSetState(byte newState)
{
  if (wifiState == newState)
    systemPrint("*");
  wifiState = newState;
  switch (newState)
  {
    default:
      systemPrintf("Unknown WiFi state: %d\r\n", newState);
      break;
    case WIFI_OFF:
      systemPrintln("WIFI_OFF");
      break;
    case WIFI_ON:
      systemPrintln("WIFI_ON");
      break;
    case WIFI_NOTCONNECTED:
      systemPrintln("WIFI_NOTCONNECTED");
      break;
    case WIFI_CONNECTED:
      systemPrintln("WIFI_CONNECTED");
      break;
  }
}

//----------------------------------------
// WiFi Config Support Routines - compiled out
//----------------------------------------

void wifiStartAP()
{
  if (settings.wifiConfigOverAP == true)
  {
    //Start in AP mode

    WiFi.mode(WIFI_AP);

#ifdef COMPILE_ESPNOW
    // Return protocol to default settings (no WIFI_PROTOCOL_LR for ESP NOW)
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); //Stops WiFi AP.
#endif

    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(local_IP, gateway, subnet);
    if (WiFi.softAP("RTK Config") == false) //Must be short enough to fit OLED Width
    {
      systemPrintln("WiFi AP failed to start");
      return;
    }
    systemPrint("WiFi AP Started with IP: ");
    systemPrintln(WiFi.softAPIP());
  }
  else
  {
    //Start webserver on local WiFi instead of AP

    WiFi.mode(WIFI_STA);

#ifdef COMPILE_ESPNOW
    // Return protocol to default settings (no WIFI_PROTOCOL_LR for ESP NOW)
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); //Stops WiFi Station
#endif

    wifiConnect(); //Attempt to connect to any SSID on settings list
    wifiPrintNetworkInfo();
  }
  
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global WiFi Routines
//----------------------------------------

//Attempts a connection to all provided SSIDs
//Returns true if successful
//Gives up if no SSID detected or connection times out
bool wifiConnect()
{
  //Check if we have any SSIDs
  if (wifiNetworkCount() == 0)
  {
    systemPrintln("No WiFi networks stored in settings.");
    return false;
  }

  if(wifiIsConnected())
    return(true);

#ifdef COMPILE_WIFI

  WiFiMulti wifiMulti;

  //Load SSIDs
  for (int x = 0 ; x < MAX_WIFI_NETWORKS ; x++)
  {
    if (strlen(settings.wifiNetworks[x].ssid) > 0)
      wifiMulti.addAP(settings.wifiNetworks[x].ssid, settings.wifiNetworks[x].password);
  }

  systemPrintln("Connecting WiFi...");

  int timeout = 0; //Give up after timeount. Increases with each failed try.
  for (int tries = 0 ; tries < 3 ; tries++)
  {
    timeout += 5000;

    if (wifiMulti.run(timeout) == WL_CONNECTED)
      return true;
    systemPrintln("WiFi connection timed out. Trying again.");
  }
#endif

  return false;
}

//Counts the number of entered SSIDs
int wifiNetworkCount()
{
  //Count SSIDs
  int networkCount = 0;
  for (int x = 0 ; x < MAX_WIFI_NETWORKS ; x++)
  {
    if (strlen(settings.wifiNetworks[x].ssid) > 0)
      networkCount++;
  }
  return networkCount;
}

//Determine if the WiFi connection has timed out
bool wifiConnectionTimeout()
{
#ifdef COMPILE_WIFI
  if ((millis() - wifiTimer) <= WIFI_CONNECTION_TIMEOUT)
    return false;
  systemPrintln("WiFi connection timeout!");
#endif  //COMPILE_WIFI
  return true;
}

//Send data to the TCP clients
void wifiSendTcpData(uint8_t * data, uint16_t length)
{
#ifdef COMPILE_WIFI
  static IPAddress ipAddress[WIFI_MAX_TCP_CLIENTS];
  int index = 0;
  static uint32_t lastTcpConnectAttempt;

  if (online.tcpClient)
  {
    //Start the TCP client if enabled
    if (((!wifiTcpClient[0]) || (!wifiTcpClient[0].connected()))
        && ((millis() - lastTcpConnectAttempt) >= 1000))
    {
      lastTcpConnectAttempt = millis();
      ipAddress[0] = WiFi.gatewayIP();
      if (settings.enablePrintTcpStatus)
      {
        systemPrint("Trying to connect TCP client to ");
        systemPrintln(ipAddress[0]);
      }
      if (wifiTcpClient[0].connect(ipAddress[0], settings.wifiTcpPort))
      {
        online.tcpClient = true;
        systemPrint("TCP client connected to ");
        systemPrintln(ipAddress[0]);
        wifiTcpConnected |= 1 << index;
      }
      else
      {
        //Release any allocated resources
        //if (wifiTcpClient[0])
        wifiTcpClient[0].stop();
      }
    }
  }

  if (online.tcpServer)
  {
    //Check for another client
    for (index = 0; index < WIFI_MAX_TCP_CLIENTS; index++)
      if (!(wifiTcpConnected & (1 << index)))
      {
        if ((!wifiTcpClient[index]) || (!wifiTcpClient[index].connected()))
        {
          wifiTcpClient[index] = wifiTcpServer->available();
          if (!wifiTcpClient[index])
            break;
          ipAddress[index] = wifiTcpClient[index].remoteIP();
          systemPrintf("Connected TCP client %d to ", index);
          systemPrintln(ipAddress[index]);
          wifiTcpConnected |= 1 << index;
        }
      }
  }

  //Walk the list of TCP clients
  for (index = 0; index < WIFI_MAX_TCP_CLIENTS; index++)
  {
    if (wifiTcpConnected & (1 << index))
    {
      //Check for a broken connection
      if ((!wifiTcpClient[index]) || (!wifiTcpClient[index].connected()))
        systemPrintf("Disconnected TCP client %d from ", index);

      //Send the data to the connected clients
      else if (((settings.enableTcpServer && online.tcpServer)
                || (settings.enableTcpClient && online.tcpClient))
               && ((!length) || (wifiTcpClient[index].write(data, length) == length)))
      {
        if (settings.enablePrintTcpStatus && length)
          systemPrintf("%d bytes written over TCP\r\n", length);
        continue;
      }

      //Failed to write the data
      else
        systemPrintf("Breaking TCP client %d connection to ", index);

      //Done with this client connection
      systemPrintln(ipAddress[index]);
      wifiTcpClient[index].stop();
      wifiTcpConnected &= ~(1 << index);

      //Shutdown the TCP server if necessary
      if (settings.enableTcpServer || online.tcpServer)
        wifiTcpServerActive();
    }
  }
#endif  //COMPILE_WIFI
}

//Check for TCP server active
bool wifiTcpServerActive()
{
#ifdef COMPILE_WIFI
  if ((settings.enableTcpServer && online.tcpServer) || wifiTcpConnected)
    return true;

  //Shutdown the TCP server
  online.tcpServer = false;

  //Stop the TCP server
  wifiTcpServer->stop();

  if (wifiTcpServer != NULL)
    free(wifiTcpServer);
#endif  //COMPILE_WIFI
  return false;
}

//If radio is off entirely, start WiFi
//If ESP-Now is active, only add the LR protocol
void wifiStart()
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_ESPNOW
  bool restartESPNow = false;
#endif

  if ((wifiState == WIFI_OFF) || (wifiState == WIFI_ON))
  {
    wifiSetState(WIFI_NOTCONNECTED);

    WiFi.mode(WIFI_STA);

#ifdef COMPILE_ESPNOW
    if (espnowState > ESPNOW_OFF)
    {
      restartESPNow = true;
      espnowStop();
      esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR); //Enable WiFi + ESP-Now. Stops WiFi Station.
    }
    else
    {
      esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); //Set basic WiFi protocols. Stops WiFi Station.
    }
#else
    //Be sure the standard protocols are turned on. ESP Now have have previously turned them off.
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); //Set basic WiFi protocols. Stops WiFi Station.
#endif

    wifiConnect(); //Attempt to connect to any SSID on settings list
    wifiTimer = millis();

#ifdef COMPILE_ESPNOW
    if (restartESPNow == true)
      espnowStart();
#endif

    //Verify WIFI_MAX_TCP_CLIENTS
    if ((sizeof(wifiTcpConnected) * 8) < WIFI_MAX_TCP_CLIENTS)
    {
      systemPrintf("Please set WIFI_MAX_TCP_CLIENTS <= %d or increase size of wifiTcpConnected\r\n", sizeof(wifiTcpConnected) * 8);
      while (true)
      {
      }
    }

    //Display the heap state
    reportHeapNow();
  }
#endif  //COMPILE_WIFI
}

//Stop WiFi and release all resources
//See WiFiBluetoothSwitch sketch for more info
//If ESP NOW is active, leave WiFi on enough for ESP NOW
void wifiStop()
{
#ifdef COMPILE_WIFI
  stopWebServer();

  //Shutdown the TCP client
  if (online.tcpClient)
  {
    //Tell the UART2 tasks that the TCP client is shutting down
    online.tcpClient = false;
    delay(5);
    systemPrintln("TCP client offline");
  }

  //Shutdown the TCP server connection
  if (online.tcpServer)
  {
    //Tell the UART2 tasks that the TCP server is shutting down
    online.tcpServer = false;

    //Wait for the UART2 tasks to close the TCP client connections
    while (wifiTcpServerActive())
      delay(5);
    systemPrintln("TCP Server offline");
  }

  if (wifiState == WIFI_OFF)
  {
    //Do nothing
  }

#ifdef COMPILE_ESPNOW
  //If WiFi is on but ESP NOW is off, then turn off radio entirely
  else if (espnowState == ESPNOW_OFF)
  {
    wifiSetState(WIFI_OFF);
    WiFi.mode(WIFI_OFF);
    log_d("WiFi Stopped");
  }
  //If ESP-Now is active, change protocol to only Long Range
  else if (espnowState > ESPNOW_OFF)
  {
    wifiSetState(WIFI_OFF);

    // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR); //Stops WiFi Station.

    WiFi.mode(WIFI_STA);

    log_d("WiFi disabled, ESP-Now left in place");
  }
#else
  //Turn off radio
  wifiSetState(WIFI_OFF);
  WiFi.mode(WIFI_OFF);
  log_d("WiFi Stopped");
#endif

  //Display the heap state
  reportHeapNow();
#endif  //COMPILE_WIFI
}

void wifiUpdate()
{
#ifdef COMPILE_WIFI
  //Periodically display the WiFi state
  if (settings.enablePrintWifiState && ((millis() - lastWifiState) > 15000))
  {
    wifiSetState (wifiState);
    lastWifiState = millis();
  }

  //Periodically display the IP address
  wifiPeriodicallyDisplayIpAddress();

  //Start the TCP client if enabled
  if (settings.enableTcpClient && (!online.tcpClient) && (!settings.enableTcpServer)
      && (wifiState == WIFI_CONNECTED))
  {
    online.tcpClient = true;
    systemPrint("TCP client online, local IP ");
    systemPrint(WiFi.localIP());
    systemPrint(", gateway IP ");
    systemPrintln(WiFi.gatewayIP());
  }

  //Start the TCP server if enabled
  if ((!wifiTcpServer) && (!settings.enableTcpClient) && settings.enableTcpServer
      && (wifiState == WIFI_CONNECTED))
  {
    if (wifiTcpServer == NULL)
      wifiTcpServer = new WiFiServer(settings.wifiTcpPort);

    wifiTcpServer->begin();
    online.tcpServer = true;
    systemPrint("TCP Server online, IP Address ");
    systemPrintln(WiFi.localIP());
  }

  //Support NTRIP client during Rover operation
  if (systemState < STATE_BASE_NOT_STARTED)
    ntripClientUpdate();

  //Support NTRIP server during Base operation
  else if (systemState < STATE_BUBBLE_LEVEL)
    ntripServerUpdate();
#endif  //COMPILE_WIFI
}

void wifiPrintNetworkInfo()
{
#ifdef COMPILE_WIFI
  systemPrintln("\n\nNetwork Configuration:");
  systemPrintln("----------------------");
  systemPrint("         SSID: "); systemPrintln(WiFi.SSID());
  systemPrint("  WiFi Status: "); systemPrintln(WiFi.status());
  systemPrint("WiFi Strength: "); systemPrint(WiFi.RSSI()); systemPrintln(" dBm");
  systemPrint("          MAC: "); systemPrintln(WiFi.macAddress());
  systemPrint("           IP: "); systemPrintln(WiFi.localIP());
  systemPrint("       Subnet: "); systemPrintln(WiFi.subnetMask());
  systemPrint("      Gateway: "); systemPrintln(WiFi.gatewayIP());
  systemPrint("        DNS 1: "); systemPrintln(WiFi.dnsIP(0));
  systemPrint("        DNS 2: "); systemPrintln(WiFi.dnsIP(1));
  systemPrint("        DNS 3: "); systemPrintln(WiFi.dnsIP(2));
  systemPrintln();
#endif
}
