//Updates the timestemp on a given data file
void updateDataFileAccess(File *dataFile)
//void updateDataFileAccess(SdFile *dataFile)
{
//    if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true)
//    {
//      //Update the file access time
//      dataFile->timestamp(T_ACCESS, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
//      //Update the file write time
//      dataFile->timestamp(T_WRITE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
//    }
}

void updateDataFileCreate(File *dataFile)
//void updateDataFileCreate(SdFile *dataFile)
{
//    if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true)
//    {
//      //Update the file create time
//      dataFile->timestamp(T_CREATE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
//    }
}

//Given a portID, return the RAWX value for the given port
uint8_t getRAWXSettings(uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_CLASS_RXM;
  settingPayload[1] = UBX_RXM_RAWX;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getNMEASettings failed!"));
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}

//Given a portID, return the RAWX value for the given port
uint8_t getSFRBXSettings(uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_CLASS_RXM;
  settingPayload[1] = UBX_RXM_SFRBX;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getNMEASettings failed!"));
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}

//Creates a log if logging is enabled, and SD is detected
//Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.txt
void beginLoggingNMEA()
{
  if (online.nmeaLogging == false)
  {
    if (online.microSD == true && settings.logNMEA == true)
    {
      char fileName[40] = "";

      //Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.txt
      if (i2cGNSS.getTimeValid() == false || i2cGNSS.getDateValid() == false)
      {
        Serial.println(F("beginLoggingNMEA: No date/time available. No file created."));
        delay(1000); //Give the receiver time to get a lock
        online.nmeaLogging = false;
        return;
      }

      sprintf(fileName, "/SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.txt",
//      sprintf(fileName, "SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.txt",
              i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
              i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
             );


      // O_CREAT - create the file if it does not exist
      // O_APPEND - seek to the end of the file prior to each write
      // O_WRITE - open for write
      nmeaFile = SD.open(fileName, FILE_WRITE);
      if (!nmeaFile)
//      if (nmeaFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println(F("Failed to create GNSS NMEA data file"));
        online.nmeaLogging = false;
        return;
      }

      updateDataFileCreate(&nmeaFile); // Update the file to create time & date

      Serial.printf("Log file created: %s\n", fileName);
      online.nmeaLogging = true;
    }
    else
      online.nmeaLogging = false;
  }
}

//Creates a log if logging is enabled, and SD is detected
//Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
void beginLoggingUBX()
{
  if (online.ubxLogging == false)
  {
    if (online.microSD == true && settings.logUBX == true)
    {
      char fileName[40] = "";

      //Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.txt
      if (i2cGNSS.getTimeValid() == false || i2cGNSS.getDateValid() == false)
      {
        Serial.println(F("beginLoggingUBX: No date/time available. No file created."));
        delay(1000); //Give the receiver time to get a lock
        online.ubxLogging = false;
        return;
      }

      sprintf(fileName, "/SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.ubx",
//      sprintf(fileName, "SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.ubx",
              i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
              i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
             );

      // O_CREAT - create the file if it does not exist
      // O_APPEND - seek to the end of the file prior to each write
      // O_WRITE - open for write
      ubxFile = SD.open(fileName, FILE_WRITE);
      if (!ubxFile)
//      if (ubxFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println(F("Failed to create GNSS UBX data file"));
        online.ubxLogging = false;
        delay(1000);
        return;
      }

      updateDataFileCreate(&ubxFile); // Update the file to create time & date

      Serial.printf("Log file created: %s\n", fileName);
      online.ubxLogging = true;
    }
    else
      online.ubxLogging = false;
  }
}
