//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  if (online.gnss == false) return (false);

  bool response = true;
  int maxWait = 2000;

  //If our settings haven't changed, and this is first config since power on, trust ZED's settings
  if (settings.updateZEDSettings == false && firstPowerOn == true)
  {
    firstPowerOn = false; //Next time user switches modes, new settings will be applied
    log_d("Skipping ZED Rover configuration");
    return (true);
  }

  firstPowerOn = false; //If we switch between rover/base in the future, force config of module.

  i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  //The first thing we do is go to 1Hz to lighten any I2C traffic from a previous configuration
  if (i2cGNSS.getNavigationFrequency(maxWait) != 1)
    response &= i2cGNSS.setNavigationFrequency(1, maxWait);
  if (response == false)
    Serial.println("Set rate failed");

  //Survey mode is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
  {
    response = i2cGNSS.setSurveyMode(0, 0, 0); //Disable Survey-In or Fixed Mode
    if (response == false)
      Serial.println("Disable TMODE3 failed");
  }

  // Set dynamic model
  if (i2cGNSS.getDynamicModel(maxWait) != settings.dynamicModel)
  {
    response = i2cGNSS.setDynamicModel((dynModel)settings.dynamicModel, maxWait);
    if (response == false)
      Serial.println("setDynamicModel failed");
  }

#define OUTPUT_SETTING 14

  getPortSettings(COM_PORT_I2C); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_UBX))
    //response &= i2cGNSS.setPortOutput(COM_PORT_I2C, COM_TYPE_UBX); //Turn off all traffic except UBX to reduce I2C bus errors and ESP32 resets as much as possible
    response &= i2cGNSS.setPortOutput(COM_PORT_I2C, COM_TYPE_NMEA | COM_TYPE_UBX | COM_TYPE_RTCM3); //We need NMEA GGA output for NTRIP Client.

  //RTCM is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
  {
    //Disable RTCM sentences on I2C, USB, and UART2
    response = true; //Reset
    response &= disableRTCMSentences(COM_PORT_I2C);
    response &= disableRTCMSentences(COM_PORT_UART2);
    response &= disableRTCMSentences(COM_PORT_USB);
  }

  //Re-enable any RTCM msgs on UART1 the user has set within settings
  response &= configureGNSSMessageRates(COM_PORT_UART1, settings.ubxMessages); //Make sure the appropriate messages are enabled

  if (response == false)
    Serial.println("Disable RTCM failed");

  response = i2cGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GN); //Turn GNGGA back on after NTRIP Client
  if (response == false)
    Serial.println("setMainTalkerID failed");

  response = setNMEASettings(); //Enable high precision NMEA and extended sentences
  if (response == false)
    Serial.println("setNMEASettings failed");

  response = true; //Reset
  if (zedModuleType == PLATFORM_F9R)
  {
    setSensorFusion(settings.enableSensorFusion); //Enable/disable sensor fusion
    i2cGNSS.setESFAutoAlignment(settings.autoIMUmountAlignment); //Configure UBX-CFG-ESFALG Automatic IMU-mount Alignment
  }

  //The last thing we do is set output rate.
  response = true; //Reset

  if (i2cGNSS.getMeasurementRate() != settings.measurementRate || i2cGNSS.getNavigationRate() != settings.navigationRate)
  {
    float secondsBetweenSolutions = (settings.measurementRate * settings.navigationRate) / 1000.0;
    setMeasurementRates(secondsBetweenSolutions); //This will set settings.measurementRate, settings.navigationRate, and GSV message
  }

  response &= i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  if (response == false)
    Serial.println("Module failed to save.");

  return (response);
}

//The u-blox library doesn't directly support NMEA configuration so let's do it manually
bool setNMEASettings()
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_NMEA; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("NMEA setting failed");
    return (false);
  }

  customPayload[3] |= (1 << 3); //Set the highPrec flag

  customPayload[8] = 1; //Enable extended satellite numbering

  // Now we write the custom packet back again to change the setting
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println("NMEA setting failed");
    return (false);
  }
  return (true);
}

//Returns true if constellation is enabled
bool getConstellation(uint8_t constellationID)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("Get Constellation failed");
    return (false);
  }

  if (customPayload[locateGNSSID(customPayload, constellationID) + 4] & (1 << 0)) return true; //Check if enable bit is set
  return false;
}

//The u-blox library doesn't directly support constellation control so let's do it manually
//Also allows the enable/disable of any constellation (BeiDou, Galileo, etc)
bool setConstellation(uint8_t constellation, bool enable)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("Set Constellation failed");
    return (false);
  }

  if (enable)
  {
    if (constellation == SFE_UBLOX_GNSS_ID_GPS || constellation == SFE_UBLOX_GNSS_ID_QZSS)
    {
      //QZSS must follow GPS
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_GPS) + 4] |= (1 << 0); //Set the enable bit
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 4] |= (1 << 0); //Set the enable bit
    }
    else
    {
      customPayload[locateGNSSID(customPayload, constellation) + 4] |= (1 << 0); //Set the enable bit
    }

    //Set sigCfgMask as well
    if (constellation == SFE_UBLOX_GNSS_ID_GPS || constellation == SFE_UBLOX_GNSS_ID_QZSS)
    {
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_GPS) + 6] |= 0x11; //Enable GPS L1C/A, and L2C

      //QZSS must follow GPS
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 6] = 0x11; //Enable QZSS L1C/A, and L2C - Follow u-center
      //customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 6] = 0x15; //Enable QZSS L1C/A, L1S, and L2C
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_SBAS)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x01; //Enable SBAS L1C/A
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_GALILEO)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x21; //Enable Galileo E1/E5b
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_BEIDOU)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x11; //Enable BeiDou B1I/B2I
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_GLONASS)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x11; //Enable GLONASS L1 and L2
    }
  }
  else //Disable
  {
    //QZSS must follow GPS
    if (constellation == SFE_UBLOX_GNSS_ID_GPS || constellation == SFE_UBLOX_GNSS_ID_QZSS)
    {
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_GPS) + 4] &= ~(1 << 0); //Clear the enable bit

      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 4] &= ~(1 << 0); //Clear the enable bit
    }
    else
    {
      customPayload[locateGNSSID(customPayload, constellation) + 4] &= ~(1 << 0); //Clear the enable bit
    }

  }

  // Now we write the custom packet back again to change the setting
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println("Constellation setting failed");
    return (false);
  }

  return (true);
}

//Given a payload, return the location of a given constellation
//This is needed because IMES is not currently returned in the query packet
//so QZSS and GLONAS are offset by -8 bytes.
uint8_t locateGNSSID(uint8_t *customPayload, uint8_t constellation)
{
  for (int x = 0 ; x < MAX_CONSTELLATIONS ; x++)
  {
    if (customPayload[4 + 8 * x] == constellation) //Test gnssid
      return (4 + x * 8);
  }

  Serial.print("locateGNSSID failed: ");
  Serial.println(constellation);
  return (0);
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
          Serial.print("Rover Accuracy (m): ");
          Serial.print(horizontalAccuracy, 4); // Print the accuracy with 4 decimal places
          Serial.println();
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
        Serial.print("Rover Accuracy: ");
        Serial.print(horizontalAccuracy);
        Serial.print(" ");
        Serial.print("No lock. SIV: ");
        Serial.print(numSV);
        Serial.println();
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
