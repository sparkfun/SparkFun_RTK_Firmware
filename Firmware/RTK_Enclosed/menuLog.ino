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
      Serial.println("microSD card is detected");
    else
    {
      beginSD(); //Test if SD is present
      if (online.microSD == true)
        Serial.println(F("microSD card online"));
      else
        Serial.println(F("No microSD card is detected"));
    }


    Serial.print(F("1) Log ZED-F9P output to microSD: "));
    if (settings.zedOutputLogging) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Toggle GNSS RAWX Output: "));
    if (settings.gnssRAWOutput) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Frequent log file access timestamps: "));
    if (settings.frequentFileAccessTimestamps == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.zedOutputLogging ^= 1;
    }
    else if (incoming == '2')
    {
      settings.gnssRAWOutput ^= 1;

      //Enable the RAWX sentence as necessary
      if (settings.gnssRAWOutput == true)
        myGPS.enableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, COM_PORT_UART1);
      else
        myGPS.disableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, COM_PORT_UART1);

      myGPS.saveConfiguration(); //Save the current settings to flash and BBR
    }
    else if (incoming == '3')
      settings.frequentFileAccessTimestamps ^= 1;
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
                  myGPS.getYear(), myGPS.getMonth(), myGPS.getDay(),
                  myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond()
                 );
        }
        else
        {
          Serial.println("beginDataLogging: No date/time available. No file created.");
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
      dataFile->timestamp(T_ACCESS, (myGPS.getYear() + 2000), myGPS.getMonth(), myGPS.getDay(), myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond());
      //Update the file write time
      dataFile->timestamp(T_WRITE, (myGPS.getYear() + 2000), myGPS.getMonth(), myGPS.getDay(), myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond());
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
      dataFile->timestamp(T_CREATE, (myGPS.getYear() + 2000), myGPS.getMonth(), myGPS.getDay(), myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond());
    }
  }
}
