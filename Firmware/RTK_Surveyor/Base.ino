
//Configure specific aspects of the receiver for base mode
bool configureUbloxModuleBase()
{
  bool response = true;
  int maxWait = 2000;

  if (productVariant == RTK_SURVEYOR)
  {
    digitalWrite(pin_positionAccuracyLED_1cm, LOW);
    digitalWrite(pin_positionAccuracyLED_10cm, LOW);
    digitalWrite(pin_positionAccuracyLED_100cm, LOW);
  }

  i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  if (i2cGNSS.getSurveyInActive() == true)
  {
    response = i2cGNSS.disableSurveyMode(maxWait); //Disable survey
    if (response == false)
      Serial.println(F("Disable Survey failed"));
  }

  //In base mode we force 1Hz
  if (i2cGNSS.getNavigationFrequency(maxWait) != 1)
    response &= i2cGNSS.setNavigationFrequency(1, maxWait);
  if (response == false)
  {
    Serial.println(F("configureUbloxModuleBase: Set rate failed"));
    return (false);
  }

  // Set dynamic model
  if (i2cGNSS.getDynamicModel(maxWait) != DYN_MODEL_STATIONARY)
  {
    response &= i2cGNSS.setDynamicModel(DYN_MODEL_STATIONARY, maxWait);
    if (response == false)
    {
      Serial.println(F("setDynamicModel failed"));
      return (false);
    }
  }

  //In base mode the Surveyor should output RTCM over UART2 and I2C ports:
  //(Primary) UART2 in case the Surveyor is connected via radio to rover
  //(Optional) I2C in case user wants base to connect to WiFi and NTRIP Serve to Caster
  //(Seconday) USB in case the Surveyor is used as an NTRIP caster
  //(Tertiary) UART1 in case Surveyor is sending RTCM to phone that is then NTRIP caster
  response &= enableRTCMSentences(COM_PORT_UART2);
  response &= enableRTCMSentences(COM_PORT_UART1);
  response &= enableRTCMSentences(COM_PORT_USB);
  response &= enableRTCMSentences(COM_PORT_I2C); //Enable for plain radio so we can count RTCM packets for display (State: Base-Temp - Transmitting)

  if (response == false)
  {
    Serial.println(F("RTCM settings failed to enable"));
    return (false);
  }

  return (response);
}

//Start survey
//The ZED-F9P is slightly different than the NEO-M8P. See the Integration manual 3.5.8 for more info.
bool beginSurveyIn()
{
  bool needSurveyReset = false;
  if (i2cGNSS.getSurveyInActive(100) == true) needSurveyReset = true;
  if (i2cGNSS.getSurveyInValid(100) == true) needSurveyReset = true;

  if (needSurveyReset == true)
  {
    Serial.println("Resetting survey");

    if (resetSurvey() == false)
    {
      Serial.println(F("Survey reset failed"));
      if (resetSurvey() == false)
      {
        Serial.println(F("Survey reset failed - 2nd attempt"));
      }
    }
  }

  bool response = i2cGNSS.enableSurveyMode(settings.observationSeconds, settings.observationPositionAccuracy, 5000); //Enable Survey in, with user parameters. Wait up to 5s.
  if (response == false)
  {
    Serial.println(F("Survey start failed"));
    return (false);
  }

  Serial.printf("Survey started. This will run until %d seconds have passed and less than %0.03f meter accuracy is achieved.\n\r",
                settings.observationSeconds,
                settings.observationPositionAccuracy
               );

  //Wait until active becomes true
  long maxTime = 5000;
  long startTime = millis();
  while(i2cGNSS.getSurveyInActive(100) == false)
  {
    delay(100);
    if(millis() - startTime > maxTime) return(false); //Reset of survey failed
  }

  return (true);
}

