
//Based on position accuracy, update the green LEDs
bool updateRoverStatus()
{
  //We're in rover mode so update the accuracy LEDs
  uint32_t accuracy = myGPS.getHorizontalAccuracy();

  // Convert the horizontal accuracy (mm * 10^-1) to a float
  float f_accuracy = accuracy;
  // Now convert to m
  f_accuracy = f_accuracy / 10000.0; // Convert from mm * 10^-1 to m

  Serial.print("Rover Accuracy (m): ");
  Serial.print(f_accuracy, 4); // Print the accuracy with 4 decimal places

  if (f_accuracy <= 0.02)
  {
    Serial.print(" 0.02m LED");
    digitalWrite(positionAccuracyLED_20mm, HIGH);
    digitalWrite(positionAccuracyLED_100mm, HIGH);
    digitalWrite(positionAccuracyLED_1000mm, HIGH);
  }
  else if (f_accuracy <= 0.100)
  {
    Serial.print(" 0.1m LED");
    digitalWrite(positionAccuracyLED_20mm, LOW);
    digitalWrite(positionAccuracyLED_100mm, HIGH);
    digitalWrite(positionAccuracyLED_1000mm, HIGH);
  }
  else if (f_accuracy <= 1.0000)
  {
    Serial.print(" 1m LED");
    digitalWrite(positionAccuracyLED_20mm, LOW);
    digitalWrite(positionAccuracyLED_100mm, LOW);
    digitalWrite(positionAccuracyLED_1000mm, HIGH);
  }
  else if (f_accuracy > 1.0)
  {
    Serial.print(" No LEDs");
    digitalWrite(positionAccuracyLED_20mm, LOW);
    digitalWrite(positionAccuracyLED_100mm, LOW);
    digitalWrite(positionAccuracyLED_1000mm, LOW);
  }
  Serial.println();
}

//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  bool response = myGPS.disableSurveyMode(); //Disable survey

  // Set dynamic model
  if (myGPS.getDynamicModel() != DYN_MODEL_PORTABLE)
  {
    if (myGPS.setDynamicModel(DYN_MODEL_PORTABLE) == false)
    {
      Serial.println(F("Warning: setDynamicModel failed!"));
      return (false);
    }
  }

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
