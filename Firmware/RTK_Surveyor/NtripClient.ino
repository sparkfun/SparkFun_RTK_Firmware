/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  NTRIP Client States:
    NTRIP_CLIENT_OFF: WiFi off or or NTRIP server
    NTRIP_CLIENT_ON: WIFI_ON state
    NTRIP_CLIENT_WIFI_CONNECTING: Connecting to WiFi access point
    NTRIP_CLIENT_WIFI_CONNECTED: WiFi connected to an access point
    NTRIP_CLIENT_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_CONNECTED: Connected to the NTRIP caster

                               NTRIP_CLIENT_OFF
                                       |   ^
                      ntripClientStart |   | ntripClientStop(true)
                                       v   |
                               NTRIP_CLIENT_ON <--------------.
                                       |                      |
                                       |                      | ntripClientStop(false)
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

#ifdef COMPILE_WIFI

//Size of the credentials buffer in bytes
static const int CREDENTIALS_BUFFER_SIZE = 512;

//Give up connecting after this number of attempts
//Connection attempts are throttled to increase the time between attempts
//30 attempts with 15 second increases will take almost two hours
static const int MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS = 30;

//NTRIP caster response timeout
static const uint32_t NTRIP_CLIENT_RESPONSE_TIMEOUT = 10 * 1000; //Milliseconds

//NTRIP client receive data timeout
static const uint32_t NTRIP_CLIENT_RECEIVE_DATA_TIMEOUT = 30 * 1000; //Milliseconds

//Most incoming data is around 500 bytes but may be larger
static const int RTCM_DATA_SIZE = 512 * 4;

//NTRIP client server request buffer size
static const int SERVER_BUFFER_SIZE = CREDENTIALS_BUFFER_SIZE + 3;

static const int NTRIPCLIENT_MS_BETWEEN_GGA = 5000; //5s between transmission of GGA messages, if enabled

//----------------------------------------
// Locals - compiled out
//----------------------------------------

//The WiFi connection to the NTRIP caster to obtain RTCM data.
static WiFiClient * ntripClient;

//Throttle the time between connection attempts
static int ntripClientConnectionAttemptTimeout = 0;
static uint32_t ntripClientLastConnectionAttempt = 0;
static uint32_t ntripClientTimeoutPrint = 0;

//Last time the NTRIP client state was displayed
static uint32_t lastNtripClientState = 0;

//Throttle GGA transmission to Caster to 1 report every 5 seconds
unsigned long lastGGAPush = 0;

//----------------------------------------
// NTRIP Client Routines - compiled out
//----------------------------------------

