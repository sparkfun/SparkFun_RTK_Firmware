//Configure SARA for use with Hologram SIM
bool initCellular()
{
  if (online.cellular == false) return (false);

  bool success = true;
  //AT+CFUN=0 - Turn off radio
  if (mySARA->functionality(MINIMUM_FUNCTIONALITY) != SARA_R5_SUCCESS)
  {
    log_d("Turn off radio failed");
    success = false;
  }

  //AT+UMNOPROF=90 - Set network profile for Hologram recommended 90
  if (mySARA->setNetworkProfile(MNO_GLOBAL) == false)
  {
    log_d("Error setting network");
    success = false;
  }

  //AT+CGDCONT=1,"IP","hologram.mnc050.mcc234.gprs" - Assign APN
  int cid = 1;
  if (mySARA->setAPN("hologram.mnc050.mcc234.gprs\0", cid) != SARA_R5_SUCCESS)
  {
    log_d("setAPN failed");
    success = false;
  }

  //AT+CFUN=1 - Radio on
  if (mySARA->functionality(FULL_FUNCTIONALITY) != SARA_R5_SUCCESS)
  {
    log_d("Turn on radio failed");
    success = false;
  }

  //Wait for network connectivity
  log_d("Wait for signal");
  int counter = 0;
  int maxWait_s = 20;
  while (1)
  {
    int rssi = mySARA->rssi();
    if (rssi >= 0 && rssi <= 40) break; //99 indicates no connectivity

    delay(500);
    if (counter++ > (maxWait_s * 1000) / 500)
    {
      log_d("No signal available");
      return (false);
    }
  }

  //Wait for network connectivity
  log_d("Wait for registration");
  counter = 0;
  maxWait_s = 40;
  while (1)
  {
    int registration = mySARA->registration();
    if (registration > 0 && registration < 11)  break; //255 indicates invalid status

    delay(500);
    if (counter++ > (maxWait_s * 1000) / 500)
    {
      log_d("No registration available");
      return (false);
    }
  }

  //AT+CGACT=1,1
  if (mySARA->activatePDPcontext(true, cid) != SARA_R5_SUCCESS)
  {
    log_d("activatePDPcontext failed");
    success = false;
  }

  //AT+UPSD=0,0,0
  if (mySARA->setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_PROTOCOL, 0) != SARA_R5_SUCCESS)
  {
    log_d("setPDPconfiguration failed");
    success = false;
  }

  //AT+UPSD=0,100,1 - Map PSD profile 0 to the selected CID
  if (mySARA->setPDPconfiguration(0, SARA_R5_PSD_CONFIG_PARAM_MAP_TO_CID, cid) != SARA_R5_SUCCESS)
  {
    log_d("setPDPconfiguration 0 failed");
    success = false;
  }

  //AT+UPSDA=0,3 - Perform PDP Action - activate
  if (mySARA->performPDPaction(0, SARA_R5_PSD_ACTION_ACTIVATE) != SARA_R5_SUCCESS)
  {
    log_d("performPDPaction failed");
    success = false;
  }

  //AT+USOCR=6 - Open TCP socket AT+USOCR=6,0, AT+UUSOCL=0
  socketNumber = mySARA->socketOpen(SARA_R5_TCP, 1); //Open port for TCP
  if (socketNumber >= 0)
  {
    log_d("Socket open");

    //AT+USOCO=0,"rtk2go.com",2101 - Connect to caster and port
    if (mySARA->socketConnect(socketNumber, settings.casterHost, settings.casterPort) != SARA_R5_SUCCESS)
    {
      log_d("socketConnect failed");
      success = false;
    }

    //AT+USODL=0 - Open direct tunnel
    char response[50] = {0};
    if (mySARA->sendCustomCommandWithResponse("+USODL=0", "CONNECT", response, 1000, true) != SARA_R5_SUCCESS)
    {
      log_d("DirectLink failed");
      success = false;
    }
  }
  else
    success = false;


  if (success) Serial.println(F("SARA connected to caster"));

  return (success);
}

bool closeSARASocket()
{
  if (online.cellular == false) return (false);

  log_d("Closing socket %d\n\r", socketNumber);
  if (mySARA->socketClose(socketNumber) != SARA_R5_SUCCESS)
  {
    log_d("Socket close fail");
    return (false);
  }

  socketNumber = -1;
  log_d("Socket closed");
  return (true);
}

bool exitSARATunnel()
{
  if (online.cellular == false) return (false);

  //Must send after 2s of no serial traffic
  for (int x = 0 ; x < 200 ; x++)
  {
    delay(10);
    while (Serial1.available()) Serial1.read(); //Clear incoming
  }

  log_d("Closing tunnel");
  //The following causes Guru panic.
  //  char response[512] = {'\0'}; //Must be large enough to handle potential incoming RTCM packet
  //  if (mySARA->sendCustomCommandWithResponse("+++", "OK", response, 3000, false) != SARA_R5_SUCCESS)
  //  {
  //    log_d("Tunnel close failed");
  //    return (false);
  //  }
  Serial1.print("+++");
  delay(1500);
  log_d("Tunnel closed");
  return (true);
}

