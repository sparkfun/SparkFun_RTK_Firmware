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
  //If we have a profile in both LFS and SD, the SD settings will overwrite LFS
  loadSystemSettingsFromFileLFS(settingsFileName, &settings);

  //Temp store any variables from LFS that should override SD
  int resetCount = settings.resetCount;

  loadSystemSettingsFromFileSD(settingsFileName, &settings);
  settings.resetCount = resetCount;

  //Change empty profile name to 'Profile1' etc
  if (strlen(settings.profileName) == 0)
    sprintf(settings.profileName, "Profile%d", profileNumber + 1);

  //Record these settings to LittleFS and SD file to be sure they are the same
  recordSystemSettings();

  //Get bitmask of active profiles
  activeProfiles = loadProfileNames();

  Serial.printf("Profile '%s' loaded\r\n", profileNames[profileNumber]);
}

//Set the settingsFileName used many places
void setSettingsFileName()
{
  sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, profileNumber);
}

//Load only LFS settings without recording
//Used at very first boot to test for resetCounter
void loadSettingsPartial()
{
  //First, look up the last used profile number
  loadProfileNumber();

  //Set the settingsFileName used many places
  setSettingsFileName();

  loadSystemSettingsFromFileLFS(settingsFileName, &settings);
}

void recordSystemSettings()
{
  settings.sizeOfSettings = sizeof(settings); //Update to current setting size

  recordSystemSettingsToFileSD(settingsFileName); //Record to SD if available
  recordSystemSettingsToFileLFS(settingsFileName); //Record to LFS if available
}

//Export the current settings to a config file on SD
//We share the recording with LittleFS so this is all the semphore and SD specific handling
void recordSystemSettingsToFileSD(char *fileName)
{
  bool gotSemaphore = false;
  bool wasSdCardOnline;

  //Try to gain access the SD card
  wasSdCardOnline = online.microSD;
  if (online.microSD != true)
    beginSD();

  while (online.microSD == true)
  {
    //Attempt to write to file system. This avoids collisions with file writing from other functions like updateLogs()
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      gotSemaphore = true;
      if (sd->exists(fileName))
      {
        log_d("Removing from SD: %s", fileName);
        sd->remove(fileName);
      }

      SdFile settingsFile; //FAT32
      if (settingsFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        Serial.println("Failed to create settings file");
        break;
      }

      updateDataFileCreate(&settingsFile); // Update the file to create time & date

      recordSystemSettingsToFile((File *)&settingsFile); //Record all the settings via strings to file

      updateDataFileAccess(&settingsFile); // Update the file access time & date

      settingsFile.close();

      log_d("Settings recorded to SD: %s", fileName);
    }
    else
    {
      //This is an error because the current settings no longer match the settings
      //on the microSD card, and will not be restored to the expected settings!
      Serial.printf("sdCardSemaphore failed to yield, NVM.ino line %d\r\n", __LINE__);
    }
    break;
  }

  //Release access the SD card
  if (online.microSD && (!wasSdCardOnline))
    endSD(gotSemaphore, true);
  else if (gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);
}

//Export the current settings to a config file on SD
//We share the recording with LittleFS so this is all the semphore and SD specific handling
void recordSystemSettingsToFileLFS(char *fileName)
{
  if (online.fs == true)
  {
    if (LittleFS.exists(fileName))
    {
      LittleFS.remove(fileName);
      log_d("Removing LittleFS: %s", fileName);
    }

    File settingsFile = LittleFS.open(fileName, FILE_WRITE);
    if (!settingsFile)
    {
      log_d("Failed to write to settings file %s", fileName);
    }
    else
    {
      recordSystemSettingsToFile(&settingsFile); //Record all the settings via strings to file
      settingsFile.close();
      log_d("Settings recorded to LittleFS: %s", fileName);
    }
  }
}

