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
static const int MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS = 3;

//----------------------------------------
// Locals - compiled out
//----------------------------------------

// WiFi connection used to push RTCM to NTRIP caster over WiFi
static WiFiClient * ntripServer;

//Count of bytes sent by the NTRIP server to the NTRIP caster
uint32_t ntripServerBytesSent = 0;

//Count the number of connection attempts
static int ntripServerConnectionAttempts;

//Last time the NTRIP server state was displayed
static uint32_t ntripServerStateLastDisplayed = 0;

//NTRIP server timer usage:
//  * Measure the connection response time
//  * Receive RTCM correction data timeout
//  * Monitor last RTCM byte received for frame counting
static uint32_t ntripServerTimer;

//Record last connection attempt
//After 6 hours, reset the connectionAttempts to enable
//multi week/month base installations
static uint32_t lastConnectionAttempt = 0;

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

  if (!limitReached)
    //Display the heap state
    reportHeapNow();
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

static byte ntripServerCrcState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;

//Parse the RTCM transport data
bool ntripServerRtcmMessage(uint8_t data)
{
  static uint16_t bytesRemaining;
  static uint16_t length;
  static uint16_t message;
  static bool sendMessage = false;

  //
  //    RTCM Standard 10403.2 - Chapter 4, Transport Layer
  //
  //    |<------------- 3 bytes ------------>|<----- length ----->|<- 3 bytes ->|
  //    |                                    |                    |             |
  //    +----------+--------+----------------+---------+----------+-------------+
  //    | Preamble |  Fill  | Message Length | Message |   Fill   |   CRC-24Q   |
  //    |  8 bits  | 6 bits |    10 bits     |  n-bits | 0-7 bits |   24 bits   |
  //    |   0xd3   | 000000 |   (in bytes)   |         |   zeros  |             |
  //    +----------+--------+----------------+---------+----------+-------------+
  //    |                                                                       |
  //    |<-------------------------------- CRC -------------------------------->|
  //

  switch (ntripServerCrcState)
  {
    //Read the upper two bits of the length
    case RTCM_TRANSPORT_STATE_READ_LENGTH_1:
      if (!(data & 3))
      {
        length = data << 8;
        ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_LENGTH_2;
        break;
      }

      //Wait for the preamble byte
      ntripServerCrcState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;

    //Fall through
    //     |
    //     |
    //     V

    //Wait for the preamble byte (0xd3)
    case RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3:
      sendMessage = false;
      if (data == 0xd3)
      {
        ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_LENGTH_1;
        sendMessage = (ntripServerState == NTRIP_SERVER_CASTING);
      }
      break;

    //Read the lower 8 bits of the length
    case RTCM_TRANSPORT_STATE_READ_LENGTH_2:
      length |= data;
      bytesRemaining = length;
      ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_MESSAGE_1;
      break;

    //Read the upper 8 bits of the message number
    case RTCM_TRANSPORT_STATE_READ_MESSAGE_1:
      message = data << 4;
      bytesRemaining -= 1;
      ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_MESSAGE_2;
      break;

    //Read the lower 4 bits of the message number
    case RTCM_TRANSPORT_STATE_READ_MESSAGE_2:
      message |= data >> 4;
      bytesRemaining -= 1;
      ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_DATA;
      break;

    //Read the rest of the message
    case RTCM_TRANSPORT_STATE_READ_DATA:
      bytesRemaining -= 1;
      if (bytesRemaining <= 0)
        ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_CRC_1;
      break;

    //Read the upper 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_1:
      ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_CRC_2;
      break;

    //Read the middle 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_2:
      ntripServerCrcState = RTCM_TRANSPORT_STATE_READ_CRC_3;
      break;

    //Read the lower 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_3:
      ntripServerCrcState = RTCM_TRANSPORT_STATE_CHECK_CRC;
      break;
  }

  //Check the CRC
  if (ntripServerCrcState == RTCM_TRANSPORT_STATE_CHECK_CRC)
  {
    ntripServerCrcState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;

    //Account for this message
    rtcmPacketsSent++;

    //Display the RTCM message header
    if (settings.enablePrintNtripServerRtcm && (!inMainMenu))
    {
      printTimeStamp();
      Serial.printf ("    Tx RTCM %d, %2d bytes\r\n", message, 3 + length + 3);
    }
  }

  //Let the upper layer know if this message should be sent
  return sendMessage && (ntripServerState == NTRIP_SERVER_CASTING);
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
  //Check for too many digits
  if (settings.enableResetDisplay == true)
  {
    if (rtcmPacketsSent > 99) rtcmPacketsSent = 1; //Trim to two digits to avoid overlap
  }
  else if (logIncreasing == true)
  {
    if (rtcmPacketsSent > 999) rtcmPacketsSent = 1; //Trim to three digits to avoid log icon
  }
  else
  {
    if (rtcmPacketsSent > 9999) rtcmPacketsSent = 1;
  }

#ifdef  COMPILE_WIFI
  uint32_t currentMilliseconds;
  static uint32_t previousMilliseconds = 0;

  if (online.rtc && (ntripServerState == NTRIP_SERVER_CASTING))
  {
    //Timestamp the RTCM messages
    currentMilliseconds = millis();
    if (settings.enablePrintNtripServerRtcm
        && (!settings.enableNtripServerMessageParsing)
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

    //Pass this message to the RTCM checker
    bool passAlongIncomingByte = true;

    //Check this byte with RTCM checker if enabled
    if (settings.enableNtripServerMessageParsing == true)
      passAlongIncomingByte &= ntripServerRtcmMessage(incoming);
    else
    {
      //If we have not gotten new RTCM bytes for a period of time, assume end of frame
      if (millis() - ntripServerTimer > 100 && ntripServerBytesSent > 0)
      {
        log_d("NTRIP Server pushed %d RTCM bytes to Caster", ntripServerBytesSent);
        ntripServerBytesSent = 0;
        rtcmPacketsSent++; //If not checking RTCM CRC, count based on timeout
      }
    }

    if (passAlongIncomingByte)
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
    ntripServerCrcState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;
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
    wifiStop();

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
          {
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
        //        sdCardServerBegin(&server, true, true);
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

        lastConnectionAttempt = millis();

        //Assume service not available
        if (ntripServerConnectLimitReached())
          Serial.println("NTRIP Server failed to connect! Do you have your caster address and port correct?");
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
            Serial.println("Caster failed to respond. Do you have your caster address and port correct?");
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

          ntripServerStop(true);  //Don't allocate new wifiClient
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
        Serial.println("NTRIP Server connection dropped");
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

        //There may be intermintant disconnects over days or weeks
        //Reset the reconnect attempts every 6 hours
        if (ntripServerConnectionAttempts > 0 && (millis() - lastConnectionAttempt) > (1000L * 60 * 60 * 6))
        {
          ntripServerConnectionAttempts = 0;
          log_d("Resetting connection attempts");
        }
      }

      break;
  }
#endif  //COMPILE_WIFI
}