bool ntripClientConnect()
{
  if (!ntripClient)
    return false;

  //Remove any http:// or https:// prefix from host name
  char hostname[50];
  strncpy(hostname, settings.ntripClient_CasterHost, 50); //strtok modifies string to be parsed so we create a copy
  char *token = strtok(hostname, "//");
  if (token != NULL)
  {
    token = strtok(NULL, "//"); //Advance to data after //
    if (token != NULL)
      strcpy(settings.ntripClient_CasterHost, token);
  }

  Serial.printf("NTRIP Client connecting to %s:%d\r\n", settings.ntripClient_CasterHost, settings.ntripClient_CasterPort);

  int connectResponse = ntripClient->connect(settings.ntripClient_CasterHost, settings.ntripClient_CasterPort);

  if (connectResponse < 1)
    return false;

  Serial.println("NTRIP Client connected");

  // Set up the server request (GET)
  char serverRequest[SERVER_BUFFER_SIZE];
  snprintf(serverRequest,
           SERVER_BUFFER_SIZE,
           "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun_RTK_%s_v%d.%d\r\n",
           settings.ntripClient_MountPoint, platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

  // Set up the credentials
  char credentials[CREDENTIALS_BUFFER_SIZE];
  if (strlen(settings.ntripClient_CasterUser) == 0)
  {
    strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", sizeof(credentials));
  }
  else
  {
    //Pass base64 encoded user:pw
    char userCredentials[sizeof(settings.ntripClient_CasterUser) + sizeof(settings.ntripClient_CasterUserPW) + 1]; //The ':' takes up a spot
    snprintf(userCredentials, sizeof(userCredentials), "%s:%s", settings.ntripClient_CasterUser, settings.ntripClient_CasterUserPW);

    Serial.print("NTRIP Client sending credentials: ");
    Serial.println(userCredentials);

    //Encode with ESP32 built-in library
    base64 b;
    String strEncodedCredentials = b.encode(userCredentials);
    char encodedCredentials[strEncodedCredentials.length() + 1];
    strEncodedCredentials.toCharArray(encodedCredentials, sizeof(encodedCredentials)); //Convert String to char array

    snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
  }

  // Add the encoded credentials to the server request
  strncat(serverRequest, credentials, SERVER_BUFFER_SIZE - 1);
  strncat(serverRequest, "\r\n", SERVER_BUFFER_SIZE - 1);

  Serial.print("NTRIP Client serverRequest size: ");
  Serial.print(strlen(serverRequest));
  Serial.print(" of ");
  Serial.print(sizeof(serverRequest));
  Serial.println(" bytes available");

  // Send the server request
  Serial.println("NTRIP Client sending server request: ");
  Serial.println(serverRequest);
  ntripClient->write(serverRequest, strlen(serverRequest));
  ntripClientTimer = millis();
  return true;
}

//Determine if another connection is possible or if the limit has been reached
bool ntripClientConnectLimitReached()
{
  //Shutdown the NTRIP client
  ntripClientStop(false); //Allocate new wifiClient

  //Retry the connection a few times
  bool limitReached = false;
  if (ntripClientConnectionAttempts++ >= MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS) limitReached = true;

  ntripClientConnectionAttemptsTotal++;

  if (limitReached == false)
  {
    ntripClientConnectionAttemptTimeout = ntripClientConnectionAttempts * 15 * 1000L; //Wait 15, 30, 45, etc seconds between attempts

    log_d("ntripClientConnectionAttemptTimeout increased to %d minutes", ntripClientConnectionAttemptTimeout / (60 * 1000L));

    reportHeapNow();
  }
  else
  {
    //No more connection attempts, switching to Bluetooth
    Serial.println("NTRIP Client connection attempts exceeded!");

    //Stop WiFi operations
    ntripClientStop(true); //Do not allocate new wifiClient
  }
  return limitReached;
}

//Determine if NTRIP client data is available
int ntripClientReceiveDataAvailable()
{
  return ntripClient->available();
}

//Read the response from the NTRIP client
void ntripClientResponse(char * response, size_t maxLength)
{
  char * responseEnd;

  //Make sure that we can zero terminate the response
  responseEnd = &response[maxLength - 1];

  // Read bytes from the caster and store them
  while ((response < responseEnd) && ntripClientReceiveDataAvailable())
    *response++ = ntripClient->read();

  // Zero terminate the response
  *response = '\0';
}

//Update the state of the NTRIP client state machine
void ntripClientSetState(byte newState)
{
  if (ntripClientState == newState)
    Serial.print("*");
  ntripClientState = newState;
  switch (newState)
  {
    default:
      Serial.printf("Unknown NTRIP Client state: %d\r\n", newState);
      break;
    case NTRIP_CLIENT_OFF:
      Serial.println("NTRIP_CLIENT_OFF");
      break;
    case NTRIP_CLIENT_ON:
      Serial.println("NTRIP_CLIENT_ON");
      break;
    case NTRIP_CLIENT_WIFI_CONNECTING:
      Serial.println("NTRIP_CLIENT_WIFI_CONNECTING");
      break;
    case NTRIP_CLIENT_WIFI_CONNECTED:
      Serial.println("NTRIP_CLIENT_WIFI_CONNECTED");
      break;
    case NTRIP_CLIENT_CONNECTING:
      Serial.println("NTRIP_CLIENT_CONNECTING");
      break;
    case NTRIP_CLIENT_CONNECTED:
      Serial.println("NTRIP_CLIENT_CONNECTED");
      break;
  }
}

#endif

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

void ntripClientStart()
{
#ifdef COMPILE_WIFI
  //Stop NTRIP client and WiFi
  ntripClientStop(true); //Do not allocate new wifiClient

  //Start the NTRIP client if enabled
  if (settings.enableNtripClient == true)
  {
    //Display the heap state
    reportHeapNow();
    Serial.println("NTRIP Client start");

    //Allocate the ntripClient structure
    ntripClient = new WiFiClient();

    //Startup WiFi and the NTRIP client
    if (ntripClient)
      ntripClientSetState(NTRIP_CLIENT_ON);
  }

  ntripClientConnectionAttempts = 0;
#endif  //COMPILE_WIFI
}

//Stop the NTRIP client
void ntripClientStop(bool wifiClientAllocated)
{
#ifdef COMPILE_WIFI
  if (ntripClient)
  {
    //Break the NTRIP client connection if necessary
    if (ntripClient->connected())
      ntripClient->stop();

    //Free the NTRIP client resources
    delete ntripClient;
    ntripClient = NULL;

    //Allocate the NTRIP client structure if not done
    if (wifiClientAllocated == false)
      ntripClient = new WiFiClient();
  }

  //Stop WiFi if in use
  if (ntripClientState > NTRIP_CLIENT_ON)
  {
    wifiStop();

    ntripClientLastConnectionAttempt = millis(); //Mark the Client stop so that we don't immediately attempt re-connect to Caster
    ntripClientConnectionAttemptTimeout = 15 * 1000L; //Wait 15s between stopping and the first re-connection attempt.
  }

  // Return the Main Talker ID to "GN".
  i2cGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GN);
  i2cGNSS.setNMEAGPGGAcallbackPtr(NULL); // Remove callback

  //Determine the next NTRIP client state
  ntripClientSetState((ntripClient && (wifiClientAllocated == false)) ? NTRIP_CLIENT_ON : NTRIP_CLIENT_OFF);
  online.ntripClient = false;
  wifiIncomingRTCM = false;
#endif  //COMPILE_WIFI
}

