/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  NTRIP Server States:
    NTRIP_SERVER_OFF: WiFi OFF or using NTRIP Client
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
                    |                  | Discard Data   WiFi  |
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

//Give up connecting after this number of attempts
//Connection attempts are throttled to increase the time between attempts
//30 attempts with 5 minute increases will take over 38 hours
static const int MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS = 30;

//----------------------------------------
// Locals - compiled out
//----------------------------------------

// WiFi connection used to push RTCM to NTRIP caster over WiFi
static WiFiClient * ntripServer;

//Count of bytes sent by the NTRIP server to the NTRIP caster
uint32_t ntripServerBytesSent = 0;

//Count the number of connection attempts
static int ntripServerConnectionAttempts;

//Throttle the time between connection attempts
static int ntripServerConnectionAttemptTimeout = 0;
static uint32_t ntripServerLastConnectionAttempt = 0;
static uint32_t ntripServerTimeoutPrint = 0;

//Last time the NTRIP server state was displayed
static uint32_t ntripServerStateLastDisplayed = 0;

//NTRIP server timer usage:
//  * Measure the connection response time
//  * Receive RTCM correction data timeout
//  * Monitor last RTCM byte received for frame counting
static uint32_t ntripServerTimer;

//----------------------------------------
// NTRIP Server Routines - compiled out
//----------------------------------------

//Initiate a connection to the NTRIP caster
bool ntripServerConnectCaster()
{
  const int SERVER_BUFFER_SIZE = 512;
  char serverBuffer[SERVER_BUFFER_SIZE];

  //Attempt a connection to the NTRIP caster
  if (!ntripServer->connect(settings.ntripServer_CasterHost,
                            settings.ntripServer_CasterPort))
    return false;

  //Note the connection to the NTRIP caster
  Serial.printf("Connected to %s:%d\n\r", settings.ntripServer_CasterHost,
                settings.ntripServer_CasterPort);

  //Build the authorization credentials message
  //  * Mount point
  //  * Password
  //  * Agent
  snprintf(serverBuffer, SERVER_BUFFER_SIZE,
           "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/v%d.%d\r\n\r\n",
           settings.ntripServer_MountPointPW,
           settings.ntripServer_MountPoint,
           platformPrefix,
           FIRMWARE_VERSION_MAJOR,
           FIRMWARE_VERSION_MINOR);

  //Send the authorization credentials to the NTRIP caster
  ntripServer->write(serverBuffer, strlen(serverBuffer));
  return true;
}

//Determine if the connection limit has been reached
bool ntripServerConnectLimitReached()
{
  //Shutdown the NTRIP server
  ntripServerStop(false); //Allocate new wifiClient

  //Retry the connection a few times
  bool limitReached = false;
  if (ntripServerConnectionAttempts++ >= MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS) limitReached = true;

  if (limitReached == false)
  {
    ntripServerConnectionAttemptTimeout = ntripServerConnectionAttempts * 5 * 60 * 1000L; //Wait 5, 10, 15, etc minutes between attempts

    log_d("ntripServerConnectionAttemptTimeout increased to %d minutes", ntripServerConnectionAttemptTimeout / (60 * 1000L));

    reportHeapNow();
  }
  else
  {
    //No more connection attempts
    Serial.println("NTRIP Server connection attempts exceeded!");
    ntripServerStop(true);  //Don't allocate new wifiClient
  }
  return limitReached;
}

//Read the authorization response from the NTRIP caster
void ntripServerResponse(char * response, size_t maxLength)
{
  char * responseEnd;

  //Make sure that we can zero terminate the response
  responseEnd = &response[maxLength - 1];

  // Read bytes from the caster and store them
  while ((response < responseEnd) && ntripServer->available())
    *response++ = ntripServer->read();

  // Zero terminate the response
  *response = '\0';
}

//Update the state of the NTRIP server state machine
void ntripServerSetState(byte newState)
{
  if (ntripServerState == newState)
    Serial.print("*");
  ntripServerState = newState;
  switch (newState)
  {
    default:
      Serial.printf("Unknown NTRIP Server state: %d\r\n", newState);
      break;
    case NTRIP_SERVER_OFF:
      Serial.println("NTRIP_SERVER_OFF");
      break;
    case NTRIP_SERVER_ON:
      Serial.println("NTRIP_SERVER_ON");
      break;
    case NTRIP_SERVER_WIFI_CONNECTING:
      Serial.println("NTRIP_SERVER_WIFI_CONNECTING");
      break;
    case NTRIP_SERVER_WIFI_CONNECTED:
      Serial.println("NTRIP_SERVER_WIFI_CONNECTED");
      break;
    case NTRIP_SERVER_WAIT_GNSS_DATA:
      Serial.println("NTRIP_SERVER_WAIT_GNSS_DATA");
      break;
    case NTRIP_SERVER_CONNECTING:
      Serial.println("NTRIP_SERVER_CONNECTING");
      break;
    case NTRIP_SERVER_AUTHORIZATION:
      Serial.println("NTRIP_SERVER_AUTHORIZATION");
      break;
    case NTRIP_SERVER_CASTING:
      Serial.println("NTRIP_SERVER_CASTING");
      break;
  }
}
#endif  // COMPILE_WIFI

