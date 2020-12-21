//Based on position accuracy, update the green LEDs
bool updateRoverStatus()
{
  //Update the horizontal accuracy LEDs only every second or so
  if (millis() - lastRoverUpdate > 2000)
  {
    lastRoverUpdate = millis();

    uint32_t accuracy = myGPS.getHorizontalAccuracy(250);

    if (accuracy > 0)
    {
      // Convert the horizontal accuracy (mm * 10^-1) to a float
      float f_accuracy = accuracy;
      f_accuracy = f_accuracy / 10000.0; // Convert from mm * 10^-1 to m

      Serial.print(F("Rover Accuracy (m): "));
      Serial.print(f_accuracy, 4); // Print the accuracy with 4 decimal places

      if (f_accuracy <= 0.02)
      {
        Serial.print(F(" 0.01m LED"));
        digitalWrite(positionAccuracyLED_1cm, HIGH);
        digitalWrite(positionAccuracyLED_10cm, HIGH);
        digitalWrite(positionAccuracyLED_100cm, HIGH);
      }
      else if (f_accuracy <= 0.100)
      {
        Serial.print(F(" 0.1m LED"));
        digitalWrite(positionAccuracyLED_1cm, LOW);
        digitalWrite(positionAccuracyLED_10cm, HIGH);
        digitalWrite(positionAccuracyLED_100cm, HIGH);
      }
      else if (f_accuracy <= 1.0000)
      {
        Serial.print(F(" 1m LED"));
        digitalWrite(positionAccuracyLED_1cm, LOW);
        digitalWrite(positionAccuracyLED_10cm, LOW);
        digitalWrite(positionAccuracyLED_100cm, HIGH);
      }
      else if (f_accuracy > 1.0)
      {
        Serial.print(F(" No LEDs"));
        digitalWrite(positionAccuracyLED_1cm, LOW);
        digitalWrite(positionAccuracyLED_10cm, LOW);
        digitalWrite(positionAccuracyLED_100cm, LOW);
      }
      Serial.println();
    }
    else
    {
      Serial.print(F("Rover Accuracy: "));
      Serial.print(accuracy);
      Serial.print(" ");
      Serial.print(F("No lock. SIV: "));
      Serial.print(myGPS.getSIV());
      Serial.println();
    }
  }
}

//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  bool response = myGPS.disableSurveyMode(); //Disable survey

  //Set output rate
  if (myGPS.getNavigationFrequency() != settings.gnssMeasurementFrequency)
  {
    response &= myGPS.setNavigationFrequency(settings.gnssMeasurementFrequency); //Set output in Hz
  }

  // Set dynamic model
  if (myGPS.getDynamicModel() != DYN_MODEL_PORTABLE)
  {
    response &= myGPS.setDynamicModel(DYN_MODEL_PORTABLE);
    if (response == false)
      Serial.println(F("setDynamicModel failed!"));
  }

  //Disable RTCM sentences
  response &= disableRTCMSentences(COM_PORT_I2C);
  response &= disableRTCMSentences(COM_PORT_UART2);
  response &= disableRTCMSentences(COM_PORT_UART1);
  response &= disableRTCMSentences(COM_PORT_USB);
  if (response == false)
    Serial.println(F("Disable RTCM failed"));

  response &= setNMEASettings(); //Enable high precision NMEA and extended sentences

  if (settings.enableSBAS == true)
    response &= setSBAS(true); //Enable SBAS
  else
    response &= setSBAS(false); //Disable SBAS. Work around for RTK LED not working in v1.13 firmware.

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

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("NMEA setting failed!"));
    return (false);
  }

  customPayload[3] |= (1 << 3); //Set the highPrec flag

  customPayload[8] = 1; //Enable extended satellite numbering

  // Now we write the custom packet back again to change the setting
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("NMEA setting failed!"));
    return (false);
  }
  return (true);
}

//Returns true if SBAS is enabled
bool getSBAS()
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Get SBAS failed!"));
    return (false);
  }

  if (customPayload[8 + 8 * 1] & (1 << 0)) return true; //Check if bit 0 is set
  return false;
}

//The u-blox library doesn't directly support SBAS control so let's do it manually
bool setSBAS(bool enableSBAS)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Set SBAS failed!"));
    return (false);
  }

  if (enableSBAS)
  {
    customPayload[8 + 8 * 1] |= (1 << 0); //Set the enable bit
    //We must enable the gnssID as well
    customPayload[8 + 8 * 1 + 2] |= (1 << 0); //Set the enable bit (16) for SBAS L1C/A
  }
  else
  {
    customPayload[8 + 8 * 1] &= ~(1 << 0); //Clear the enable bit
  }

  // Now we write the custom packet back again to change the setting
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("SBAS setting failed!"));
    return (false);
  }

  return (true);
}