//Check for the arrival of any correction data. Push it to the GNSS.
//Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
#ifdef COMPILE_WIFI
  //Periodically display the NTRIP client state
  if (settings.enablePrintNtripClientState && ((millis() - lastNtripClientState) > 15000))
  {
    ntripClientSetState (ntripClientState);
    lastNtripClientState = millis();
  }

  //Enable WiFi and the NTRIP client if requested
  switch (ntripClientState)
  {
    case NTRIP_CLIENT_OFF:
      break;

    //Start WiFi
    case NTRIP_CLIENT_ON:
      if (strlen(settings.ntripClient_wifiSSID) == 0)
      {
        Serial.println("Error: Please enter SSID before starting NTRIP Client");
        ntripClientSetState(NTRIP_CLIENT_OFF);
      }
      else
      {
        //Pause until connection timeout has passed
        if (millis() - ntripClientLastConnectionAttempt > ntripClientConnectionAttemptTimeout)
        {
          ntripClientLastConnectionAttempt = millis();
          wifiStart(settings.ntripClient_wifiSSID, settings.ntripClient_wifiPW);
          ntripClientSetState(NTRIP_CLIENT_WIFI_CONNECTING);
        }
        else
        {
          if (millis() - ntripClientTimeoutPrint > 1000)
          {
            ntripClientTimeoutPrint = millis();
            Serial.printf("NTRIP Client connection timeout wait: %ld of %d seconds \r\n",
                          (millis() - ntripClientLastConnectionAttempt) / 1000,
                          ntripClientConnectionAttemptTimeout / 1000
                         );
          }
        }

      }
      break;

    case NTRIP_CLIENT_WIFI_CONNECTING:
      if (!wifiIsConnected())
      {
        //Throttle if SSID is not detected
        if (wifiConnectionTimeout() || wifiGetStatus() == WL_NO_SSID_AVAIL)
        {
          if (wifiGetStatus() == WL_NO_SSID_AVAIL)
            Serial.printf("WiFi network '%s' not found\r\n", settings.ntripClient_wifiSSID);

          if (ntripClientConnectLimitReached()) //Stop WiFi, give up
            paintNtripWiFiFail(4000, true);
        }
      }
      else
      {
        //WiFi connection established
        ntripClientSetState(NTRIP_CLIENT_WIFI_CONNECTED);
      }
      break;

    case NTRIP_CLIENT_WIFI_CONNECTED:
      {
        //If GGA transmission is enabled, wait for GNSS lock before connecting to NTRIP Caster
        //If GGA transmission is not enabled, start connecting to NTRIP Caster
        if ( (settings.ntripClient_TransmitGGA == true && (fixType == 3 || fixType == 4 || fixType == 5))
             || settings.ntripClient_TransmitGGA == false)
        {
          //Open connection to caster service
          if (!ntripClientConnect())
          {
            log_d("NTRIP Client caster failed to connect. Trying again.");

            //Assume service not available
            if (ntripClientConnectLimitReached())
              Serial.println("NTRIP Client caster failed to connect. Do you have your caster address and port correct?");
          }
          else
            //Socket opened to NTRIP system
            ntripClientSetState(NTRIP_CLIENT_CONNECTING);
        }
        else
        {
          log_d("Waiting for Fix");
        }
      }
      break;

    case NTRIP_CLIENT_CONNECTING:
      //Check for no response from the caster service
      if (ntripClientReceiveDataAvailable() < strlen("ICY 200 OK")) //Wait until at least a few bytes have arrived
      {
        //Check for response timeout
        if (millis() - ntripClientTimer > NTRIP_CLIENT_RESPONSE_TIMEOUT)
        {
          //NTRIP web service did not respone
          if (ntripClientConnectLimitReached())
            Serial.println("NTRIP Client caster failed to respond. Do you have your caster address and port correct?");
        }
      }
      else
      {
        // Caster web service responsed
        char response[512];
        ntripClientResponse(&response[0], sizeof(response));

        log_d("Caster Response: %s", response);

        //Look for various responses
        if (strstr(response, "401") != NULL)
        {
          //Look for '401 Unauthorized'
          Serial.printf("NTRIP Caster responded with bad news: %s. Are you sure your caster credentials are correct?\r\n", response);

          //Stop WiFi operations
          ntripClientStop(true); //Do not allocate new wifiClient
        }
        else if (strstr(response, "banned") != NULL)
        {
          //Look for 'HTTP/1.1 200 OK' and banned IP information
          Serial.printf("NTRIP Client connected to caster but caster reponded with problem: %s", response);

          //Stop WiFi operations
          ntripClientStop(true); //Do not allocate new wifiClient
        }
        else if (strstr(response, "200") != NULL)
        {
          log_d("NTRIP Client connected to caster");

          //Connection is now open, start the NTRIP receive data timer
          ntripClientTimer = millis();

          if (settings.ntripClient_TransmitGGA == true)
          {
            // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
            i2cGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);
            i2cGNSS.setNMEAGPGGAcallbackPtr(&pushGPGGA); // Set up the callback for GPGGA

            float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;
            if (measurementFrequency < 0.2) measurementFrequency = 0.2; //0.2Hz * 5 = 1 measurement every 5 seconds
            i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, measurementFrequency * 5); // Enable GGA over I2C. Tell the module to output GGA every 5 seconds

            log_d("Adjusting GGA setting to %f", measurementFrequency);

            i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, measurementFrequency); // Enable GGA over I2C. Tell the module to output GGA every second

            lastGGAPush = millis() - NTRIPCLIENT_MS_BETWEEN_GGA; //Force immediate transmission of GGA message
          }

          //We don't use a task because we use I2C hardware (and don't have a semphore).
          online.ntripClient = true;
          ntripClientStartTime = millis();
          ntripClientConnectionAttempts = 0;
          ntripClientSetState(NTRIP_CLIENT_CONNECTED);
        }
      }
      break;

    case NTRIP_CLIENT_CONNECTED:
      //Check for a broken connection
      if (!ntripClient->connected())
      {
        //Broken connection, retry the NTRIP client connection
        Serial.println("NTRIP Client connection dropped");
        ntripClientStop(false); //Allocate new wifiClient
      }
      else
      {
        //Check for timeout receiving NTRIP data
        if (!ntripClient->available())
        {
          if ((millis() - ntripClientTimer) > NTRIP_CLIENT_RECEIVE_DATA_TIMEOUT)
          {
            //Timeout receiving NTRIP data, retry the NTRIP client connection
            Serial.println("NTRIP Client: No data received timeout");
            ntripClientStop(false); //Allocate new wifiClient
          }
        }
        else
        {
          //Receive data from the NTRIP Caster
          uint8_t rtcmData[RTCM_DATA_SIZE];
          size_t rtcmCount = 0;

          //Collect any available RTCM data
          while (ntripClient->available())
          {
            rtcmData[rtcmCount++] = ntripClient->read();
            if (rtcmCount == sizeof(rtcmData))
              break;
          }

          //Restart the NTRIP receive data timer
          ntripClientTimer = millis();

          //Push RTCM to GNSS module over I2C
          i2cGNSS.pushRawData(rtcmData, rtcmCount);
          wifiIncomingRTCM = true;

          if (!inMainMenu) log_d("NTRIP Client received %d RTCM bytes, pushed to ZED", rtcmCount);
        }
      }

      break;
  }
#endif  //COMPILE_WIFI
}

void pushGPGGA(NMEA_GGA_data_t *nmeaData)
{
#ifdef COMPILE_WIFI
  //Provide the caster with our current position as needed
  if (ntripClient->connected() == true && settings.ntripClient_TransmitGGA == true)
  {
    if (millis() - lastGGAPush > NTRIPCLIENT_MS_BETWEEN_GGA)
    {
      lastGGAPush = millis();

      log_d("Pushing GGA to server: %s", (const char *)nmeaData->nmea); // .nmea is printable (NULL-terminated) and already has \r\n on the end

      //Push our current GGA sentence to caster
      ntripClient->print((const char *)nmeaData->nmea);
    }
  }
#endif
}
