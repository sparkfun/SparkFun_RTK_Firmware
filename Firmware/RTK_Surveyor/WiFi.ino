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

#define WIFI_MAX_NMEA_CLIENTS         4
#define WIFI_NMEA_TCP_PORT            1958

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

//NMEA TCP server
static WiFiServer wifiNmeaServer(WIFI_NMEA_TCP_PORT);
static WiFiClient wifiNmeaClient[WIFI_MAX_NMEA_CLIENTS];

//----------------------------------------
// WiFi Routines - compiled out
//----------------------------------------

void wifiDisplayIpAddress()
{
  Serial.print("WiFi IP address: ");
  Serial.print(WiFi.localIP());
  Serial.printf(" RSSI: %d\r\n", WiFi.RSSI());

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
    Serial.print("*");
  wifiState = newState;
  switch (newState)
  {
    default:
      Serial.printf("Unknown WiFi state: %d\r\n", newState);
      break;
    case WIFI_OFF:
      Serial.println("WIFI_OFF");
      break;
    case WIFI_ON:
      Serial.println("WIFI_ON");
      break;
    case WIFI_NOTCONNECTED:
      Serial.println("WIFI_NOTCONNECTED");
      break;
    case WIFI_CONNECTED:
      Serial.println("WIFI_CONNECTED");
      break;
  }
}

//----------------------------------------
// WiFi Config Support Routines - compiled out
//----------------------------------------

void wifiStartAP()
{
  //When testing, operate on local WiFi instead of AP
  //#define LOCAL_WIFI_TESTING 1
#ifdef LOCAL_WIFI_TESTING
  //Connect to local router
#define WIFI_SSID "TRex"
#define WIFI_PASSWORD "parachutes"

  WiFi.mode(WIFI_STA);

#ifdef COMPILE_ESPNOW
  // Return protocol to default settings (no WIFI_PROTOCOL_LR for ESP NOW)
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); //Stops WiFi Station
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting to");
  while (wifiGetStatus() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
#else   //End LOCAL_WIFI_TESTING
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
    Serial.println("WiFi AP failed to start");
    return;
  }
  Serial.print("WiFi AP Started with IP: ");
  Serial.println(WiFi.softAPIP());
#endif  //End AP Testing
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global WiFi Routines
//----------------------------------------

//Determine if the WiFi connection has timed out
bool wifiConnectionTimeout()
{
#ifdef COMPILE_WIFI
  if ((millis() - wifiTimer) <= WIFI_CONNECTION_TIMEOUT)
    return false;
  Serial.println("WiFi connection timeout!");
#endif  //COMPILE_WIFI
  return true;
}

//Send NMEA data to the NMEA clients
void wifiNmeaData(uint8_t * data, uint16_t length)
{
#ifdef COMPILE_WIFI
  static IPAddress ipAddress[WIFI_MAX_NMEA_CLIENTS];
  int index = 0;
  static uint32_t lastNmeaConnectAttempt;

  if (online.nmeaClient)
  {
    //Start the NMEA client if enabled
    if (((!wifiNmeaClient[0]) || (!wifiNmeaClient[0].connected()))
        && ((millis() - lastNmeaConnectAttempt) >= 1000))
    {
      lastNmeaConnectAttempt = millis();
      ipAddress[0] = WiFi.gatewayIP();
      if (settings.enablePrintNmeaTcpStatus)
      {
        Serial.print("Trying to connect NMEA client to ");
        Serial.println(ipAddress[0]);
      }
      if (wifiNmeaClient[0].connect(ipAddress[0], WIFI_NMEA_TCP_PORT))
      {
        online.nmeaClient = true;
        Serial.print("NMEA client connected to ");
        Serial.println(ipAddress[0]);
        wifiNmeaConnected |= 1 << index;
      }
      else
      {
        //Release any allocated resources
        //if (wifiNmeaClient[0])
        wifiNmeaClient[0].stop();
      }
    }
  }

  if (online.nmeaServer)
  {
    //Check for another client
    for (index = 0; index < WIFI_MAX_NMEA_CLIENTS; index++)
      if (!(wifiNmeaConnected & (1 << index)))
      {
        if ((!wifiNmeaClient[index]) || (!wifiNmeaClient[index].connected()))
        {
          wifiNmeaClient[index] = wifiNmeaServer.available();
          if (!wifiNmeaClient[index])
            break;
          ipAddress[index] = wifiNmeaClient[index].remoteIP();
          Serial.printf("Connected NMEA client %d to ", index);
          Serial.println(ipAddress[index]);
          wifiNmeaConnected |= 1 << index;
        }
      }
  }

  //Walk the list of NMEA TCP clients
  for (index = 0; index < WIFI_MAX_NMEA_CLIENTS; index++)
  {
    if (wifiNmeaConnected & (1 << index))
    {
      //Check for a broken connection
      if ((!wifiNmeaClient[index]) || (!wifiNmeaClient[index].connected()))
        Serial.printf("Disconnected NMEA client %d from ", index);

      //Send the NMEA data to the connected clients
      else if (((settings.enableNmeaServer && online.nmeaServer)
                || (settings.enableNmeaClient && online.nmeaClient))
               && ((!length) || (wifiNmeaClient[index].write(data, length) == length)))
      {
        if (settings.enablePrintNmeaTcpStatus && length)
          Serial.printf("NMEA %d bytes written\r\n", length);
        continue;
      }

      //Failed to write the data
      else
        Serial.printf("Breaking NMEA client %d connection to ", index);

      //Done with this client connection
      Serial.println(ipAddress[index]);
      wifiNmeaClient[index].stop();
      wifiNmeaConnected &= ~(1 << index);

      //Shutdown the NMEA server if necessary
      if (settings.enableNmeaServer || online.nmeaServer)
        wifiNmeaTcpServerActive();
    }
  }
#endif  //COMPILE_WIFI
}

