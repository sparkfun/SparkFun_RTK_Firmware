/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  NTRIP Server States:
    NTRIP_SERVER_OFF: WiFi off or using NTRIP Client
    NTRIP_SERVER_ON: WIFI_START state
    NTRIP_SERVER_WIFI_ETHERNET_STARTED: Connecting to WiFi access point
    NTRIP_SERVER_WIFI_ETHERNET_CONNECTED: WiFi connected to an access point
    NTRIP_SERVER_WAIT_GNSS_DATA: Waiting for correction data from GNSS
    NTRIP_SERVER_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_SERVER_AUTHORIZATION: Validate the credentials
    NTRIP_SERVER_CASTING: Sending correction data to the NTRIP caster

                               NTRIP_SERVER_OFF
                                       |   ^
                      ntripServerStart |   | ntripServerStop(true)
                                       v   |
                    .---------> NTRIP_SERVER_ON <-------------------.
                    |                  |                            |
                    |                  |                            | ntripServerStop(false)
                    |                  v                      Fail  |
                    |    NTRIP_SERVER_WIFI_ETHERNET_STARTED ------->+
                    |                  |                            ^
                    |                  |                            |
                    |                  v                      Fail  |
                    |    NTRIP_SERVER_WIFI_ETHERNET_CONNECTED ----->+
                    |                  |                            ^
                    |                  |                      WiFi  |
                    |                  v                      Fail  |
                    |    NTRIP_SERVER_WAIT_GNSS_DATA -------------->+
                    |                  |                            ^
                    |                  | Discard Data         WiFi  |
                    |                  v                      Fail  |
                    |      NTRIP_SERVER_CONNECTING ---------------->+
                    |                  |                            ^
                    |                  | Discard Data         WiFi  |
                    |                  v                      Fail  |
                    |     NTRIP_SERVER_AUTHORIZATION -------------->+
                    |                  |                            ^
                    |                  | Discard Data         WiFi  |
                    |                  v                      Fail  |
                    |         NTRIP_SERVER_CASTING -----------------'
                    |                  |
                    |                  | Data timeout
                    |                  |
                    |                  | Close Server connection
                    '------------------'

  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
//Constants - compiled out
//----------------------------------------

#ifdef COMPILE_WIFI

//Give up connecting after this number of attempts
//Connection attempts are throttled to increase the time between attempts
//30 attempts with 5 minute increases will take over 38 hours
static const int MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS = 30;

//----------------------------------------
//Locals - compiled out
//----------------------------------------

//WiFi connection used to push RTCM to NTRIP caster over WiFi
static NTRIPClient * ntripServer;

//Count of bytes sent by the NTRIP server to the NTRIP caster
uint32_t ntripServerBytesSent = 0;

//Throttle the time between connection attempts
//ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
static uint32_t ntripServerConnectionAttemptTimeout = 0;
static uint32_t ntripServerLastConnectionAttempt = 0;

//Last time the NTRIP server state was displayed
static uint32_t ntripServerStateLastDisplayed = 0;

//----------------------------------------
//NTRIP Server Routines - compiled out
//----------------------------------------

