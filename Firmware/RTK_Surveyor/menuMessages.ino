//Control the messages that get logged to SD
//Control max logging time (limit to a certain number of minutes)
//The main use case is the setup for a base station to log RAW sentences that then get post processed
void menuLog()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Logging Menu"));

    if (settings.enableSD && online.microSD)
      Serial.println(F("microSD card is online"));
    else
    {
      beginSD(); //Test if SD is present
      if (online.microSD == true)
        Serial.println(F("microSD card online"));
      else
        Serial.println(F("No microSD card is detected"));
    }

    Serial.print(F("1) Log to microSD: "));
    if (settings.enableLogging == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (settings.enableLogging == true)
    {
      Serial.print(F("2) Set max logging time: "));
      Serial.print(settings.maxLogTime_minutes);
      Serial.println(F(" minutes"));
    }

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enableLogging ^= 1;
    }
    else if (incoming == '2' && settings.enableLogging == true)
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
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessages()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Messages Menu"));

    Serial.printf("Active messages: %d\n\r", getActiveMessageCount());

    Serial.println(F("1) Set NMEA Messages"));
    Serial.println(F("2) Set RTCM Messages"));
    Serial.println(F("3) Set RXM Messages"));
    Serial.println(F("4) Set NAV Messages"));
    Serial.println(F("5) Set MON Messages"));
    Serial.println(F("6) Set TIM Messages"));
    Serial.println(F("7) Reset to Surveying Defaults (NMEAx5)"));
    Serial.println(F("8) Reset to PPP Logging Defaults (NMEAx5 + RXMx2)"));
    Serial.println(F("9) Turn off all messages"));
    Serial.println(F("10) Turn on all messages"));

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      menuMessagesSubtype("NMEA");
    else if (incoming == 2)
      menuMessagesSubtype("RTCM");
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
      setGNSSMessageRates(ubxMessages, 0); //Turn off all messages
      setMessageRateByName("UBX_NMEA_GGA", 1);
      setMessageRateByName("UBX_NMEA_GSA", 1);
      setMessageRateByName("UBX_NMEA_GST", 1);
      setMessageRateByName("UBX_NMEA_GSV", 4); //One update per 4 fixes to avoid swamping SPP connection
      setMessageRateByName("UBX_NMEA_RMC", 1);
      Serial.println(F("Reset to Surveying Defaults (NMEAx5)"));
    }
    else if (incoming == 8)
    {
      setGNSSMessageRates(ubxMessages, 0); //Turn off all messages
      setMessageRateByName("UBX_NMEA_GGA", 1);
      setMessageRateByName("UBX_NMEA_GSA", 1);
      setMessageRateByName("UBX_NMEA_GST", 1);
      setMessageRateByName("UBX_NMEA_GSV", 4); //One update per 4 fixes to avoid swamping SPP connection
      setMessageRateByName("UBX_NMEA_RMC", 1);

      setMessageRateByName("UBX_RXM_RAWX", 1);
      setMessageRateByName("UBX_RXM_SFRBX", 1);
      Serial.println(F("Reset to PPP Logging Defaults (NMEAx5 + RXMx2)"));
    }
    else if (incoming == 9)
    {
      setGNSSMessageRates(ubxMessages, 0); //Turn off all messages
      Serial.println(F("All messages disabled"));
    }
    else if (incoming == 10)
    {
      setGNSSMessageRates(ubxMessages, 1); //Turn on all messages to report once per fix
      Serial.println(F("All messages enabled"));
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars

  bool response = configureGNSSMessageRates(COM_PORT_UART1, ubxMessages); //Make sure the appropriate messages are enabled
  if (response == false)
  {
    Serial.println(F("menuMessages: Failed to enable UART1 messages - Try 1"));
    //Try again
    response = configureGNSSMessageRates(COM_PORT_UART1, ubxMessages); //Make sure the appropriate messages are enabled
    if (response == false)
      Serial.println(F("menuMessages: Failed to enable UART1 messages - Try 2"));
    else
      Serial.println(F("menuMessages: UART1 messages successfully enabled"));
  }
  else
  {
    Serial.println(F("menuMessages: UART1 messages successfully enabled"));
  }

}

