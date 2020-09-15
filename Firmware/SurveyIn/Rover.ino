//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  bool response = myGPS.disableSurveyMode(); //Disable survey

  return (setNMEASettings());
}

//The Ublox library doesn't directly support NMEA configuration so let's do it manually
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