//Write the settings struct to a clear text file
void recordSystemSettingsToFile(File * settingsFile)
{
  settingsFile->printf("%s=%d\r\n", "sizeOfSettings", settings.sizeOfSettings);
  settingsFile->printf("%s=%d\r\n", "rtkIdentifier", settings.rtkIdentifier);

  char firmwareVersion[30]; //v1.3 December 31 2021
  sprintf(firmwareVersion, "v%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
  settingsFile->printf("%s=%s\r\n", "rtkFirmwareVersion", firmwareVersion);

  settingsFile->printf("%s=%s\r\n", "zedFirmwareVersion", zedFirmwareVersion);
  if (productVariant == RTK_FACET_LBAND)
    settingsFile->printf("%s=%s\r\n", "neoFirmwareVersion", neoFirmwareVersion);
  settingsFile->printf("%s=%d\r\n", "printDebugMessages", settings.printDebugMessages);
  settingsFile->printf("%s=%d\r\n", "enableSD", settings.enableSD);
  settingsFile->printf("%s=%d\r\n", "enableDisplay", settings.enableDisplay);
  settingsFile->printf("%s=%d\r\n", "maxLogTime_minutes", settings.maxLogTime_minutes);
  settingsFile->printf("%s=%d\r\n", "maxLogLength_minutes", settings.maxLogLength_minutes);
  settingsFile->printf("%s=%d\r\n", "observationSeconds", settings.observationSeconds);
  settingsFile->printf("%s=%0.2f\r\n", "observationPositionAccuracy", settings.observationPositionAccuracy);
  settingsFile->printf("%s=%d\r\n", "fixedBase", settings.fixedBase);
  settingsFile->printf("%s=%d\r\n", "fixedBaseCoordinateType", settings.fixedBaseCoordinateType);
  settingsFile->printf("%s=%0.3f\r\n", "fixedEcefX", settings.fixedEcefX); //-1280206.568
  settingsFile->printf("%s=%0.3f\r\n", "fixedEcefY", settings.fixedEcefY);
  settingsFile->printf("%s=%0.3f\r\n", "fixedEcefZ", settings.fixedEcefZ);
  settingsFile->printf("%s=%0.9f\r\n", "fixedLat", settings.fixedLat); //40.09029479
  settingsFile->printf("%s=%0.9f\r\n", "fixedLong", settings.fixedLong);
  settingsFile->printf("%s=%0.4f\r\n", "fixedAltitude", settings.fixedAltitude);
  settingsFile->printf("%s=%d\r\n", "dataPortBaud", settings.dataPortBaud);
  settingsFile->printf("%s=%d\r\n", "radioPortBaud", settings.radioPortBaud);
  settingsFile->printf("%s=%0.1f\r\n", "surveyInStartingAccuracy", settings.surveyInStartingAccuracy);
  settingsFile->printf("%s=%d\r\n", "measurementRate", settings.measurementRate);
  settingsFile->printf("%s=%d\r\n", "navigationRate", settings.navigationRate);
  settingsFile->printf("%s=%d\r\n", "enableI2Cdebug", settings.enableI2Cdebug);
  settingsFile->printf("%s=%d\r\n", "enableHeapReport", settings.enableHeapReport);
  settingsFile->printf("%s=%d\r\n", "enableTaskReports", settings.enableTaskReports);
  settingsFile->printf("%s=%d\r\n", "dataPortChannel", (uint8_t)settings.dataPortChannel);
  settingsFile->printf("%s=%d\r\n", "spiFrequency", settings.spiFrequency);
  settingsFile->printf("%s=%d\r\n", "sppRxQueueSize", settings.sppRxQueueSize);
  settingsFile->printf("%s=%d\r\n", "sppTxQueueSize", settings.sppTxQueueSize);
  settingsFile->printf("%s=%d\r\n", "dynamicModel", settings.dynamicModel);
  settingsFile->printf("%s=%d\r\n", "lastState", settings.lastState);
  settingsFile->printf("%s=%d\r\n", "enableSensorFusion", settings.enableSensorFusion);
  settingsFile->printf("%s=%d\r\n", "autoIMUmountAlignment", settings.autoIMUmountAlignment);
  settingsFile->printf("%s=%d\r\n", "enableResetDisplay", settings.enableResetDisplay);
  settingsFile->printf("%s=%d\r\n", "enableExternalPulse", settings.enableExternalPulse);
  settingsFile->printf("%s=%d\r\n", "externalPulseTimeBetweenPulse_us", settings.externalPulseTimeBetweenPulse_us);
  settingsFile->printf("%s=%d\r\n", "externalPulseLength_us", settings.externalPulseLength_us);
  settingsFile->printf("%s=%d\r\n", "externalPulsePolarity", settings.externalPulsePolarity);
  settingsFile->printf("%s=%d\r\n", "enableExternalHardwareEventLogging", settings.enableExternalHardwareEventLogging);
  settingsFile->printf("%s=%s\r\n", "profileName", settings.profileName);
  settingsFile->printf("%s=%d\r\n", "enableNtripServer", settings.enableNtripServer);
  settingsFile->printf("%s=%d\r\n", "ntripServer_StartAtSurveyIn", settings.ntripServer_StartAtSurveyIn);
  settingsFile->printf("%s=%s\r\n", "ntripServer_CasterHost", settings.ntripServer_CasterHost);
  settingsFile->printf("%s=%d\r\n", "ntripServer_CasterPort", settings.ntripServer_CasterPort);
  settingsFile->printf("%s=%s\r\n", "ntripServer_CasterUser", settings.ntripServer_CasterUser);
  settingsFile->printf("%s=%s\r\n", "ntripServer_CasterUserPW", settings.ntripServer_CasterUserPW);
  settingsFile->printf("%s=%s\r\n", "ntripServer_MountPoint", settings.ntripServer_MountPoint);
  settingsFile->printf("%s=%s\r\n", "ntripServer_MountPointPW", settings.ntripServer_MountPointPW);
  settingsFile->printf("%s=%s\r\n", "ntripServer_wifiSSID", settings.ntripServer_wifiSSID);
  settingsFile->printf("%s=%s\r\n", "ntripServer_wifiPW", settings.ntripServer_wifiPW);
  settingsFile->printf("%s=%d\r\n", "enableNtripClient", settings.enableNtripClient);
  settingsFile->printf("%s=%s\r\n", "ntripClient_CasterHost", settings.ntripClient_CasterHost);
  settingsFile->printf("%s=%d\r\n", "ntripClient_CasterPort", settings.ntripClient_CasterPort);
  settingsFile->printf("%s=%s\r\n", "ntripClient_CasterUser", settings.ntripClient_CasterUser);
  settingsFile->printf("%s=%s\r\n", "ntripClient_CasterUserPW", settings.ntripClient_CasterUserPW);
  settingsFile->printf("%s=%s\r\n", "ntripClient_MountPoint", settings.ntripClient_MountPoint);
  settingsFile->printf("%s=%s\r\n", "ntripClient_MountPointPW", settings.ntripClient_MountPointPW);
  settingsFile->printf("%s=%s\r\n", "ntripClient_wifiSSID", settings.ntripClient_wifiSSID);
  settingsFile->printf("%s=%s\r\n", "ntripClient_wifiPW", settings.ntripClient_wifiPW);
  settingsFile->printf("%s=%d\r\n", "ntripClient_TransmitGGA", settings.ntripClient_TransmitGGA);
  settingsFile->printf("%s=%d\r\n", "serialTimeoutGNSS", settings.serialTimeoutGNSS);
  settingsFile->printf("%s=%s\r\n", "pointPerfectDeviceProfileToken", settings.pointPerfectDeviceProfileToken);
  settingsFile->printf("%s=%d\r\n", "enablePointPerfectCorrections", settings.enablePointPerfectCorrections);
  settingsFile->printf("%s=%s\r\n", "home_wifiSSID", settings.home_wifiSSID);
  settingsFile->printf("%s=%s\r\n", "home_wifiPW", settings.home_wifiPW);
  settingsFile->printf("%s=%d\r\n", "autoKeyRenewal", settings.autoKeyRenewal);
  settingsFile->printf("%s=%s\r\n", "pointPerfectClientID", settings.pointPerfectClientID);
  settingsFile->printf("%s=%s\r\n", "pointPerfectBrokerHost", settings.pointPerfectBrokerHost);
  settingsFile->printf("%s=%s\r\n", "pointPerfectLBandTopic", settings.pointPerfectLBandTopic);
  settingsFile->printf("%s=%s\r\n", "pointPerfectCurrentKey", settings.pointPerfectCurrentKey);
  settingsFile->printf("%s=%llu\r\n", "pointPerfectCurrentKeyDuration", settings.pointPerfectCurrentKeyDuration);
  settingsFile->printf("%s=%llu\r\n", "pointPerfectCurrentKeyStart", settings.pointPerfectCurrentKeyStart);
  settingsFile->printf("%s=%s\r\n", "pointPerfectNextKey", settings.pointPerfectNextKey);
  settingsFile->printf("%s=%llu\r\n", "pointPerfectNextKeyDuration", settings.pointPerfectNextKeyDuration);
  settingsFile->printf("%s=%llu\r\n", "pointPerfectNextKeyStart", settings.pointPerfectNextKeyStart);
  settingsFile->printf("%s=%llu\r\n", "lastKeyAttempt", settings.lastKeyAttempt);
  settingsFile->printf("%s=%d\r\n", "updateZEDSettings", settings.updateZEDSettings);
  settingsFile->printf("%s=%d\r\n", "LBandFreq", settings.LBandFreq);
  settingsFile->printf("%s=%d\r\n", "enableLogging", settings.enableLogging);
  settingsFile->printf("%s=%d\r\n", "timeZoneHours", settings.timeZoneHours);
  settingsFile->printf("%s=%d\r\n", "timeZoneMinutes", settings.timeZoneMinutes);
  settingsFile->printf("%s=%d\r\n", "timeZoneSeconds", settings.timeZoneSeconds);
  settingsFile->printf("%s=%d\r\n", "enablePrintWifiIpAddress", settings.enablePrintWifiIpAddress);
  settingsFile->printf("%s=%d\r\n", "enablePrintState", settings.enablePrintState);
  settingsFile->printf("%s=%d\r\n", "enablePrintWifiState", settings.enablePrintWifiState);
  settingsFile->printf("%s=%d\r\n", "enablePrintNtripClientState", settings.enablePrintNtripClientState);
  settingsFile->printf("%s=%d\r\n", "enablePrintNtripServerState", settings.enablePrintNtripServerState);
  settingsFile->printf("%s=%d\r\n", "enablePrintPosition", settings.enablePrintPosition);
  settingsFile->printf("%s=%d\r\n", "enableMarksFile", settings.enableMarksFile);
  settingsFile->printf("%s=%d\r\n", "enablePrintBatteryMessages", settings.enablePrintBatteryMessages);
  settingsFile->printf("%s=%d\r\n", "enablePrintRoverAccuracy", settings.enablePrintRoverAccuracy);
  settingsFile->printf("%s=%d\r\n", "enablePrintBadMessages", settings.enablePrintBadMessages);
  settingsFile->printf("%s=%d\r\n", "enablePrintLogFileMessages", settings.enablePrintLogFileMessages);
  settingsFile->printf("%s=%d\r\n", "enablePrintLogFileStatus", settings.enablePrintLogFileStatus);
  settingsFile->printf("%s=%d\r\n", "enablePrintRingBufferOffsets", settings.enablePrintRingBufferOffsets);
  settingsFile->printf("%s=%d\r\n", "enablePrintNtripServerRtcm", settings.enablePrintNtripServerRtcm);
  settingsFile->printf("%s=%d\r\n", "enablePrintNtripClientRtcm", settings.enablePrintNtripClientRtcm);
  settingsFile->printf("%s=%d\r\n", "enablePrintStates", settings.enablePrintStates);
  settingsFile->printf("%s=%d\r\n", "enablePrintDuplicateStates", settings.enablePrintDuplicateStates);
  settingsFile->printf("%s=%d\r\n", "radioType", settings.radioType);

  //Record peer MAC addresses
  for (int x = 0 ; x < settings.espnowPeerCount ; x++)
  {
    char tempString[50]; //espnowPeers.1=B4,C1,33,42,DE,01,
    sprintf(tempString, "espnowPeers.%d=%02X,%02X,%02X,%02X,%02X,%02X,", x,
            settings.espnowPeers[x][0],
            settings.espnowPeers[x][1],
            settings.espnowPeers[x][2],
            settings.espnowPeers[x][3],
            settings.espnowPeers[x][4],
            settings.espnowPeers[x][5]
           );
    settingsFile->println(tempString);
  }
  settingsFile->printf("%s=%d\r\n", "espnowPeerCount", settings.espnowPeerCount);
  settingsFile->printf("%s=%d\r\n", "enableRtcmMessageChecking", settings.enableRtcmMessageChecking);
  settingsFile->printf("%s=%d\r\n", "bluetoothRadioType", settings.bluetoothRadioType);
  settingsFile->printf("%s=%d\r\n", "enableNmeaClient", settings.enableNmeaClient);
  settingsFile->printf("%s=%d\r\n", "enableNmeaServer", settings.enableNmeaServer);
  settingsFile->printf("%s=%d\r\n", "espnowBroadcast", settings.espnowBroadcast);
  settingsFile->printf("%s=%d\r\n", "antennaHeight", settings.antennaHeight);
  settingsFile->printf("%s=%0.2f\r\n", "antennaReferencePoint", settings.antennaReferencePoint);

  //Record constellation settings
  for (int x = 0 ; x < MAX_CONSTELLATIONS ; x++)
  {
    char tempString[50]; //constellation.BeiDou=1
    sprintf(tempString, "constellation.%s=%d", settings.ubxConstellations[x].textName, settings.ubxConstellations[x].enabled);
    settingsFile->println(tempString);
  }

  //Record message settings
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
  {
    char tempString[50]; //message.nmea_dtm.msgRate=5
    sprintf(tempString, "message.%s.msgRate=%d", settings.ubxMessages[x].msgTextName, settings.ubxMessages[x].msgRate);
    settingsFile->println(tempString);
  }
}

//Given a fileName, parse the file and load the given settings struct
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadSystemSettingsFromFileSD(char* fileName, Settings *settings)
{
  bool gotSemaphore = false;
  bool status = false;
  bool wasSdCardOnline;

  //Try to gain access the SD card
  wasSdCardOnline = online.microSD;
  if (online.microSD != true)
    beginSD();

  while (online.microSD == true)
  {
    //Attempt to access file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      gotSemaphore = true;
      if (sd->exists(fileName))
      {
        SdFile settingsFile; //FAT32
        if (settingsFile.open(fileName, O_READ) == false)
        {
          Serial.println("Failed to open settings file");
          break;
        }

        char line[60];
        int lineNumber = 0;

        while (settingsFile.available())
        {
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
              Serial.println("Giving up on settings file");
              break;
            }
          }
          else if (parseLine(line, settings) == false) {
            Serial.printf("Failed to parse line %d: %s\r\n", lineNumber, line);
            if (lineNumber == 0)
            {
              //If we can't read the first line of the settings file, give up
              Serial.println("Giving up on settings file");
              break;
            }
          }

          lineNumber++;
        }

        //Serial.println("Config file read complete");
        settingsFile.close();
        status = true;
        break;
      }
      else
      {
        log_d("File %s not found", fileName);
        break;
      }
    } //End Semaphore check
    else
    {
      //This is an error because if the settings exist on the microSD card that
      //those settings are not overriding the current settings as documented!
      Serial.printf("sdCardSemaphore failed to yield, NVM.ino line %d\r\n", __LINE__);
    }
    break;
  } //End SD online

  if (online.microSD != true)
    log_d("Config file read failed: SD offline");

  //Release access the SD card
  if (online.microSD && (!wasSdCardOnline))
    endSD(gotSemaphore, true);
  else if (gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);

  return status;
}

