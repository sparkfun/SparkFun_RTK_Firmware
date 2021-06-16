void loadSettings()
{
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

//Record the current settings struct to EEPROM and then to config file
void recordSystemSettings()
{
  settings.sizeOfSettings = sizeof(settings);
  if (settings.sizeOfSettings > EEPROM_SIZE)
  {
    while (1) //Hard freeze
    {
      Serial.printf("Size of settings is %d bytes\n\r", sizeof(settings));
      Serial.println(F("Increase the EEPROM footprint!"));
      delay(1000);
    }
  }

  EEPROM.put(0, settings);
  EEPROM.commit();
  delay(1); //Give CPU time to pet WDT
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
      settingsFile.println("enableSBAS=" + (String)settings.enableSBAS);
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
      settingsFile.println("dataPortChannel=" + (String)settings.dataPortChannel);
      settingsFile.println("spiFrequency=" + (String)settings.spiFrequency);
      settingsFile.println("sppRxQueueSize=" + (String)settings.sppRxQueueSize);
      settingsFile.println("sppTxQueueSize=" + (String)settings.sppTxQueueSize);
      settingsFile.println("dynamicModel=" + (String)settings.dynamicModel);

      //NMEA
      settingsFile.println("message.nmea_dtm.msgRate=" + (String)settings.message.nmea_dtm.msgRate);
      settingsFile.println("message.nmea_gbs.msgRate=" + (String)settings.message.nmea_gbs.msgRate);
      settingsFile.println("message.nmea_gga.msgRate=" + (String)settings.message.nmea_gga.msgRate);
      settingsFile.println("message.nmea_gll.msgRate=" + (String)settings.message.nmea_gll.msgRate);
      settingsFile.println("message.nmea_gns.msgRate=" + (String)settings.message.nmea_gns.msgRate);

      settingsFile.println("message.nmea_grs.msgRate=" + (String)settings.message.nmea_grs.msgRate);
      settingsFile.println("message.nmea_gsa.msgRate=" + (String)settings.message.nmea_gsa.msgRate);
      settingsFile.println("message.nmea_gst.msgRate=" + (String)settings.message.nmea_gst.msgRate);
      settingsFile.println("message.nmea_gsv.msgRate=" + (String)settings.message.nmea_gsv.msgRate);
      settingsFile.println("message.nmea_rmc.msgRate=" + (String)settings.message.nmea_rmc.msgRate);

      settingsFile.println("message.nmea_vlw.msgRate=" + (String)settings.message.nmea_vlw.msgRate);
      settingsFile.println("message.nmea_vtg.msgRate=" + (String)settings.message.nmea_vtg.msgRate);
      settingsFile.println("message.nmea_zda.msgRate=" + (String)settings.message.nmea_zda.msgRate);

      //NAV
      settingsFile.println("message.nav_clock.msgRate=" + (String)settings.message.nav_clock.msgRate);
      settingsFile.println("message.nav_dop.msgRate=" + (String)settings.message.nav_dop.msgRate);
      settingsFile.println("message.nav_eoe.msgRate=" + (String)settings.message.nav_eoe.msgRate);
      settingsFile.println("message.nav_geofence.msgRate=" + (String)settings.message.nav_geofence.msgRate);
      settingsFile.println("message.nav_hpposecef.msgRate=" + (String)settings.message.nav_hpposecef.msgRate);

      settingsFile.println("message.nav_hpposllh.msgRate=" + (String)settings.message.nav_hpposllh.msgRate);
      settingsFile.println("message.nav_odo.msgRate=" + (String)settings.message.nav_odo.msgRate);
      settingsFile.println("message.nav_orb.msgRate=" + (String)settings.message.nav_orb.msgRate);
      settingsFile.println("message.nav_posecef.msgRate=" + (String)settings.message.nav_posecef.msgRate);
      settingsFile.println("message.nav_posllh.msgRate=" + (String)settings.message.nav_posllh.msgRate);

      settingsFile.println("message.nav_pvt.msgRate=" + (String)settings.message.nav_pvt.msgRate);
      settingsFile.println("message.nav_relposned.msgRate=" + (String)settings.message.nav_relposned.msgRate);
      settingsFile.println("message.nav_sat.msgRate=" + (String)settings.message.nav_sat.msgRate);
      settingsFile.println("message.nav_sig.msgRate=" + (String)settings.message.nav_sig.msgRate);
      settingsFile.println("message.nav_status.msgRate=" + (String)settings.message.nav_status.msgRate);

      settingsFile.println("message.nav_svin.msgRate=" + (String)settings.message.nav_svin.msgRate);
      settingsFile.println("message.nav_timebds.msgRate=" + (String)settings.message.nav_timebds.msgRate);
      settingsFile.println("message.nav_timegal.msgRate=" + (String)settings.message.nav_timegal.msgRate);
      settingsFile.println("message.nav_timeglo.msgRate=" + (String)settings.message.nav_timeglo.msgRate);
      settingsFile.println("message.nav_timegps.msgRate=" + (String)settings.message.nav_timegps.msgRate);

      settingsFile.println("message.nav_timels.msgRate=" + (String)settings.message.nav_timels.msgRate);
      settingsFile.println("message.nav_timeutc.msgRate=" + (String)settings.message.nav_timeutc.msgRate);
      settingsFile.println("message.nav_velecef.msgRate=" + (String)settings.message.nav_velecef.msgRate);
      settingsFile.println("message.nav_velned.msgRate=" + (String)settings.message.nav_velned.msgRate);

      //RXM
      settingsFile.println("message.rxm_measx.msgRate=" + (String)settings.message.rxm_measx.msgRate);
      settingsFile.println("message.rxm_rawx.msgRate=" + (String)settings.message.rxm_rawx.msgRate);
      settingsFile.println("message.rxm_rlm.msgRate=" + (String)settings.message.rxm_rlm.msgRate);
      settingsFile.println("message.rxm_rtcm.msgRate=" + (String)settings.message.rxm_rtcm.msgRate);
      settingsFile.println("message.rxm_sfrbx.msgRate=" + (String)settings.message.rxm_sfrbx.msgRate);

      //MON
      settingsFile.println("message.mon_comms.msgRate=" + (String)settings.message.mon_comms.msgRate);
      settingsFile.println("message.mon_hw2.msgRate=" + (String)settings.message.mon_hw2.msgRate);
      settingsFile.println("message.mon_hw3.msgRate=" + (String)settings.message.mon_hw3.msgRate);
      settingsFile.println("message.mon_hw.msgRate=" + (String)settings.message.mon_hw.msgRate);
      settingsFile.println("message.mon_io.msgRate=" + (String)settings.message.mon_io.msgRate);

      settingsFile.println("message.mon_msgpp.msgRate=" + (String)settings.message.mon_msgpp.msgRate);
      settingsFile.println("message.mon_rf.msgRate=" + (String)settings.message.mon_rf.msgRate);
      settingsFile.println("message.mon_rxbuf.msgRate=" + (String)settings.message.mon_rxbuf.msgRate);
      settingsFile.println("message.mon_rxr.msgRate=" + (String)settings.message.mon_rxr.msgRate);
      settingsFile.println("message.mon_txbuf.msgRate=" + (String)settings.message.mon_txbuf.msgRate);

      //TIM
      settingsFile.println("message.tim_tm2.msgRate=" + (String)settings.message.tim_tm2.msgRate);
      settingsFile.println("message.tim_tp.msgRate=" + (String)settings.message.tim_tp.msgRate);
      settingsFile.println("message.tim_vrfy.msgRate=" + (String)settings.message.tim_vrfy.msgRate);

      //RTCM
      settingsFile.println("message.rtcm_1005.msgRate=" + (String)settings.message.rtcm_1005.msgRate);
      settingsFile.println("message.rtcm_1074.msgRate=" + (String)settings.message.rtcm_1074.msgRate);
      settingsFile.println("message.rtcm_1077.msgRate=" + (String)settings.message.rtcm_1077.msgRate);
      settingsFile.println("message.rtcm_1084.msgRate=" + (String)settings.message.rtcm_1084.msgRate);
      settingsFile.println("message.rtcm_1087.msgRate=" + (String)settings.message.rtcm_1087.msgRate);

      settingsFile.println("message.rtcm_1094.msgRate=" + (String)settings.message.rtcm_1094.msgRate);
      settingsFile.println("message.rtcm_1097.msgRate=" + (String)settings.message.rtcm_1097.msgRate);
      settingsFile.println("message.rtcm_1124.msgRate=" + (String)settings.message.rtcm_1124.msgRate);
      settingsFile.println("message.rtcm_1127.msgRate=" + (String)settings.message.rtcm_1127.msgRate);
      settingsFile.println("message.rtcm_1230.msgRate=" + (String)settings.message.rtcm_1230.msgRate);

      settingsFile.println("message.rtcm_4072_0.msgRate=" + (String)settings.message.rtcm_4072_0.msgRate);
      settingsFile.println("message.rtcm_4072_1.msgRate=" + (String)settings.message.rtcm_4072_1.msgRate);

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

      Serial.printf("RTK %s has been factory reset via settings file. Freezing. Please restart and open terminal at 115200bps.\n\r", platformPrefix);

      while (1)
        delay(1); //Prevent CPU freakout
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
  else if (strcmp(settingName, "enableSBAS") == 0)
    settings.enableSBAS = d;
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

  //NMEA
  else if (strcmp(settingName, "message.nmea_dtm.msgRate") == 0)
    settings.message.nmea_dtm.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gbs.msgRate") == 0)
    settings.message.nmea_gbs.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gga.msgRate") == 0)
    settings.message.nmea_gga.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gll.msgRate") == 0)
    settings.message.nmea_gll.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gns.msgRate") == 0)
    settings.message.nmea_gns.msgRate = d;

  else if (strcmp(settingName, "message.nmea_grs.msgRate") == 0)
    settings.message.nmea_grs.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gsa.msgRate") == 0)
    settings.message.nmea_gsa.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gst.msgRate") == 0)
    settings.message.nmea_gst.msgRate = d;
  else if (strcmp(settingName, "message.nmea_gsv.msgRate") == 0)
    settings.message.nmea_gsv.msgRate = d;
  else if (strcmp(settingName, "message.nmea_rmc.msgRate") == 0)
    settings.message.nmea_rmc.msgRate = d;

  else if (strcmp(settingName, "message.nmea_vlw.msgRate") == 0)
    settings.message.nmea_vlw.msgRate = d;
  else if (strcmp(settingName, "message.nmea_vtg.msgRate") == 0)
    settings.message.nmea_vtg.msgRate = d;
  else if (strcmp(settingName, "message.nmea_zda.msgRate") == 0)
    settings.message.nmea_zda.msgRate = d;

  //NAV
  else if (strcmp(settingName, "message.nav_clock.msgRate") == 0)
    settings.message.nav_clock.msgRate = d;
  else if (strcmp(settingName, "message.nav_dop.msgRate") == 0)
    settings.message.nav_dop.msgRate = d;
  else if (strcmp(settingName, "message.nav_eoe.msgRate") == 0)
    settings.message.nav_eoe.msgRate = d;
  else if (strcmp(settingName, "message.nav_geofence.msgRate") == 0)
    settings.message.nav_geofence.msgRate = d;
  else if (strcmp(settingName, "message.nav_hpposecef.msgRate") == 0)
    settings.message.nav_hpposecef.msgRate = d;

  else if (strcmp(settingName, "message.nav_hpposllh.msgRate") == 0)
    settings.message.nav_hpposllh.msgRate = d;
  else if (strcmp(settingName, "message.nav_odo.msgRate") == 0)
    settings.message.nav_odo.msgRate = d;
  else if (strcmp(settingName, "message.nav_orb.msgRate") == 0)
    settings.message.nav_orb.msgRate = d;
  else if (strcmp(settingName, "message.nav_posecef.msgRate") == 0)
    settings.message.nav_posecef.msgRate = d;
  else if (strcmp(settingName, "message.nav_posllh.msgRate") == 0)
    settings.message.nav_posllh.msgRate = d;

  else if (strcmp(settingName, "message.nav_pvt.msgRate") == 0)
    settings.message.nav_pvt.msgRate = d;
  else if (strcmp(settingName, "message.nav_relposned.msgRate") == 0)
    settings.message.nav_relposned.msgRate = d;
  else if (strcmp(settingName, "message.nav_sat.msgRate") == 0)
    settings.message.nav_sat.msgRate = d;
  else if (strcmp(settingName, "message.nav_sig.msgRate") == 0)
    settings.message.nav_sig.msgRate = d;
  else if (strcmp(settingName, "message.nav_status.msgRate") == 0)
    settings.message.nav_status.msgRate = d;

  else if (strcmp(settingName, "message.nav_svin.msgRate") == 0)
    settings.message.nav_svin.msgRate = d;
  else if (strcmp(settingName, "message.nav_timebds.msgRate") == 0)
    settings.message.nav_timebds.msgRate = d;
  else if (strcmp(settingName, "message.nav_timegal.msgRate") == 0)
    settings.message.nav_timegal.msgRate = d;
  else if (strcmp(settingName, "message.nav_timeglo.msgRate") == 0)
    settings.message.nav_timeglo.msgRate = d;
  else if (strcmp(settingName, "message.nav_timegps.msgRate") == 0)
    settings.message.nav_timegps.msgRate = d;

  else if (strcmp(settingName, "message.nav_timels.msgRate") == 0)
    settings.message.nav_timels.msgRate = d;
  else if (strcmp(settingName, "message.nav_timeutc.msgRate") == 0)
    settings.message.nav_timeutc.msgRate = d;
  else if (strcmp(settingName, "message.nav_velecef.msgRate") == 0)
    settings.message.nav_velecef.msgRate = d;
  else if (strcmp(settingName, "message.nav_velned.msgRate") == 0)
    settings.message.nav_velned.msgRate = d;

  //RXM
  else if (strcmp(settingName, "message.rxm_measx.msgRate") == 0)
    settings.message.rxm_measx.msgRate = d;
  else if (strcmp(settingName, "message.rxm_rawx.msgRate") == 0)
    settings.message.rxm_rawx.msgRate = d;
  else if (strcmp(settingName, "message.rxm_rlm.msgRate") == 0)
    settings.message.rxm_rlm.msgRate = d;
  else if (strcmp(settingName, "message.rxm_rtcm.msgRate") == 0)
    settings.message.rxm_rtcm.msgRate = d;
  else if (strcmp(settingName, "message.rxm_sfrbx.msgRate") == 0)
    settings.message.rxm_sfrbx.msgRate = d;

  //MON
  else if (strcmp(settingName, "message.mon_comms.msgRate") == 0)
    settings.message.mon_comms.msgRate = d;
  else if (strcmp(settingName, "message.mon_hw2.msgRate") == 0)
    settings.message.mon_hw2.msgRate = d;
  else if (strcmp(settingName, "message.mon_hw3.msgRate") == 0)
    settings.message.mon_hw3.msgRate = d;
  else if (strcmp(settingName, "message.mon_hw.msgRate") == 0)
    settings.message.mon_hw.msgRate = d;
  else if (strcmp(settingName, "message.mon_io.msgRate") == 0)
    settings.message.mon_io.msgRate = d;

  else if (strcmp(settingName, "message.mon_msgpp.msgRate") == 0)
    settings.message.mon_msgpp.msgRate = d;
  else if (strcmp(settingName, "message.mon_rf.msgRate") == 0)
    settings.message.mon_rf.msgRate = d;
  else if (strcmp(settingName, "message.mon_rxbuf.msgRate") == 0)
    settings.message.mon_rxbuf.msgRate = d;
  else if (strcmp(settingName, "message.mon_rxr.msgRate") == 0)
    settings.message.mon_rxr.msgRate = d;
  else if (strcmp(settingName, "message.mon_txbuf.msgRate") == 0)
    settings.message.mon_txbuf.msgRate = d;

  //TIM
  else if (strcmp(settingName, "message.tim_tm2.msgRate") == 0)
    settings.message.tim_tm2.msgRate = d;
  else if (strcmp(settingName, "message.tim_tp.msgRate") == 0)
    settings.message.tim_tp.msgRate = d;
  else if (strcmp(settingName, "message.tim_vrfy.msgRate") == 0)
    settings.message.tim_vrfy.msgRate = d;

  //RTCM
  else if (strcmp(settingName, "message.rtcm_1005.msgRate") == 0)
    settings.message.rtcm_1005.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1074.msgRate") == 0)
    settings.message.rtcm_1074.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1077.msgRate") == 0)
    settings.message.rtcm_1077.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1084.msgRate") == 0)
    settings.message.rtcm_1084.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1087.msgRate") == 0)
    settings.message.rtcm_1087.msgRate = d;

  else if (strcmp(settingName, "message.rtcm_1094.msgRate") == 0)
    settings.message.rtcm_1094.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1097.msgRate") == 0)
    settings.message.rtcm_1097.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1124.msgRate") == 0)
    settings.message.rtcm_1124.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1127.msgRate") == 0)
    settings.message.rtcm_1127.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_1230.msgRate") == 0)
    settings.message.rtcm_1230.msgRate = d;

  else if (strcmp(settingName, "message.rtcm_4072_0.msgRate") == 0)
    settings.message.rtcm_4072_0.msgRate = d;
  else if (strcmp(settingName, "message.rtcm_4072_1.msgRate") == 0)
    settings.message.rtcm_4072_1.msgRate = d;

  else
    Serial.printf("Unknown setting %s on line: %s\r\n", settingName, str);

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
