//Configure specific aspects of the receiver for base mode
bool configureUbloxModuleBase()
{
  if (online.gnss == false) return (false);

  //If our settings haven't changed, and this is first config since power on, trust ZED's settings
  if (settings.updateZEDSettings == false && firstPowerOn == true)
  {
    firstPowerOn = false; //Next time user switches modes, new settings will be applied
    log_d("Skipping ZED Base configuration");
    return (true);
  }

  firstPowerOn = false; //If we switch between rover/base in the future, force config of module.

  theGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
  theGNSS.checkCallbacks(); //Process any callbacks: ie, storePVTdata

  theGNSS.setNMEAGPGGAcallbackPtr(nullptr); //Disable GPGGA call back that may have been set during Rover NTRIP Client mode

  bool success = false;
  int tryNo = -1;

  //Try up to MAX_SET_MESSAGES_RETRIES times to configure the GNSS
  //This corrects occasional failures seen on the Reference Station where the GNSS is connected via SPI
  //instead of I2C and UART1. I believe the SETVAL ACK is occasionally missed due to the level of messages being processed.
  while ((++tryNo < MAX_SET_MESSAGES_RETRIES) && !success)
  {
    bool response = true;

    //In Base mode we force 1Hz
    response &= theGNSS.newCfgValset();
    response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, 1000);
    response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, 1);

    //Since we are at 1Hz, allow GSV NMEA to be reported at whatever the user has chosen
    uint32_t spiOffset = 0; //Set to 3 if using SPI to convert UART1 keys to SPI. This is brittle and non-perfect, but works.
    if (USE_SPI_GNSS)
      spiOffset = 3;
    response &= theGNSS.addCfgValset(ubxMessages[8].msgConfigKey + spiOffset, settings.ubxMessageRates[8]); //Update rate on module

    if (USE_I2C_GNSS)
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C, 0); //Disable NMEA message that may have been set during Rover NTRIP Client mode

    //Survey mode is only available on ZED-F9P modules
    if (commandSupported(UBLOX_CFG_TMODE_MODE) == true)
      response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0); //Disable survey-in mode

    response &= theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_DYNMODEL, (dynModel)settings.dynamicModel); //Set dynamic model

    //RTCM is only available on ZED-F9P modules
    //
    //For most RTK products, the GNSS is interfaced via both I2C and UART1. Configuration and PVT/HPPOS messages are
    //configured over I2C. Any messages that need to be logged are output on UART1, and received by this code using
    //serialGNSS.
    //In base mode the RTK device should output RTCM over all ports:
    //(Primary) UART2 in case the Surveyor is connected via radio to rover
    //(Optional) I2C in case user wants base to connect to WiFi and NTRIP Caster
    //(Seconday) USB in case the Surveyor is used as an NTRIP caster connected to SBC or other
    //(Tertiary) UART1 in case Surveyor is sending RTCM to phone that is then NTRIP Caster
    //
    //But, on the Reference Station, the GNSS is interfaced via SPI. It has no access to I2C and UART1.
    //We use the GNSS library's built-in logging buffer to mimic UART1. The code in Tasks.ino reads
    //data from the logging buffer as if it had come from UART1.
    //So for that product - in Base mode - we can only output RTCM on SPI, USB and UART2.
    //If we want to log the RTCM messages, we need to add them to the logging buffer inside the GNSS library.
    //If we want to pass them along to (e.g.) radio, we do that using processRTCM (defined below).

    //Find first RTCM record in ubxMessage array
    int firstRTCMRecord = getMessageNumberByName("UBX_RTCM_1005");

    //ubxMessageRatesBase is an array of ~12 uint8_ts
    //ubxMessage is an array of ~80 messages
    //We use firstRTCMRecord as an offset for the keys, but use x as the rate

    if (USE_I2C_GNSS)
    {
      for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
      {
        response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey - 1, settings.ubxMessageRatesBase[x]); //UBLOX_CFG UART1 - 1 = I2C
        response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey, settings.ubxMessageRatesBase[x]); //UBLOX_CFG UART1

        //Disable messages on SPI
        response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey + 3, 0); //UBLOX_CFG UART1 + 3 = SPI
      }
    }
    else //SPI GNSS
    {
      for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
      {
        response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey + 3, settings.ubxMessageRatesBase[x]); //UBLOX_CFG UART1 + 3 = SPI

        //Disable messages on I2C and UART1
        response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey - 1, 0); //UBLOX_CFG UART1 - 1 = I2C
        response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey, 0); //UBLOX_CFG UART1
      }

      //Enable logging of these messages so the RTCM will be stored automatically in the logging buffer.
      //This mimics the data arriving via UART1.
      uint32_t logRTCMMessages = theGNSS.getRTCMLoggingMask();
      logRTCMMessages |= ( SFE_UBLOX_FILTER_RTCM_TYPE1005 | SFE_UBLOX_FILTER_RTCM_TYPE1074 | SFE_UBLOX_FILTER_RTCM_TYPE1084
                           | SFE_UBLOX_FILTER_RTCM_TYPE1094 | SFE_UBLOX_FILTER_RTCM_TYPE1124 | SFE_UBLOX_FILTER_RTCM_TYPE1230 );
      theGNSS.setRTCMLoggingMask(logRTCMMessages);
    }

    //Update message rates for UART2 and USB
    for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
    {
      response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey + 1 , settings.ubxMessageRatesBase[x]); //UBLOX_CFG UART1 + 1 = UART2
      response &= theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey + 2 , settings.ubxMessageRatesBase[x]); //UBLOX_CFG UART1 + 2 = USB
    }

    response &= theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_INFIL_MINELEV, settings.minElev); //Set minimum elevation

    response &= theGNSS.sendCfgValset(); //Closing value

    if (response)
      success = true;
  }

  if (!success)
    systemPrintln("Base config fail");

  return (success);
}

