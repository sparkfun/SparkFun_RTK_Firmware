
//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  bool response = true;
  int maxWait = 2000;

  //Survey mode is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
  {
    if (i2cGNSS.getSurveyInActive(100) == true)
    {
      response = i2cGNSS.disableSurveyMode(maxWait); //Disable survey
      if (response == false)
        Serial.println(F("Disable Survey failed"));
    }
  }

  // Set dynamic model
  if (i2cGNSS.getDynamicModel(maxWait) != settings.dynamicModel)
  {
    response = i2cGNSS.setDynamicModel((dynModel)settings.dynamicModel, maxWait);
    if (response == false)
      Serial.println(F("setDynamicModel failed"));
  }

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
  response &= configureGNSSMessageRates(COM_PORT_UART1, ubxMessages); //Make sure the appropriate messages are enabled

  if (response == false)
    Serial.println(F("Disable RTCM failed"));

  response = setNMEASettings(); //Enable high precision NMEA and extended sentences
  if (response == false)
    Serial.println(F("setNMEASettings failed"));

  response = true; //Reset
  if (zedModuleType == PLATFORM_F9R)
  {
    i2cGNSS.setESFAutoAlignment(settings.autoIMUmountAlignment); //Configure UBX-CFG-ESFALG Automatic IMU-mount Alignment
  }

  //The last thing we do is set output rate.
  response = true; //Reset
  if (i2cGNSS.getMeasurementRate() != settings.measurementRate)
  {
    response &= i2cGNSS.setMeasurementRate(settings.measurementRate);
  }
  if (i2cGNSS.getNavigationRate() != settings.navigationRate)
  {
    response &= i2cGNSS.setNavigationRate(settings.navigationRate);
  }
  if (response == false)
    Serial.println(F("Set Nav Rate failed"));

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
    Serial.println(F("NMEA setting failed"));
    return (false);
  }

  customPayload[3] |= (1 << 3); //Set the highPrec flag

  customPayload[8] = 1; //Enable extended satellite numbering

  // Now we write the custom packet back again to change the setting
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("NMEA setting failed"));
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
    Serial.println(F("Get Constellation failed"));
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
    Serial.println(F("Set Constellation failed"));
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
    Serial.println(F("Constellation setting failed"));
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

  Serial.print(F("locateGNSSID failed: "));
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

    uint32_t accuracy = i2cGNSS.getHorizontalAccuracy();

    if (accuracy > 0)
    {
      // Convert the horizontal accuracy (mm * 10^-1) to a float
      float f_accuracy = accuracy;
      f_accuracy = f_accuracy / 10000.0; // Convert from mm * 10^-1 to m

      Serial.print(F("Rover Accuracy (m): "));
      Serial.print(f_accuracy, 4); // Print the accuracy with 4 decimal places
      Serial.println();

      if (productVariant == RTK_SURVEYOR)
      {
        if (f_accuracy <= 0.02)
        {
          digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
          digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
          digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
        }
        else if (f_accuracy <= 0.100)
        {
          digitalWrite(pin_positionAccuracyLED_1cm, LOW);
          digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
          digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
        }
        else if (f_accuracy <= 1.0000)
        {
          digitalWrite(pin_positionAccuracyLED_1cm, LOW);
          digitalWrite(pin_positionAccuracyLED_10cm, LOW);
          digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
        }
        else if (f_accuracy > 1.0)
        {
          digitalWrite(pin_positionAccuracyLED_1cm, LOW);
          digitalWrite(pin_positionAccuracyLED_10cm, LOW);
          digitalWrite(pin_positionAccuracyLED_100cm, LOW);
        }
      }
    }
    else
    {
      Serial.print(F("Rover Accuracy: "));
      Serial.print(accuracy);
      Serial.print(" ");
      Serial.print(F("No lock. SIV: "));
      Serial.print(i2cGNSS.getSIV());
      Serial.println();
    }
  }
}
