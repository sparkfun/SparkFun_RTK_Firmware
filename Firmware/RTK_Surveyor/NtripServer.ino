/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
NTRIP Server States:
    NTRIP_SERVER_OFF: Using Bluetooth or NTRIP server
    NTRIP_SERVER_ON: WIFI_ON state
    NTRIP_SERVER_WIFI_CONNECTING: Connecting to WiFi access point
    NTRIP_SERVER_WIFI_CONNECTED: WiFi connected to an access point
    NTRIP_SERVER_WAIT_GNSS_DATA: Waiting for correction data from GNSS
    NTRIP_SERVER_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_SERVER_AUTHORIZATION: Validate the credentials
    NTRIP_SERVER_CASTING: Sending correction data to the NTRIP caster

                               NTRIP_SERVER_OFF
                                       |   ^
                      ntripServerStart |   | ntripServerStop(true)
                                       v   |
                    .---------> NTRIP_SERVER_ON <-------------.
                    |                  |                      |
                    |                  |                      | ntripServerStop(false)
                    |                  v                Fail  |
                    |    NTRIP_SERVER_WIFI_CONNECTING ------->+
                    |                  |                      ^
                    |                  |                      |
                    |                  v                Fail  |
                    |    NTRIP_SERVER_WIFI_CONNECTED -------->+
                    |                  |                      ^
                    |                  |                WiFi  |
                    |                  v                Fail  |
                    |    NTRIP_SERVER_WAIT_GNSS_DATA -------->+
                    |                  |                      ^
                    |                  | Discard Data   WiFi  |
                    |                  v                Fail  |
                    |      NTRIP_SERVER_CONNECTING ---------->+
                    |                  |                      ^
                    |                  | Discard Data   WiFi  |
                    |                  v                Fail  |
                    |     NTRIP_SERVER_AUTHORIZATION -------->+
                    |                  |                      ^
                    |                  | Discard Data   Wifi  |
                    |                  v                Fail  |
                    |         NTRIP_SERVER_CASTING -----------'
                    |                  |
                    |                  | Data timeout
                    |                  |
                    |                  | Close Server connection
                    '------------------'

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Constants - compiled out
//----------------------------------------

#ifdef  COMPILE_WIFI

//----------------------------------------
// Locals - compiled out
//----------------------------------------

//Last time the NTRIP server state was displayed
static uint32_t ntripServerStateLastDisplayed = 0;

//----------------------------------------
// NTRIP Server Routines - compiled out
//----------------------------------------

//Determine if the connection limit has been reached
bool ntripServerConnectLimitReached()
{
  return true;
}

//Update the state of the NTRIP server state machine
void ntripServerSetState(byte newState)
{
  if (ntripServerState == newState)
    Serial.print(F("*"));
  ntripServerState = newState;
  switch (newState)
  {
    default:
      Serial.printf("Unknown NTRIP Server state: %d\r\n", newState);
      break;
    case NTRIP_SERVER_OFF:
      Serial.println(F("NTRIP_SERVER_OFF"));
      break;
    case NTRIP_SERVER_ON:
      Serial.println(F("NTRIP_SERVER_ON"));
      break;
    case NTRIP_SERVER_WIFI_CONNECTING:
      Serial.println(F("NTRIP_SERVER_WIFI_CONNECTING"));
      break;
    case NTRIP_SERVER_WIFI_CONNECTED:
      Serial.println(F("NTRIP_SERVER_WIFI_CONNECTED"));
      break;
    case NTRIP_SERVER_WAIT_GNSS_DATA:
      Serial.println(F("NTRIP_SERVER_WAIT_GNSS_DATA"));
      break;
    case NTRIP_SERVER_CONNECTING:
      Serial.println(F("NTRIP_SERVER_CONNECTING"));
      break;
    case NTRIP_SERVER_AUTHORIZATION:
      Serial.println(F("NTRIP_SERVER_AUTHORIZATION"));
      break;
    case NTRIP_SERVER_CASTING:
      Serial.println(F("NTRIP_SERVER_CASTING"));
      break;
  }
}

#endif  // COMPILE_WIFI

//----------------------------------------
// Global NTRIP Server Routines
//----------------------------------------

//Update the NTRIP server state machine
void ntripServerUpdate()
{
#ifdef  COMPILE_WIFI
  //Periodically display the NTRIP server state
  if (settings.enablePrintNtripServerState && ((millis() - ntripServerStateLastDisplayed) > 15000))
  {
    ntripServerSetState (ntripServerState);
    ntripServerStateLastDisplayed = millis();
  }

  //Periodically display the IP address
  wifiPeriodicallyDisplayIpAddress();

  //Enable WiFi and the NTRIP server if requested
  switch (ntripServerState)
  {
    //Bluetooth enabled
    case NTRIP_SERVER_OFF:
      break;

    //Start WiFi
    case NTRIP_SERVER_ON:
      wifiStart(settings.ntripServer_wifiSSID, settings.ntripServer_wifiPW);
      ntripServerSetState(NTRIP_SERVER_WIFI_CONNECTING);
      break;

    //Wait for connection to an access point
    case NTRIP_SERVER_WIFI_CONNECTING:
      if (!wifiIsConnected())
      {
        if (wifiConnectionTimeout())
        {
          //Assume AP weak signal, the AP is unable to respond successfully
          if (ntripServerConnectLimitReached())
            //Display the WiFi failure
            paintNtripWiFiFail(4000, false);
        }
      }
      else
      {
        //WiFi connection established
        ntripServerSetState(NTRIP_SERVER_WIFI_CONNECTED);

        // Start the SD card server
//        sdCardServerBegin(&server, true, true);
      }
      break;
  }
#endif  //COMPILE_WIFI
}