//----------------------------------------
// Global NTRIP Server Routines
//----------------------------------------

//This function gets called as each RTCM byte comes in
void ntripServerProcessRTCM(uint8_t incoming)
{
#ifdef  COMPILE_WIFI

  if (ntripServerState == NTRIP_SERVER_CASTING)
  {
    //Generate and print timestamp if needed
    uint32_t currentMilliseconds;
    static uint32_t previousMilliseconds = 0;
    if (online.rtc)
    {
      //Timestamp the RTCM messages
      currentMilliseconds = millis();
      if (settings.enablePrintNtripServerRtcm
          && (!settings.enableRtcmMessageChecking)
          && (!inMainMenu)
          && ((currentMilliseconds - previousMilliseconds) > 5))
      {
        printTimeStamp();
        //         1         2         3
        //123456789012345678901234567890
        //YYYY-mm-dd HH:MM:SS.xxxrn0
        struct tm timeinfo = rtc.getTimeStruct();
        char timestamp[30];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("    Tx RTCM: %s.%03ld\r\n", timestamp, rtc.getMillis());
      }
      previousMilliseconds = currentMilliseconds;
    }

    //If we have not gotten new RTCM bytes for a period of time, assume end of frame
    if (millis() - ntripServerTimer > 100 && ntripServerBytesSent > 0)
    {
      if (!inMainMenu) log_d("NTRIP Server transmitted %d RTCM bytes to Caster", ntripServerBytesSent);
      ntripServerBytesSent = 0;
    }

    ntripServer->write(incoming); //Send this byte to socket
    ntripServerBytesSent++;
    ntripServerTimer = millis();
    wifiOutgoingRTCM = true;
  }

  //Indicate that the GNSS is providing correction data
  else if (ntripServerState == NTRIP_SERVER_WAIT_GNSS_DATA)
  {
    ntripServerSetState(NTRIP_SERVER_CONNECTING);
    rtcmParsingState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;
  }
#endif  //COMPILE_WIFI
}

//Start the NTRIP server
void ntripServerStart()
{
#ifdef  COMPILE_WIFI
  //Stop NTRIP server and WiFi
  ntripServerStop(true); //Don't allocate new wifiClient

  //Start the NTRIP server if enabled
  if ((settings.ntripServer_StartAtSurveyIn == true)
      || (settings.enableNtripServer == true))
  {
    //Display the heap state
    reportHeapNow();

    //Allocate the ntripServer structure
    ntripServer = new WiFiClient();

    //Restart WiFi and the NTRIP server if possible
    if (ntripServer)
      ntripServerSetState(NTRIP_SERVER_ON);
  }

  ntripServerConnectionAttempts = 0;
#endif  //COMPILE_WIFI
}

