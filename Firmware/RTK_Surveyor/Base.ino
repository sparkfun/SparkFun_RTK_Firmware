
//Configure specific aspects of the receiver for base mode
bool configureUbloxModuleBase()
{
  bool response = true;

  digitalWrite(positionAccuracyLED_1cm, LOW);
  digitalWrite(positionAccuracyLED_10cm, LOW);
  digitalWrite(positionAccuracyLED_100cm, LOW);

  //In base mode we force 1Hz
  if (i2cGNSS.getNavigationFrequency() != 1)
  {
    response &= i2cGNSS.setNavigationFrequency(1); //Set output in Hz
  }

  // Set dynamic model
  if (i2cGNSS.getDynamicModel() != DYN_MODEL_STATIONARY)
  {
    response &= i2cGNSS.setDynamicModel(DYN_MODEL_STATIONARY);
    if (response == false)
      Serial.println(F("setDynamicModel failed!"));
  }

  //In base mode the Surveyor should output RTCM over UART2 and I2C ports:
  //(Primary) UART2 in case the Surveyor is connected via radio to rover
  //(Optional) I2C in case user wants base to connect to WiFi and NTRIP Serve to Caster
  //(Seconday) USB in case the Surveyor is used as an NTRIP caster
  //(Tertiary) UART1 in case Surveyor is sending RTCM to phone that is then NTRIP caster
  response &= enableRTCMSentences(COM_PORT_UART2);
  response &= enableRTCMSentences(COM_PORT_UART1);
  response &= enableRTCMSentences(COM_PORT_USB);

  if (settings.enableNtripServer == true)
  {
    //Turn on RTCM over I2C port so that we can harvest RTCM over I2C and send out over WiFi
    //This is easier than parsing over UART because the library handles the frame detection

#define OUTPUT_SETTING 14
#define INPUT_SETTING 12
    getPortSettings(COM_PORT_I2C); //Load the settingPayload with this port's settings
    if (settingPayload[OUTPUT_SETTING] != COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3 || settingPayload[INPUT_SETTING] != COM_TYPE_UBX)
    {
      response &= i2cGNSS.setPortOutput(COM_PORT_I2C, COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //UBX+RTCM3 is not a valid option so we enable all three.
      response &= i2cGNSS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX); //Set the I2C port to input UBX only
    }

    //Disable any NMEA sentences
    response &= disableNMEASentences(COM_PORT_I2C);

    //Enable necessary RTCM sentences
    response &= enableRTCMSentences(COM_PORT_I2C);
  }

  if (response == false)
    Serial.println(F("RTCM settings failed to enable"));

  return (response);
}

//Start survey
//The ZED-F9P is slightly different than the NEO-M8P. See the Integration manual 3.5.8 for more info.
bool surveyIn()
{
  resetSurvey();

  bool response = i2cGNSS.enableSurveyMode(settings.observationSeconds, settings.observationPositionAccuracy); //Enable Survey in, with user parameters
  if (response == false)
  {
    Serial.println(F("Survey start failed"));
    return (false);
  }

  Serial.printf("Survey started. This will run until %d seconds have passed and less than %0.03f meter accuracy is achieved.\n",
                settings.observationSeconds,
                settings.observationPositionAccuracy
               );

  return (true);
}

void resetSurvey()
{
  //Slightly modified method for restarting survey-in from: https://portal.u-blox.com/s/question/0D52p00009IsVoMCAV/restarting-surveyin-on-an-f9p
  bool response = i2cGNSS.disableSurveyMode(); //Disable survey
  delay(500);
  response &= i2cGNSS.enableSurveyMode(1000, 400.000); //Enable Survey in with bogus values
  delay(500);
  response &= i2cGNSS.disableSurveyMode(); //Disable survey
  delay(500);
}

//Start the base using fixed coordinates
bool startFixedBase()
{
  bool response = false;

  if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
  {
    //Break ECEF into main and high precision parts
    //The type casting should not effect rounding of original double cast coordinate
    long majorEcefX = settings.fixedEcefX * 100;
    long minorEcefX = ((settings.fixedEcefX * 100.0) - majorEcefX) * 100.0;
    long majorEcefY = settings.fixedEcefY * 100;
    long minorEcefY = ((settings.fixedEcefY * 100.0) - majorEcefY) * 100.0;
    long majorEcefZ = settings.fixedEcefZ * 100;
    long minorEcefZ = ((settings.fixedEcefZ * 100.0) - majorEcefZ) * 100.0;

    //    Serial.printf("fixedEcefY (should be -4716808.5807): %0.04f\n", settings.fixedEcefY);
    //    Serial.printf("major (should be -471680858): %d\n", majorEcefY);
    //    Serial.printf("minor (should be -7): %d\n", minorEcefY);

    //Units are cm with a high precision extension so -1234.5678 should be called: (-123456, -78)
    //-1280208.308,-4716803.847,4086665.811 is SparkFun HQ so...
    response = i2cGNSS.setStaticPosition(majorEcefX, minorEcefX,
                                         majorEcefY, minorEcefY,
                                         majorEcefZ, minorEcefZ
                                        ); //With high precision 0.1mm parts
  }
  else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEOGRAPHIC)
  {
    //Break coordinates into main and high precision parts
    //The type casting should not effect rounding of original double cast coordinate
    int64_t majorLat = settings.fixedLat * 10000000;
    int64_t minorLat = ((settings.fixedLat * 10000000) - majorLat) * 100;
    int64_t majorLong = settings.fixedLong * 10000000;
    int64_t minorLong = ((settings.fixedLong * 10000000) - majorLong) * 100;
    int32_t majorAlt = settings.fixedAltitude * 100;
    int32_t minorAlt = ((settings.fixedAltitude * 100) - majorAlt) * 100;

    //    Serial.printf("fixedLat (should be -105.184774720): %0.09f\n", settings.fixedLat);
    //    Serial.printf("major (should be -1051847747): %lld\n", majorLat);
    //    Serial.printf("minor (should be -20): %lld\n", minorLat);
    //
    //    Serial.printf("fixedLong (should be 40.090335429): %0.09f\n", settings.fixedLong);
    //    Serial.printf("major (should be 400903354): %lld\n", majorLong);
    //    Serial.printf("minor (should be 29): %lld\n", minorLong);
    //
    //    Serial.printf("fixedAlt (should be 1560.2284): %0.04f\n", settings.fixedAltitude);
    //    Serial.printf("major (should be 156022): %ld\n", majorAlt);
    //    Serial.printf("minor (should be 84): %ld\n", minorAlt);

    response = i2cGNSS.setStaticPosition(
                 majorLat, minorLat,
                 majorLong, minorLong,
                 majorAlt, minorAlt,
                 true);
  }

  if (response == true)
  {
    Serial.println(F("Static Base Started Successfully"));
  }

  return (response);
}