bool resetSurvey()
{
  int maxWait = 2000;

  //Slightly modified method for restarting survey-in from: https://portal.u-blox.com/s/question/0D52p00009IsVoMCAV/restarting-surveyin-on-an-f9p
  bool response = i2cGNSS.disableSurveyMode(maxWait); //Disable survey
  delay(1000);
  response &= i2cGNSS.enableSurveyMode(1000, 400.000, maxWait); //Enable Survey in with bogus values
  delay(1000);
  response &= i2cGNSS.disableSurveyMode(maxWait); //Disable survey

  if(response == false)
    return(response);

  //Wait until active and valid becomes false
  long maxTime = 5000;
  long startTime = millis();
  while(i2cGNSS.getSurveyInActive(100) == true || i2cGNSS.getSurveyInValid(100) == true)
  {
    delay(100);
    if(millis() - startTime > maxTime) return(false); //Reset of survey failed
  }

  return(true);
}

//Start the base using fixed coordinates
bool startFixedBase()
{
  bool response = false;
  int maxWait = 2000;

  if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
  {
    //Break ECEF into main and high precision parts
    //The type casting should not effect rounding of original double cast coordinate
    long majorEcefX = floor((settings.fixedEcefX * 100.0) + 0.5);
    long minorEcefX = floor((((settings.fixedEcefX * 100.0) - majorEcefX) * 100.0) + 0.5);
    long majorEcefY = floor((settings.fixedEcefY * 100) + 0.5);
    long minorEcefY = floor((((settings.fixedEcefY * 100.0) - majorEcefY) * 100.0) + 0.5);
    long majorEcefZ = floor((settings.fixedEcefZ * 100) + 0.5);
    long minorEcefZ = floor((((settings.fixedEcefZ * 100.0) - majorEcefZ) * 100.0) + 0.5);

    //    Serial.printf("fixedEcefY (should be -4716808.5807): %0.04f\n\r", settings.fixedEcefY);
    //    Serial.printf("major (should be -471680858): %ld\n\r", majorEcefY);
    //    Serial.printf("minor (should be -7): %ld\n\r", minorEcefY);

    //Units are cm with a high precision extension so -1234.5678 should be called: (-123456, -78)
    //-1280208.308,-4716803.847,4086665.811 is SparkFun HQ so...
    response = i2cGNSS.setStaticPosition(majorEcefX, minorEcefX,
                                         majorEcefY, minorEcefY,
                                         majorEcefZ, minorEcefZ,
                                         false,
                                         maxWait
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

    //    Serial.printf("fixedLong (should be -105.184774720): %0.09f\n\r", settings.fixedLong);
    //    Serial.printf("major (should be -1051847747): %lld\n\r", majorLat);
    //    Serial.printf("minor (should be -20): %lld\n\r", minorLat);
    //
    //    Serial.printf("fixedLat (should be 40.090335429): %0.09f\n\r", settings.fixedLat);
    //    Serial.printf("major (should be 400903354): %lld\n\r", majorLong);
    //    Serial.printf("minor (should be 29): %lld\n\r", minorLong);
    //
    //    Serial.printf("fixedAlt (should be 1560.2284): %0.04f\n\r", settings.fixedAltitude);
    //    Serial.printf("major (should be 156022): %ld\n\r", majorAlt);
    //    Serial.printf("minor (should be 84): %ld\n\r", minorAlt);

    response = i2cGNSS.setStaticPosition(
                 majorLat, minorLat,
                 majorLong, minorLong,
                 majorAlt, minorAlt,
                 true, //Use lat/long as input
                 maxWait);
  }

  return (response);
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
    if (rtcmPacketsSent > 999) rtcmPacketsSent = 1; //Trim to three digits to avoid log icon
  }
  else
  {
    if (rtcmPacketsSent > 9999) rtcmPacketsSent = 1;
  }

#ifdef COMPILE_WIFI
  if (caster.connected() == true)
  {
    caster.write(incoming); //Send this byte to socket
    casterBytesSent++;
    lastServerSent_ms = millis();
  }
#endif
}
