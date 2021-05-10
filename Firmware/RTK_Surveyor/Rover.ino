
//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  bool response = true;
  int maxWait = 2000;

  response = i2cGNSS.disableSurveyMode(maxWait); //Disable survey
  if (response == false)
    Serial.println(F("Disable Survey failed"));

  // Set dynamic model
  if (i2cGNSS.getDynamicModel(maxWait) != DYN_MODEL_PORTABLE)
  {
    response = i2cGNSS.setDynamicModel(DYN_MODEL_PORTABLE, maxWait);
    if (response == false)
      Serial.println(F("setDynamicModel failed"));
  }

  //Disable RTCM sentences
  response = true; //Reset
  response &= disableRTCMSentences(COM_PORT_I2C);
  response &= disableRTCMSentences(COM_PORT_UART2);
  response &= disableRTCMSentences(COM_PORT_UART1);
  response &= disableRTCMSentences(COM_PORT_USB);
  if (response == false)
    Serial.println(F("Disable RTCM failed"));

  response = setNMEASettings(); //Enable high precision NMEA and extended sentences
  if (response == false)
    Serial.println(F("setNMEASettings failed"));

  response = true; //Reset
  if (settings.enableSBAS == true)
    response &= setSBAS(true); //Enable SBAS
  else
    response &= setSBAS(false); //Disable SBAS. Work around for RTK LED not working in v1.13 firmware.
  if (response == false)
    Serial.println(F("Set SBAS failed"));

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

//Returns true if SBAS is enabled
bool getSBAS()
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
    Serial.println(F("Get SBAS failed"));
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

  uint16_t maxWait = 1250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Set SBAS failed"));
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
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("SBAS setting failed"));
    return (false);
  }

  return (true);
}

//Turn on the three accuracy LEDs depending on our current HPA (horizontal positional accuracy)
void updateAccuracyLEDs()
{
  //Update the horizontal accuracy LEDs only every second or so
  if (millis() - lastAccuracyLEDUpdate > 2000)
  {
    lastAccuracyLEDUpdate = millis();

    uint32_t accuracy = i2cGNSS.getHorizontalAccuracy(250);

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