//Start survey
//The ZED-F9P is slightly different than the NEO-M8P. See the Integration manual 3.5.8 for more info.
bool surveyInStart()
{
  theGNSS.setVal8(UBLOX_CFG_TMODE_MODE, 0); //Disable survey-in mode
  delay(100);

  bool needSurveyReset = false;
  if (theGNSS.getSurveyInActive(100) == true) needSurveyReset = true;
  if (theGNSS.getSurveyInValid(100) == true) needSurveyReset = true;

  if (needSurveyReset == true)
  {
    systemPrintln("Resetting survey");

    if (surveyInReset() == false)
    {
      systemPrintln("Survey reset failed");
      if (surveyInReset() == false)
        systemPrintln("Survey reset failed - 2nd attempt");
    }
  }

  bool response = true;
  response &= theGNSS.setVal8(UBLOX_CFG_TMODE_MODE, 1); //Survey-in enable
  response &= theGNSS.setVal32(UBLOX_CFG_TMODE_SVIN_ACC_LIMIT, settings.observationPositionAccuracy * 10000);
  response &= theGNSS.setVal32(UBLOX_CFG_TMODE_SVIN_MIN_DUR, settings.observationSeconds);

  if (response == false)
  {
    systemPrintln("Survey start failed");
    return (false);
  }

  systemPrintf("Survey started. This will run until %d seconds have passed and less than %0.03f meter accuracy is achieved.\r\n",
               settings.observationSeconds,
               settings.observationPositionAccuracy
              );

  //Wait until active becomes true
  long maxTime = 5000;
  long startTime = millis();
  while (theGNSS.getSurveyInActive(100) == false)
  {
    delay(100);
    if (millis() - startTime > maxTime) return (false); //Reset of survey failed
  }

  return (true);
}

//Slightly modified method for restarting survey-in from: https://portal.u-blox.com/s/question/0D52p00009IsVoMCAV/restarting-surveyin-on-an-f9p
bool surveyInReset()
{
  bool response = true;

  //Disable survey-in mode
  response &= theGNSS.setVal8(UBLOX_CFG_TMODE_MODE, 0);
  delay(1000);

  //Enable Survey in with bogus values
  response &= theGNSS.newCfgValset();
  response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 1); //Survey-in enable
  response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_SVIN_ACC_LIMIT, 40 * 10000); //40.0m
  response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_SVIN_MIN_DUR, 1000); //1000s
  response &= theGNSS.sendCfgValset();
  delay(1000);

  //Disable survey-in mode
  response &= theGNSS.setVal8(UBLOX_CFG_TMODE_MODE, 0);

  if (response == false)
    return (response);

  //Wait until active and valid becomes false
  long maxTime = 5000;
  long startTime = millis();
  while (theGNSS.getSurveyInActive(100) == true || theGNSS.getSurveyInValid(100) == true)
  {
    delay(100);
    if (millis() - startTime > maxTime) return (false); //Reset of survey failed
  }

  return (true);
}

