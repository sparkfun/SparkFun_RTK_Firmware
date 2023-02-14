//Control the messages that get logged to SD
//Control max logging time (limit to a certain number of minutes)
//The main use case is the setup for a base station to log RAW sentences that then get post processed
void menuLog()
{
  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: Logging");

    if (settings.enableSD && online.microSD)
    {
      char sdCardSizeChar[20];
      stringHumanReadableSize(sdCardSize).toCharArray(sdCardSizeChar, sizeof(sdCardSizeChar));
      char sdFreeSpaceChar[20];
      stringHumanReadableSize(sdFreeSpace).toCharArray(sdFreeSpaceChar, sizeof(sdFreeSpaceChar));

      char myString[200];
      snprintf(myString, sizeof(myString),
               "SD card size: %s / Free space: %s",
               sdCardSizeChar,
               sdFreeSpaceChar
              );
      systemPrintln(myString);
    }
    else
      systemPrintln("No microSD card is detected");

    if(bufferOverruns) 
      systemPrintf("Buffer overruns: %d\r\n", bufferOverruns);

    systemPrint("1) Log to microSD: ");
    if (settings.enableLogging == true) systemPrintln("Enabled");
    else systemPrintln("Disabled");

    if (settings.enableLogging == true)
    {
      systemPrint("2) Set max logging time: ");
      systemPrint(settings.maxLogTime_minutes);
      systemPrintln(" minutes");

      systemPrint("3) Set max log length: ");
      systemPrint(settings.maxLogLength_minutes);
      systemPrintln(" minutes");

      if (online.logging == true) systemPrintln("4) Start new log");
    }

    systemPrint("5) Write Marks_date.csv file to microSD: ");
    if (settings.enableMarksFile == true) systemPrintln("Enabled");
    else systemPrintln("Disabled");

    systemPrint("6) Reset system if the SD card is detected but fails to initialize: ");
    if (settings.forceResetOnSDFail == true) systemPrintln("Enabled");
    else systemPrintln("Disabled");

    systemPrintln("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      settings.enableLogging ^= 1;

      //Reset the maximum logging time when logging is disabled to ensure that
      //the next time logging is enabled that the maximum amount of data can be
      //captured.
      if (settings.enableLogging == false)
        startLogTime_minutes = 0;
    }
    else if (incoming == 2 && settings.enableLogging == true)
    {
      systemPrint("Enter max minutes before logging stops: ");
      int maxMinutes = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((maxMinutes != INPUT_RESPONSE_GETNUMBER_EXIT) && (maxMinutes != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (maxMinutes < 0 || maxMinutes > (60 * 24 * 365 * 2)) //Arbitrary 2 year limit. See https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/86
          systemPrintln("Error: Max minutes out of range");
        else
          settings.maxLogTime_minutes = maxMinutes; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 3 && settings.enableLogging == true)
    {
      systemPrint("Enter max minutes of logging before new log is created: ");
      int maxLogMinutes = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((maxLogMinutes != INPUT_RESPONSE_GETNUMBER_EXIT) && (maxLogMinutes != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (maxLogMinutes < 0 || maxLogMinutes > 60 * 48) //Arbitrary 48 hour limit
          systemPrintln("Error: Max minutes out of range");
        else
          settings.maxLogLength_minutes = maxLogMinutes; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 4 && settings.enableLogging == true && online.logging == true)
    {
      endSD(false, true); //Close down file. A new one will be created at the next calling of updateLogs().
      beginLogging();
      setLoggingType(); //Determine if we are standard, PPP, or custom. Changes logging icon accordingly.
    }
    else if (incoming == 5)
    {
      settings.enableMarksFile ^= 1;
    }
    else if (incoming == 6)
    {
      settings.forceResetOnSDFail ^= 1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessages()
{
  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: GNSS Messages");

    systemPrintf("Active messages: %d\r\n", getActiveMessageCount());

    systemPrintln("1) Set NMEA Messages");
    if (zedModuleType == PLATFORM_F9P)
      systemPrintln("2) Set RTCM Messages");
    else if (zedModuleType == PLATFORM_F9R)
      systemPrintln("2) Set ESF Messages");
    systemPrintln("3) Set RXM Messages");
    systemPrintln("4) Set NAV Messages");
    systemPrintln("5) Set MON Messages");
    systemPrintln("6) Set TIM Messages");
    systemPrintln("7) Reset to Surveying Defaults (NMEAx5)");
    systemPrintln("8) Reset to PPP Logging Defaults (NMEAx5 + RXMx2)");
    systemPrintln("9) Turn off all messages");
    systemPrintln("10) Turn on all messages");

    systemPrintln("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
      menuMessagesSubtype("NMEA");
    else if (incoming == 2 && zedModuleType == PLATFORM_F9P)
      menuMessagesSubtype("RTCM");
    else if (incoming == 2 && zedModuleType == PLATFORM_F9R)
      menuMessagesSubtype("ESF");
    else if (incoming == 3)
      menuMessagesSubtype("RXM");
    else if (incoming == 4)
      menuMessagesSubtype("NAV");
    else if (incoming == 5)
      menuMessagesSubtype("MON");
    else if (incoming == 6)
      menuMessagesSubtype("TIM");
    else if (incoming == 7)
    {
      setGNSSMessageRates(settings.ubxMessages, 0); //Turn off all messages
      setMessageRateByName("UBX_NMEA_GGA", 1);
      setMessageRateByName("UBX_NMEA_GSA", 1);
      setMessageRateByName("UBX_NMEA_GST", 1);

      //We want GSV NMEA to be reported at 1Hz to avoid swamping SPP connection
      float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;
      if (measurementFrequency < 1.0) measurementFrequency = 1.0;
      setMessageRateByName("UBX_NMEA_GSV", measurementFrequency); //One report per second

      setMessageRateByName("UBX_NMEA_RMC", 1);
      systemPrintln("Reset to Surveying Defaults (NMEAx5)");
    }
    else if (incoming == 8)
    {
      setGNSSMessageRates(settings.ubxMessages, 0); //Turn off all messages
      setMessageRateByName("UBX_NMEA_GGA", 1);
      setMessageRateByName("UBX_NMEA_GSA", 1);
      setMessageRateByName("UBX_NMEA_GST", 1);

      //We want GSV NMEA to be reported at 1Hz to avoid swamping SPP connection
      float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;
      if (measurementFrequency < 1.0) measurementFrequency = 1.0;
      setMessageRateByName("UBX_NMEA_GSV", measurementFrequency); //One report per second

      setMessageRateByName("UBX_NMEA_RMC", 1);

      setMessageRateByName("UBX_RXM_RAWX", 1);
      setMessageRateByName("UBX_RXM_SFRBX", 1);
      systemPrintln("Reset to PPP Logging Defaults (NMEAx5 + RXMx2)");
    }
    else if (incoming == 9)
    {
      setGNSSMessageRates(settings.ubxMessages, 0); //Turn off all messages
      systemPrintln("All messages disabled");
    }
    else if (incoming == 10)
    {
      setGNSSMessageRates(settings.ubxMessages, 1); //Turn on all messages to report once per fix
      systemPrintln("All messages enabled");
    }
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars

  //Make sure the appropriate messages are enabled
  bool response = setMessages(); //Does a complete open/closed val set
  if (response == false)
  {
    systemPrintln("menuMessages: Failed to enable UART1 messages - Try 1");

    response = setMessages(); //Does a complete open/closed val set

    if (response == false)
      systemPrintln("menuMessages: Failed to enable UART1 messages - Try 2");
    else
      systemPrintln("menuMessages: UART1 messages successfully enabled");
  }
  else
  {
    systemPrintln("menuMessages: UART1 messages successfully enabled");
  }

  setLoggingType(); //Update Standard, PPP, or custom for icon selection
}

//Given a sub type (ie "RTCM", "NMEA") present menu showing messages with this subtype
//Controls the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesSubtype(const char* messageType)
{
  while (1)
  {
    systemPrintln();
    systemPrintf("Menu: Message %s\r\n", messageType);

    int startOfBlock = 0;
    int endOfBlock = 0;
    setMessageOffsets(messageType, startOfBlock, endOfBlock); //Find start and stop of given messageType in message array
    for (int x = 0 ; x < (endOfBlock - startOfBlock) ; x++)
    {
      //Check to see if this ZED platform supports this message
      if (settings.ubxMessages[x + startOfBlock].supported & zedModuleType)
      {
        systemPrintf("%d) Message %s: ", x + 1, settings.ubxMessages[x + startOfBlock].msgTextName);
        systemPrintln(settings.ubxMessages[x + startOfBlock].msgRate);
      }
    }

    systemPrintln("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming >= 1 && incoming <= (endOfBlock - startOfBlock))
    {
      //Check to see if this ZED platform supports this message
      if (settings.ubxMessages[(incoming - 1) + startOfBlock].supported & zedModuleType)
        inputMessageRate(settings.ubxMessages[(incoming - 1) + startOfBlock]);
      else
        printUnknown(incoming);
    }
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Prompt the user to enter the message rate for a given ID
//Assign the given value to the message
void inputMessageRate(ubxMsg &localMessage)
{
  systemPrintf("Enter %s message rate (0 to disable): ", localMessage.msgTextName);
  int rate = getNumber(); //Returns EXIT, TIMEOUT, or long

  if (rate == INPUT_RESPONSE_GETNUMBER_TIMEOUT || rate == INPUT_RESPONSE_GETNUMBER_EXIT)
    return;

  while (rate < 0 || rate > 255) //8 bit limit
  {
    systemPrintln("Error: Message rate out of range");
    systemPrintf("Enter %s message rate (0 to disable): ", localMessage.msgTextName);
    rate = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (rate == INPUT_RESPONSE_GETNUMBER_TIMEOUT || rate == INPUT_RESPONSE_GETNUMBER_EXIT)
      return; //Give up
  }

  localMessage.msgRate = rate;
}

//Set all GNSS message report rates to one value
//Useful for turning on or off all messages for resetting and testing
//We pass in the message array by reference so that we can modify a temp struct
//like a dummy struct for USB message changes (all on/off) for testing
void setGNSSMessageRates(ubxMsg *localMessage, uint8_t msgRate)
{
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    localMessage[x].msgRate = msgRate;
}

//Creates a log if logging is enabled, and SD is detected
//Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
void beginLogging()
{
  beginLogging("");
}

void beginLogging(const char *customFileName)
{
  if (online.microSD == false)
    beginSD();

  if (online.logging == false)
  {
    if (online.microSD == true && settings.enableLogging == true && online.rtc == true) //We can't create a file until we have date/time
    {
      char fileName[66 + 6 + 40] = "";

      if (strlen(customFileName) == 0)
      {
        //Generate a standard log file name
        if (reuseLastLog == true) //attempt to use previous log
        {
          if (findLastLog(fileName) == false)
            log_d("Failed to find last log. Making new one.");
          else
            log_d("Using last log file.");
        }

        if (strlen(fileName) == 0)
        {
          sprintf(fileName, "%s_%02d%02d%02d_%02d%02d%02d.ubx", //SdFat library
                  platformFilePrefix,
                  rtc.getYear() - 2000, rtc.getMonth() + 1, rtc.getDay(), //ESP32Time returns month:0-11
                  rtc.getHour(true), rtc.getMinute(), rtc.getSecond() //ESP32Time getHour(true) returns hour:0-23
                 );
        }
      }
      else
      {
        strcpy(fileName, customFileName);
      }

      //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
      if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
      {
        markSemaphore(FUNCTION_CREATEFILE);

        if (USE_SPI_MICROSD)
        {
          // O_CREAT - create the file if it does not exist
          // O_APPEND - seek to the end of the file prior to each write
          // O_WRITE - open for write
          if (ubxFile->open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
          {
            systemPrintf("Failed to create GNSS UBX data file: %s\r\n", fileName);
            online.logging = false;
            xSemaphoreGive(sdCardSemaphore);
            return;
          }
        }
#ifdef COMPILE_SD_MMC
        else
        {
          *ubxFile_SD_MMC = SD_MMC.open(fileName, FILE_APPEND);
          if (!ubxFile_SD_MMC)
          {
            systemPrintf("Failed to create GNSS UBX data file: %s\r\n", fileName);
            online.logging = false;
            xSemaphoreGive(sdCardSemaphore);
            return;
          }
        }
#endif

        fileSize = 0;
        lastLogSize = 0; //Reset counter - used for displaying active logging icon

        bufferOverruns = 0; //Reset counter

        if (USE_SPI_MICROSD)
          updateDataFileCreate(ubxFile); // Update the file to create time & date

        startCurrentLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging

        //If it hasn't been done before, mark the initial start of logging for total run time
        if (startLogTime_minutes == 0) startLogTime_minutes = millis() / 1000L / 60;

        //Add NMEA txt message with restart reason
        char rstReason[30];
        switch (esp_reset_reason())
        {
          case ESP_RST_UNKNOWN: strcpy(rstReason, "ESP_RST_UNKNOWN"); break;
          case ESP_RST_POWERON : strcpy(rstReason, "ESP_RST_POWERON"); break;
          case ESP_RST_SW : strcpy(rstReason, "ESP_RST_SW"); break;
          case ESP_RST_PANIC : strcpy(rstReason, "ESP_RST_PANIC"); break;
          case ESP_RST_INT_WDT : strcpy(rstReason, "ESP_RST_INT_WDT"); break;
          case ESP_RST_TASK_WDT : strcpy(rstReason, "ESP_RST_TASK_WDT"); break;
          case ESP_RST_WDT : strcpy(rstReason, "ESP_RST_WDT"); break;
          case ESP_RST_DEEPSLEEP : strcpy(rstReason, "ESP_RST_DEEPSLEEP"); break;
          case ESP_RST_BROWNOUT : strcpy(rstReason, "ESP_RST_BROWNOUT"); break;
          case ESP_RST_SDIO : strcpy(rstReason, "ESP_RST_SDIO"); break;
          default : strcpy(rstReason, "Unknown");
        }

        //Mark top of log with system information
        char nmeaMessage[82]; //Max NMEA sentence length is 82
        createNMEASentence(CUSTOM_NMEA_TYPE_RESET_REASON, nmeaMessage, rstReason); //textID, buffer, text
        if (USE_SPI_MICROSD)
          ubxFile->println(nmeaMessage);
#ifdef COMPILE_SD_MMC
        else
          ubxFile_SD_MMC->println(nmeaMessage);
#endif

        //Record system firmware versions and info to log

        //SparkFun RTK Express v1.10-Feb 11 2022
        char firmwareVersion[30]; //v1.3 December 31 2021
        sprintf(firmwareVersion, "v%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
        createNMEASentence(CUSTOM_NMEA_TYPE_SYSTEM_VERSION, nmeaMessage, firmwareVersion); //textID, buffer, text
        if (USE_SPI_MICROSD)
          ubxFile->println(nmeaMessage);
#ifdef COMPILE_SD_MMC
        else
          ubxFile_SD_MMC->println(nmeaMessage);
#endif


        //ZED-F9P firmware: HPG 1.30
        createNMEASentence(CUSTOM_NMEA_TYPE_ZED_VERSION, nmeaMessage, zedFirmwareVersion); //textID, buffer, text
        if (USE_SPI_MICROSD)
          ubxFile->println(nmeaMessage);
#ifdef COMPILE_SD_MMC
        else
          ubxFile_SD_MMC->println(nmeaMessage);
#endif

        //Device BT MAC. See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/346
        char macAddress[5];
        sprintf(macAddress, "%02X%02X", btMACAddress[4], btMACAddress[5]);
        createNMEASentence(CUSTOM_NMEA_TYPE_DEVICE_BT_ID, nmeaMessage, macAddress); //textID, buffer, text
        if (USE_SPI_MICROSD)
          ubxFile->println(nmeaMessage);
#ifdef COMPILE_SD_MMC
        else
          ubxFile_SD_MMC->println(nmeaMessage);
#endif

        if (reuseLastLog == true)
        {
          systemPrintln("Appending last available log");
        }

        xSemaphoreGive(sdCardSemaphore);
      }
      else
      {
        //A retry will happen during the next loop, the log will eventually be opened
        log_d("Failed to get file system lock to create GNSS UBX data file");
        online.logging = false;
        return;
      }

      systemPrintf("Log file name: %s\r\n", fileName);
      online.logging = true;
    } //online.sd, enable.logging, online.rtc
  } //online.logging
}

//Stop writing to the log file on the microSD card
void endLogging(bool gotSemaphore, bool releaseSemaphore)
{
  if (online.logging == true)
  {
    //Wait up to 1000ms to allow hanging SD writes to time out
    if (gotSemaphore || (xSemaphoreTake(sdCardSemaphore, 1000 / portTICK_PERIOD_MS) == pdPASS))
    {
      markSemaphore(FUNCTION_ENDLOGGING);

      online.logging = false;

      //Record the number of NMEA/RTCM/UBX messages that were filtered out
      char parserStats[50];

      sprintf(parserStats, "%d,%d,%d,",
              failedParserMessages_NMEA,
              failedParserMessages_RTCM,
              failedParserMessages_UBX);

      char nmeaMessage[82]; //Max NMEA sentence length is 82
      createNMEASentence(CUSTOM_NMEA_TYPE_PARSER_STATS, nmeaMessage, parserStats); //textID, buffer, text
      if (USE_SPI_MICROSD)
      {
        ubxFile->println(nmeaMessage);
        ubxFile->sync();
      }
#ifdef COMPILE_SD_MMC
      else
        ubxFile_SD_MMC->println(nmeaMessage);
#endif

      //Reset stats in case a new log is created
      failedParserMessages_NMEA = 0;
      failedParserMessages_RTCM = 0;
      failedParserMessages_UBX = 0;

      //Close down file system
      if (USE_SPI_MICROSD)
      {
        ubxFile->close();
        //Done with the log file
        delete ubxFile;
        ubxFile = NULL;
      }
#ifdef COMPILE_SD_MMC
      else
      {
        ubxFile_SD_MMC->close();
        //Done with the log file
        delete ubxFile_SD_MMC;
        ubxFile_SD_MMC = NULL;
      }
#endif
      systemPrintln("Log file closed");

      //Release the semaphore if requested
      if (releaseSemaphore)
        xSemaphoreGive(sdCardSemaphore);
    } //End sdCardSemaphore
    else
    {
      char semaphoreHolder[50];
      getSemaphoreFunction(semaphoreHolder);

      //This is OK because in the interim more data will be written to the log
      //and the log file will eventually be closed by the next call in loop
      log_d("sdCardSemaphore failed to yield, held by %s, menuMessages.ino line %d\r\n", semaphoreHolder, __LINE__);
    }
  }
}

//Update the file access and write time with date and time obtained from GNSS
void updateDataFileAccess(SdFile *dataFile)
{
  if (online.rtc == true)
  {
    //ESP32Time returns month:0-11
    dataFile->timestamp(T_ACCESS, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
    dataFile->timestamp(T_WRITE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
  }
}

//Update the file create time with date and time obtained from GNSS
void updateDataFileCreate(SdFile *dataFile)
{
  if (online.rtc == true)
    dataFile->timestamp(T_CREATE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(), rtc.getHour(true), rtc.getMinute(), rtc.getSecond()); //ESP32Time returns month:0-11
}

//Finds last log
//Returns true if succesful
bool findLastLog(char *lastLogName)
{
  bool foundAFile = false;

  if (online.microSD == true)
  {
    //Attempt to access file system. This avoids collisions with file writing in F9PSerialReadTask()
    //Wait up to 5s, this is important
    if (xSemaphoreTake(sdCardSemaphore, 5000 / portTICK_PERIOD_MS) == pdPASS)
    {
      markSemaphore(FUNCTION_FINDLOG);

      //Count available binaries
      if (USE_SPI_MICROSD)
      {
        SdFile tempFile;
        SdFile dir;
        const char* LOG_EXTENSION = "ubx";
        const char* LOG_PREFIX = platformFilePrefix;
        char fname[50]; //Handle long file names
  
        dir.open("/"); //Open root
  
        while (tempFile.openNext(&dir, O_READ))
        {
          if (tempFile.isFile())
          {
            tempFile.getName(fname, sizeof(fname));
  
            //Check for matching file name prefix and extension
            if (strcmp(LOG_EXTENSION, &fname[strlen(fname) - strlen(LOG_EXTENSION)]) == 0)
            {
              if (strstr(fname, LOG_PREFIX) != NULL)
              {
                strcpy(lastLogName, fname); //Store this file as last known log file
                foundAFile = true;
              }
            }
          }
          tempFile.close();
        }
      }
#ifdef COMPILE_SD_MMC
      else
      {
        File tempFile;
        File dir;
        const char* LOG_EXTENSION = "ubx";
        const char* LOG_PREFIX = platformFilePrefix;
        char fname[50]; //Handle long file names
  
        dir = SD_MMC.open("/"); //Open root

        if (dir && dir.isDirectory())
        {
          tempFile = dir.openNextFile();
          while (tempFile)
          {
            if (!tempFile.isDirectory())
            {
              snprintf(fname, sizeof(fname), "%s", tempFile.name());
    
              //Check for matching file name prefix and extension
              if (strcmp(LOG_EXTENSION, &fname[strlen(fname) - strlen(LOG_EXTENSION)]) == 0)
              {
                if (strstr(fname, LOG_PREFIX) != NULL)
                {
                  strcpy(lastLogName, fname); //Store this file as last known log file
                  foundAFile = true;
                }
              }
            }
            tempFile.close();
            tempFile = dir.openNextFile();
          }
        }
      }
#endif

      xSemaphoreGive(sdCardSemaphore);
    }
    else
    {
      //Error when a log file exists on the microSD card, data should be appended
      //to the existing log file
      systemPrintf("sdCardSemaphore failed to yield, menuMessages.ino line %d\r\n", __LINE__);
    }
  }

  return (foundAFile);
}

//Given a unique string, find first and last records containing that string in message array
void setMessageOffsets(const char* messageType, int& startOfBlock, int& endOfBlock)
{
  char messageNamePiece[40]; //UBX_RTCM
  sprintf(messageNamePiece, "UBX_%s", messageType); //Put UBX_ infront of type

  //Find the first occurrence
  for (startOfBlock = 0 ; startOfBlock < MAX_UBX_MSG ; startOfBlock++)
  {
    if (strstr(settings.ubxMessages[startOfBlock].msgTextName, messageNamePiece) != NULL) break;
  }
  if (startOfBlock == MAX_UBX_MSG)
  {
    //Error out
    startOfBlock = 0;
    endOfBlock = 0;
    return;
  }

  //Find the last occurrence
  for (endOfBlock = startOfBlock + 1 ; endOfBlock < MAX_UBX_MSG ; endOfBlock++)
  {
    if (strstr(settings.ubxMessages[endOfBlock].msgTextName, messageNamePiece) == NULL) break;
  }
}

//Return the number of active/enabled messages
uint8_t getActiveMessageCount()
{
  uint8_t count = 0;
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    if (settings.ubxMessages[x].msgRate > 0) count++;
  return (count);
}

//Given the name of a message, find it, and set the rate
bool setMessageRateByName(const char *msgName, uint8_t msgRate)
{
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
  {
    if (strcmp(settings.ubxMessages[x].msgTextName, msgName) == 0)
    {
      settings.ubxMessages[x].msgRate = msgRate;
      return (true);
    }
  }

  systemPrintf("setMessageRateByName: %s not found\r\n", msgName);
  return (false);
}

//Given the name of a message, find it, and return the rate
uint8_t getMessageRateByName(const char *msgName)
{
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
  {
    if (strcmp(settings.ubxMessages[x].msgTextName, msgName) == 0)
      return (settings.ubxMessages[x].msgRate);
  }

  systemPrintf("getMessageRateByName: %s not found\r\n", msgName);
  return (0);
}

//Determine logging type
//If user is logging basic 5 sentences, this is 'S'tandard logging
//If user is logging 7 PPP sentences, this is 'P'PP logging
//If user has other setences turned on, it's custom logging
//This controls the type of icon displayed
void setLoggingType()
{
  loggingType = LOGGING_CUSTOM;

  int messageCount = getActiveMessageCount();
  if (messageCount == 5 || messageCount == 7)
  {
    if (getMessageRateByName("UBX_NMEA_GGA") > 0
        && getMessageRateByName("UBX_NMEA_GSA") > 0
        && getMessageRateByName("UBX_NMEA_GST") > 0
        && getMessageRateByName("UBX_NMEA_GSV") > 0
        && getMessageRateByName("UBX_NMEA_RMC") > 0
       )
    {
      loggingType = LOGGING_STANDARD;

      if (getMessageRateByName("UBX_RXM_RAWX") > 0 && getMessageRateByName("UBX_RXM_SFRBX") > 0)
        loggingType = LOGGING_PPP;
    }
  }
}

//During the logging test, we have to modify the messages and rate of the device
void setLogTestFrequencyMessages(int rate, int messages)
{
  //Set measurement frequency
  setRate(1.0 / rate); //Convert Hz to seconds. This will set settings.measurementRate, settings.navigationRate, and GSV message

  //Set messages
  setGNSSMessageRates(settings.ubxMessages, 0); //Turn off all messages
  if (messages == 5)
  {
    setMessageRateByName("UBX_NMEA_GGA", 1);
    setMessageRateByName("UBX_NMEA_GSA", 1);
    setMessageRateByName("UBX_NMEA_GST", 1);
    setMessageRateByName("UBX_NMEA_GSV", rate); //One report per second
    setMessageRateByName("UBX_NMEA_RMC", 1);

    log_d("Messages: Surveying Defaults (NMEAx5)");
  }
  else if (messages == 7)
  {
    setMessageRateByName("UBX_NMEA_GGA", 1);
    setMessageRateByName("UBX_NMEA_GSA", 1);
    setMessageRateByName("UBX_NMEA_GST", 1);
    setMessageRateByName("UBX_NMEA_GSV", rate); //One report per second
    setMessageRateByName("UBX_NMEA_RMC", 1);
    setMessageRateByName("UBX_RXM_RAWX", 1);
    setMessageRateByName("UBX_RXM_SFRBX", 1);

    log_d("Messages: PPP NMEAx5+RXMx2");
  }
  else
    log_d("Unknown message amount");


  //Apply these message rates to both UART1 and USB
  setMessages(); //Does a complete open/closed val set
  setMessagesUSB();
}

//The log test allows us to record a series of different system configurations into
//one file. At the same time, we log the output of the ZED via the USB connection.
//Once complete, the SD log is compared against the USB log to verify both are identical.
//Be sure to set maxLogLength_minutes before running test. maxLogLength_minutes will
//set the length of each test.
void updateLogTest()
{
  //Log is complete, run next text
  int rate = 4;
  int messages = 5;
  int semaphoreWait = 10;

  logTestState++; //Advance to next state

  switch (logTestState)
  {
    case (LOGTEST_4HZ_5MSG_10MS):
      //During the first test, create the log file
      reuseLastLog = false;
      char fileName[100];
      sprintf(fileName, "%s_LogTest_%02d%02d%02d_%02d%02d%02d.ubx", //SdFat library
              platformFilePrefix,
              rtc.getYear() - 2000, rtc.getMonth() + 1, rtc.getDay(), //ESP32Time returns month:0-11
              rtc.getHour(true), rtc.getMinute(), rtc.getSecond() //ESP32Time getHour(true) returns hour:0-23
             );
      endSD(false, true); //End previous log

      beginLogging(fileName);

      rate = 4;
      messages = 5;
      semaphoreWait = 10;
      break;
    case (LOGTEST_4HZ_7MSG_10MS):
      rate = 4;
      messages = 7;
      semaphoreWait = 10;
      break;
    case (LOGTEST_10HZ_5MSG_10MS):
      rate = 10;
      messages = 5;
      semaphoreWait = 10;
      break;
    case (LOGTEST_10HZ_7MSG_10MS):
      rate = 10;
      messages = 7;
      semaphoreWait = 10;
      break;

    case (LOGTEST_4HZ_5MSG_0MS):
      rate = 4;
      messages = 5;
      semaphoreWait = 0;
      break;
    case (LOGTEST_4HZ_7MSG_0MS):
      rate = 4;
      messages = 7;
      semaphoreWait = 0;
      break;
    case (LOGTEST_10HZ_5MSG_0MS):
      rate = 10;
      messages = 5;
      semaphoreWait = 0;
      break;
    case (LOGTEST_10HZ_7MSG_0MS):
      rate = 10;
      messages = 7;
      semaphoreWait = 0;
      break;

    case (LOGTEST_4HZ_5MSG_50MS):
      rate = 4;
      messages = 5;
      semaphoreWait = 50;
      break;
    case (LOGTEST_4HZ_7MSG_50MS):
      rate = 4;
      messages = 7;
      semaphoreWait = 50;
      break;
    case (LOGTEST_10HZ_5MSG_50MS):
      rate = 10;
      messages = 5;
      semaphoreWait = 50;
      break;
    case (LOGTEST_10HZ_7MSG_50MS):
      rate = 10;
      messages = 7;
      semaphoreWait = 50;
      break;

    case (LOGTEST_END):
      //Reduce rate
      rate = 4;
      messages = 5;
      semaphoreWait = 10;
      setLogTestFrequencyMessages(rate, messages); //Set messages and rate for both UART1 and USB ports
      log_d("Log Test Complete");
      break;

    default:
      logTestState = LOGTEST_END;
      settings.runLogTest = false;
      break;
  }

  if (settings.runLogTest == true)
  {
    setLogTestFrequencyMessages(rate, messages); //Set messages and rate for both UART1 and USB ports

    loggingSemaphoreWait_ms = semaphoreWait / portTICK_PERIOD_MS; //Update variable

    startCurrentLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging

    char logMessage[100];
    sprintf(logMessage, "Start log test: %dHz, %dMsg, %dMS", rate, messages, semaphoreWait);

    char nmeaMessage[100]; //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_LOGTEST_STATUS, nmeaMessage, logMessage); //textID, buffer, text

    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      markSemaphore(FUNCTION_LOGTEST);

      if (USE_SPI_MICROSD)
        ubxFile->println(nmeaMessage);
#ifdef COMPILE_SD_MMC
      else
        ubxFile_SD_MMC->println(nmeaMessage);
#endif

      xSemaphoreGive(sdCardSemaphore);
    }
    else
    {
      log_w("sdCardSemaphore failed to yield, menuMessages.ino line %d", __LINE__);
    }

    systemPrintf("%s\r\n", logMessage);
  }
}