//Check for NMEA TCP server active
bool wifiNmeaTcpServerActive()
{
#ifdef COMPILE_WIFI
  if ((settings.enableNmeaServer && online.nmeaServer) || wifiNmeaConnected)
    return true;

  //Shutdown the NMEA server
  online.nmeaServer = false;

  //Stop the NMEA server
  wifiNmeaServer.stop();
#endif  //COMPILE_WIFI
  return false;
}

//If radio is off entirely, start WiFi
//If ESP-Now is active, only add the LR protocol
void wifiStart(char* ssid, char* pw)
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

    Serial.printf("WiFi connecting to %s\r\n", ssid);
    WiFi.begin(ssid, pw);
    wifiTimer = millis();

#ifdef COMPILE_ESPNOW
    if (restartESPNow == true)
      espnowStart();
#endif

    //Verify WIFI_MAX_NMEA_CLIENTS
    if ((sizeof(wifiNmeaConnected) * 8) < WIFI_MAX_NMEA_CLIENTS)
    {
      Serial.printf("Please set WIFI_MAX_NMEA_CLIENTS <= %d or increase size of wifiNmeaConnected\r\n", sizeof(wifiNmeaConnected) * 8);
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

  //Shutdown the NMEA client
  if (online.nmeaClient)
  {
    //Tell the UART2 tasks that the NMEA client is shutting down
    online.nmeaClient = false;
    delay(5);
    Serial.println("NMEA TCP client offline");
  }

  //Shutdown the NMEA server connection
  if (online.nmeaServer)
  {
    //Tell the UART2 tasks that the NMEA server is shutting down
    online.nmeaServer = false;

    //Wait for the UART2 tasks to close the NMEA client connections
    while (wifiNmeaTcpServerActive())
      delay(5);
    Serial.println("NMEA Server offline");
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

  //Start the NMEA client if enabled
  if (settings.enableNmeaClient && (!online.nmeaClient) && (!settings.enableNmeaServer)
      && (wifiState == WIFI_CONNECTED))
  {
    online.nmeaClient = true;
    Serial.print("NMEA TCP client online, local IP ");
    Serial.print(WiFi.localIP());
    Serial.print(", gateway IP ");
    Serial.println(WiFi.gatewayIP());
  }

  //Start the NMEA server if enabled
  if ((!wifiNmeaServer) && (!settings.enableNmeaClient) && settings.enableNmeaServer
      && (wifiState == WIFI_CONNECTED))
  {
    wifiNmeaServer.begin();
    online.nmeaServer = true;
    Serial.print("NMEA Server online, IP Address ");
    Serial.println(WiFi.localIP());
  }

  //Support NTRIP client during Rover operation
  if (systemState < STATE_BASE_NOT_STARTED)
    ntripClientUpdate();

  //Support NTRIP server during Base operation
  else if (systemState < STATE_BUBBLE_LEVEL)
    ntripServerUpdate();
#endif  //COMPILE_WIFI
}