//A socket to the chosen caster must be open before calling
void startSARANTRIPClient()
{
  if (socketNumber == -1)
  {
    log_d("Socket not open");
    return;
  }

  log_d("Requesting NTRIP Data from mount point");

  const int SERVER_BUFFER_SIZE = 512;
  char serverRequest[SERVER_BUFFER_SIZE];

  snprintf(serverRequest,
           SERVER_BUFFER_SIZE,
           "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun Facet-Cellular v1.0\r\n",
           settings.mountPointDownload);

  char credentials[512];
  if (strlen(settings.casterUser) == 0)
  {
    strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", sizeof(credentials));
  }
  else
  {
    //Pass base64 encoded user:pw
    char userCredentials[sizeof(settings.casterUser) + sizeof(settings.casterUserPW) + 1]; //The ':' takes up a spot
    snprintf(userCredentials, sizeof(userCredentials), "%s:%s", settings.casterUser, settings.casterUserPW);

    //Encode with ESP32 built-in library
    base64 b;
    String strEncodedCredentials = b.encode(userCredentials);
    char encodedCredentials[strEncodedCredentials.length() + 1];
    strEncodedCredentials.toCharArray(encodedCredentials, sizeof(encodedCredentials)); //Convert String to char array

    snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
  }
  strncat(serverRequest, credentials, sizeof(credentials) - 1);
  strncat(serverRequest, "\r\n", 3);

  log_d("serverRequest size: %d of %d bytes available\n\r", strlen(serverRequest), sizeof(serverRequest));

  //At this point we have a direct TCP tunnel
  //We can read/write to the serial port as if we are connected to the NTRIP Caster via TCP

  while (Serial1.available()) Serial1.read(); //Clear incoming

  log_d("Sending server request: %s\n\r", serverRequest);

  Serial1.write(serverRequest); //Request connection

  //Wait for response
  unsigned long timeout = millis();
  while (Serial1.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      log_d("Caster timed out!");
      exitSARATunnel();
      closeSARASocket();
      return;
    }
    delay(10);
  }

  //Check reply
  bool connectionSuccess = false;
  char response[512];
  int responseSpot = 0;
  while (Serial1.available())
  {
    if (responseSpot == sizeof(response) - 1)
      break;

    response[responseSpot++] = Serial1.read();
    if (strstr(response, "ICY 200") > 0) //Look for 'ICY 200 OK'
      connectionSuccess = true;
    if (strstr(response, "SOURCETABLE 200") > 0)
    {
      log_d("Sourcetable - Check if mountpoint is up and running");
      connectionSuccess = true;
    }
    if (strstr(response, "401") > 0) //Look for '401 Unauthorized'
    {
      log_d("Credentials look bad. Check your caster username and password.");
      connectionSuccess = false;
    }
  }
  response[responseSpot] = '\0';

  log_d("Caster responded with: %s\n\r", response);

  if (connectionSuccess == false)
  {
    log_d("Failed to connect to caster");
    return;
  }

  log_d("Connected to %s\n\r", settings.casterHost);

  while (Serial.available()) Serial.read();

  lastReceivedRTCM_ms = millis(); //Reset timeout
  ggaTransmitComplete = true; //Reset to start polling for new GGA data

  int rtcmCount = 0;
  uint8_t rtcmData[512 * 4]; //Most incoming data is around 500 bytes but may be larger

  while (1)
  {
    while (Serial1.available())
    {
      rtcmData[rtcmCount++] = Serial1.read();
      if (rtcmCount == sizeof(rtcmData))
        break;
    }

    if (rtcmCount > 0)
    {
      lastReceivedRTCM_ms = millis();

      //Push RTCM to GNSS module over I2C
      i2cGNSS.pushRawData(rtcmData, rtcmCount, false);
      log_d("RTCM pushed to ZED: %d\n\r", rtcmCount);
      rtcmCount = 0;
    }

    //Close socket if we don't have new data for 10s
    if (millis() - lastReceivedRTCM_ms > maxTimeBeforeHangup_ms)
    {
      log_d("RTCM timeout. Disconnecting.");
      exitSARATunnel();
      closeSARASocket();
      return;
    }

    //Provide the caster with our current position as needed
    if (settings.casterTransmitGGA == true
        && (millis() - lastTransmittedGGA_ms) > timeBetweenGGAUpdate_ms
        && ggaSentenceComplete == true
        && ggaTransmitComplete == false
       )
    {
      if(pushGGA() == false)
      {
        //TODO - Count failures?
      }
    }


    delay(10);

    if (Serial.available())
    {
      Serial.println("User break");
      exitSARATunnel();
      closeSARASocket();
      return;
    }
  }
}

//Push current GGA location info to caster
bool pushGGA()
{
  log_d("Pushing GGA to server");

  lastTransmittedGGA_ms = millis();

  //Push our current GGA sentence to caster
  Serial1.print(ggaSentence);
  Serial1.print("\r\n");

  ggaTransmitComplete = true;

  //Wait for response
  unsigned long timeout = millis();
  while (Serial1.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      log_d("Caster timed out");
      return(false);
    }
    delay(10);
  }

  //Check reply
  bool connectionSuccess = false;
  char response[512];
  int responseSpot = 0;
  while (Serial1.available())
  {
    if (responseSpot == sizeof(response) - 1)
      break;

    response[responseSpot++] = Serial1.read();
    if (strstr(response, "200") > 0) //Look for '200 OK'
      connectionSuccess = true;
  }
  response[responseSpot] = '\0';

  log_d("Caster responded with: %s\n\r", response);
  return(connectionSuccess);
}

//Resets the SARA module
void resetSARA()
{
  log_d("Resetting SARA");

  //Pull reset low to reset
  //A reset will cause the device not to respond for ~3.5s.
  //A reset device will maintain previous baud setting
  //Immediately after a reset device will respond. Give it time to start reset.
  pinMode(pin_radio_rst, OUTPUT);
  digitalWrite(pin_radio_rst, LOW);
  delay(100); //50 fails. 100 works.
  digitalWrite(pin_radio_rst, HIGH);

  delay(3500); //Wait for module to be ready for commands
}
