/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
NTRIP Client States:
    NTRIP_CLIENT_OFF: Using Bluetooth or NTRIP server
    NTRIP_CLIENT_ON: WIFI_ON state
    NTRIP_CLIENT_WIFI_CONNECTING: Connecting to WiFi access point
    NTRIP_CLIENT_WIFI_CONNECTED: WiFi connected to an access point
    NTRIP_CLIENT_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_CONNECTED: Connected to the NTRIP caster

                               NTRIP_CLIENT_OFF
                                       |   ^
                      ntripClientStart |   | No more retries
                                       v   |
                               NTRIP_CLIENT_ON <--------------.
                                       |                      |
                                       |                      | ntripClientStop
                                       v                Fail  |
                         NTRIP_CLIENT_WIFI_CONNECTING ------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                         NTRIP_CLIENT_WIFI_CONNECTED -------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                           NTRIP_CLIENT_CONNECTING ---------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                            NTRIP_CLIENT_CONNECTED -----------'

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Constants - compiled out
//----------------------------------------

#ifdef  COMPILE_WIFI

//Give up connecting after this number of attempts
static const int MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS = 3;

//----------------------------------------
// Locals - compiled out
//----------------------------------------

//The WiFi connection to the NTRIP caster to obtain RTCM data.
static WiFiClient * ntripClient;

//Count the number of connection attempts
static int ntripClientConnectionAttempts;

//----------------------------------------
// NTRIP Client Routines - compiled out
//----------------------------------------

void ntripClientAllowMoreConnections()
{
  ntripClientConnectionAttempts = 0;
}

//Determine if another connection is possible or if the limit has been reached
bool ntripClientConnectLimitReached()
{
  //Shutdown the NTRIP client
  ntripClientStop();

  //Retry the connection a few times
  bool limitReached = (ntripClientConnectionAttempts++ >= MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS);
  if (!limitReached)
    //Display the heap state
    reportHeapNow();
  else
    //No more connection attempts, switching to Bluetooth
    ntripClientSwitchToBluetooth();
  return limitReached;
}

//Stop the NTRIP client
void ntripClientStop()
{
  if (ntripClient)
  {
    if (ntripClient->connected())
      ntripClient->stop();
    delete ntripClient;
    ntripClient = NULL;
  }
  if (ntripClientState > NTRIP_CLIENT_ON)
    wifiStop();
  ntripClientState = NTRIP_CLIENT_ON;
}

//Switch to Bluetooth operation
void ntripClientSwitchToBluetooth()
{
  Serial.println(F("NTRIP Client failure, switching to Bluetooth!"));

  //Stop WiFi operations
  ntripClientStop();

  //No more connection attempts
  ntripClientState = NTRIP_CLIENT_OFF;

  //Turn on Bluetooth with 'Rover' name
  startBluetooth();
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

void ntripClientStart()
{
#ifdef  COMPILE_WIFI
  //Stop NTRIP client and WiFi
  ntripClientStop();
  ntripClientState = NTRIP_CLIENT_OFF;

  //Start the NTRIP client if enabled
  if (settings.enableNtripClient == true)
  {
    //Display the heap state
    reportHeapNow();
    Serial.println(F("NTRIP Client start"));

    //Allocate the ntripClient structure
    ntripClient = new WiFiClient();

    //Startup WiFi and the NTRIP client
    if (ntripClient)
      ntripClientState = NTRIP_CLIENT_ON;
  }

  //Only fallback to Bluetooth once, then try WiFi again.  This enables changes
  //to the WiFi SSID and password to properly restart the WiFi.
  ntripClientAllowMoreConnections();
#endif  //COMPILE_WIFI
}

//Check for the arrival of any correction data. Push it to the GNSS.
//Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
#ifdef  COMPILE_WIFI
  //Enable WiFi and the NTRIP client if requested
  switch (ntripClientState)
  {
    case NTRIP_CLIENT_OFF:
      break;

    //Start WiFi
    case NTRIP_CLIENT_ON:
      wifiStart(settings.ntripClient_wifiSSID, settings.ntripClient_wifiPW);
      ntripClientState = NTRIP_CLIENT_WIFI_CONNECTING;
      break;

    case NTRIP_CLIENT_WIFI_CONNECTING:
      if (!wifiIsConnected())
      {
        if (wifiConnectionTimeout())
        {
          //Assume AP weak signal, the AP is unable to respond successfully
          if (ntripClientConnectLimitReached())
            //Display the WiFi failure
            paintNClientWiFiFail(4000);
        }
      }
      else
      {
        //WiFi connection established
        ntripClientState = NTRIP_CLIENT_WIFI_CONNECTED;
      }
      break;
  }
 #endif  //COMPILE_WIFI
}
