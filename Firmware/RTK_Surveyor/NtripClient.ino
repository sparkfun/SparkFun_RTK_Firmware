//----------------------------------------
// Constants
//----------------------------------------

static const int CREDENTIALS_BUFFER_SIZE = 512;

//Give up connecting after this number of attempts
static const int MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS = 3;

//Most incoming data is around 500 bytes but may be larger
static const int RTCM_DATA_SIZE = 512 * 4;

//NTRIP client server request buffer size
static const int SERVER_BUFFER_SIZE = 512;

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#ifdef  COMPILE_WIFI

//The WiFi connection to the NTRIP caster to obtain RTCM data.
static WiFiClient ntripClient;

//Goes true during WiFi and NTRIP connection attempts.
//Allows graceful fail back to Bluetooth
static bool ntripClientAttempted = false;
static int ntripClientConnectionAttempts = 0;

//----------------------------------------
// NTRIP Client Routines - compiled out
//----------------------------------------

bool ntripClientConnect()
{
  if (!ntripClient.connect(settings.ntripClient_CasterHost, settings.ntripClient_CasterPort))
    return false;
  Serial.printf("Connected to %s:%d\n\r", settings.ntripClient_CasterHost, settings.ntripClient_CasterPort);

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

    Serial.print(F("Sending credentials: "));
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

  Serial.print(F("serverRequest size: "));
  Serial.print(strlen(serverRequest));
  Serial.print(F(" of "));
  Serial.print(sizeof(serverRequest));
  Serial.println(F(" bytes available"));

  // Send the server request
  Serial.println(F("Sending server request: "));
  Serial.println(serverRequest);
  ntripClient.write(serverRequest, strlen(serverRequest));
  return true;
}

//Determine if another retry is possible or if the limit has been reached
bool ntripClientConnectLimitReached()
{
  return (ntripClientConnectionAttempts++ >= MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS);
}

//Determine if NTRIP client data is available
int ntripClientReceiveDataAvailable()
{
  return ntripClient.available();
}

//Read the response from the NTRIP client
void ntripClientResponse(char * response, size_t maxLength)
{
  char * responseEnd;

  //Make sure that we can zero terminate the response
  responseEnd = &response[maxLength - 1];

  // Read bytes from the caster and store them
  while ((response < responseEnd) && ntripClientReceiveDataAvailable())
    *response++ = ntripClient.read();

  // Zero terminate the response
  *response = '\0';
}

//Stop the NTRIP client
void ntripClientStop()
{
  ntripClient.stop();
}

//Used during Rover+WiFi NTRIP Client mode to provide caster with GGA sentence every 10 seconds
void ntripClientPushGPGGA(NMEA_GGA_data_t *nmeaData)
{
  //Provide the caster with our current position as needed
  if ((ntripClient.connected() == true) && (settings.ntripClient_TransmitGGA == true))
  {
    log_d("Pushing GGA to server: %s", nmeaData->nmea); //nmea is printable (NULL-terminated) and already has \r\n on the end

    ntripClient.print((const char *)nmeaData->nmea); //Push our current GGA sentence to caster
  }
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

bool ntripClientStart()
{
#ifdef  COMPILE_WIFI
  if (settings.enableNtripClient == true && ntripClientAttempted == false)
  {
    //Start WiFi
    wifiStart(settings.ntripClient_wifiSSID, settings.ntripClient_wifiPW);

    ntripClientAttempted = true; //Do not allow re-entry into STATE_ROVER_CLIENT_WIFI_STARTED
    return true;
  }
#endif  //COMPILE_WIFI
  return false;
}

//Check for the arrival of any correction data. Push it to the GNSS.
//Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
#ifdef  COMPILE_WIFI
  if (online.ntripClient == true)
  {
    if (ntripClient.connected() == true) // Check that the connection is still open
    {
      uint8_t rtcmData[RTCM_DATA_SIZE];
      size_t rtcmCount = 0;

      //Collect any available RTCM data
      while (ntripClient.available())
      {
        rtcmData[rtcmCount++] = ntripClient.read();
        if (rtcmCount == sizeof(rtcmData))
          break;
      }

      if (rtcmCount > 0)
      {
        lastReceivedRTCM_ms = millis();

        //Push RTCM to GNSS module over I2C
        i2cGNSS.pushRawData(rtcmData, rtcmCount);

        //log_d("Pushed %d RTCM bytes to ZED", rtcmCount);
      }
    }
    else
    {
      Serial.println(F("NTRIP Client connection dropped"));
      online.ntripClient = false;
    }

    //Timeout if we don't have new data for maxTimeBeforeHangup_ms
    if ((millis() - lastReceivedRTCM_ms) > maxTimeBeforeHangup_ms)
    {
      Serial.println(F("NTRIP Client timeout"));
      ntripClientStop();
      online.ntripClient = false;
    }
  }
#endif  //COMPILE_WIFI
}
