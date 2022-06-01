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

#ifdef  COMPILE_WIFI

//----------------------------------------
// Locals - compiled out
//----------------------------------------

//Count the number of connection attempts
static int ntripClientConnectionAttempts;

//----------------------------------------
// NTRIP Client Routines - compiled out
//----------------------------------------

void ntripClientAllowMoreConnections()
{
  ntripClientConnectionAttempts = 0;
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

void ntripClientStart()
{
#ifdef  COMPILE_WIFI
  ntripClientState = NTRIP_CLIENT_OFF;

  //Start the NTRIP client if enabled
  if (settings.enableNtripClient == true)
  {
    //Display the heap state
    reportHeapNow();

    //Startup WiFi and the NTRIP client
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
  }
 #endif  //COMPILE_WIFI
}
