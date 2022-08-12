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

#ifdef  COMPILE_WIFI

//Size of the credentials buffer in bytes
static const int CREDENTIALS_BUFFER_SIZE = 512;

//Give up connecting after this number of attempts
static const int MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS = 3;

//NTRIP caster response timeout
static const uint32_t NTRIP_CLIENT_RESPONSE_TIMEOUT = 10 * 1000; //Milliseconds

//NTRIP client receive data timeout
static const uint32_t NTRIP_CLIENT_RECEIVE_DATA_TIMEOUT = 10 * 1000; //Milliseconds

//Most incoming data is around 500 bytes but may be larger
static const int RTCM_DATA_SIZE = 512 * 4;

//NTRIP client server request buffer size
static const int SERVER_BUFFER_SIZE = CREDENTIALS_BUFFER_SIZE + 3;

//----------------------------------------
// Locals - compiled out
//----------------------------------------

//The WiFi connection to the NTRIP caster to obtain RTCM data.
static WiFiClient * ntripClient;

//Count the number of connection attempts
static int ntripClientConnectionAttempts;

//NTRIP client timer usage:
//  * Measure the connection response time
//  * Receive NTRIP data timeout
static uint32_t ntripClientTimer;

//Last time the NTRIP client state was displayed
static uint32_t lastNtripClientState = 0;

//Throttle GGA transmission to Caster to 1 report every 5 seconds
unsigned long lastGGAPush = 0;

//----------------------------------------
// NTRIP Client Routines - compiled out
//----------------------------------------

void ntripClientAllowMoreConnections()
{
  ntripClientConnectionAttempts = 0;
}

bool ntripClientConnect()
{
  if ((!ntripClient)
      || (!ntripClient->connect(settings.ntripClient_CasterHost, settings.ntripClient_CasterPort)))
    return false;
  Serial.printf("NTRIP Client connected to %s:%d\n\r", settings.ntripClient_CasterHost, settings.ntripClient_CasterPort);

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
  ntripClientStop(false);

  //Retry the connection a few times
  bool limitReached = (ntripClientConnectionAttempts++ >= MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS);
  if (!limitReached)
    //Display the heap state
    reportHeapNow();
  else
  {
    //No more connection attempts, switching to Bluetooth
    Serial.println("NTRIP Client connection attempts exceeded!");
    ntripClientSwitchToBluetooth();
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

//Switch to Bluetooth operation
void ntripClientSwitchToBluetooth()
{
  Serial.println("NTRIP Client failure, switching to Bluetooth!");

  //Stop WiFi operations
  ntripClientStop(true);

  //Turn on Bluetooth with 'Rover' name
  bluetoothStart();
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
#ifdef  COMPILE_WIFI
  //Stop NTRIP client and WiFi
  ntripClientStop(true);

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

  //Only fallback to Bluetooth once, then try WiFi again.  This enables changes
  //to the WiFi SSID and password to properly restart the WiFi.
  ntripClientAllowMoreConnections();
#endif  //COMPILE_WIFI
}

//Stop the NTRIP client
void ntripClientStop(bool done)
{
#ifdef  COMPILE_WIFI
  if (ntripClient)
  {
    //Break the NTRIP client connection if necessary
    if (ntripClient->connected())
      ntripClient->stop();

    //Free the NTRIP client resources
    delete ntripClient;
    ntripClient = NULL;

    //Allocate the NTRIP client structure if not done
    if (!done)
      ntripClient = new WiFiClient();
  }

  //Stop WiFi if in use
  if (ntripClientState > NTRIP_CLIENT_ON)
    wifiStop();

  // Return the Main Talker ID to "GN".
  i2cGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GN);
  i2cGNSS.setNMEAGPGGAcallbackPtr(NULL); // Remove callback

  //Determine the next NTRIP client state
  ntripClientSetState((ntripClient && (!done)) ? NTRIP_CLIENT_ON : NTRIP_CLIENT_OFF);
  online.ntripClient = false;
  wifiIncomingRTCM = false;
#endif  //COMPILE_WIFI
}

//Check for the arrival of any correction data. Push it to the GNSS.
//Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
#ifdef  COMPILE_WIFI
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
      wifiStart(settings.ntripClient_wifiSSID, settings.ntripClient_wifiPW);
      ntripClientSetState(NTRIP_CLIENT_WIFI_CONNECTING);
      break;

    case NTRIP_CLIENT_WIFI_CONNECTING:
      if (!wifiIsConnected())
      {
        if (wifiConnectionTimeout())
        {
          //Assume AP weak signal, the AP is unable to respond successfully
          if (ntripClientConnectLimitReached())
            //Display the WiFi failure
            paintNtripWiFiFail(4000, true);

          //TODO WiFi not available, give up, disable future attempts
        }
      }
      else
      {
        //WiFi connection established
        ntripClientSetState(NTRIP_CLIENT_WIFI_CONNECTED);
      }
      break;

    case NTRIP_CLIENT_WIFI_CONNECTED:
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

        //Serial.printf("Response: %s\n\r", response);

        //Look for '200 OK'
        if (strstr(response, "200") == NULL)
        {
          //Look for '401 Unauthorized'
          Serial.printf("NTRIP Client caster responded with bad news: %s. Are you sure your caster credentials are correct?\n\r", response);

          //Switch to Bluetooth operation
          ntripClientSwitchToBluetooth();
        }
        else
        {
          log_d("NTRIP Client connected to caster");

          //Connection is now open, start the NTRIP receive data timer
          ntripClientTimer = millis();

          // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
          i2cGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);
          i2cGNSS.setNMEAGPGGAcallbackPtr(&pushGPGGA); // Set up the callback for GPGGA

          float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;
          if (measurementFrequency < 0.2) measurementFrequency = 0.2; //0.2Hz * 5 = 1 measurement every 5 seconds
          i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, measurementFrequency * 5); // Enable GGA over I2C. Tell the module to output GGA every 5 seconds

          lastGGAPush = millis();

          //We don't use a task because we use I2C hardware (and don't have a semphore).
          online.ntripClient = true;
          ntripClientAllowMoreConnections();
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
        ntripClientStop(false);
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
            ntripClientStop(false);
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

          log_d("NTRIP Client pushed %d RTCM bytes to ZED", rtcmCount);
        }
      }

      break;
  }
#endif  //COMPILE_WIFI
}

void pushGPGGA(NMEA_GGA_data_t *nmeaData)
{
#ifdef  COMPILE_WIFI
  //Provide the caster with our current position as needed
  if ((ntripClient->connected() == true) && (settings.ntripClient_TransmitGGA == true))
  {
    if (millis() - lastGGAPush > 5000)
    {
      lastGGAPush = millis();
      //Serial.print(F("Pushing GGA to server: "));
      //Serial.print((const char *)nmeaData->nmea); // .nmea is printable (NULL-terminated) and already has \r\n on the end

      //Push our current GGA sentence to caster
      ntripClient->print((const char *)nmeaData->nmea);
    }
  }
#endif
}