//Given a sub type (ie "RTCM", "NMEA") present menu showing messages with this subtype
//Controls the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesSubtype(const char* messageType)
{
  while (1)
  {
    Serial.println();
    Serial.printf("Menu: Message %s Menu\n\r", messageType);

    int startOfBlock = 0;
    int endOfBlock = 0;
    setMessageOffsets(messageType, startOfBlock, endOfBlock); //Find start and stop of RTCM records in message array
    for (int x = 0 ; x < (endOfBlock - startOfBlock) ; x++)
    {
      Serial.printf("%d) Message %s: ", x + 1, ubxMessages[x + startOfBlock].msgTextName);
      Serial.println(ubxMessages[x + startOfBlock].msgRate);
    }

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming >= 1 && incoming <= (endOfBlock - startOfBlock))
    {
      inputMessageRate(ubxMessages[ (incoming - 1) + startOfBlock]);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Prompt the user to enter the message rate for a given ID
//Assign the given value to the message
void inputMessageRate(ubxMsg &localMessage)
{
  Serial.printf("Enter %s message rate (0 to disable): ", localMessage.msgTextName);
  int64_t rate = getNumber(menuTimeout); //Timeout after x seconds

  while (rate < 0 || rate > 60) //Arbitrary 60 fixes per report limit
  {
    Serial.println(F("Error: message rate out of range"));
    Serial.printf("Enter %s message rate (0 to disable): ", localMessage.msgTextName);
    rate = getNumber(menuTimeout); //Timeout after x seconds

    if (rate == STATUS_GETNUMBER_TIMEOUT || rate == STATUS_PRESSED_X)
      return; //Give up
  }

  if (rate == STATUS_GETNUMBER_TIMEOUT || rate == STATUS_PRESSED_X)
    return;

  localMessage.msgRate = rate;
}

//Updates the message rates on the ZED-F9P for all known messages
//Any port and messages by reference can be passed in. This allows us to modify the USB
//port settings a separate (not NVM backed) message struct for testing
bool configureGNSSMessageRates(uint8_t portType, ubxMsg *localMessage)
{
  bool response = true;

  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    response &= configureMessageRate(portType, localMessage[x]);

  return (response);
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

//Given a message, set the message rate on the ZED-F9P
bool configureMessageRate(uint8_t portID, ubxMsg localMessage)
{
  uint8_t currentSendRate = getMessageRate(localMessage.msgClass, localMessage.msgID, portID); //Qeury the module for the current setting

  bool response = true;
  if (currentSendRate != localMessage.msgRate)
    response &= i2cGNSS.configureMessage(localMessage.msgClass, localMessage.msgID, portID, localMessage.msgRate); //Update setting
  return response;
}

//Lookup the send rate for a given message+port
uint8_t getMessageRate(uint8_t msgClass, uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = msgClass;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.printf("getMessageSetting failed: Class-0x%02X ID-0x%02X\n\r", msgClass, msgID);
    return (false);
  }

  uint8_t sendRate = settingPayload[2 + portID];

  return (sendRate);
}

//Creates a log if logging is enabled, and SD is detected
//Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
void beginLogging()
{
  if (online.logging == false)
  {
    if (online.microSD == true && settings.enableLogging == true)
    {
      //Wait 1000ms between newLog creation for ZED to get date/time
      if (millis() - lastBeginLoggingAttempt > 1000)
      {
        lastBeginLoggingAttempt = millis();

        char fileName[40] = "";

        i2cGNSS.checkUblox();

        if (reuseLastLog == true) //attempt to use previous log
        {
          if (findLastLog(fileName) == false)
          {
            Serial.println(F("Failed to find last log. Making new one."));
          }
        }

        if (strlen(fileName) == 0)
        {
          //Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
          bool timeValid = false;
          //        if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
          //          timeValid = true;
          if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
            timeValid = true;

          if (timeValid == false)
          {
            Serial.println(F("beginLoggingUBX: No date/time available. No file created."));
            online.logging = false;
            return;
          }

          sprintf(fileName, "%s_%02d%02d%02d_%02d%02d%02d.ubx", //SdFat library
                  platformFilePrefix,
                  i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
                  i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
                 );
        }

        //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
        if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
          // O_CREAT - create the file if it does not exist
          // O_APPEND - seek to the end of the file prior to each write
          // O_WRITE - open for write
          if (ubxFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
          {
            Serial.printf("Failed to create GNSS UBX data file: %s\n\r", fileName);
            online.logging = false;
            xSemaphoreGive(xFATSemaphore);
            return;
          }

          updateDataFileCreate(&ubxFile); // Update the file to create time & date

          startLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging

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

          char nmeaMessage[82]; //Max NMEA sentence length is 82
          createNMEASentence(1, 1, nmeaMessage, rstReason); //sentenceNumber, textID, buffer, text
          ubxFile.println(nmeaMessage);

          if (reuseLastLog == true)
          {
            Serial.println(F("Appending last available log"));
          }

          xSemaphoreGive(xFATSemaphore);
        }
        else
        {
          Serial.println(F("Failed to get file system lock to create GNSS UBX data file"));
          online.logging = false;
          return;
        }

        Serial.printf("Log file created: %s\n\r", fileName);
        online.logging = true;
      } //lastBeginLoggingAttempt > 1000ms
    } //sdOnline, settings.logging = true
    else
      online.logging = false;
  } //online.logging = false
}

//Updates the timestemp on a given data file
void updateDataFileAccess(SdFile *dataFile)
{
  bool timeValid = false;
  if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
    timeValid = true;
  if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
    timeValid = true;

  if (timeValid == true)
  {
    //Update the file access time
    dataFile->timestamp(T_ACCESS, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
    //Update the file write time
    dataFile->timestamp(T_WRITE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
  }
}

void updateDataFileCreate(SdFile *dataFile)
{
  bool timeValid = false;
  if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
    timeValid = true;
  if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
    timeValid = true;

  if (timeValid == true)
  {
    //Update the file create time
    dataFile->timestamp(T_CREATE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
  }
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
    if (xSemaphoreTake(xFATSemaphore, 5000 / portTICK_PERIOD_MS) == pdPASS)
    {
      //Count available binaries
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

      xSemaphoreGive(xFATSemaphore);
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
    if (strstr(ubxMessages[startOfBlock].msgTextName, messageNamePiece) != NULL) break;
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
    if (strstr(ubxMessages[endOfBlock].msgTextName, messageNamePiece) == NULL) break;
  }
}

//Return the number of active/enabled messages
uint8_t getActiveMessageCount()
{
  uint8_t count = 0;
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    if (ubxMessages[x].msgRate > 0) count++;
  return (count);
}

//Given the name of a message, find it, and set the rate
bool setMessageRateByName(const char *msgName, uint8_t msgRate)
{
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
  {
    if (strcmp(ubxMessages[x].msgTextName, msgName) == 0)
    {
      ubxMessages[x].msgRate = msgRate;
      return (true);
    }
  }

  Serial.printf("setMessageRateByName: %s not found\n\r", msgName);
  return (false);
}
