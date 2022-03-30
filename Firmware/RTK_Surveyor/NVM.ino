/*
  For any new setting added to the settings struct, we must add it to setting file
  recording and logging, and to the WiFi AP load/read in the following places:

  recordSystemSettingsToFile();
  parseLine();
  createSettingsString();
  updateSettingWithValue();

  form.h also needs to be updated to include a space for user input. This is best
  edited in the index.html and main.js files.
*/

//We use the LittleFS library to store user profiles in SPIFFs
//Move selected user profile from SPIFFs into settings struct (RAM)
//We originally used EEPROM but it was limited to 4096 bytes. Each settings struct is ~4000 bytes
//so multiple user profiles wouldn't fit. Prefences was limited to a single putBytes of ~3000 bytes.
//So we moved again to SPIFFs. It's being replaced by LittleFS so here we are.
void loadSettings()
{
  //First, look up the last used profile number
  uint8_t profileNumber = getProfileNumber();

  //Load the settings file into a temp holder until we know it's valid
  Settings tempSettings;
  if (getSettings(profileNumber, tempSettings) == true)
    settings = tempSettings; //Settings are good. Move them over.

  loadSystemSettingsFromFile(); //Load any settings from config file. This will over-write any pre-existing LittleFS settings.

  //Change default profile names to 'Profile1' etc
  if (strcmp(settings.profileName, "Default") == 0)
    sprintf(settings.profileName, "Profile%d", profileNumber + 1);

  //Record these settings to LittleFS and SD file to be sure they are the same
  recordSystemSettings();

  activeProfiles = getActiveProfiles(); //Count is used during menu display

  log_d("Settings profile #%d loaded", profileNumber);
}

//Load a given settings file into a given settings array
//Check settings file for correct length and ID
//Returns true if settings file was loaded successfully
bool getSettings(uint8_t fileNumber, Settings &localSettings)
{
  //Read the file into tempSettings for further verification
  uint8_t *settingsBytes = (uint8_t *)&localSettings; // Cast the struct into a uint8_t ptr

  //With the given profile number, load appropriate settings file
  char settingsFileName[40];
  sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, fileNumber);

  File settingsFile = LittleFS.open(settingsFileName, FILE_READ);
  if (!settingsFile)
  {
    log_d("Setting file %s not found. Using default settings.", settingsFileName);
    return (false);
  }

  uint16_t fileSize = settingsFile.size();
  if (fileSize > sizeof(Settings)) fileSize = sizeof(Settings); //Trim to max setting struct size
  settingsFile.read(settingsBytes, fileSize); //Copy the bytes from file into testSettings struct
  settingsFile.close();

  //Check that the current settings struct size matches what is stored in this settings file
  //Misalignment happens when we add a new feature or setting
  if (fileSize != sizeof(Settings))
  {
    log_d("Settings file wrong size: %d bytes, should be %d bytes. Using default settings.", fileSize, sizeof(settings));
    return (false);
  }

  //Check that the rtkIdentifier is correct
  //(It is possible for two different versions of the code to have the same sizeOfSettings - which causes problems!)
  if (localSettings.rtkIdentifier != RTK_IDENTIFIER)
  {
    log_d("Settings are not valid for this variant of RTK %s. Found %s, should be %s. Using default settings.", platformPrefix, settings.rtkIdentifier, RTK_IDENTIFIER);
    return (false);
  }

  log_d("Using LittleFS file: %s", settingsFileName);

  return (true);
}

//Load the special profileNumber file in LittleFS and return one byte value
uint8_t getProfileNumber()
{
  uint8_t profileNumber = 0;

  File fileProfileNumber = LittleFS.open("/profileNumber.txt", FILE_READ);
  if (!fileProfileNumber)
  {
    log_d("profileNumber.txt not found");
    profileNumber = 0;
    recordProfileNumber(profileNumber);
  }
  else
  {
    profileNumber = fileProfileNumber.read();
    fileProfileNumber.close();
  }

  //We have arbitrary limit of 4 user profiles
  if (profileNumber >= MAX_PROFILE_COUNT)
  {
    log_d("ProfileNumber invalid. Going to zero.");
    profileNumber = 0;
    recordProfileNumber(profileNumber);
  }

  log_d("Using profile #%d", profileNumber);
  return (profileNumber);
}

//Return the number of non-empty settings files
uint8_t getActiveProfiles()
{
  int profileCount = 0;

  for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
  {
    //With the given profile number, load appropriate settings file
    char settingsFileName[40];
    sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, x);

    if (LittleFS.exists(settingsFileName))
      profileCount++;
  }

  log_d("%d active profiles", profileCount);
  return (profileCount);
}

