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
static uint32_t ntripServerTimer;

//----------------------------------------
// NTRIP Server Routines - compiled out
//----------------------------------------

//Determine if more connections are allowed
void ntripServerAllowMoreConnections()
{
  ntripServerConnectionAttempts = 0;
}

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
  ntripServerStop(false);

  //Retry the connection a few times
  bool limitReached = (ntripServerConnectionAttempts++ >= MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS);
  if (!limitReached)
    //Display the heap state
    reportHeapNow();
  else
  {
    //No more connection attempts, switching to Bluetooth
    Serial.println(F("NTRIP Server connection attempts exceeded!"));
    ntripServerSwitchToBluetooth();
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

//Parse the RTCM transport data
bool ntripServerRtcmMessage(uint8_t data)
{
  static uint16_t bytesRemaining;
  static byte crcState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;
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

  switch (crcState)
  {
    //Wait for the preamble byte (0xd3)
    case RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3:
      sendMessage = false;
      if (data == 0xd3)
      {
        crcState = RTCM_TRANSPORT_STATE_READ_LENGTH_1;
        sendMessage = (ntripServerState == NTRIP_SERVER_CASTING);
      }
      break;

    //Read the upper two bits of the length
    case RTCM_TRANSPORT_STATE_READ_LENGTH_1:
      length = data << 8;
      crcState = RTCM_TRANSPORT_STATE_READ_LENGTH_2;
      break;

    //Read the lower 8 bits of the length
    case RTCM_TRANSPORT_STATE_READ_LENGTH_2:
      length |= data;
      bytesRemaining = length;
      crcState = RTCM_TRANSPORT_STATE_READ_MESSAGE_1;
      break;

    //Read the upper 8 bits of the message number
    case RTCM_TRANSPORT_STATE_READ_MESSAGE_1:
      message = data << 4;
      bytesRemaining -= 1;
      crcState = RTCM_TRANSPORT_STATE_READ_MESSAGE_2;
      break;

    //Read the lower 4 bits of the message number
    case RTCM_TRANSPORT_STATE_READ_MESSAGE_2:
      message |= data >> 4;
      bytesRemaining -= 1;
      crcState = RTCM_TRANSPORT_STATE_READ_DATA;
      break;

    //Read the rest of the message
    case RTCM_TRANSPORT_STATE_READ_DATA:
      bytesRemaining -= 1;
      if (bytesRemaining <= 0)
        crcState = RTCM_TRANSPORT_STATE_READ_CRC_1;
      break;

    //Read the upper 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_1:
      crcState = RTCM_TRANSPORT_STATE_READ_CRC_2;
      break;

    //Read the middle 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_2:
      crcState = RTCM_TRANSPORT_STATE_READ_CRC_3;
      break;

    //Read the lower 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_3:
      crcState = RTCM_TRANSPORT_STATE_CHECK_CRC;
      break;
  }

  //Check the CRC
  if (crcState == RTCM_TRANSPORT_STATE_CHECK_CRC)
  {
    crcState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;

    //Display the RTCM message header
    if (settings.enablePrintNtripServerRtcm)
      Serial.printf ("    Message %d, %2d bytes\r\n", message, 3 + 1 + length + 3);
  }

  //Let the upper layer know if this message should be sent
  return sendMessage && (ntripServerState == NTRIP_SERVER_CASTING);
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

//Switch to Bluetooth operation
void ntripServerSwitchToBluetooth()
{
  Serial.println(F("NTRIP Server failure, switching to Bluetooth!"));

  //Stop WiFi operations
  ntripServerStop(true);

  //Turn on Bluetooth with 'Rover' name
  startBluetooth();
}

#endif  // COMPILE_WIFI

//----------------------------------------
// Global NTRIP Server Routines
//----------------------------------------

//This function gets called as each RTCM byte comes in
void ntripServerProcessRTCM(uint8_t incoming)
{
  uint32_t currentMilliseconds;
  static uint32_t previousMilliseconds = 0;

  //Count outgoing packets for display
  //Assume 1Hz RTCM transmissions
  currentMilliseconds = millis();
  if (currentMilliseconds - lastRTCMPacketSent > 500)
  {
    lastRTCMPacketSent = millis();
    rtcmPacketsSent++;
  }

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
  if (online.rtc)
  {
    //Timestamp the RTCM messages
    if (settings.enablePrintNtripServerRtcm
        && ((currentMilliseconds - previousMilliseconds) > 1))
    {
      //         1         2         3
      //123456789012345678901234567890
      //YYYY-mm-dd HH:MM:SS.xxxrn0
      struct tm timeinfo = rtc.getTimeStruct();
      char timestamp[30];
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.printf("RTCM: %s.%03d\r\n", timestamp, rtc.getMillis());
    }
    previousMilliseconds = currentMilliseconds;

    //Parse the RTCM message
    if (ntripServerRtcmMessage(incoming))
    {
      ntripServer->write(incoming); //Send this byte to socket
      ntripServerBytesSent++;
      ntripServerTimer = millis();
    }

    //Indicate that the GNSS is providing correction data
    else if (ntripServerState == NTRIP_SERVER_WAIT_GNSS_DATA)
      ntripServerSetState(NTRIP_SERVER_CONNECTING);
  }
#endif  //COMPILE_WIFI
}

//Start the NTRIP server
void ntripServerStart()
{
#ifdef  COMPILE_WIFI
  //Stop NTRIP server and WiFi
  ntripServerStop(true);

  //Start the NTRIP server if enabled
  if ((settings.ntripServer_StartAtSurveyIn == true)
    || (settings.enableNtripServer == true))
  {
    //Display the heap state
    reportHeapNow();
    Serial.println(F("NTRIP Server start"));

    //Allocate the ntripServer structure
    ntripServer = new WiFiClient();

    //Restart WiFi and the NTRIP server if possible
    if (ntripServer)
      ntripServerSetState(NTRIP_SERVER_ON);
  }

  //Only fallback to Bluetooth once, then try WiFi again.  This enables changes
  //to the WiFi SSID and password to properly restart the WiFi.
  ntripServerAllowMoreConnections();
#endif  //COMPILE_WIFI
}

//Stop the NTRIP server
void ntripServerStop(bool done)
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
    if (!done)
      ntripServer = new WiFiClient();
  }

  //Stop WiFi if in use
  if (ntripServerState > NTRIP_SERVER_ON)
    wifiStop();

  //Determine the next NTRIP server state
  ntripServerSetState((ntripServer && (!done)) ? NTRIP_SERVER_ON : NTRIP_SERVER_OFF);
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
            Serial.println(F("NTRIP Server failed to connect! Do you have your caster address and port correct?"));
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
      if (ntripServer->available() == 0)
      {
        //Check for response timeout
        if (millis() - ntripServerTimer > 5000)
        {
          //NTRIP web service did not respone
          if (ntripServerConnectLimitReached())
            Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
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

          //Switch to Bluetooth operation
          ntripServerSwitchToBluetooth();
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
          ntripServerAllowMoreConnections();
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
        Serial.println(F("NTRIP Server connection dropped"));
        ntripServerStop(false);
      }
      else if ((millis() - ntripServerTimer) > 1000)
      {
        //GNSS stopped sending RTCM correction data
        Serial.println(F("NTRIP Server breaking caster connection due to lack of RTCM data!"));
        ntripServerStop(false);
      }
      else
        cyclePositionLEDs();
      break;
  }
#endif  //COMPILE_WIFI
}
