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

    Serial.print(F("1) Log to microSD: "));
    if (settings.zedOutputLogging) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (settings.zedOutputLogging == true)
    {
      Serial.print(F("2) Update file timestamp with each write: "));
      if (settings.frequentFileAccessTimestamps == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("3) Set max logging time: "));
      Serial.print(settings.maxLogTime_minutes);
      Serial.println(F(" minutes"));
    }

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.zedOutputLogging ^= 1;

      if(online.microSD == true && settings.zedOutputLogging == true) 
        startLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging
    }
    else if (settings.zedOutputLogging == true)
    {
      if (incoming == '2')
        settings.frequentFileAccessTimestamps ^= 1;
      else if (incoming == '3')
      {
        Serial.print(F("Enter max minutes to log data: "));
        int maxMinutes = getNumber(menuTimeout); //Timeout after x seconds
        if (maxMinutes < 0 || maxMinutes > 60*48) //Arbitrary 48 hour limit
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
void beginDataLogging()
{
  if (online.dataLogging == false)
  {
    if (online.microSD == true && settings.zedOutputLogging == true)
    {
      char gnssDataFileName[40] = "";

      //If we don't have a file yet, create one
      if (strlen(gnssDataFileName) == 0)
      {
        //Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.txt
        if (myGPS.getTimeValid() == true && myGPS.getDateValid() == true)
        {
          sprintf(gnssDataFileName, "SFE_Surveyor_%02d%02d%02d_%02d%02d%02d.txt",
                  myGPS.getYear() - 2000, myGPS.getMonth(), myGPS.getDay(),
                  myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond()
                 );
        }
        else
        {
          Serial.println(F("beginDataLogging: No date/time available. No file created."));
          delay(1000); //Give the receiver time to get a lock
          online.dataLogging = false;
          return;
        }
      }

      // O_CREAT - create the file if it does not exist
      // O_APPEND - seek to the end of the file prior to each write
      // O_WRITE - open for write
      if (gnssDataFile.open(gnssDataFileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println(F("Failed to create sensor data file"));
        online.dataLogging = false;
        return;
      }

      updateDataFileCreate(&gnssDataFile); // Update the file to create time & date

      Serial.printf("Log file created: %s\n", gnssDataFileName);
      online.dataLogging = true;
    }
    else
      online.dataLogging = false;
  }
}

//Updates the timestemp on a given data file
void updateDataFileAccess(SdFile *dataFile)
{
  if (online.dataLogging == true)
  {
    if (myGPS.getTimeValid() == true && myGPS.getDateValid() == true)
    {
      //Update the file access time
      dataFile->timestamp(T_ACCESS, myGPS.getYear(), myGPS.getMonth(), myGPS.getDay(), myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond());
      //Update the file write time
      dataFile->timestamp(T_WRITE, myGPS.getYear(), myGPS.getMonth(), myGPS.getDay(), myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond());
    }
  }
}

void updateDataFileCreate(SdFile *dataFile)
{
  if (online.dataLogging == true)
  {
    if (myGPS.getTimeValid() == true && myGPS.getDateValid() == true)
    {
      //Update the file create time
      dataFile->timestamp(T_CREATE, myGPS.getYear(), myGPS.getMonth(), myGPS.getDay(), myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond());
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
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getNMEASettings failed!"));
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}