//Loads a given profile name.
//Profiles may not be sequential (user might have empty profile #2, but filled #3) so we load the profile unit, not the number
//Return true if successful
bool getProfileName(uint8_t profileUnit, char *profileName, uint8_t profileNameLength)
{
  uint8_t located = 0;

  //Step through possible profiles looking for the 1st, 2nd, 3rd, or 4th unit
  for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
  {
    //With the given profile number, load appropriate settings file
    char settingsFileName[40];
    sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, x);

    if (LittleFS.exists(settingsFileName))
    {
      if (located == profileUnit)
      {
        //Open this profile and get the profile name from it
        File settingsFile = LittleFS.open(settingsFileName, FILE_READ);

        Settings tempSettings;
        uint8_t *settingsBytes = (uint8_t *)&tempSettings; // Cast the struct into a uint8_t ptr

        uint16_t fileSize = settingsFile.size();
        if (fileSize > sizeof(tempSettings)) fileSize = sizeof(tempSettings); //Trim to max setting struct size

        settingsFile.read(settingsBytes, fileSize); //Copy the bytes from file into testSettings struct
        settingsFile.close();

        snprintf(profileName, profileNameLength, "%s", tempSettings.profileName); //snprintf handles null terminator
        return (true);
      }

      located++; //Valid settingFileName but not the unit we are looking for
    }
  }
  log_d("Profile unit %d not found", profileUnit);

  return (false);
}

//Return profile number based on units
//Profiles may not be sequential (user might have empty profile #2, but filled #3) so we look up the profile unit and return the count
uint8_t getProfileNumberFromUnit(uint8_t profileUnit)
{
  uint8_t located = 0;

  //Step through possible profiles looking for the 1st, 2nd, 3rd, or 4th unit
  for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
  {
    //With the given profile number, load appropriate settings file
    char settingsFileName[40];
    sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, x);

    if (LittleFS.exists(settingsFileName))
    {
      if (located == profileUnit)
      {
        return (located);
      }

      located++; //Valid settingFileName but not the unit we are looking for
    }
  }
  log_d("Profile unit %d not found", profileUnit);

  return (false);
}

//Record the given profile number
void recordProfileNumber(uint8_t profileNumber)
{
  File fileProfileNumber = LittleFS.open("/profileNumber.txt", FILE_WRITE);
  if (!fileProfileNumber)
  {
    log_d("profileNumber.txt failed to open");
    return;
  }
  fileProfileNumber.write(profileNumber);
  fileProfileNumber.close();
}

//Record the current settings struct to LittleFS and then to SD file if available
void recordSystemSettings()
{
  char settingsFileName[40];
  sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, getProfileNumber());

  File settingsFile = LittleFS.open(settingsFileName, FILE_WRITE);
  if (!settingsFile)
  {
    log_d("Failed to write to settings file %s", settingsFileName);
  }
  else
  {
    settings.sizeOfSettings = sizeof(settings); //Update to current setting size

    uint8_t *settingsBytes = (uint8_t *)&settings; // cast the struct into a uint8_t ptr
    settingsFile.write(settingsBytes, sizeof(settings)); //Store raw settings bytes into file
    settingsFile.close();
    log_d("Settings recorded to LittleFS: %s", settingsFileName);
  }

  recordSystemSettingsToFile();
}

//Load settings without recording
//Used at very first boot to test for resetCounter
void loadSettingsPartial()
{
  //First, look up the last used profile number
  uint8_t profileNumber = getProfileNumber();

  //Load the settings file into a temp holder until we know it's valid
  Settings tempSettings;
  if (getSettings(profileNumber, tempSettings) == true)
    settings = tempSettings; //Settings are good. Move them over.
}