//Initiate a connection to the NTRIP caster
bool ntripServerConnectCaster()
{
  const int SERVER_BUFFER_SIZE = 512;
  char serverBuffer[SERVER_BUFFER_SIZE];

  //Remove any http:// or https:// prefix from host name
  char hostname[51];
  strncpy(hostname, settings.ntripServer_CasterHost, sizeof(hostname) - 1); //strtok modifies string to be parsed so we create a copy
  char *token = strtok(hostname, "//");
  if (token != nullptr)
  {
    token = strtok(nullptr, "//"); //Advance to data after //
    if (token != nullptr)
      strcpy(settings.ntripServer_CasterHost, token);
  }

  systemPrintf("NTRIP Server connecting to %s:%d\r\n", settings.ntripServer_CasterHost,
               settings.ntripServer_CasterPort);

  //Attempt a connection to the NTRIP caster
  if (!ntripServer->connect(settings.ntripServer_CasterHost,
                            settings.ntripServer_CasterPort))
    return false;

  systemPrintln("NTRIP Server connected");

  //Build the authorization credentials message
  // * Mount point
  // * Password
  // * Agent
  snprintf(serverBuffer, SERVER_BUFFER_SIZE,
           "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/v%d.%d\r\n\r\n",
           settings.ntripServer_MountPointPW,
           settings.ntripServer_MountPoint,
           platformPrefix,
           FIRMWARE_VERSION_MAJOR,
           FIRMWARE_VERSION_MINOR);

  //Send the authorization credentials to the NTRIP caster
  ntripServer->write((const uint8_t *)serverBuffer, strlen(serverBuffer));
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

  ntripServerConnectionAttemptsTotal++;

  if (limitReached == false)
  {
    ntripServerConnectionAttemptTimeout = ntripServerConnectionAttempts * 5 * 60 * 1000L; //Wait 5, 10, 15, etc minutes between attempts

    reportHeapNow();
  }
  else
  {
    //No more connection attempts
    systemPrintln("NTRIP Server connection attempts exceeded!");
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

  //Read bytes from the caster and store them
  while ((response < responseEnd) && ntripServer->available())
    *response++ = ntripServer->read();

  //Zero terminate the response
  *response = '\0';
}

//Update the state of the NTRIP server state machine
void ntripServerSetState(byte newState)
{
  if (ntripServerState == newState)
    systemPrint("*");
  ntripServerState = newState;
  switch (newState)
  {
    default:
      systemPrintf("Unknown NTRIP Server state: %d\r\n", newState);
      break;
    case NTRIP_SERVER_OFF:
      systemPrintln("NTRIP_SERVER_OFF");
      break;
    case NTRIP_SERVER_ON:
      systemPrintln("NTRIP_SERVER_ON");
      break;
    case NTRIP_SERVER_WIFI_ETHERNET_STARTED:
      systemPrintln("NTRIP_SERVER_WIFI_ETHERNET_STARTED");
      break;
    case NTRIP_SERVER_WIFI_ETHERNET_CONNECTED:
      systemPrintln("NTRIP_SERVER_WIFI_ETHERNET_CONNECTED");
      break;
    case NTRIP_SERVER_WAIT_GNSS_DATA:
      systemPrintln("NTRIP_SERVER_WAIT_GNSS_DATA");
      break;
    case NTRIP_SERVER_CONNECTING:
      systemPrintln("NTRIP_SERVER_CONNECTING");
      break;
    case NTRIP_SERVER_AUTHORIZATION:
      systemPrintln("NTRIP_SERVER_AUTHORIZATION");
      break;
    case NTRIP_SERVER_CASTING:
      systemPrintln("NTRIP_SERVER_CASTING");
      break;
  }
}
#endif  //COMPILE_WIFI

//----------------------------------------
//Global NTRIP Server Routines
//----------------------------------------

//This function gets called as each RTCM byte comes in
void ntripServerProcessRTCM(uint8_t incoming)
{
#ifdef COMPILE_WIFI

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
        systemPrintf("    Tx RTCM: %s.%03ld\r\n", timestamp, rtc.getMillis());
      }
      previousMilliseconds = currentMilliseconds;
    }

    //If we have not gotten new RTCM bytes for a period of time, assume end of frame
    if (millis() - ntripServerTimer > 100 && ntripServerBytesSent > 0)
    {
      if (!inMainMenu && settings.enablePrintNtripServerState)
        systemPrintf("NTRIP Server transmitted %d RTCM bytes to Caster\r\n", ntripServerBytesSent);

      ntripServerBytesSent = 0;
    }

    if (ntripServer->connected())
    {
      ntripServer->write(incoming); //Send this byte to socket
      ntripServerBytesSent++;
      ntripServerTimer = millis();
      wifiOutgoingRTCM = true;
    }
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
#ifdef COMPILE_WIFI
  //Stop NTRIP server and WiFi
  ntripServerStop(true); //Don't allocate new wifiClient

  //Start the NTRIP server if enabled
  if ((settings.ntripServer_StartAtSurveyIn == true)
      || (settings.enableNtripServer == true))
  {
    //Display the heap state
    reportHeapNow();

    //Allocate the ntripServer structure
    ntripServer = new NTRIPClient();

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
#ifdef COMPILE_WIFI
  if (ntripServer)
  {
    //Break the NTRIP server connection if necessary
    if (ntripServer->connected())
      ntripServer->stop();

    //Free the NTRIP server resources
    delete ntripServer;
    ntripServer = nullptr;

    //Allocate the NTRIP server structure if not done
    if (wifiClientAllocated == false)
      ntripServer = new NTRIPClient();
  }

  //Increase timeouts if we started WiFi
  if (ntripServerState > NTRIP_SERVER_ON)
  {
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
  // Skip if in configure-via-ethernet mode
  if (configureViaEthernet)
  {
    //log_d("configureViaEthernet: skipping ntripServerUpdate");
    return;
  }
    
  if (settings.enableNtripServer == false)
  {
    //If user turns off NTRIP Server via settings, stop server
    if (ntripServerState > NTRIP_SERVER_OFF)
      ntripServerStop(true);  //Don't allocate new wifiClient
    return;
  }

  if (wifiInConfigMode()) return; //Do not service NTRIP during WiFi config

#ifdef COMPILE_WIFI
  //Periodically display the NTRIP server state
  if (settings.enablePrintNtripServerState && ((millis() - ntripServerStateLastDisplayed) > 15000))
  {
    ntripServerSetState(ntripServerState);
    ntripServerStateLastDisplayed = millis();
  }

  //Enable WiFi and the NTRIP server if requested
  switch (ntripServerState)
  {
    case NTRIP_SERVER_OFF:
      break;

    //Start WiFi
    case NTRIP_SERVER_ON:
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
      {
        if (online.ethernetStatus < ETH_BEGUN_NO_LINK)
        {
          systemPrintln("Error: Please connect Ethernet before starting NTRIP Server");
          ntripServerStop(true); //Do not allocate new wifiClient
        }
        else
        {
          //Pause until connection timeout has passed
          if (millis() - ntripServerLastConnectionAttempt > ntripServerConnectionAttemptTimeout)
          {
            ntripServerLastConnectionAttempt = millis();
            log_d("NTRIP Server starting on Ethernet");
            ntripServerSetState(NTRIP_SERVER_WIFI_ETHERNET_STARTED);
          }
        }
      }
      else
#endif
      {
        if (wifiNetworkCount() == 0)
        {
          systemPrintln("Error: Please enter at least one SSID before starting NTRIP Server");
          ntripServerStop(true); //Do not allocate new wifiClient
        }
        else
        {
          //Pause until connection timeout has passed
          if (millis() - ntripServerLastConnectionAttempt > ntripServerConnectionAttemptTimeout)
          {
            ntripServerLastConnectionAttempt = millis();
            log_d("NTRIP Server starting WiFi");
            wifiStart();
            ntripServerSetState(NTRIP_SERVER_WIFI_ETHERNET_STARTED);
          }
        }
      }
      break;

    //Wait for connection to an access point
    case NTRIP_SERVER_WIFI_ETHERNET_STARTED:
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
      {
        if (online.ethernetStatus == ETH_LINK)
          ntripServerSetState(NTRIP_SERVER_WIFI_ETHERNET_CONNECTED);
        else if (online.ethernetStatus >= ETH_BEGUN_NO_LINK)
          ntripServerSetState(NTRIP_SERVER_OFF);
      }
      else
#endif
      {
        if (wifiIsConnected())
          ntripServerSetState(NTRIP_SERVER_WIFI_ETHERNET_CONNECTED);
        else if (wifiState == WIFI_OFF)
          ntripServerSetState(NTRIP_SERVER_OFF);
      }
      break;

    //WiFi connected to an access point
    case NTRIP_SERVER_WIFI_ETHERNET_CONNECTED:
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
        if (ntripServerConnectionAttemptTimeout / 1000 < 120)
          systemPrintf("NTRIP Server failed to connect to caster. Trying again in %d seconds.\r\n", ntripServerConnectionAttemptTimeout / 1000);
        else
          systemPrintf("NTRIP Server failed to connect to caster. Trying again in %d minutes.\r\n", ntripServerConnectionAttemptTimeout / 1000 / 60);

        //Assume service not available
        if (ntripServerConnectLimitReached())
        {
          systemPrintln("NTRIP Server failed to connect! Do you have your caster address and port correct?");
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
        if (millis() - ntripServerTimer > 10000)
        {
          if (ntripServerConnectLimitReached())
          {
            systemPrintln("Caster failed to respond. Do you have your caster address and port correct?");
          }
          else
          {
            if (ntripServerConnectionAttemptTimeout / 1000 < 120)
              systemPrintf("NTRIP caster failed to respond. Trying again in %d seconds.\r\n", ntripServerConnectionAttemptTimeout / 1000);
            else
              systemPrintf("NTRIP caster failed to respond. Trying again in %d minutes.\r\n", ntripServerConnectionAttemptTimeout / 1000 / 60);
          }
        }
      }
      else
      {
        //NTRIP caster's authorization response received
        char response[512];
        ntripServerResponse(response, sizeof(response));

        //Look for various responses
        if (strstr(response, "401") != nullptr)
        {
          //Look for '401 Unauthorized'
          systemPrintf("NTRIP Caster responded with bad news: %s. Are you sure your caster credentials are correct?\r\n", response);

          //Give up - Stop WiFi operations
          ntripServerStop(true); //Do not allocate new wifiClient
        }
        else if (strstr(response, "banned") != nullptr) //'Banned' found
        {
          //Look for 'HTTP/1.1 200 OK' and banned IP information
          systemPrintf("NTRIP Server connected to caster but caster responded with problem: %s", response);

          //Give up - Stop WiFi operations
          ntripServerStop(true); //Do not allocate new wifiClient
        }
        else if (strstr(response, "200") == nullptr) //'200' not found
        {
          //Look for 'ERROR - Mountpoint taken' from Emlid.
          systemPrintf("NTRIP Server connected but caster responded with problem: %s", response);

          //Attempt to reconnect after throttle controlled timeout
          if (ntripServerConnectLimitReached())
          {
            systemPrintln("Caster failed to respond. Do you have your caster address and port correct?");
          }
        }
        else if (strstr(response, "200") != nullptr) //'200' found

        {
          systemPrintf("NTRIP Server connected to %s:%d %s\r\n",
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
        //Broken connection, retry the NTRIP connection
        systemPrintln("Connection to NTRIP Caster was lost");
        ntripServerStop(false); //Allocate new wifiClient
      }
      else if ((millis() - ntripServerTimer) > (3 * 1000))
      {
        //GNSS stopped sending RTCM correction data
        systemPrintln("NTRIP Server breaking connection to caster due to lack of RTCM data!");
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
