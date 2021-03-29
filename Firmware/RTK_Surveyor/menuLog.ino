//Enable/disable logging of NMEA sentences and RAW
//Control max logging time (limit to a certain number of minutes)
//The main use case is the setup for a base station to log RAW sentences that then get post processed
void menuLog()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Logging Menu"));

    if (settings.enableSD && online.microSD)
      Serial.println(F("microSD card is detected"));
    else
    {
      beginSD(); //Test if SD is present
      if (online.microSD == true)
        Serial.println(F("microSD card online"));
      else
        Serial.println(F("No microSD card is detected"));
    }

    Serial.print(F("1) Log NMEA to microSD: "));
    if (settings.logNMEA) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Log UBX to microSD: "));
    if (settings.logUBX) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (settings.logNMEA == true || settings.logUBX == true)
    {
      Serial.print(F("3) Update file timestamp with each write: "));
      if (settings.frequentFileAccessTimestamps == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("4) Set max logging time: "));
      Serial.print(settings.maxLogTime_minutes);
      Serial.println(F(" minutes"));
    }

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.logNMEA ^= 1;

      if (online.microSD == true && settings.logNMEA == true)
        startLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging
    }
    else if (incoming == '2')
    {
      settings.logUBX ^= 1;

      if (online.microSD == true && settings.logUBX == true)
        startLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging
    }
    else if (settings.logNMEA == true || settings.logUBX == true)
    {
      if (incoming == '3')
        settings.frequentFileAccessTimestamps ^= 1;
      else if (incoming == '4')
      {
        Serial.print(F("Enter max minutes to log data: "));
        int maxMinutes = getNumber(menuTimeout); //Timeout after x seconds
        if (maxMinutes < 0 || maxMinutes > 60 * 48) //Arbitrary 48 hour limit
        {
          Serial.println(F("Error: max minutes out of range"));
        }
        else
        {
          settings.maxLogTime_minutes = maxMinutes; //Recorded to NVM and file at main menu exit
        }
      }
      else if (incoming == 'x')
        break;
      else if (incoming == STATUS_GETBYTE_TIMEOUT)
        break;
      else
        printUnknown(incoming);
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
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

      sprintf(fileName, "SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.txt",
              i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
              i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
             );


      // O_CREAT - create the file if it does not exist
      // O_APPEND - seek to the end of the file prior to each write
      // O_WRITE - open for write
      if (nmeaFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
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

      sprintf(fileName, "SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.ubx",
              i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
              i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
             );


      // O_CREAT - create the file if it does not exist
      // O_APPEND - seek to the end of the file prior to each write
      // O_WRITE - open for write
      if (ubxFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println(F("Failed to create GNSS UBX data file"));
        online.ubxLogging = false;
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

//Updates the timestemp on a given data file
void updateDataFileAccess(SdFile *dataFile)
{
  if (online.gnss == true) //This function is called at startup and may be called before GNSS is begin()'d
  {
    if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true)
    {
      //Update the file access time
      dataFile->timestamp(T_ACCESS, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
      //Update the file write time
      dataFile->timestamp(T_WRITE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
    }
  }
}

void updateDataFileCreate(SdFile *dataFile)
{
  if (online.gnss == true)
  {
    if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true)
    {
      //Update the file create time
      dataFile->timestamp(T_CREATE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
    }
  }
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