//Given a fileName, parse the file and load the given settings struct
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadSystemSettingsFromFileLFS(char* fileName, Settings *settings)
{
  File settingsFile = LittleFS.open(fileName, FILE_READ);
  if (!settingsFile)
    return (false);

  char line[100];
  int lineNumber = 0;

  while (settingsFile.available())
  {
    //Get the next line from the file
    int n = getLine(&settingsFile, line, sizeof(line)); //Use with SD library
    //int n = settingsFile.fgets(line, sizeof(line)); //Use with SdFat library
    if (n <= 0) {
      Serial.printf("Failed to read line %d from settings file\r\n", lineNumber);
    }
    else if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) {
      Serial.printf("Settings line %d too long\r\n", lineNumber);
      if (lineNumber == 0)
      {
        //If we can't read the first line of the settings file, give up
        Serial.println("Giving up on settings file");
        return (false);
      }
    }
    else if (parseLine(line, settings) == false) {
      Serial.printf("Failed to parse line %d: %s\r\n", lineNumber, line);
      if (lineNumber == 0)
      {
        //If we can't read the first line of the settings file, give up
        Serial.println("Giving up on settings file");
        return (false);
      }
    }

    lineNumber++;
  }

  settingsFile.close();
  return (true);
}

//Convert a given line from file into a settingName and value
//Sets the setting if the name is known
bool parseLine(char* str, Settings *settings)
{
  char* ptr;

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str)
  {
    log_d("Fail");
    return false;
  }

  //Store this setting name
  char settingName[100];
  sprintf(settingName, "%s", str);

  double d = 0.0;
  char settingValue[100] = "";

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
    //if (strcmp(settingName, "ntripServer_CasterHost") == 0) //Debug
    //if (strcmp(settingName, "profileName") == 0) //Debug
    //  Serial.printf("Found problem spot raw: %s\r\n", str);

    //Assume the value is a string such as 8d8a48b. The leading number causes skipSpace to fail.
    //If settingValue has a mix of letters and numbers, just convert to string
    sprintf(settingValue, "%s", str);

    //Check if string is mixed: 8a011EF, 192.168.1.1, -102.4, t6-h4$, etc.
    bool hasSymbol = false;
    int decimalCount = 0;
    for (int x = 0 ; x < strlen(settingValue) ; x++)
    {
      if (settingValue[x] == '.') decimalCount++;
      else if (x == 0 && settingValue[x] == '-')
      {
        ; //Do nothing
      }
      else if (isAlpha(settingValue[x])) hasSymbol = true;
      else if (isDigit(settingValue[x]) == false) hasSymbol = true;
    }

    //See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/274
    if (hasSymbol || decimalCount > 1)
    {
      //It's a mix. Skip strtod.

      //if (strcmp(settingName, "ntripServer_CasterHost") == 0) //Debug
      //  Serial.printf("Skipping strtod - settingValue: %s\r\n", settingValue);
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
  }

  //log_d("settingName: %s - value: %s - d: %0.9f", settingName, settingValue, d);

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
    if (d != sizeof(Settings))
      Serial.printf("Warning: Settings size is %d but current firmware expects %d. Attempting to use settings from file.\r\n", (int)d, sizeof(Settings));

  }
  else if (strcmp(settingName, "rtkIdentifier") == 0)
  {} //Do nothing. Just read it to avoid 'Unknown setting' error
  else if (strcmp(settingName, "rtkFirmwareVersion") == 0)
  {} //Do nothing. Just read it to avoid 'Unknown setting' error
  else if (strcmp(settingName, "zedFirmwareVersion") == 0)
  {} //Do nothing. Just read it to avoid 'Unknown setting' error
  else if (strcmp(settingName, "neoFirmwareVersion") == 0)
  {} //Do nothing. Just read it to avoid 'Unknown setting' error
  else if (strcmp(settingName, "printDebugMessages") == 0)
    settings->printDebugMessages = d;
  else if (strcmp(settingName, "enableSD") == 0)
    settings->enableSD = d;
  else if (strcmp(settingName, "enableDisplay") == 0)
    settings->enableDisplay = d;
  else if (strcmp(settingName, "maxLogTime_minutes") == 0)
    settings->maxLogTime_minutes = d;
  else if (strcmp(settingName, "maxLogLength_minutes") == 0)
    settings->maxLogLength_minutes = d;
  else if (strcmp(settingName, "observationSeconds") == 0)
  {
    if (settings->observationSeconds != d) //If a setting for the ZED has changed, apply, and trigger module config update
    {
      settings->observationSeconds = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "observationPositionAccuracy") == 0)
  {
    if (settings->observationPositionAccuracy != d)
    {
      settings->observationPositionAccuracy = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedBase") == 0)
  {
    if (settings->fixedBase != d)
    {
      settings->fixedBase = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedBaseCoordinateType") == 0)
  {
    if (settings->fixedBaseCoordinateType != d)
    {
      settings->fixedBaseCoordinateType = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedEcefX") == 0)
  {
    if (settings->fixedEcefX != d)
    {
      settings->fixedEcefX = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedEcefY") == 0)
  {
    if (settings->fixedEcefY != d)
    {
      settings->fixedEcefY = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedEcefZ") == 0)
  {
    if (settings->fixedEcefZ != d)
    {
      settings->fixedEcefZ = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedLat") == 0)
  {
    if (settings->fixedLat != d)
    {
      settings->fixedLat = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedLong") == 0)
  {
    if (settings->fixedLong != d)
    {
      settings->fixedLong = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "fixedAltitude") == 0)
  {
    if (settings->fixedAltitude != d)
    {
      settings->fixedAltitude = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "dataPortBaud") == 0)
  {
    if (settings->dataPortBaud != d)
    {
      settings->dataPortBaud = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "radioPortBaud") == 0)
  {
    if (settings->radioPortBaud != d)
    {
      settings->radioPortBaud = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "surveyInStartingAccuracy") == 0)
    settings->surveyInStartingAccuracy = d;
  else if (strcmp(settingName, "measurementRate") == 0)
  {
    if (settings->measurementRate != d)
    {
      settings->measurementRate = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "navigationRate") == 0)
  {
    if (settings->navigationRate != d)
    {
      settings->navigationRate = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "enableI2Cdebug") == 0)
    settings->enableI2Cdebug = d;
  else if (strcmp(settingName, "enableHeapReport") == 0)
    settings->enableHeapReport = d;
  else if (strcmp(settingName, "enableTaskReports") == 0)
    settings->enableTaskReports = d;
  else if (strcmp(settingName, "dataPortChannel") == 0)
    settings->dataPortChannel = (muxConnectionType_e)d;
  else if (strcmp(settingName, "spiFrequency") == 0)
    settings->spiFrequency = d;
  else if (strcmp(settingName, "enableLogging") == 0)
    settings->enableLogging = d;
  else if (strcmp(settingName, "enableMarksFile") == 0)
    settings->enableMarksFile = d;
  else if (strcmp(settingName, "sppRxQueueSize") == 0)
    settings->sppRxQueueSize = d;
  else if (strcmp(settingName, "sppTxQueueSize") == 0)
    settings->sppTxQueueSize = d;
  else if (strcmp(settingName, "dynamicModel") == 0)
  {
    if (settings->dynamicModel != d)
    {
      settings->dynamicModel = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "lastState") == 0)
  {
    if (settings->lastState != (SystemState)d)
    {
      settings->lastState = (SystemState)d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "enableSensorFusion") == 0)
  {
    if (settings->enableSensorFusion != d)
    {
      settings->enableSensorFusion = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "autoIMUmountAlignment") == 0)
  {
    if (settings->autoIMUmountAlignment != d)
    {
      settings->autoIMUmountAlignment = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "enableResetDisplay") == 0)
    settings->enableResetDisplay = d;
  else if (strcmp(settingName, "enableExternalPulse") == 0)
  {
    if (settings->enableExternalPulse != d)
    {
      settings->enableExternalPulse = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "externalPulseTimeBetweenPulse_us") == 0)
  {
    if (settings->externalPulseTimeBetweenPulse_us != d)
    {
      settings->externalPulseTimeBetweenPulse_us = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "externalPulseLength_us") == 0)
  {
    if (settings->externalPulseLength_us != d)
    {
      settings->externalPulseLength_us = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "externalPulsePolarity") == 0)
  {
    if (settings->externalPulsePolarity != (pulseEdgeType_e)d)
    {
      settings->externalPulsePolarity = (pulseEdgeType_e)d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "enableExternalHardwareEventLogging") == 0)
  {
    if (settings->enableExternalHardwareEventLogging != d)
    {
      settings->enableExternalHardwareEventLogging = d;
      settings->updateZEDSettings = true;
    }
  }
  else if (strcmp(settingName, "profileName") == 0)
    strcpy(settings->profileName, settingValue);
  else if (strcmp(settingName, "enableNtripServer") == 0)
    settings->enableNtripServer = d;
  else if (strcmp(settingName, "ntripServer_StartAtSurveyIn") == 0)
    settings->ntripServer_StartAtSurveyIn = d;
  else if (strcmp(settingName, "ntripServer_CasterHost") == 0)
    strcpy(settings->ntripServer_CasterHost, settingValue);
  else if (strcmp(settingName, "ntripServer_CasterPort") == 0)
    settings->ntripServer_CasterPort = d;
  else if (strcmp(settingName, "ntripServer_CasterUser") == 0)
    strcpy(settings->ntripServer_CasterUser, settingValue);
  else if (strcmp(settingName, "ntripServer_CasterUserPW") == 0)
    strcpy(settings->ntripServer_CasterUserPW, settingValue);
  else if (strcmp(settingName, "ntripServer_MountPoint") == 0)
    strcpy(settings->ntripServer_MountPoint, settingValue);
  else if (strcmp(settingName, "ntripServer_MountPointPW") == 0)
    strcpy(settings->ntripServer_MountPointPW, settingValue);
  else if (strcmp(settingName, "ntripServer_wifiSSID") == 0)
    strcpy(settings->ntripServer_wifiSSID, settingValue);
  else if (strcmp(settingName, "ntripServer_wifiPW") == 0)
    strcpy(settings->ntripServer_wifiPW, settingValue);
  else if (strcmp(settingName, "enableNtripClient") == 0)
    settings->enableNtripClient = d;
  else if (strcmp(settingName, "ntripClient_CasterHost") == 0)
    strcpy(settings->ntripClient_CasterHost, settingValue);
  else if (strcmp(settingName, "ntripClient_CasterPort") == 0)
    settings->ntripClient_CasterPort = d;
  else if (strcmp(settingName, "ntripClient_CasterUser") == 0)
    strcpy(settings->ntripClient_CasterUser, settingValue);
  else if (strcmp(settingName, "ntripClient_CasterUserPW") == 0)
    strcpy(settings->ntripClient_CasterUserPW, settingValue);
  else if (strcmp(settingName, "ntripClient_MountPoint") == 0)
    strcpy(settings->ntripClient_MountPoint, settingValue);
  else if (strcmp(settingName, "ntripClient_MountPointPW") == 0)
    strcpy(settings->ntripClient_MountPointPW, settingValue);
  else if (strcmp(settingName, "ntripClient_wifiSSID") == 0)
    strcpy(settings->ntripClient_wifiSSID, settingValue);
  else if (strcmp(settingName, "ntripClient_wifiPW") == 0)
    strcpy(settings->ntripClient_wifiPW, settingValue);
  else if (strcmp(settingName, "ntripClient_TransmitGGA") == 0)
    settings->ntripClient_TransmitGGA = d;
  else if (strcmp(settingName, "serialTimeoutGNSS") == 0)
    settings->serialTimeoutGNSS = d;
  else if (strcmp(settingName, "pointPerfectDeviceProfileToken") == 0)
    strcpy(settings->pointPerfectDeviceProfileToken, settingValue);
  else if (strcmp(settingName, "enablePointPerfectCorrections") == 0)
    settings->enablePointPerfectCorrections = d;
  else if (strcmp(settingName, "home_wifiSSID") == 0)
    strcpy(settings->home_wifiSSID, settingValue);
  else if (strcmp(settingName, "home_wifiPW") == 0)
    strcpy(settings->home_wifiPW, settingValue);
  else if (strcmp(settingName, "autoKeyRenewal") == 0)
    settings->autoKeyRenewal = d;
  else if (strcmp(settingName, "pointPerfectClientID") == 0)
    strcpy(settings->pointPerfectClientID, settingValue);
  else if (strcmp(settingName, "pointPerfectBrokerHost") == 0)
    strcpy(settings->pointPerfectBrokerHost, settingValue);
  else if (strcmp(settingName, "pointPerfectLBandTopic") == 0)
    strcpy(settings->pointPerfectLBandTopic, settingValue);

  else if (strcmp(settingName, "pointPerfectCurrentKey") == 0)
    strcpy(settings->pointPerfectCurrentKey, settingValue);
  else if (strcmp(settingName, "pointPerfectCurrentKeyDuration") == 0)
    settings->pointPerfectCurrentKeyDuration = d;
  else if (strcmp(settingName, "pointPerfectCurrentKeyStart") == 0)
    settings->pointPerfectCurrentKeyStart = d;

  else if (strcmp(settingName, "pointPerfectNextKey") == 0)
    strcpy(settings->pointPerfectNextKey, settingValue);
  else if (strcmp(settingName, "pointPerfectNextKeyDuration") == 0)
    settings->pointPerfectNextKeyDuration = d;
  else if (strcmp(settingName, "pointPerfectNextKeyStart") == 0)
    settings->pointPerfectNextKeyStart = d;

  else if (strcmp(settingName, "lastKeyAttempt") == 0)
    settings->lastKeyAttempt = d;
  else if (strcmp(settingName, "updateZEDSettings") == 0)
  {
    if (settings->updateZEDSettings != d)
      settings->updateZEDSettings = true; //If there is a discrepancy, push ZED reconfig
  }
  else if (strcmp(settingName, "LBandFreq") == 0)
    settings->LBandFreq = d;
  else if (strcmp(settingName, "timeZoneHours") == 0)
    settings->timeZoneHours = d;
  else if (strcmp(settingName, "timeZoneMinutes") == 0)
    settings->timeZoneMinutes = d;
  else if (strcmp(settingName, "timeZoneSeconds") == 0)
    settings->timeZoneSeconds = d;
  else if (strcmp(settingName, "enablePrintWifiIpAddress") == 0)
    settings->enablePrintWifiIpAddress = d;
  else if (strcmp(settingName, "enablePrintState") == 0)
    settings->enablePrintState = d;
  else if (strcmp(settingName, "enablePrintWifiState") == 0)
    settings->enablePrintWifiState = d;
  else if (strcmp(settingName, "enablePrintNtripClientState") == 0)
    settings->enablePrintNtripClientState = d;
  else if (strcmp(settingName, "enablePrintNtripServerState") == 0)
    settings->enablePrintNtripServerState = d;
  else if (strcmp(settingName, "enablePrintPosition") == 0)
    settings->enablePrintPosition = d;
  else if (strcmp(settingName, "enablePrintBatteryMessages") == 0)
    settings->enablePrintBatteryMessages = d;
  else if (strcmp(settingName, "enablePrintRoverAccuracy") == 0)
    settings->enablePrintRoverAccuracy = d;
  else if (strcmp(settingName, "enablePrintBadMessages") == 0)
    settings->enablePrintBadMessages = d;
  else if (strcmp(settingName, "enablePrintLogFileMessages") == 0)
    settings->enablePrintLogFileMessages = d;
  else if (strcmp(settingName, "enablePrintLogFileStatus") == 0)
    settings->enablePrintLogFileStatus = d;
  else if (strcmp(settingName, "enablePrintRingBufferOffsets") == 0)
    settings->enablePrintRingBufferOffsets = d;
  else if (strcmp(settingName, "enablePrintNtripServerRtcm") == 0)
    settings->enablePrintNtripServerRtcm = d;
  else if (strcmp(settingName, "enablePrintNtripClientRtcm") == 0)
    settings->enablePrintNtripClientRtcm = d;
  else if (strcmp(settingName, "enablePrintStates") == 0)
    settings->enablePrintStates = d;
  else if (strcmp(settingName, "enablePrintDuplicateStates") == 0)
    settings->enablePrintDuplicateStates = d;
  else if (strcmp(settingName, "radioType") == 0)
    settings->radioType = (RadioType_e)d;
  else if (strcmp(settingName, "espnowPeerCount") == 0)
    settings->espnowPeerCount = d;
  else if (strcmp(settingName, "enableRtcmMessageChecking") == 0)
    settings->enableRtcmMessageChecking = d;
  else if (strcmp(settingName, "radioType") == 0)
    settings->radioType = (RadioType_e)d;
  else if (strcmp(settingName, "bluetoothRadioType") == 0)
    settings->bluetoothRadioType = (BluetoothRadioType_e)d;
  else if (strcmp(settingName, "enableNmeaClient") == 0)
    settings->enableNmeaClient = d;
  else if (strcmp(settingName, "enableNmeaServer") == 0)
    settings->enableNmeaServer = d;
  else if (strcmp(settingName, "espnowBroadcast") == 0)
    settings->espnowBroadcast = d;
  else if (strcmp(settingName, "antennaHeight") == 0)
    settings->antennaHeight = d;
  else if (strcmp(settingName, "antennaReferencePoint") == 0)
    settings->antennaReferencePoint = d;

  //Check for bulk settings (constellations, message rates, ESPNOW Peers)
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
        sprintf(tempString, "constellation.%s", settings->ubxConstellations[x].textName);

        if (strcmp(settingName, tempString) == 0)
        {
          if (settings->ubxConstellations[x].enabled != d)
          {
            settings->ubxConstellations[x].enabled = d;
            settings->updateZEDSettings = true;
          }

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
        sprintf(tempString, "message.%s.msgRate", settings->ubxMessages[x].msgTextName);

        if (strcmp(settingName, tempString) == 0)
        {
          if (settings->ubxMessages[x].msgRate != d)
          {
            settings->ubxMessages[x].msgRate = d;
            settings->updateZEDSettings = true;
          }

          knownSetting = true;
          break;
        }
      }
    }

    //Scan for ESPNOW peers
#ifdef COMPILE_ESPNOW
    if (knownSetting == false)
    {
      for (int x = 0 ; x < ESPNOW_MAX_PEERS ; x++)
      {
        char tempString[50]; //espnowPeers.1=B4,C1,33,42,DE,01,
        sprintf(tempString, "espnowPeers.%d", x);

        if (strcmp(settingName, tempString) == 0)
        {
          uint8_t macAddress[6];
          uint8_t macByte = 0;

          char* token = strtok(settingValue, ","); //Break string up on ,
          while (token != NULL && macByte < sizeof(macAddress))
          {
            settings->espnowPeers[x][macByte++] = (uint8_t)strtol(token, NULL, 16);
            token = strtok(NULL, ",");
          }

          knownSetting = true;
          break;
        }
      }
    }
#endif //ifdef COMPILE_ESPNOW

    //Last catch
    if (knownSetting == false)
    {
      Serial.printf("Unknown setting %s\r\n", settingName);
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

// Check for extra characters in field or find minus sign.
char* skipSpace(char* str) {
  while (isspace(*str)) str++;
  return str;
}

//Load the special profileNumber file in LittleFS and return one byte value
void loadProfileNumber()
{
  if (profileNumber < MAX_PROFILE_COUNT) return; //Only load it once

  File fileProfileNumber = LittleFS.open("/profileNumber.txt", FILE_READ);
  if (!fileProfileNumber)
  {
    Serial.println("profileNumber.txt not found");
    settings.updateZEDSettings = true; //Force module update
    recordProfileNumber(0); //Record profile
  }
  else
  {
    profileNumber = fileProfileNumber.read();
    fileProfileNumber.close();
  }

  //We have arbitrary limit of user profiles
  if (profileNumber >= MAX_PROFILE_COUNT)
  {
    Serial.println("ProfileNumber invalid. Going to zero.");
    settings.updateZEDSettings = true; //Force module update
    recordProfileNumber(0); //Record profile
  }

  log_d("Using profile #%d", profileNumber);
}

//Record the given profile number as well as a config bool
void recordProfileNumber(uint8_t newProfileNumber)
{
  profileNumber = newProfileNumber;
  File fileProfileNumber = LittleFS.open("/profileNumber.txt", FILE_WRITE);
  if (!fileProfileNumber)
  {
    log_d("profileNumber.txt failed to open");
    return;
  }
  fileProfileNumber.write(newProfileNumber);
  fileProfileNumber.close();
}

//Populate profileNames[][] based on names found in LittleFS and SD
//If both SD and LittleFS contain a profile, SD wins.
uint8_t loadProfileNames()
{
  int profiles = 0;

  for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
    profileNames[x][0] = '\0'; //Ensure every profile name is terminated

  //Check LittleFS and SD for profile names
  for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
  {
    char fileName[56];
    sprintf(fileName, "/%s_Settings_%d.txt", platformFilePrefix, x);

    if (getProfileName(fileName, profileNames[x], sizeof(profileNames[x])) == true)
      //Mark this profile as active
      profiles |= 1 << x;
  }

  return (profiles);
}

//Copy the profile name into the array of profile names
void setProfileName(uint8_t ProfileNumber)
{
  //Update the name in the array of profile names
  strcpy(profileNames[profileNumber], settings.profileName);

  //Mark this profile as active
  activeProfiles |= 1 << ProfileNumber;
}

//Open the clear text file, scan for 'profileName' and return the string
//Returns true if successfully found tag in file, length may be zero
//Looks at LittleFS first, then SD
bool getProfileName(char *fileName, char *profileName, uint8_t profileNameLength)
{
  //Create a temporary settings struc on the heap (not the stack because it is ~4500 bytes)
  Settings *tempSettings = new Settings;

  //If we have a profile in both LFS and SD, SD wins
  bool responseLFS = loadSystemSettingsFromFileLFS(fileName, tempSettings);
  bool responseSD = loadSystemSettingsFromFileSD(fileName, tempSettings);

  //Zero terminate the profile name
  *profileName = 0;
  if (responseLFS == true || responseSD == true)
    snprintf(profileName, profileNameLength, "%s", tempSettings->profileName); //snprintf handles null terminator

  delete tempSettings;

  return (responseLFS | responseSD);
}

//Loads a given profile name.
//Profiles may not be sequential (user might have empty profile #2, but filled #3) so we load the profile unit, not the number
//Return true if successful
bool getProfileNameFromUnit(uint8_t profileUnit, char *profileName, uint8_t profileNameLength)
{
  uint8_t located = 0;

  //Step through possible profiles looking for the specified unit
  for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
  {
    if (activeProfiles & (1 << x))
    {
      if (located == profileUnit)
      {
        snprintf(profileName, profileNameLength, "%s", profileNames[x]); //snprintf handles null terminator
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
    if (activeProfiles & (1 << x))
    {
      if (located == profileUnit)
        return (x);

      located++; //Valid settingFileName but not the unit we are looking for
    }
  }
  log_d("Profile unit %d not found", profileUnit);

  return (0);
}

//Record large character blob to file
void recordFile(const char* fileID, char* fileContents, uint32_t fileSize)
{
  char fileName[80];
  sprintf(fileName, "/%s_%s_%d.txt", platformFilePrefix, fileID, profileNumber);

  if (LittleFS.exists(fileName))
  {
    LittleFS.remove(fileName);
    log_d("Removing LittleFS: %s", fileName);
  }

  File fileToWrite = LittleFS.open(fileName, FILE_WRITE);
  if (!fileToWrite)
  {
    log_d("Failed to write to file %s", fileName);
  }
  else
  {
    fileToWrite.write((uint8_t*)fileContents, fileSize); //Store cert into file
    fileToWrite.close();
    log_d("File recorded to LittleFS: %s", fileName);
  }
}

void loadFile(const char* fileID, char* fileContents)
{
  char fileName[80];
  sprintf(fileName, "/%s_%s_%d.txt", platformFilePrefix, fileID, profileNumber);

  File fileToRead = LittleFS.open(fileName, FILE_READ);
  if (fileToRead)
  {
    fileToRead.read((uint8_t*)fileContents, fileToRead.size()); //Read contents into pointer
    fileToRead.close();
    log_d("File loaded from LittleFS: %s", fileName);
  }
  else
  {
    log_d("Failed to read from LittleFS: %s", fileName);
  }
}
