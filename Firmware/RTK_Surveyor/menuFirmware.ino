//Update firmware if bin files found
void menuFirmware()
{
  bool newOTAFirmwareAvailable = false;
  char reportedVersion[50] = {'\0'};

  while (1)
  {
    systemPrintln();
    systemPrintln("Menu: Update Firmware");

    if (btPrintEcho == true)
      systemPrintln("Firmware update not available while configuration over Bluetooth is active");

    char currentVersion[20];
    if (enableRCFirmware == false)
      snprintf(currentVersion, sizeof(currentVersion), "%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    else
      snprintf(currentVersion, sizeof(currentVersion), "%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

    systemPrintf("Current firmware: v%s\r\n", currentVersion);

    if (strlen(reportedVersion) > 0)
    {
      if (newOTAFirmwareAvailable == false)
        systemPrintf("c) Check SparkFun for device firmware: Up to date\r\n");
    }
    else
      systemPrintln("c) Check SparkFun for device firmware");

    systemPrintf("e) Allow Beta Firmware: %s\r\n", enableRCFirmware ? "Enabled" : "Disabled");

    if (newOTAFirmwareAvailable)
      systemPrintf("u) Update to new firmware: v%s\r\n", reportedVersion);

    for (int x = 0 ; x < binCount ; x++)
      systemPrintf("%d) Load SD file: %s\r\n", x + 1, binFileNames[x]);

    systemPrintln("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming > 0 && incoming < (binCount + 1))
    {
      //Adjust incoming to match array
      incoming--;
      updateFromSD(binFileNames[incoming]);
    }
    else if (incoming == 'c' && btPrintEcho == false)
    {
      bool previouslyConnected = wifiIsConnected();

      bluetoothStop(); //Stop Bluetooth to allow for SSL on the heap

      //Attempt to connect to local WiFi
      if (wifiConnect(10000) == true)
      {
        //Get firmware version from server
        if (otaCheckVersion(reportedVersion, sizeof(reportedVersion)))
        {
          //We got a version number, now determine if it's newer or not
          char currentVersion[20];
          if (enableRCFirmware == false)
            snprintf(currentVersion, sizeof(currentVersion), "%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
          else
            snprintf(currentVersion, sizeof(currentVersion), "%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

          if (isReportedVersionNewer(reportedVersion, currentVersion) == true)
          {
            log_d("New version detected");
            newOTAFirmwareAvailable = true;
          }
          else
          {
            log_d("No new firmware available");
          }
        }
        else
        {
          //Failed to get version number
          systemPrintln("Failed to get version number from server.");
        }
      }
      else if (incoming == 'c' && btPrintEcho == false)
      {
        bool previouslyConnected = wifiIsConnected();

        bluetoothStop(); //Stop Bluetooth to allow for SSL on the heap

        //Attempt to connect to local WiFi
        if (wifiConnect(10000) == true)
        {
          //Get firmware version from server
          if (otaCheckVersion(reportedVersion, sizeof(reportedVersion)))
          {
            //We got a version number, now determine if it's newer or not
            char currentVersion[20];
            if (enableRCFirmware == false)
              snprintf(currentVersion, sizeof(currentVersion), "%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
            else
              snprintf(currentVersion, sizeof(currentVersion), "%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

            if (isReportedVersionNewer(reportedVersion, currentVersion) == true)
            {
              log_d("New version detected");
              newOTAFirmwareAvailable = true;
            }
            else
            {
              log_d("No new firmware available");
            }
          }
          else
          {
            //Failed to get version number
            systemPrintln("Failed to get version number from server.");
          }
        }
        else
          systemPrintln("Firmware update failed to connect to WiFi.");

        if (previouslyConnected == false)
          wifiStop();

        bluetoothStart(); //Restart BT according to settings
      }

    }
    else if (incoming == 'c' && btPrintEcho == true)
    {
      systemPrintln("Firmware update not available while configuration over Bluetooth is active");
      delay(2000);
    }
    else if (newOTAFirmwareAvailable && incoming == 'u')
    {
      bool previouslyConnected = wifiIsConnected();

      otaUpdate();

      //We get here if WiFi failed or the server was not available

      if (previouslyConnected == false)
        wifiStop();
    }

    else if (incoming == 'e')
    {
      enableRCFirmware ^= 1;
      strncpy(reportedVersion, "", sizeof(reportedVersion) - 1); //Reset to force c) menu
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

void mountSDThenUpdate(const char * firmwareFileName)
{
  bool gotSemaphore;
  bool wasSdCardOnline;

  //Try to gain access the SD card
  gotSemaphore = false;
  wasSdCardOnline = online.microSD;
  if (online.microSD != true)
    beginSD();

  if (online.microSD != true)
    systemPrintln("microSD card is offline!");
  else
  {
    //Attempt to access file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      gotSemaphore = true;
      updateFromSD(firmwareFileName);
    } //End Semaphore check
    else
    {
      systemPrintf("sdCardSemaphore failed to yield, menuFirmware.ino line %d\r\n", __LINE__);
    }
  }

  //Release access the SD card
  if (online.microSD && (!wasSdCardOnline))
    endSD(gotSemaphore, true);
  else if (gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);
}

//Looks for matching binary files in root
//Loads a global called binCount
//Called from beginSD with microSD card mounted and sdCardsemaphore held
void scanForFirmware()
{
  //Count available binaries
  if (USE_SPI_MICROSD)
  {
    SdFile tempFile;
    SdFile dir;
    const char* BIN_EXT = "bin";
    const char* BIN_HEADER = "RTK_Surveyor_Firmware";
  
    char fname[50]; //Handle long file names
  
    dir.open("/"); //Open root
  
    binCount = 0; //Reset count in case scanForFirmware is called again
  
    while (tempFile.openNext(&dir, O_READ) && binCount < maxBinFiles)
    {
      if (tempFile.isFile())
      {
        tempFile.getName(fname, sizeof(fname));
  
        if (strcmp(forceFirmwareFileName, fname) == 0)
        {
          systemPrintln("Forced firmware detected. Loading...");
          displayForcedFirmwareUpdate();
          updateFromSD(forceFirmwareFileName);
        }
  
        //Check 'bin' extension
        if (strcmp(BIN_EXT, &fname[strlen(fname) - strlen(BIN_EXT)]) == 0)
        {
          //Check for 'RTK_Surveyor_Firmware' start of file name
          if (strncmp(fname, BIN_HEADER, strlen(BIN_HEADER)) == 0)
          {
            strncpy(binFileNames[binCount++], fname, sizeof(binFileNames[0]) - 1); //Add this to the array
          }
          else
            systemPrintf("Unknown: %s\r\n", fname);
        }
      }
      tempFile.close();
    }
  }
#ifdef COMPILE_SD_MMC
  else
  {
    const char* BIN_EXT = "bin";
    const char* BIN_HEADER = "/RTK_Surveyor_Firmware";
  
    char fname[50]; //Handle long file names
  
    File dir = SD_MMC.open("/"); //Open root
    if (!dir || !dir.isDirectory())
      return;
  
    binCount = 0; //Reset count in case scanForFirmware is called again

    File tempFile = dir.openNextFile();
    while (tempFile && (binCount < maxBinFiles))
    {
      if (!tempFile.isDirectory())
      {
        snprintf(fname, sizeof(fname), "%s", tempFile.name());
  
        if (strcmp(forceFirmwareFileName, fname) == 0)
        {
          systemPrintln("Forced firmware detected. Loading...");
          displayForcedFirmwareUpdate();
          updateFromSD(forceFirmwareFileName);
        }
  
        //Check 'bin' extension
        if (strcmp(BIN_EXT, &fname[strlen(fname) - strlen(BIN_EXT)]) == 0)
        {
          //Check for 'RTK_Surveyor_Firmware' start of file name
          if (strncmp(fname, BIN_HEADER, strlen(BIN_HEADER)) == 0)
          {
            strncpy(binFileNames[binCount++], fname, sizeof(binFileNames[0]) - 1); //Add this to the array
          }
          else
            systemPrintf("Unknown: %s\r\n", fname);
        }
      }
      tempFile.close();
      tempFile = dir.openNextFile();
    }    
  }
#endif
}

//Look for firmware file on SD card and update as needed
//Called from scanForFirmware with microSD card mounted and sdCardsemaphore held
//Called from mountSDThenUpdate with microSD card mounted and sdCardsemaphore held
void updateFromSD(const char *firmwareFileName)
{
  //Count app partitions
  int appPartitions = 0;
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  while (it != nullptr)
  {
    appPartitions++;
    it = esp_partition_next(it);
  }

  //We cannot do OTA if there is only one partition
  if (appPartitions < 2)
  {
    systemPrintln("SD firmware updates are not available on 4MB devices. Please use the GUI or CLI update methods.");
    return;
  }

  //Turn off any tasks so that we are not disrupted
  espnowStop();
  wifiStop();
  bluetoothStop();

  //Delete tasks if running
  tasksStopUART2();

  systemPrintf("Loading %s\r\n", firmwareFileName);

  if (USE_SPI_MICROSD)
  {
    if (!sd->exists(firmwareFileName))
    {
      systemPrintln("No firmware file found");
      return;
    }
  }
#ifdef COMPILE_SD_MMC
  else
  {
    if (!SD_MMC.exists(firmwareFileName))
    {
      systemPrintln("No firmware file found");
      return;
    }
  }
#endif
  
  FileSdFatMMC firmwareFile;
  firmwareFile.open(firmwareFileName, O_READ);

  size_t updateSize = firmwareFile.size();

  if (updateSize == 0)
  {
    systemPrintln("Error: Binary is empty");
    firmwareFile.close();
    return;
  }

  if (Update.begin(updateSize) == false)
  {
    systemPrintln("Update begin failed. Not enough partition space available.");
    firmwareFile.close();
    return;
  }

  systemPrintln("Moving file to OTA section");
  systemPrint("Bytes to write: ");
  systemPrint(updateSize);

  const int pageSize = 512 * 4;
  byte dataArray[pageSize];
  int bytesWritten = 0;

  //Indicate progress
  int barWidthInCharacters = 20; //Width of progress bar, ie [###### % complete
  long portionSize = updateSize / barWidthInCharacters;
  int barWidth = 0;

  //Bulk write from the SD file to flash
  while (firmwareFile.available())
  {
    if (productVariant == RTK_SURVEYOR)
      digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Toggle LED to indcate activity

    int bytesToWrite = pageSize; //Max number of bytes to read
    if (firmwareFile.available() < bytesToWrite) bytesToWrite = firmwareFile.available(); //Trim this read size as needed

    firmwareFile.read(dataArray, bytesToWrite); //Read the next set of bytes from file into our temp array

    if (Update.write(dataArray, bytesToWrite) != bytesToWrite)
    {
      systemPrintln("\nWrite failed. Binary may be incorrectly aligned.");
      break;
    }
    else
      bytesWritten += bytesToWrite;

    //Indicate progress
    if (bytesWritten > barWidth * portionSize)
    {
      //Advance the bar
      barWidth++;
      systemPrint("\n[");
      for (int x = 0 ; x < barWidth ; x++)
        systemPrint("=");
      systemPrintf("%d%%", bytesWritten * 100 / updateSize);
      if (bytesWritten == updateSize) systemPrintln("]");

      displayFirmwareUpdateProgress(bytesWritten * 100 / updateSize);
    }
  }
  systemPrintln("\nFile move complete");

  if (Update.end())
  {
    if (Update.isFinished())
    {
      displayFirmwareUpdateProgress(100);

      //Clear all settings from LittleFS
      LittleFS.format();

      systemPrintln("Firmware updated successfully. Rebooting. Goodbye!");

      //If forced firmware is detected, do a full reset of config as well
      if (strcmp(forceFirmwareFileName, firmwareFileName) == 0)
      {
        systemPrintln("Removing firmware file");

        //Remove forced firmware file to prevent endless loading
        firmwareFile.close();

        if (USE_SPI_MICROSD)
          sd->remove(firmwareFileName);
#ifdef COMPILE_SD_MMC
        else
          SD_MMC.remove(firmwareFileName);
#endif

        theGNSS.factoryDefault(); //Reset everything: baud rate, I2C address, update rate, everything. And save to BBR.
      }

      delay(1000);
      ESP.restart();
    }
    else
      systemPrintln("Update not finished? Something went wrong!");
  }
  else
  {
    systemPrint("Error Occurred. Error #: ");
    systemPrintln(String(Update.getError()));
  }

  firmwareFile.close();

  displayMessage("Update Failed", 0);

  systemPrintln("Firmware update failed. Please try again.");
}

//Returns true if we successfully got the versionAvailable
//Modifies versionAvailable with OTA getVersion response
//Connects to WiFi as needed
bool otaCheckVersion(char *versionAvailable, uint8_t versionAvailableLength)
{
  bool gotVersion = false;
#ifdef COMPILE_WIFI
  bool previouslyConnected = wifiIsConnected();

  if (wifiConnect(10000) == true)
  {
    char versionString[20];

    if (enableRCFirmware == false)
      snprintf(versionString, sizeof(versionString), "%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    else
      snprintf(versionString, sizeof(versionString), "%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

    systemPrintf("Current firmware version: v%s\r\n", versionString);

    if (enableRCFirmware == false)
      systemPrintf("Checking to see if an update is available from %s\r\n", OTA_FIRMWARE_JSON_URL);
    else
      systemPrintf("Checking to see if an update is available from %s\r\n", OTA_RC_FIRMWARE_JSON_URL);

    ESP32OTAPull ota;

    int response;
    if (enableRCFirmware == false)
      response = ota.CheckForOTAUpdate(OTA_FIRMWARE_JSON_URL, versionString, ESP32OTAPull::DONT_DO_UPDATE);
    else
      response = ota.CheckForOTAUpdate(OTA_RC_FIRMWARE_JSON_URL, versionString, ESP32OTAPull::DONT_DO_UPDATE);

    //We don't care if the library thinks the available firmware is newer, we just need a successful JSON parse
    if (response == ESP32OTAPull::UPDATE_AVAILABLE || response == ESP32OTAPull::NO_UPDATE_AVAILABLE)
    {
      gotVersion = true;

      //Call getVersion after original inquiry
      String otaVersion = ota.GetVersion();
      otaVersion.toCharArray(versionAvailable, versionAvailableLength);
    }
    else if (response == ESP32OTAPull::HTTP_FAILED)
    {
      systemPrintln("Firmware server not available");
    }
    else
    {
      systemPrintln("OTA failed");
    }
  }
  else
  {
    systemPrintln("WiFi not available.");
  }

  if (systemState != STATE_WIFI_CONFIG)
  {
    //wifiStop() turns off the entire radio including the webserver. We need to turn off Station mode only.
    //For now, unit exits AP mode via reset so if we are in AP config mode, leave WiFi Station running.

    //If WiFi was originally off, turn it off again
    if (previouslyConnected == false)
      wifiStop();
  }

  if (gotVersion == true)
    log_d("Available OTA firmware version: %s\r\n", versionAvailable);

#endif
  return (gotVersion);
}

//Force updates firmware using OTA pull
//Exits by either updating firmware and resetting, or failing to connect
void otaUpdate()
{
#ifdef COMPILE_WIFI
  bool previouslyConnected = wifiIsConnected();

  if (wifiConnect(10000) == true)
  {
    char versionString[20];
    snprintf(versionString, sizeof(versionString), "%d.%d", 0, 0); //Force update with version 0.0

    ESP32OTAPull ota;

    int response;
    if (enableRCFirmware == false)
      response = ota.CheckForOTAUpdate(OTA_FIRMWARE_JSON_URL, versionString, ESP32OTAPull::DONT_DO_UPDATE);
    else
      response = ota.CheckForOTAUpdate(OTA_RC_FIRMWARE_JSON_URL, versionString, ESP32OTAPull::DONT_DO_UPDATE);

    if (response == ESP32OTAPull::UPDATE_AVAILABLE)
    {
      systemPrintln("Installing new firmware");
      ota.SetCallback(otaPullCallback);
      if (enableRCFirmware == false)
        ota.CheckForOTAUpdate(OTA_FIRMWARE_JSON_URL, versionString); //Install new firmware, no reset
      else
        ota.CheckForOTAUpdate(OTA_RC_FIRMWARE_JSON_URL, versionString); //Install new firmware, no reset

      if (apConfigFirmwareUpdateInProcess)
      {
#ifdef COMPILE_AP
        //Tell AP page to display reset info
        websocket->textAll("confirmReset,1,");
#endif
      }
      ESP.restart();
    }
    else if (response == ESP32OTAPull::NO_UPDATE_AVAILABLE)
    {
      systemPrintln("OTA Update: Current firmware is up to date");
    }
    else if (response == ESP32OTAPull::HTTP_FAILED)
    {
      systemPrintln("OTA Update: Firmware server not available");
    }
    else
    {
      systemPrintln("OTA Update: OTA failed");
    }
  }
  else
  {
    systemPrintln("WiFi not available.");
  }

  //Update failed. If WiFi was originally off, turn it off again
  if (previouslyConnected == false)
    wifiStop();

#endif

}

//Called while the OTA Pull update is happening
void otaPullCallback(int bytesWritten, int totalLength)
{
  static int previousPercent = -1;
  int percent = 100 * bytesWritten / totalLength;
  if (percent != previousPercent)
  {
    //Indicate progress
    int barWidthInCharacters = 20; //Width of progress bar, ie [###### % complete
    long portionSize = totalLength / barWidthInCharacters;

    //Indicate progress
    systemPrint("\r\n[");
    int barWidth = bytesWritten / portionSize;
    for (int x = 0 ; x < barWidth ; x++)
      systemPrint("=");
    systemPrintf(" %d%%", percent);
    if (bytesWritten == totalLength) systemPrintln("]");

    displayFirmwareUpdateProgress(percent);

    if (apConfigFirmwareUpdateInProcess == true)
    {
#ifdef COMPILE_AP
      char myProgress[50];
      snprintf(myProgress, sizeof(myProgress), "otaFirmwareStatus,%d,", percent);
      websocket->textAll(myProgress);
#endif
    }

    previousPercent = percent;
  }
}

const char *otaPullErrorText(int code)
{
#ifdef COMPILE_WIFI
  switch (code)
  {
    case ESP32OTAPull::UPDATE_AVAILABLE:
      return "An update is available but wasn't installed";
    case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
      return "No profile matches";
    case ESP32OTAPull::NO_UPDATE_AVAILABLE:
      return "Profile matched, but update not applicable";
    case ESP32OTAPull::UPDATE_OK:
      return "An update was done, but no reboot";
    case ESP32OTAPull::HTTP_FAILED:
      return "HTTP GET failure";
    case ESP32OTAPull::WRITE_ERROR:
      return "Write error";
    case ESP32OTAPull::JSON_PROBLEM:
      return "Invalid JSON";
    case ESP32OTAPull::OTA_UPDATE_FAIL:
      return "Update fail (no OTA partition?)";
    default:
      if (code > 0)
        return "Unexpected HTTP response code";
      break;
  }
#endif
  return "Unknown error";
}

//Returns true if reportedVersion is newer than currentVersion
//Version number comes in as v2.7-Jan 5 2023
//2.7-Jan 5 2023 is newer than v2.7-Jan 1 2023
bool isReportedVersionNewer(char* reportedVersion, char *currentVersion)
{
  float currentVersionNumber = 0.0;
  int currentDay = 0;
  int currentMonth = 0;
  int currentYear = 0;

  float reportedVersionNumber = 0.0;
  int reportedDay = 0;
  int reportedMonth = 0;
  int reportedYear = 0;

  breakVersionIntoParts(currentVersion, &currentVersionNumber, &currentYear, &currentMonth, &currentDay);
  breakVersionIntoParts(reportedVersion, &reportedVersionNumber, &reportedYear, &reportedMonth, &reportedDay);

  log_d("currentVersion: %f %d %d %d", currentVersionNumber, currentYear, currentMonth, currentDay);
  log_d("reportedVersion: %f %d %d %d", reportedVersionNumber, reportedYear, reportedMonth, reportedDay);
  if (enableRCFirmware) log_d("RC firmware enabled");

  //Production firmware is named "2.6"
  //Release Candidate firmware is named "2.6-Dec 5 2022"

  //If the user is not using Release Candidate firmware, then check only the version number
  if (enableRCFirmware == false)
  {
    //Check only the version number
    if (reportedVersionNumber > currentVersionNumber)
      return (true);
    return (false);
  }

  //For RC firmware, compare firmware date as well
  //Check version number
  if (reportedVersionNumber > currentVersionNumber)
    return (true);

  //Check which date is more recent
  //https://stackoverflow.com/questions/5283120/date-comparison-to-find-which-is-bigger-in-c
  int reportedVersionScore = reportedDay + reportedMonth * 100 + reportedYear * 2000;
  int currentVersionScore = currentDay + currentMonth * 100 + currentYear * 2000;

  if (reportedVersionScore > currentVersionScore) {
    log_d("Reported version is greater");
    return (true);
  }

  return (false);
}

//Version number comes in as v2.7-Jan 5 2023
//Given a char string, break into version number, year, month, day
//Returns false if parsing failed
bool breakVersionIntoParts(char *version, float *versionNumber, int *year, int *month, int *day)
{
  char monthStr[20];
  int placed = 0;

  if (enableRCFirmware == false)
  {
    placed = sscanf(version, "%f", versionNumber);
    if (placed != 1)
    {
      log_d("Failed to sscanf basic");
      return (false); //Something went wrong
    }
  }
  else
  {
    placed = sscanf(version, "%f-%s %d %d", versionNumber, monthStr, day, year);

    if (placed != 4)
    {
      log_d("Failed to sscanf RC");
      return (false); //Something went wrong
    }

    (*month) = mapMonthName(monthStr);
    if (*month == -1) return (false); //Something went wrong
  }

  return (true);
}

//https://stackoverflow.com/questions/21210319/assign-month-name-and-integer-values-from-string-using-sscanf
int mapMonthName(char *mmm)
{
  static char const *months[] =
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  for (size_t i = 0; i < sizeof(months) / sizeof(months[0]); i++)
  {
    if (strcmp(mmm, months[i]) == 0)
      return i + 1;
  }
  return -1;
}
