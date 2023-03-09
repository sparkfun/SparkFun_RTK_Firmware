//Configure the survey in settings (time and 3D dev max)
//Set the ECEF coordinates for a known location
void menuBase()
{
  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: Base");

    //Print the combined HAE APC if we are in the given mode
    if (settings.fixedBase == true && settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC)
    {
      systemPrintf("Total Height Above Ellipsoid - Antenna Phase Center (HAE APC): %0.3fmm\r\n",
                   (((settings.fixedAltitude * 1000) + settings.antennaHeight + settings.antennaReferencePoint) / 1000)
                  );
    }

    systemPrint("1) Toggle Base Mode: ");
    if (settings.fixedBase == true) systemPrintln("Fixed/Static Position");
    else systemPrintln("Use Survey-In");

    if (settings.fixedBase == true)
    {
      systemPrint("2) Toggle Coordinate System: ");
      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF) systemPrintln("ECEF");
      else systemPrintln("Geodetic");

      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
      {
        systemPrint("3) Set ECEF X/Y/Z coordinates: ");
        systemPrint(settings.fixedEcefX, 4);
        systemPrint("m, ");
        systemPrint(settings.fixedEcefY, 4);
        systemPrint("m, ");
        systemPrint(settings.fixedEcefZ, 4);
        systemPrintln("m");
      }
      else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC)
      {
        systemPrint("3) Set Lat/Long/Altitude coordinates: ");
        systemPrint(settings.fixedLat, haeNumberOfDecimals);
        systemWrite(167); //°
        systemPrint(", ");
        systemPrint(settings.fixedLong, haeNumberOfDecimals);
        systemWrite(167); //°
        systemPrint(", ");
        systemPrint(settings.fixedAltitude, 4);
        systemPrintln("m");

        systemPrintf("4) Set Antenna Height: %dmm\r\n", settings.antennaHeight);

        systemPrintf("5) Set Antenna Reference Point: %0.1fmm\r\n", settings.antennaReferencePoint);
      }
    }
    else
    {
      systemPrint("2) Set minimum observation time: ");
      systemPrint(settings.observationSeconds);
      systemPrintln(" seconds");

      systemPrint("3) Set required Mean 3D Standard Deviation: ");
      systemPrint(settings.observationPositionAccuracy, 3);
      systemPrintln(" meters");

      systemPrint("4) Set required initial positional accuracy before Survey-In: ");
      systemPrint(settings.surveyInStartingAccuracy, 3);
      systemPrintln(" meters");
    }

    systemPrint("6) Toggle NTRIP Server: ");
    if (settings.enableNtripServer == true) systemPrintln("Enabled");
    else systemPrintln("Disabled");

    if (settings.enableNtripServer == true)
    {
      systemPrint("7) Set Caster Address: ");
      systemPrintln(settings.ntripServer_CasterHost);

      systemPrint("8) Set Caster Port: ");
      systemPrintln(settings.ntripServer_CasterPort);

      systemPrint("9) Set Mountpoint: ");
      systemPrintln(settings.ntripServer_MountPoint);

      systemPrint("10) Set Mountpoint PW: ");
      systemPrintln(settings.ntripServer_MountPointPW);

      //Because we may be in rover mode, do not get freq from module, use settings instead
      float measurementFrequency = (1000.0 / settings.measurementRateBase) / settings.navigationRateBase;

      systemPrint("11) Set base measurement rate in Hz: ");
      systemPrintln(measurementFrequency, 5);

      systemPrint("12) Set base measurement rate in seconds between measurements: ");
      systemPrintln(1 / measurementFrequency, 5);

      if (settings.fixedBase == false) //Survey-in
      {
        systemPrint("13) Select survey-in radio: ");
        systemPrintf("%s\r\n", settings.ntripServer_StartAtSurveyIn ? "WiFi" : "Bluetooth");

      }
    }
    else
    {
      //Because we may be in rover mode, do not get freq from module, use settings instead
      float measurementFrequency = (1000.0 / settings.measurementRateBase) / settings.navigationRateBase;

      systemPrint("7) Set base measurement rate in Hz: ");
      systemPrintln(measurementFrequency, 5);

      systemPrint("8) Set base measurement rate in seconds between measurements: ");
      systemPrintln(1 / measurementFrequency, 5);

      if (settings.fixedBase == false)  //Survey-in
      {
        systemPrint("9) Select survey-in radio: ");
        systemPrintf("%s\r\n", settings.ntripServer_StartAtSurveyIn ? "WiFi" : "Bluetooth");
      }

    }


    systemPrintln("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      settings.fixedBase ^= 1;
      restartBase = true;
    }
    else if (settings.fixedBase == true && incoming == 2)
    {
      settings.fixedBaseCoordinateType ^= 1;
      restartBase = true;
    }

    else if (settings.fixedBase == true && incoming == 3)
    {
      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
      {
        systemPrintln("Enter the fixed ECEF coordinates that will be used in Base mode:");

        systemPrint("ECEF X in meters (ex: -1280182.9200): ");
        double fixedEcefX = getDouble();

        //Progress with additional prompts only if the user enters valid data
        if (fixedEcefX != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedEcefX != INPUT_RESPONSE_GETNUMBER_EXIT)
        {
          settings.fixedEcefX = fixedEcefX;

          systemPrint("\nECEF Y in meters (ex: -4716808.5807): ");
          double fixedEcefY = getDouble();
          if (fixedEcefY != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedEcefY != INPUT_RESPONSE_GETNUMBER_EXIT)
          {
            settings.fixedEcefY = fixedEcefY;

            systemPrint("\nECEF Z in meters (ex: 4086669.6393): ");
            double fixedEcefZ = getDouble();
            if (fixedEcefZ != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedEcefZ != INPUT_RESPONSE_GETNUMBER_EXIT)
              settings.fixedEcefZ = fixedEcefZ;
          }
        }
      }
      else  if (settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC)
      {
        systemPrintln("Enter the fixed Lat/Long/Altitude coordinates that will be used in Base mode:");

        systemPrint("Lat in degrees (ex: 40.090335429): ");
        double fixedLat = getDouble();

        //Progress with additional prompts only if the user enters valid data
        if (fixedLat != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedLat != INPUT_RESPONSE_GETNUMBER_EXIT)
        {
          settings.fixedLat = fixedLat;

          systemPrint("\nLong in degrees (ex: -105.184774720): ");
          double fixedLong = getDouble();
          if (fixedLong != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedLong != INPUT_RESPONSE_GETNUMBER_EXIT)
          {
            settings.fixedLong = fixedLong;

            systemPrint("\nAltitude in meters (ex: 1560.2284): ");
            double fixedAltitude = getDouble();
            if (fixedAltitude != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedAltitude != INPUT_RESPONSE_GETNUMBER_EXIT)
              settings.fixedAltitude = fixedAltitude;
          }
        }
      }
    }
    else if (settings.fixedBase == true && settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC && incoming == 4)
    {
      systemPrint("Enter the antenna height (a.k.a. pole length) in millimeters (-15000 to 15000mm): ");
      int antennaHeight = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((antennaHeight != INPUT_RESPONSE_GETNUMBER_EXIT) && (antennaHeight != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (antennaHeight < -15000 || antennaHeight > 15000) //Arbitrary 15m max
          systemPrintln("Error: Antenna Height out of range");
        else
          settings.antennaHeight = antennaHeight; //Recorded to NVM and file at main menu exit
      }
    }
    else if (settings.fixedBase == true && settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC && incoming == 5)
    {
      systemPrint("Enter the antenna reference point (a.k.a. ARP) in millimeters (-200.0 to 200.0mm): ");
      float antennaReferencePoint = getDouble();
      if (antennaReferencePoint < -200.0 || antennaReferencePoint > 200.0) //Arbitrary 200mm max
        systemPrintln("Error: Antenna Reference Point out of range");
      else
        settings.antennaReferencePoint = antennaReferencePoint; //Recorded to NVM and file at main menu exit
    }

    else if (settings.fixedBase == false && incoming == 2)
    {
      systemPrint("Enter the number of seconds for survey-in obseration time (60 to 600s): ");
      int observationSeconds = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((observationSeconds != INPUT_RESPONSE_GETNUMBER_EXIT) && (observationSeconds != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (observationSeconds < 60 || observationSeconds > 60 * 10) //Arbitrary 10 minute limit
          systemPrintln("Error: Observation seconds out of range");
        else
          settings.observationSeconds = observationSeconds; //Recorded to NVM and file at main menu exit
      }
    }
    else if (settings.fixedBase == false && incoming == 3)
    {
      systemPrint("Enter the number of meters for survey-in required position accuracy (1.0 to 5.0m): ");
      float observationPositionAccuracy = getDouble();

      float maxObservationPositionAccuracy = 5.0;
      if (ENABLE_DEVELOPER)
        maxObservationPositionAccuracy = 10.0;

      if (observationPositionAccuracy < 1.0 || observationPositionAccuracy > maxObservationPositionAccuracy) //Arbitrary 1m minimum
        systemPrintln("Error: Observation positional accuracy requirement out of range");
      else
        settings.observationPositionAccuracy = observationPositionAccuracy; //Recorded to NVM and file at main menu exit
    }
    else if (settings.fixedBase == false && incoming == 4)
    {
      systemPrint("Enter the positional accuracy required before Survey-In begins (0.1 to 5.0m): ");
      float surveyInStartingAccuracy = getDouble();
      if (surveyInStartingAccuracy < 0.1 || surveyInStartingAccuracy > 5.0) //Arbitrary 0.1m minimum
        systemPrintln("Error: Starting accuracy out of range");
      else
        settings.surveyInStartingAccuracy = surveyInStartingAccuracy; //Recorded to NVM and file at main menu exit
    }

    else if (incoming == 6)
    {
      settings.enableNtripServer ^= 1;
      restartBase = true;
    }
    else if (incoming == 7 && settings.enableNtripServer == true)
    {
      systemPrint("Enter new Caster Address: ");
      getString(settings.ntripServer_CasterHost, sizeof(settings.ntripServer_CasterHost));
      restartBase = true;
    }
    else if (incoming == 8 && settings.enableNtripServer == true)
    {
      systemPrint("Enter new Caster Port: ");

      int ntripServer_CasterPort = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((ntripServer_CasterPort != INPUT_RESPONSE_GETNUMBER_EXIT) && (ntripServer_CasterPort != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (ntripServer_CasterPort < 1 || ntripServer_CasterPort > 99999) //Arbitrary 99k max port #
          systemPrintln("Error: Caster port out of range");
        else
          settings.ntripServer_CasterPort = ntripServer_CasterPort; //Recorded to NVM and file at main menu exit
        restartBase = true;
      }
    }
    else if (incoming == 9 && settings.enableNtripServer == true)
    {
      systemPrint("Enter new Mount Point: ");
      getString(settings.ntripServer_MountPoint, sizeof(settings.ntripServer_MountPoint));
      restartBase = true;
    }
    else if (incoming == 10 && settings.enableNtripServer == true)
    {
      systemPrintf("Enter password for Mount Point %s: ", settings.ntripServer_MountPoint);
      getString(settings.ntripServer_MountPointPW, sizeof(settings.ntripServer_MountPointPW));
      restartBase = true;
    }
    else if ( ((settings.enableNtripServer == true) && (incoming == 11))
              || ((settings.enableNtripServer == false) && (incoming == 7))
            )
    {
      systemPrint("Enter GNSS measurement rate in Hz during base mode: ");
      double rate = getDouble();
      if (rate < 0.00012 || rate > 20.0) //20Hz limit with all constellations enabled
      {
        systemPrintln("Error: Measurement rate out of range");
      }
      else
      {
        //Do not set the rate here because we may be in a non-base mode
        restartBase = true;
      }
    }
    else if ( ((settings.enableNtripServer == true) && (incoming == 12))
              || ((settings.enableNtripServer == false) && (incoming == 8))
            )
    {
      systemPrint("Enter GNSS measurement rate in seconds between measurements during base mode: ");
      float rate = getDouble();
      if (rate < 0.0 || rate > 8255.0) //Limit of 127 (navRate) * 65000ms (measRate) = 137 minute limit.
      {
        systemPrintln("Error: Measurement rate out of range");
      }
      else
      {
        //Do not set the rate here because we may be in a non-base mode
        restartBase = true;
      }
    }
    else if ( ((settings.enableNtripServer == true) && (settings.fixedBase == false) && (incoming == 13))
              || ((settings.enableNtripServer == false) && (settings.fixedBase == false) && (incoming == 9))
            )
    {
      settings.ntripServer_StartAtSurveyIn ^= 1;
      restartBase = true;
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

//Configure ESF settings
void menuSensorFusion()
{
  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: Sensor Fusion");

    systemPrint("Fusion Mode: ");
    systemPrint(theGNSS.packetUBXESFSTATUS->data.fusionMode);
    systemPrint(" - ");
    if (theGNSS.packetUBXESFSTATUS->data.fusionMode == 0)
      systemPrintln("Initializing");
    else if (theGNSS.packetUBXESFSTATUS->data.fusionMode == 1)
      systemPrintln("Calibrated");
    else if (theGNSS.packetUBXESFSTATUS->data.fusionMode == 2)
      systemPrintln("Suspended");
    else if (theGNSS.packetUBXESFSTATUS->data.fusionMode == 3)
      systemPrintln("Disabled");

    if (settings.enableSensorFusion == true && settings.dynamicModel != DYN_MODEL_AUTOMOTIVE)
      systemPrintln("Warning: Dynamic Model not set to Automotive. Sensor Fusion is best used with the Automotive Dynamic Model.");

    systemPrint("1) Toggle Sensor Fusion: ");
    if (settings.enableSensorFusion == true) systemPrintln("Enabled");
    else systemPrintln("Disabled");

    systemPrint("2) Toggle Automatic IMU-mount Alignment: ");
    if (settings.autoIMUmountAlignment == true) systemPrintln("Enabled");
    else systemPrintln("Disabled");

    systemPrintln("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      settings.enableSensorFusion ^= 1;
      setSensorFusion(settings.enableSensorFusion); //Enable/disable sensor fusion
    }
    else if (incoming == 2)
    {
      settings.autoIMUmountAlignment ^= 1;
    }

    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  theGNSS.setVal8(UBLOX_CFG_SFCORE_USE_SF, settings.enableSensorFusion); //Enable/disable sensor fusion
  theGNSS.setVal8(UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA, settings.autoIMUmountAlignment); //Enable/disable Automatic IMU-mount Alignment

  clearBuffer(); //Empty buffer of any newline chars
}

//Enable or disable sensor fusion using keys
void setSensorFusion(bool enable)
{
  if (getSensorFusion() != enable)
    theGNSS.setVal8(UBLOX_CFG_SFCORE_USE_SF, enable, VAL_LAYER_ALL);
}

bool getSensorFusion()
{
  return (theGNSS.getVal8(UBLOX_CFG_SFCORE_USE_SF, VAL_LAYER_RAM, 1200));
}

//Open the given file and load a given line to the given pointer
bool getFileLineLFS(const char* fileName, int lineToFind, char* lineData, int lineDataLength)
{
  File file = LittleFS.open(fileName, FILE_READ);
  if (!file)
  {
    log_d("File %s not found", fileName);
    return (false);
  }

  //We cannot be sure how the user will terminate their files so we avoid the use of readStringUntil
  int lineNumber = 0;
  int x = 0;
  bool lineFound = false;

  while (file.available())
  {
    byte incoming = file.read();
    if (incoming == '\r' || incoming == '\n')
    {
      lineData[x] = '\0'; //Terminate

      if (lineNumber == lineToFind)
      {
        lineFound = true; //We found the line. We're done!
        break;
      }

      //Sometimes a line has multiple terminators
      while (file.peek() == '\r' || file.peek() == '\n')
        file.read(); //Dump it to prevent next line read corruption

      lineNumber++; //Advance
      x = 0; //Reset
    }
    else
    {
      if (x == (lineDataLength - 1))
      {
        lineData[x] = '\0'; //Terminate
        break;//Max size hit
      }

      //Record this character to the lineData array
      lineData[x++] = incoming;
    }
  }
  file.close();
  return (lineFound);
}

//Given a fileName, return the given line number
//Returns true if line was loaded
//Returns false if a file was not opened/loaded
bool getFileLineSD(const char* fileName, int lineToFind, char* lineData, int lineDataLength)
{
  bool gotSemaphore = false;
  bool lineFound = false;
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
      markSemaphore(FUNCTION_GETLINE);

      gotSemaphore = true;

      if (USE_SPI_MICROSD)
      {
        SdFile file; //FAT32
        if (file.open(fileName, O_READ) == false)
        {
          log_d("File %s not found", fileName);
          break;
        }

        int lineNumber = 0;

        while (file.available())
        {
          //Get the next line from the file
          int n = file.fgets(lineData, lineDataLength);
          if (n <= 0)
          {
            systemPrintf("Failed to read line %d from settings file\r\n", lineNumber);
            break;
          }
          else
          {
            if (lineNumber == lineToFind)
            {
              lineFound = true;
              break;
            }
          }

          if (strlen(lineData) > 0) //Ignore single \n or \r
            lineNumber++;
        }

        file.close();
      }
#ifdef COMPILE_SD_MMC
      else
      {
        File file = SD_MMC.open(fileName, FILE_READ);

        if (!file)
        {
          log_d("File %s not found", fileName);
          break;
        }

        int lineNumber = 0;

        while (file.available())
        {
          //Get the next line from the file
          int n = getLine(&file, lineData, lineDataLength);
          if (n <= 0)
          {
            systemPrintf("Failed to read line %d from settings file\r\n", lineNumber);
            break;
          }
          else
          {
            if (lineNumber == lineToFind)
            {
              lineFound = true;
              break;
            }
          }

          if (strlen(lineData) > 0) //Ignore single \n or \r
            lineNumber++;
        }

        file.close();
      }
#endif
      break;
    } //End Semaphore check
    else
    {
      systemPrintf("sdCardSemaphore failed to yield, menuBase.ino line %d\r\n", __LINE__);
    }
    break;
  } //End SD online

  //Release access the SD card
  if (online.microSD && (!wasSdCardOnline))
    endSD(gotSemaphore, true);
  else if (gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);

  return (lineFound);
}

//Given a string, replace a single char with another char
void replaceCharacter(char *myString, char toReplace, char replaceWith)
{
  for (int i = 0 ; i < strlen(myString) ; i++)
  {
    if (myString[i] == toReplace)
      myString[i] = replaceWith;
  }
}

//Remove a given filename from SD
bool removeFileSD(const char* fileName)
{
  bool removed = false;

  bool gotSemaphore = false;
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
      markSemaphore(FUNCTION_REMOVEFILE);

      gotSemaphore = true;

      if (USE_SPI_MICROSD)
      {
        if (sd->exists(fileName))
        {
          log_d("Removing from SD: %s", fileName);
          sd->remove(fileName);
          removed = true;
        }
      }
#ifdef COMPILE_SD_MMC
      else
      {
        if (SD_MMC.exists(fileName))
        {
          log_d("Removing from SD: %s", fileName);
          SD_MMC.remove(fileName);
          removed = true;
        }
      }
#endif

      break;
    } //End Semaphore check
    else
    {
      systemPrintf("sdCardSemaphore failed to yield, menuBase.ino line %d\r\n", __LINE__);
    }
    break;
  } //End SD online

  //Release access the SD card
  if (online.microSD && (!wasSdCardOnline))
    endSD(gotSemaphore, true);
  else if (gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);

  return (removed);
}