//Start the base using fixed coordinates
bool startFixedBase()
{
  bool response = true;

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

    //    systemPrintf("fixedEcefY (should be -4716808.5807): %0.04f\r\n", settings.fixedEcefY);
    //    systemPrintf("major (should be -471680858): %ld\r\n", majorEcefY);
    //    systemPrintf("minor (should be -7): %ld\r\n", minorEcefY);

    //Units are cm with a high precision extension so -1234.5678 should be called: (-123456, -78)
    //-1280208.308,-4716803.847,4086665.811 is SparkFun HQ so...

    response &= theGNSS.newCfgValset();
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 2); //Fixed
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_POS_TYPE, 0); //Position in ECEF
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_ECEF_X, majorEcefX);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_ECEF_X_HP, minorEcefX);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_ECEF_Y, majorEcefY);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_ECEF_Y_HP, minorEcefY);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_ECEF_Z, majorEcefZ);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_ECEF_Z_HP, minorEcefZ);
    response &= theGNSS.sendCfgValset();
  }
  else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC)
  {
    //Add height of instrument (HI) to fixed altitude
    //https://www.e-education.psu.edu/geog862/node/1853
    //For example, if HAE is at 100.0m, + 2m stick + 73mm ARP = 102.073
    float totalFixedAltitude = settings.fixedAltitude + (settings.antennaHeight / 1000.0) + (settings.antennaReferencePoint / 1000.0);

    //Break coordinates into main and high precision parts
    //The type casting should not effect rounding of original double cast coordinate
    int64_t majorLat = settings.fixedLat * 10000000;
    int64_t minorLat = ((settings.fixedLat * 10000000) - majorLat) * 100;
    int64_t majorLong = settings.fixedLong * 10000000;
    int64_t minorLong = ((settings.fixedLong * 10000000) - majorLong) * 100;
    int32_t majorAlt = totalFixedAltitude * 100;
    int32_t minorAlt = ((totalFixedAltitude * 100) - majorAlt) * 100;

    //    systemPrintf("fixedLong (should be -105.184774720): %0.09f\r\n", settings.fixedLong);
    //    systemPrintf("major (should be -1051847747): %lld\r\n", majorLat);
    //    systemPrintf("minor (should be -20): %lld\r\n", minorLat);
    //
    //    systemPrintf("fixedLat (should be 40.090335429): %0.09f\r\n", settings.fixedLat);
    //    systemPrintf("major (should be 400903354): %lld\r\n", majorLong);
    //    systemPrintf("minor (should be 29): %lld\r\n", minorLong);
    //
    //    systemPrintf("fixedAlt (should be 1560.2284): %0.04f\r\n", settings.fixedAltitude);
    //    systemPrintf("major (should be 156022): %ld\r\n", majorAlt);
    //    systemPrintf("minor (should be 84): %ld\r\n", minorAlt);

    response &= theGNSS.newCfgValset();
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 2); //Fixed
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_POS_TYPE, 1); //Position in LLH
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_LAT, majorLat);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_LAT_HP, minorLat);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_LON, majorLong);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_LON_HP, minorLong);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_HEIGHT, majorAlt);
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_HEIGHT_HP, minorAlt);
    response &= theGNSS.sendCfgValset();
  }

  return (response);
}

//This function gets called from the SparkFun u-blox Arduino Library.
//As each RTCM byte comes in you can specify what to do with it
//Useful for passing the RTCM correction data to a radio, Ntrip broadcaster, etc.
void DevUBLOXGNSS::processRTCM(uint8_t incoming)
{
  //Check for too many digits
  if (settings.enableResetDisplay == true)
  {
    if (rtcmPacketsSent > 99) rtcmPacketsSent = 1; //Trim to two digits to avoid overlap
  }
  else
  {
    if (rtcmPacketsSent > 999) rtcmPacketsSent = 1; //Trim to three digits to avoid log icon and increasing bar
  }

  //Determine if we should check this byte with the RTCM checker or simply pass it along
  bool passAlongIncomingByte = true;

  if (settings.enableRtcmMessageChecking == true)
    passAlongIncomingByte &= checkRtcmMessage(incoming);

  //Give this byte to the various possible transmission methods
  if (passAlongIncomingByte)
  {
    rtcmLastReceived = millis();
    rtcmBytesSent++;

    ntripServerProcessRTCM(incoming);

    espnowProcessRTCM(incoming);
  }
}