//Stop the NTRIP server
void ntripServerStop(bool wifiClientAllocated)
{
#ifdef  COMPILE_WIFI
  if (ntripServer)
  {
    //Break the NTRIP server connection if necessary
    if (ntripServer->connected())
      ntripServer->stop();

    //Free the NTRIP server resources
    delete ntripServer;
    ntripServer = NULL;

    //Allocate the NTRIP server structure if not done
    if (wifiClientAllocated == false)
      ntripServer = new WiFiClient();
  }

  //Stop WiFi if in use
  if (ntripServerState > NTRIP_SERVER_ON)
  {
    wifiStop();

    ntripServerLastConnectionAttempt = millis(); //Mark the Server stop so that we don't immediately attempt re-connect to Caster
    ntripServerConnectionAttemptTimeout = 15 * 1000L; //Wait 15s between stopping and the first re-connection attempt. 5 is too short for Emlid.
  }

  //Determine the next NTRIP server state
  ntripServerSetState((ntripServer && (wifiClientAllocated == false)) ? NTRIP_SERVER_ON : NTRIP_SERVER_OFF);

  online.ntripServer = false;
#endif  //COMPILE_WIFI
}

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

  //If user turns off NTRIP Server via settings, stop server
  if (settings.enableNtripServer == false && ntripServerState > NTRIP_SERVER_OFF)
    ntripServerStop(true);  //Don't allocate new wifiClient

  //Enable WiFi and the NTRIP server if requested
  switch (ntripServerState)
  {
    case NTRIP_SERVER_OFF:
      break;

    //Start WiFi
    case NTRIP_SERVER_ON:
      //Pause until connection timeout has passed
      if (millis() - ntripServerLastConnectionAttempt > ntripServerConnectionAttemptTimeout)
      {
        ntripServerLastConnectionAttempt = millis();
        wifiStart(settings.ntripServer_wifiSSID, settings.ntripServer_wifiPW);
        ntripServerSetState(NTRIP_SERVER_WIFI_CONNECTING);
      }
      else
      {
        if (millis() - ntripServerTimeoutPrint > 1000)
        {
          ntripServerTimeoutPrint = millis();
          Serial.printf("NTRIP Server connection timeout wait: %d of %d seconds \n\r",
                        (millis() - ntripServerLastConnectionAttempt) / 1000,
                        ntripServerConnectionAttemptTimeout / 1000
                       );
        }
      }
      break;

    //Wait for connection to an access point
    case NTRIP_SERVER_WIFI_CONNECTING:
      if (!wifiIsConnected())
      {
        //Stop if SSID is not detected
        if (wifiGetStatus() == WL_NO_SSID_AVAIL)
        {
          Serial.printf("WiFi network '%s' not found\n\r", settings.ntripServer_wifiSSID);

          wifiStop();

          paintNtripWiFiFail(4000, false); //True = 'Client', False = 'Server'
          
          ntripServerStop(true); //Do not allocate new wifiClient
        }
        else if (wifiConnectionTimeout())
        {
          //Assume AP weak signal, the AP is unable to respond successfully
          if (ntripServerConnectLimitReached())
          {
            Serial.println("NTRIP Server failed to get WiFi. Are your WiFi credentials correct?");

            //Display the WiFi failure
            paintNtripWiFiFail(4000, false);
          }
        }
      }
      else
      {
        //WiFi connection established
        ntripServerSetState(NTRIP_SERVER_WIFI_CONNECTED);

        // Start the SD card server
        // sdCardServerBegin(&server, true, true);
      }
      break;

    //WiFi connected to an access point
    case NTRIP_SERVER_WIFI_CONNECTED:
      if (settings.enableNtripServer)
      {
        //No RTCM correction data sent yet
        rtcmPacketsSent = 0;

        //Open socket to NTRIP caster
        ntripServerSetState(NTRIP_SERVER_WAIT_GNSS_DATA);
      }
      break;

    //Wait for GNSS correction data
    case NTRIP_SERVER_WAIT_GNSS_DATA:
      //State change handled in ntripServerProcessRTCM
      break;

    //Initiate the connection to the NTRIP caster
    case NTRIP_SERVER_CONNECTING:
      //Attempt a connection to the NTRIP caster
      if (!ntripServerConnectCaster())
      {
        log_d("NTRIP Server caster failed to connect. Trying again.");

        //Assume service not available
        if (ntripServerConnectLimitReached())
        {
          Serial.println("NTRIP Server failed to connect! Do you have your caster address and port correct?");
        }
      }
      else
      {
        //Connection open to NTRIP caster, wait for the authorization response
        ntripServerTimer = millis();
        ntripServerSetState(NTRIP_SERVER_AUTHORIZATION);
      }
      break;

    //Wait for authorization response
    case NTRIP_SERVER_AUTHORIZATION:
      //Check if caster service responded
      if (ntripServer->available() < strlen("ICY 200 OK")) //Wait until at least a few bytes have arrived
      {
        //Check for response timeout
        if (millis() - ntripServerTimer > 5000)
        {
          //NTRIP web service did not respone
          if (ntripServerConnectLimitReached())
          {
            Serial.println("Caster failed to respond. Do you have your caster address and port correct?");
          }
        }
      }
      else
      {
        //NTRIP caster's authorization response received
        char response[512];
        ntripServerResponse(response, sizeof(response));

        //Look for '200 OK'
        if (strstr(response, "200") == NULL)
        {
          //Look for '401 Unauthorized'
          Serial.printf("NTRIP Server caster responded with bad news: %s. Are you sure your caster credentials are correct?\n\r", response);

          ntripServerStop(true); //Don't allocate new wifiClient
        }
        else
        {
          Serial.printf("NTRIP Server connected to %s:%d %s\r\n",
                        settings.ntripServer_CasterHost,
                        settings.ntripServer_CasterPort,
                        settings.ntripServer_MountPoint);

          //Connection is now open, start the RTCM correction data timer
          ntripServerTimer = millis();

          //We don't use a task because we use I2C hardware (and don't have a semphore).
          online.ntripServer = true;
          ntripServerConnectionAttempts = 0;
          ntripServerSetState(NTRIP_SERVER_CASTING);
        }
      }
      break;

    //NTRIP server authorized to send RTCM correction data to NTRIP caster
    case NTRIP_SERVER_CASTING:
      //Check for a broken connection
      if (!ntripServer->connected())
      {
        //Broken connection, retry the NTRIP server connection
        Serial.println("Connection to NTRIP Server was closed");
        ntripServerStop(false); //Allocate new wifiClient
      }
      else if ((millis() - ntripServerTimer) > (3 * 1000))
      {
        //GNSS stopped sending RTCM correction data
        Serial.println("NTRIP Server breaking caster connection due to lack of RTCM data!");
        ntripServerStop(false); //Allocate new wifiClient
      }
      else
      {
        //All is well
        cyclePositionLEDs();
      }

      break;
  }
#endif  //COMPILE_WIFI
}