//Call regularly to push latest u-blox data out over local WiFi
//We make sure we are connected to WiFi, then
bool updateNtripServer()
{
  //Is WiFi setup?
  if (WiFi.isConnected() == false)
  {
    //Turn off Bluetooth and turn on WiFi
    endBluetooth();

    Serial.printf("Connecting to local WiFi: %s", settings.wifiSSID);
    WiFi.begin(settings.wifiSSID, settings.wifiPW);

    int maxTime = 10000;
    long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(F("."));

      if (millis() - startTime > maxTime)
      {
        Serial.println(F("\nFailed to connect to WiFi. Are you sure your WiFi credentials are correct?"));
        return (false);
      }

      if (Serial.available()) return (false); //User has pressed a key
    }
    Serial.println();

    radioState = WIFI_CONNECTED;
  } //End WiFi connect check

  //Are we connected to caster?
  if (caster.connected() == false)
  {
    Serial.printf("Opening socket to %s\n", settings.casterHost);

    if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
    {
      Serial.printf("Connected to %s:%d\n", settings.casterHost, settings.casterPort);

      const int SERVER_BUFFER_SIZE  = 512;
      char serverBuffer[SERVER_BUFFER_SIZE];

      snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/%s\r\n\r\n",
               settings.mountPointPW, settings.mountPoint, ntrip_server_name, "App Version 1.0");

      Serial.printf("Sending credentials:\n%s\n", serverBuffer);
      caster.write(serverBuffer, strlen(serverBuffer));

      //Wait for response
      unsigned long timeout = millis();
      while (caster.available() == 0)
      {
        if (millis() - timeout > 5000)
        {
          Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
          caster.stop();
          delay(10);
          return (false);
        }
        delay(10);
      }

      delay(10); //Yield to RTOS

      //Check reply
      bool connectionSuccess = false;
      char response[512];
      int responseSpot = 0;
      while (caster.available())
      {
        response[responseSpot++] = caster.read();
        if (strstr(response, "200") > 0) //Look for 'ICY 200 OK'
          connectionSuccess = true;
        if (responseSpot == 512 - 1) break;
      }
      response[responseSpot] = '\0';
      Serial.printf("Caster responded with: %s\n", response);

      if (connectionSuccess == false)
      {
        Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?", response);
      }
      else
      {
        //We're connected!
        Serial.println(F("Connected to caster"));

        //Reset flags
        lastServerReport_ms = millis();
        lastServerSent_ms = millis();
        casterBytesSent = 0;
      }
    } //End attempt to connect
    else
    {
      Serial.println(F("Failed to connect to caster. Are you sure your Caster address is correct?"));
      delay(10); //Give RTOS some time
      return (false);
    }
  } //End connected == false


  //Close socket if we don't have new data for 10s
  //RTK2Go will ban your IP address if you abuse it. See http://www.rtk2go.com/how-to-get-your-ip-banned/
  //So let's not leave the socket open/hanging without data
  if (millis() - lastServerSent_ms > maxTimeBeforeHangup_ms)
  {
    Serial.println(F("RTCM timeout. Disconnecting from caster."));
    caster.stop();
    return (false);
  }

  //Report some statistics every 250
  if (millis() - lastServerReport_ms > 250)
  {
    lastServerReport_ms = millis();
    Serial.printf("Total bytes sent to caster: %d\n", casterBytesSent);
  }

  return (true);
}

//This function gets called from the SparkFun u-blox Arduino Library.
//As each RTCM byte comes in you can specify what to do with it
//Useful for passing the RTCM correction data to a radio, Ntrip broadcaster, etc.
void SFE_UBLOX_GNSS::processRTCM(uint8_t incoming)
{
  //Count outgoing packets for display
  //Assume 1Hz RTCM transmissions
  if (millis() - lastRTCMPacketSent > 500)
  {
    lastRTCMPacketSent = millis();
    rtcmPacketsSent++;
  }

  //Check for too many digits
  if (logIncreasing == true)
  {
    if (rtcmPacketsSent > 999) rtcmPacketsSent = 1; //Trim to three digits to avoid logging icon
  }
  else
  {
    if (rtcmPacketsSent > 9999) rtcmPacketsSent = 1;
  }

  if (caster.connected() == true)
  {
    caster.write(incoming); //Send this byte to socket
    casterBytesSent++;
    lastServerSent_ms = millis();
  }
}
