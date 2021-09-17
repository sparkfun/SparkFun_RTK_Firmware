void loadSettings()
{
  if (online.eeprom == false)
  {
    ESP_LOGD(TAG, "Error: EEPROM not online");
    return;
  }

  //First load any settings from NVM
  //After, we'll load settings from config file if available
  //We'll then re-record settings so that the settings from the file over-rides internal NVM settings

  //Check to see if EEPROM is blank
  uint32_t testRead = 0;
  if (EEPROM.get(0, testRead) == 0xFFFFFFFF)
  {
    Serial.println(F("EEPROM is blank. Default settings applied"));
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Check that the current settings struct size matches what is stored in EEPROM
  //Misalignment happens when we add a new feature or setting
  int tempSize = 0;
  EEPROM.get(0, tempSize); //Load the sizeOfSettings
  if (tempSize != sizeof(settings))
  {
    Serial.println(F("Settings wrong size. Default settings applied"));
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Check that the rtkIdentifier is correct
  //(It is possible for two different versions of the code to have the same sizeOfSettings - which causes problems!)
  int tempIdentifier = 0;
  EEPROM.get(sizeof(int), tempIdentifier); //Load the identifier from the EEPROM location after sizeOfSettings (int)
  if (tempIdentifier != RTK_IDENTIFIER)
  {
    Serial.println(F("Settings are not valid for this variant of RTK Surveyor. Default settings applied"));
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Read current settings
  EEPROM.get(0, settings);

  loadSystemSettingsFromFile(); //Load any settings from config file. This will over-write any pre-existing EEPROM settings.
  //Record these new settings to EEPROM and config file to be sure they are the same
  //(do this even if loadSystemSettingsFromFile returned false)
  recordSystemSettings();
}

//Load settings without recording 
//Used at very first boot to test for resetCounter
void loadSettingsPartial()
{
  if (online.eeprom == false)
  {
    ESP_LOGD(TAG, "Error: EEPROM not online");
    return;
  }

  //Check to see if EEPROM is blank
  uint32_t testRead = 0;
  if (EEPROM.get(0, testRead) == 0xFFFFFFFF)
    return; //EEPROM is blank, assume default settings
  
  EEPROM.get(0, settings); //Read current settings
}

//Record the current settings struct to EEPROM and then to config file
void recordSystemSettings()
{
  settings.sizeOfSettings = sizeof(settings);
  if (settings.sizeOfSettings > EEPROM_SIZE)
  {
    Serial.printf("Size of settings is %d bytes\n\r", sizeof(settings));
    Serial.println(F("Increase the EEPROM footprint!"));
    displayError("EEPROM"); //Hard freeze
  }

  if (online.eeprom == true)
  {
    EEPROM.put(0, settings);
    EEPROM.commit();
    delay(1); //Give CPU time to pet WDT
  }
  else
    ESP_LOGD(TAG, "Error: EEPROM not online");

  recordSystemSettingsToFile();
}

//Export the current settings to a config file
void recordSystemSettingsToFile()
{
  if (online.microSD == true)
  {
    //Attempt to write to file system. This avoids collisions with file writing from other functions like updateLogs()
    if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      //Assemble settings file name
      char settingsFileName[40]; //SFE_Surveyor_Settings.txt
      strcpy(settingsFileName, platformFilePrefix);
      strcat(settingsFileName, "_Settings.txt");

      if (sd.exists(settingsFileName))
        sd.remove(settingsFileName);

      SdFile settingsFile; //FAT32
      if (settingsFile.open(settingsFileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println(F("Failed to create settings file"));
        return;
      }
      if (online.gnss)
        updateDataFileCreate(&settingsFile); // Update the file to create time & date

      settingsFile.println("sizeOfSettings=" + (String)settings.sizeOfSettings);
      settingsFile.println("rtkIdentifier=" + (String)settings.rtkIdentifier);

      char firmwareVersion[30]; //v1.3 December 31 2021
      sprintf(firmwareVersion, "v%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
      settingsFile.println("rtkFirmwareVersion=" + (String)firmwareVersion);

      settingsFile.println("zedFirmwareVersion=" + (String)zedFirmwareVersion);
      settingsFile.println("printDebugMessages=" + (String)settings.printDebugMessages);
      settingsFile.println("enableSD=" + (String)settings.enableSD);
      settingsFile.println("enableDisplay=" + (String)settings.enableDisplay);
      settingsFile.println("maxLogTime_minutes=" + (String)settings.maxLogTime_minutes);
      settingsFile.println("observationSeconds=" + (String)settings.observationSeconds);
      settingsFile.println("observationPositionAccuracy=" + (String)settings.observationPositionAccuracy);
      settingsFile.println("fixedBase=" + (String)settings.fixedBase);
      settingsFile.println("fixedBaseCoordinateType=" + (String)settings.fixedBaseCoordinateType);
      settingsFile.println("fixedEcefX=" + (String)settings.fixedEcefX);
      settingsFile.println("fixedEcefY=" + (String)settings.fixedEcefY);
      settingsFile.println("fixedEcefZ=" + (String)settings.fixedEcefZ);

      //Print Lat/Long doubles with 9 decimals
      char longPrint[20]; //-105.123456789
      sprintf(longPrint, "%0.9f", settings.fixedLat);
      settingsFile.println("fixedLat=" + (String)longPrint);
      sprintf(longPrint, "%0.9f", settings.fixedLong);
      settingsFile.println("fixedLong=" + (String)longPrint);
      sprintf(longPrint, "%0.4f", settings.fixedAltitude);
      settingsFile.println("fixedAltitude=" + (String)longPrint);

      settingsFile.println("dataPortBaud=" + (String)settings.dataPortBaud);
      settingsFile.println("radioPortBaud=" + (String)settings.radioPortBaud);
      settingsFile.println("enableNtripServer=" + (String)settings.enableNtripServer);
      settingsFile.println("casterHost=" + (String)settings.casterHost);
      settingsFile.println("casterPort=" + (String)settings.casterPort);
      settingsFile.println("mountPoint=" + (String)settings.mountPoint);
      settingsFile.println("mountPointPW=" + (String)settings.mountPointPW);
      settingsFile.println("wifiSSID=" + (String)settings.wifiSSID);
      settingsFile.println("wifiPW=" + (String)settings.wifiPW);
      settingsFile.println("surveyInStartingAccuracy=" + (String)settings.surveyInStartingAccuracy);
      settingsFile.println("measurementRate=" + (String)settings.measurementRate);
      settingsFile.println("navigationRate=" + (String)settings.navigationRate);
      settingsFile.println("enableI2Cdebug=" + (String)settings.enableI2Cdebug);
      settingsFile.println("enableHeapReport=" + (String)settings.enableHeapReport);
      settingsFile.println("enableTaskReports=" + (String)settings.enableTaskReports);
      settingsFile.println("dataPortChannel=" + (String)settings.dataPortChannel);
      settingsFile.println("spiFrequency=" + (String)settings.spiFrequency);
      settingsFile.println("sppRxQueueSize=" + (String)settings.sppRxQueueSize);
      settingsFile.println("sppTxQueueSize=" + (String)settings.sppTxQueueSize);
      settingsFile.println("dynamicModel=" + (String)settings.dynamicModel);
      settingsFile.println("lastState=" + (String)settings.lastState);
      settingsFile.println("throttleDuringSPPCongestion=" + (String)settings.throttleDuringSPPCongestion);
      settingsFile.println("enableSensorFusion=" + (String)settings.enableSensorFusion);
      settingsFile.println("autoIMUmountAlignment=" + (String)settings.autoIMUmountAlignment);
      settingsFile.println("enableResetDisplay=" + (String)settings.enableResetDisplay);

      //Record constellation settings
      for (int x = 0 ; x < MAX_CONSTELLATIONS ; x++)
      {
        char tempString[50]; //constellation.BeiDou=1
        sprintf(tempString, "constellation.%s=%d", ubxConstellations[x].textName, ubxConstellations[x].enabled);
        settingsFile.println(tempString);
      }

      //Record message settings
      for (int x = 0 ; x < MAX_UBX_MSG ; x++)
      {
        char tempString[50]; //message.nmea_dtm.msgRate=5
        sprintf(tempString, "message.%s.msgRate=%d", ubxMessages[x].msgTextName, ubxMessages[x].msgRate);
        settingsFile.println(tempString);
      }

      if (online.gnss)
        updateDataFileAccess(&settingsFile); // Update the file access time & date

      settingsFile.close();

      xSemaphoreGive(xFATSemaphore);
    }
  }
}

//If a config file exists on the SD card, load them and overwrite the local settings
//Heavily based on ReadCsvFile from SdFat library
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadSystemSettingsFromFile()
{
  if (online.microSD == true)
  {
    //Attempt to access file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
    if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      //Assemble settings file name
      char settingsFileName[40]; //SFE_Surveyor_Settings.txt
      strcpy(settingsFileName, platformFilePrefix);
      strcat(settingsFileName, "_Settings.txt");

      if (sd.exists(settingsFileName))
      {
        SdFile settingsFile; //FAT32
        if (settingsFile.open(settingsFileName, O_READ) == false)
        {
          Serial.println(F("Failed to open settings file"));
          xSemaphoreGive(xFATSemaphore);
          return (false);
        }

        char line[60];
        int lineNumber = 0;

        while (settingsFile.available()) {

          //Get the next line from the file
          //int n = getLine(&settingsFile, line, sizeof(line)); //Use with SD library
          int n = settingsFile.fgets(line, sizeof(line)); //Use with SdFat library
          if (n <= 0) {
            Serial.printf("Failed to read line %d from settings file\r\n", lineNumber);
          }
          else if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) {
            Serial.printf("Settings line %d too long\r\n", lineNumber);
            if (lineNumber == 0)
            {
              //If we can't read the first line of the settings file, give up
              Serial.println(F("Giving up on settings file"));
              xSemaphoreGive(xFATSemaphore);
              return (false);
            }
          }
          else if (parseLine(line) == false) {
            Serial.printf("Failed to parse line %d: %s\r\n", lineNumber, line);
            if (lineNumber == 0)
            {
              //If we can't read the first line of the settings file, give up
              Serial.println(F("Giving up on settings file"));
              xSemaphoreGive(xFATSemaphore);
              return (false);
            }
          }

          lineNumber++;
        }

        //Serial.println(F("Config file read complete"));
        settingsFile.close();
        xSemaphoreGive(xFATSemaphore);
        return (true);
      }
      else
      {
        Serial.println(F("No config file found. Using settings from EEPROM."));
        //The defaults of the struct will be recorded to a file later on.
        xSemaphoreGive(xFATSemaphore);
        return (false);
      }

    } //End Semaphore check
  } //End SD online

  Serial.println(F("Config file read failed: SD offline"));
  return (false); //SD offline
}

// Check for extra characters in field or find minus sign.
char* skipSpace(char* str) {
  while (isspace(*str)) str++;
  return str;
}

//Convert a given line from file into a settingName and value
//Sets the setting if the name is known
bool parseLine(char* str) {
  char* ptr;

  //Debug
  //Serial.printf("Line contents: %s", str);
  //Serial.flush();

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str) return false;

  //Store this setting name
  char settingName[40];
  sprintf(settingName, "%s", str);

  //Move pointer to end of line
  str = strtok(nullptr, "\n");
  if (!str) return false;

  //Serial.printf("s = %s\r\n", str);
  //Serial.flush();

  //Attempt to convert string to double.
  double d = strtod(str, &ptr);

  //Serial.printf("d = %lf\r\n", d);
  //Serial.flush();

  char settingValue[50];
  if (d == 0.0) //strtod failed, may be string or may be 0 but let it pass
  {
    sprintf(settingValue, "%s", str);
  }
  else
  {
    if (str == ptr || *skipSpace(ptr)) return false; //Check str pointer

    //See issue https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/47
    sprintf(settingValue, "%1.0lf", d); //Catch when the input is pure numbers (strtod was successful), store as settingValue
  }

  // Get setting name
  if (strcmp(settingName, "sizeOfSettings") == 0)
  {
    //We may want to cause a factory reset from the settings file rather than the menu
    //If user sets sizeOfSettings to -1 in config file, RTK Surveyor will factory reset
    if (d == -1)
    {
      eepromErase();

      //Assemble settings file name
      char settingsFileName[40]; //SFE_Surveyor_Settings.txt
      strcpy(settingsFileName, platformFilePrefix);
      strcat(settingsFileName, "_Settings.txt");
      sd.remove(settingsFileName);

      Serial.printf("RTK %s has been factory reset via settings file. Unit restarting. Please open terminal at 115200bps.\n\r", platformPrefix);
      delay(2000);
      ESP.restart();
    }

    //Check to see if this setting file is compatible with this version of RTK Surveyor
    if (d != sizeof(settings))
      Serial.printf("Warning: Settings size is %d but current firmware expects %d. Attempting to use settings from file.\r\n", (int)d, sizeof(settings));

  }
  else if (strcmp(settingName, "rtkIdentifier") == 0)
    settings.rtkIdentifier = d;
  else if (strcmp(settingName, "rtkFirmwareVersion") == 0)
  {} //Do nothing. Just read it to avoid 'Unknown setting' error
  else if (strcmp(settingName, "zedFirmwareVersion") == 0)
  {} //Do nothing. Just read it to avoid 'Unknown setting' error
  else if (strcmp(settingName, "printDebugMessages") == 0)
    settings.printDebugMessages = d;
  else if (strcmp(settingName, "enableSD") == 0)
    settings.enableSD = d;
  else if (strcmp(settingName, "enableDisplay") == 0)
    settings.enableDisplay = d;
  else if (strcmp(settingName, "maxLogTime_minutes") == 0)
    settings.maxLogTime_minutes = d;
  else if (strcmp(settingName, "observationSeconds") == 0)
    settings.observationSeconds = d;
  else if (strcmp(settingName, "observationPositionAccuracy") == 0)
    settings.observationPositionAccuracy = d;
  else if (strcmp(settingName, "fixedBase") == 0)
    settings.fixedBase = d;
  else if (strcmp(settingName, "fixedBaseCoordinateType") == 0)
    settings.fixedBaseCoordinateType = d;
  else if (strcmp(settingName, "fixedEcefX") == 0)
    settings.fixedEcefX = d;
  else if (strcmp(settingName, "fixedEcefY") == 0)
    settings.fixedEcefY = d;
  else if (strcmp(settingName, "fixedEcefZ") == 0)
    settings.fixedEcefZ = d;
  else if (strcmp(settingName, "fixedLat") == 0)
    settings.fixedLat = d;
  else if (strcmp(settingName, "fixedLong") == 0)
    settings.fixedLong = d;
  else if (strcmp(settingName, "fixedAltitude") == 0)
    settings.fixedAltitude = d;
  else if (strcmp(settingName, "dataPortBaud") == 0)
    settings.dataPortBaud = d;
  else if (strcmp(settingName, "radioPortBaud") == 0)
    settings.radioPortBaud = d;
  else if (strcmp(settingName, "enableNtripServer") == 0)
    settings.enableNtripServer = d;
  else if (strcmp(settingName, "casterHost") == 0)
    strcpy(settings.casterHost, settingValue);
  else if (strcmp(settingName, "casterPort") == 0)
    settings.casterPort = d;
  else if (strcmp(settingName, "mountPoint") == 0)
    strcpy(settings.mountPoint, settingValue);
  else if (strcmp(settingName, "mountPointPW") == 0)
    strcpy(settings.mountPointPW, settingValue);
  else if (strcmp(settingName, "wifiSSID") == 0)
    strcpy(settings.wifiSSID, settingValue);
  else if (strcmp(settingName, "wifiPW") == 0)
    strcpy(settings.wifiPW, settingValue);
  else if (strcmp(settingName, "surveyInStartingAccuracy") == 0)
    settings.surveyInStartingAccuracy = d;
  else if (strcmp(settingName, "measurementRate") == 0)
    settings.measurementRate = d;
  else if (strcmp(settingName, "navigationRate") == 0)
    settings.navigationRate = d;
  else if (strcmp(settingName, "enableI2Cdebug") == 0)
    settings.enableI2Cdebug = d;
  else if (strcmp(settingName, "enableHeapReport") == 0)
    settings.enableHeapReport = d;
  else if (strcmp(settingName, "enableTaskReports") == 0)
    settings.enableTaskReports = d;
  else if (strcmp(settingName, "dataPortChannel") == 0)
    settings.dataPortChannel = (muxConnectionType_e)d;
  else if (strcmp(settingName, "spiFrequency") == 0)
    settings.spiFrequency = d;
  else if (strcmp(settingName, "enableLogging") == 0)
    settings.enableLogging = d;
  else if (strcmp(settingName, "sppRxQueueSize") == 0)
    settings.sppRxQueueSize = d;
  else if (strcmp(settingName, "sppTxQueueSize") == 0)
    settings.sppTxQueueSize = d;
  else if (strcmp(settingName, "dynamicModel") == 0)
    settings.dynamicModel = d;
  else if (strcmp(settingName, "lastState") == 0)
    settings.lastState = (SystemState)d;
  else if (strcmp(settingName, "throttleDuringSPPCongestion") == 0)
    settings.throttleDuringSPPCongestion = d;
  else if (strcmp(settingName, "enableSensorFusion") == 0)
    settings.enableSensorFusion = d;
  else if (strcmp(settingName, "autoIMUmountAlignment") == 0)
    settings.autoIMUmountAlignment = d;
  else if (strcmp(settingName, "enableResetDisplay") == 0)
    settings.enableResetDisplay = d;

  //Check for bulk settings (constellations and message rates)
  //Must be last on else list
  else
  {
    bool knownSetting = false;

    //Scan for constellation settings
    if (knownSetting == false)
    {
      for (int x = 0 ; x < MAX_CONSTELLATIONS ; x++)
      {
        char tempString[50]; //constellation.GPS=1
        sprintf(tempString, "constellation.%s", ubxConstellations[x].textName);

        if (strcmp(settingName, tempString) == 0)
        {
          ubxConstellations[x].enabled = d;
          knownSetting = true;
          break;
        }
      }
    }

    //Scan for message settings
    if (knownSetting == false)
    {
      for (int x = 0 ; x < MAX_UBX_MSG ; x++)
      {
        char tempString[50]; //message.nmea_dtm.msgRate=5
        sprintf(tempString, "message.%s.msgRate", ubxMessages[x].msgTextName);

        if (strcmp(settingName, tempString) == 0)
        {
          ubxMessages[x].msgRate = d;
          knownSetting = true;
          break;
        }
      }
    }

    //Last catch
    if (knownSetting == false)
    {
      Serial.printf("Unknown setting %s on line: %s\r\n", settingName, str);
    }
  }

  return (true);
}

//ESP32 doesn't have erase command so we do it here
void eepromErase()
{
  for (int i = 0 ; i < EEPROM_SIZE ; i++) {
    EEPROM.write(i, 0xFF); //Reset to all 1s
  }
  EEPROM.commit();
}

//The SD library doesn't have a fgets function like SD fat so recreate it here
//Read the current line in the file until we hit a EOL char \r or \n
int getLine(File * openFile, char * lineChars, int lineSize)
{
  int count = 0;
  while (openFile->available())
  {
    byte incoming = openFile->read();
    if (incoming == '\r' || incoming == '\n')
    {
      //Sometimes a line has multiple terminators
      while (openFile->peek() == '\r' || openFile->peek() == '\n')
        openFile->read(); //Dump it to prevent next line read corruption
      break;
    }

    lineChars[count++] = incoming;

    if (count == lineSize - 1)
      break; //Stop before overun of buffer
  }
  lineChars[count] = '\0'; //Terminate string
  return (count);
}
