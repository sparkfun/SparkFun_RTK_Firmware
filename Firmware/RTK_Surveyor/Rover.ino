//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  if (online.gnss == false)
  {
    log_d("GNSS not online");
    return (false);
  }

  //If our settings haven't changed, and this is first config since power on, trust ZED's settings
  if (settings.updateZEDSettings == false && firstPowerOn == true)
  {
    firstPowerOn = false; //Next time user switches modes, new settings will be applied
    log_d("Skipping ZED Rover configuration");
    return (true);
  }

  firstPowerOn = false; //If we switch between rover/base in the future, force config of module.

  theGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  bool response = true;

  //Set output rate
  response &= theGNSS.newCfgValset();
  response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, settings.measurementRate);
  response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, settings.navigationRate);

  //Survey mode is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0); //Disable survey-in mode
  
  response &= theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_DYNMODEL, (dynModel)settings.dynamicModel); // Set dynamic model

  //RTCM is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
  {
    //Disable RTCM sentences from being generated on I2C / SPI, USB, and UART2
    if (USE_I2C_GNSS)
    {
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_I2C, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_I2C, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_I2C, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_I2C, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_I2C, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_I2C, 0);
    }
    else
    {
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_SPI, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_SPI, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_SPI, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_SPI, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_SPI, 0);
      response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_SPI, 0);
    }
  
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_USB, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_USB, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_USB, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_USB, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_USB, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_USB, 0);

    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART2, 0);
  }

  response &= theGNSS.addCfgValset(UBLOX_CFG_NMEA_MAINTALKERID, 3); //Return talker ID to GNGGA after NTRIP Client set to GPGGA

  response &= theGNSS.addCfgValset(UBLOX_CFG_NMEA_HIGHPREC, 1); //Enable high precision NMEA
  response &= theGNSS.addCfgValset(UBLOX_CFG_NMEA_SVNUMBERING, 1); //Enable extended satellite numbering

  if (zedModuleType == PLATFORM_F9R)
  {
    response &= theGNSS.addCfgValset(UBLOX_CFG_SFCORE_USE_SF, settings.enableSensorFusion); //Enable/disable sensor fusion
    response &= theGNSS.addCfgValset(UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA, settings.autoIMUmountAlignment); //Enable/disable Automatic IMU-mount Alignment
  }

  response &= theGNSS.sendCfgValset(); //Closing - 27 keys
  
  if (response == false)
    log_d("Rover config failed");

  return (response);
}

//Turn on the three accuracy LEDs depending on our current HPA (horizontal positional accuracy)
void updateAccuracyLEDs()
{
  //Update the horizontal accuracy LEDs only every second or so
  if (millis() - lastAccuracyLEDUpdate > 2000)
  {
    lastAccuracyLEDUpdate = millis();

    if (online.gnss == true)
    {
      if (horizontalAccuracy > 0)
      {
        if (settings.enablePrintRoverAccuracy)
        {
          systemPrint("Rover Accuracy (m): ");
          systemPrint(horizontalAccuracy, 4); // Print the accuracy with 4 decimal places
          systemPrintln();
        }

        if (productVariant == RTK_SURVEYOR)
        {
          if (horizontalAccuracy <= 0.02)
          {
            digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
            digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
            digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
          }
          else if (horizontalAccuracy <= 0.100)
          {
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
            digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
          }
          else if (horizontalAccuracy <= 1.0000)
          {
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
          }
          else if (horizontalAccuracy > 1.0)
          {
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
          }
        }
      }
      else if (settings.enablePrintRoverAccuracy)
      {
        systemPrint("Rover Accuracy: ");
        systemPrint(horizontalAccuracy);
        systemPrint(" ");
        systemPrint("No lock. SIV: ");
        systemPrint(numSV);
        systemPrintln();
      }
    } //End GNSS online checking
  } //Check every 2000ms
}

//These are the callbacks that get regularly called, globals are updated
void storePVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  altitude = ubxDataStruct->height / 1000.0;

  gnssDay = ubxDataStruct->day;
  gnssMonth = ubxDataStruct->month;
  gnssYear = ubxDataStruct->year;

  gnssHour = ubxDataStruct->hour;
  gnssMinute = ubxDataStruct->min;
  gnssSecond = ubxDataStruct->sec;
  mseconds = ceil((ubxDataStruct->iTOW % 1000) / 10.0); //Limit to first two digits

  numSV = ubxDataStruct->numSV;
  fixType = ubxDataStruct->fixType;
  carrSoln = ubxDataStruct->flags.bits.carrSoln;

  validDate = ubxDataStruct->valid.bits.validDate;
  validTime = ubxDataStruct->valid.bits.validTime;
  confirmedDate = ubxDataStruct->flags2.bits.confirmedDate;
  confirmedTime = ubxDataStruct->flags2.bits.confirmedTime;

  pvtUpdated = true;
}

void storeHPdata(UBX_NAV_HPPOSLLH_data_t *ubxDataStruct)
{
  horizontalAccuracy = ((float)ubxDataStruct->hAcc) / 10000.0; // Convert hAcc from mm*0.1 to m

  latitude = ((double)ubxDataStruct->lat) / 10000000.0;
  latitude += ((double)ubxDataStruct->latHp) / 1000000000.0;
  longitude = ((double)ubxDataStruct->lon) / 10000000.0;
  longitude += ((double)ubxDataStruct->lonHp) / 1000000000.0;
}
