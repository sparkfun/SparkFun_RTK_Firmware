//Control the messages that get logged to SD (NMEA messages to NMEA file, UBX raw to UBX)
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

    Serial.print(F("1) Log NMEA GGA: "));
    if (settings.log.gga == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Log NMEA GSA: "));
    if (settings.log.gsa == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Log NMEA GSV: "));
    if (settings.log.gsv == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Log NMEA RMC: "));
    if (settings.log.rmc == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("5) Log NMEA GST: "));
    if (settings.log.gst == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("6) Log UBX RAWX: "));
    if (settings.log.rawx == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("7) Log UBX SFRBX: "));
    if (settings.log.sfrbx == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (logNMEAMessages() == true || logUBXMessages() == true)
    {
      Serial.print(F("8) Set max logging time: "));
      Serial.print(settings.maxLogTime_minutes);
      Serial.println(F(" minutes"));

      Serial.print(F("9) Update file timestamp with each write: "));
      if (settings.frequentFileAccessTimestamps == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));
    }

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.log.gga ^= 1;
    }
    else if (incoming == '2')
    {
      settings.log.gsa ^= 1;
    }
    else if (incoming == '3')
    {
      settings.log.gsv ^= 1;
    }
    else if (incoming == '4')
    {
      settings.log.rmc ^= 1;
    }
    else if (incoming == '5')
    {
      settings.log.gst ^= 1;
    }
    else if (incoming == '6')
    {
      settings.log.rawx ^= 1;
    }
    else if (incoming == '7')
    {
      settings.log.sfrbx ^= 1;
    }
    else if (logNMEAMessages() == true || logUBXMessages() == true)
    {
      if (incoming == '8')
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
      else if (incoming == '9')
      {
        settings.frequentFileAccessTimestamps ^= 1;
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

  //Based on current settings, update the logging options within the GNSS library
  i2cGNSS.logRXMRAWX(settings.log.rawx);
  i2cGNSS.logRXMSFRBX(settings.log.sfrbx);

  if (logNMEAMessages() == true)
    i2cGNSS.setNMEALoggingMask(SFE_UBLOX_FILTER_NMEA_ALL); // Enable logging of all enabled NMEA messages
  else
    i2cGNSS.setNMEALoggingMask(0); // Disable logging of all enabled NMEA messages

  //Push these options to the ZED-F9P
  bool response = enableMessages(COM_PORT_I2C, settings.log); //Make sure the appropriate messages are enabled
  if (response == false)
  {
    Serial.println(F("menuLog: Failed to enable I2C messages - Try 1"));
    //Try again
    response = enableMessages(COM_PORT_I2C, settings.log); //Make sure the appropriate messages are enabled
    if (response == false)
      Serial.println(F("menuLog: Failed to enable I2C messages - Try 2"));
    else
      Serial.println(F("menuLog: I2C messages successfully enabled"));
  }
}

//Creates a log if logging is enabled, and SD is detected
//Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
void beginLogging()
{
  if (online.logging == false)
  {
    if (online.microSD == true &&
        (logUBXMessages() == true || logNMEAMessages() == true)
       )
    {
      char fileName[40] = "";

      i2cGNSS.checkUblox();

      //Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
      bool haveLogTime = false;
      if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
        haveLogTime = true;
      if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
        haveLogTime = true;

      if (haveLogTime == false)
      {
        Serial.println(F("beginLoggingUBX: No date/time available. No file created."));
        delay(1000); //Give the receiver time to get a lock
        online.logging = false;
        return;
      }

      sprintf(fileName, "SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.ubx", //SdFat library
              i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
              i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
             );

      //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
      if (xSemaphoreTake(xFATSemaphore, fatSemaphore_maxWait) == pdPASS)
      {
        // O_CREAT - create the file if it does not exist
        // O_APPEND - seek to the end of the file prior to each write
        // O_WRITE - open for write
        if (ubxFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
        {
          Serial.printf("Failed to create GNSS UBX data file: %s\n", fileName);
          delay(1000);
          online.logging = false;
          xSemaphoreGive(xFATSemaphore);
          return;
        }

        updateDataFileCreate(&ubxFile); // Update the file to create time & date

        startLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging

        xSemaphoreGive(xFATSemaphore);
      }
      else
      {
        Serial.println(F("Failed to get file system lock to create GNSS UBX data file"));
        online.logging = false;
        return;
      }

      Serial.printf("Log file created: %s\n", fileName);
      online.logging = true;
    }
    else
      online.logging = false;
  }
}

//Updates the timestemp on a given data file
void updateDataFileAccess(SdFile *dataFile)
{
  if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true)
  {
    //Update the file access time
    dataFile->timestamp(T_ACCESS, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
    //Update the file write time
    dataFile->timestamp(T_WRITE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
  }
}

void updateDataFileCreate(SdFile *dataFile)
{
  if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true)
  {
    //Update the file create time
    dataFile->timestamp(T_CREATE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
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
    Serial.println(F("getRAWXSettings failed!"));
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
    Serial.println(F("getSFRBXSettings failed!"));
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}