//Export the current settings to a config file
//We only record the active profile to the appropriate 'SFE_Facet_Settings_2.txt' file.
void recordSystemSettingsToFile()
{
  if (online.microSD == true)
  {
    //Attempt to write to file system. This avoids collisions with file writing from other functions like updateLogs()
    if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      //Assemble settings file name
      char settingsFileName[40]; //SFE_Surveyor_Settings.txt
      sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, getProfileNumber());

      if (sd.exists(settingsFileName))
        sd.remove(settingsFileName);

      SdFile settingsFile; //FAT32
      if (settingsFile.open(settingsFileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println(F("Failed to create settings file"));
        return;
      }

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
      settingsFile.println("maxLogLength_minutes=" + (String)settings.maxLogLength_minutes);
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
      settingsFile.println("casterUser=" + (String)settings.casterUser);
      settingsFile.println("casterUserPW=" + (String)settings.casterUserPW);
      settingsFile.println("mountPointUpload=" + (String)settings.mountPointUpload);
      settingsFile.println("mountPointUploadPW=" + (String)settings.mountPointUploadPW);
      settingsFile.println("mountPointDownload=" + (String)settings.mountPointDownload);
      settingsFile.println("mountPointDownloadPW=" + (String)settings.mountPointDownloadPW);
      settingsFile.println("casterTransmitGGA=" + (String)settings.casterTransmitGGA);
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
      settingsFile.println("enableExternalPulse=" + (String)settings.enableExternalPulse);
      settingsFile.println("externalPulseTimeBetweenPulse_us=" + (String)settings.externalPulseTimeBetweenPulse_us);
      settingsFile.println("externalPulseLength_us=" + (String)settings.externalPulseLength_us);
      settingsFile.println("externalPulsePolarity=" + (String)settings.externalPulsePolarity);
      settingsFile.println("enableExternalHardwareEventLogging=" + (String)settings.enableExternalHardwareEventLogging);
      settingsFile.println("profileName=" + (String)settings.profileName);

      //Record constellation settings
      for (int x = 0 ; x < MAX_CONSTELLATIONS ; x++)
      {
        char tempString[50]; //constellation.BeiDou=1
        sprintf(tempString, "constellation.%s=%d", settings.ubxConstellations[x].textName, settings.ubxConstellations[x].enabled);
        settingsFile.println(tempString);
      }

      //Record message settings
      for (int x = 0 ; x < MAX_UBX_MSG ; x++)
      {
        char tempString[50]; //message.nmea_dtm.msgRate=5
        sprintf(tempString, "message.%s.msgRate=%d", settings.ubxMessages[x].msgTextName, settings.ubxMessages[x].msgRate);
        settingsFile.println(tempString);
      }

      updateDataFileAccess(&settingsFile); // Update the file access time & date

      settingsFile.close();

      log_d("System settings recorded to file");

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
      sprintf(settingsFileName, "%s_Settings_%d.txt", platformFilePrefix, getProfileNumber());

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
        Serial.println(F("No config file found. Using settings from internal FS."));
        //The defaults of the struct will be recorded to a file later on.
        xSemaphoreGive(xFATSemaphore);
        return (false);
      }

    } //End Semaphore check
  } //End SD online

  log_d("Config file read failed: SD offline");
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

  double d = 0.0;
  char settingValue[50] = "";

  //Move pointer to end of line
  str = strtok(nullptr, "\n");
  if (!str)
  {
    //This line does not contain a \n or the settingValue is zero length
    //so there is nothing to parse
    //https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/77
  }
  else
  {
    //Attempt to convert string to double
    d = strtod(str, &ptr);

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
  }

  //  log_d("settingName: %s", settingName);
  //  log_d("settingValue: %s", settingValue);
  //  log_d("d: %0.3f", d);

  // Get setting name
  if (strcmp(settingName, "sizeOfSettings") == 0)
  {
    //We may want to cause a factory reset from the settings file rather than the menu
    //If user sets sizeOfSettings to -1 in config file, RTK Surveyor will factory reset
    if (d == -1)
    {
      factoryReset(); //Erase file system, erase settings file, reset u-blox module, display message on OLED
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
  else if (strcmp(settingName, "maxLogLength_minutes") == 0)
    settings.maxLogLength_minutes = d;
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

  else if (strcmp(settingName, "casterUser") == 0)
    strcpy(settings.casterUser, settingValue);
  else if (strcmp(settingName, "casterUserPW") == 0)
    strcpy(settings.casterUserPW, settingValue);
  else if (strcmp(settingName, "mountPointUpload") == 0)
    strcpy(settings.mountPointUpload, settingValue);
  else if (strcmp(settingName, "mountPointUploadPW") == 0)
    strcpy(settings.mountPointUploadPW, settingValue);
  else if (strcmp(settingName, "mountPointDownload") == 0)
    strcpy(settings.mountPointDownload, settingValue);
  else if (strcmp(settingName, "mountPointDownloadPW") == 0)
    strcpy(settings.mountPointDownloadPW, settingValue);
  else if (strcmp(settingName, "casterTransmitGGA") == 0)
    settings.casterTransmitGGA = d;
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
  else if (strcmp(settingName, "enableExternalPulse") == 0)
    settings.enableExternalPulse = d;
  else if (strcmp(settingName, "externalPulseTimeBetweenPulse_us") == 0)
    settings.externalPulseTimeBetweenPulse_us = d;
  else if (strcmp(settingName, "externalPulseLength_us") == 0)
    settings.externalPulseLength_us = d;
  else if (strcmp(settingName, "externalPulsePolarity") == 0)
    settings.externalPulsePolarity = (pulseEdgeType_e)d;
  else if (strcmp(settingName, "enableExternalHardwareEventLogging") == 0)
    settings.enableExternalHardwareEventLogging = d;
  else if (strcmp(settingName, "profileName") == 0)
    strcpy(settings.profileName, settingValue);

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
        sprintf(tempString, "constellation.%s", settings.ubxConstellations[x].textName);

        if (strcmp(settingName, tempString) == 0)
        {
          settings.ubxConstellations[x].enabled = d;
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
        sprintf(tempString, "message.%s.msgRate", settings.ubxMessages[x].msgTextName);

        if (strcmp(settingName, tempString) == 0)
        {
          settings.ubxMessages[x].msgRate = d;
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