//Remove a given filename from LFS
bool removeFileLFS(const char* fileName)
{
  if (LittleFS.exists(fileName))
  {
    LittleFS.remove(fileName);
    log_d("Removing LittleFS: %s", fileName);
    return (true);
  }

  return (false);
}

//Remove a given filename from SD and LFS
bool removeFile(const char* fileName)
{
  bool removed = true;

  removed &= removeFileSD(fileName);
  removed &= removeFileLFS(fileName);

  return (removed);
}

//Given a filename and char array, append to file
void recordLineToSD(const char* fileName, const char* lineData)
{
  bool gotSemaphore = false;
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
      markSemaphore(FUNCTION_RECORDLINE);

      gotSemaphore = true;

      FileSdFatMMC file;
      if (file.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
      {
        log_d("File %s not found", fileName);
        break;
      }

      file.println(lineData);
      file.close();
      break;
    } //End Semaphore check
    else
    {
      systemPrintf("sdCardSemaphore failed to yield, menuBase.ino line %d\r\n", __LINE__);
    }
    break;
  } //End SD online

  //Release access the SD card
  if (online.microSD && (!wasSdCardOnline))
    endSD(gotSemaphore, true);
  else if (gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);
}

//Given a filename and char array, append to file
void recordLineToLFS(const char* fileName, const char* lineData)
{
  File file = LittleFS.open(fileName, FILE_APPEND);
  if (!file)
  {
    systemPrintf("File %s failed to create\r\n", fileName);
    return;
  }

  file.println(lineData);
  file.close();
}

//Remove ' ', \t, \v, \f, \r, \n from end of a char array
void trim(char *str)
{
  int x = 0;
  for ( ; str[x] != '\0' ; x++)
    ;
  if (x > 0) x--;

  for ( ; isspace(str[x]) ; x--)
    str[x] = '\0';
}
