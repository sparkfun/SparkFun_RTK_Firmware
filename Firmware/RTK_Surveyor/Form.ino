//Once connected to the access point for WiFi Config, the ESP32 sends current setting values in one long string to websocket
//After user clicks 'save', data is validated via main.js and a long string of values is returned.

bool websocketConnected = false;

//Start webserver in AP mode
void startWebServer()
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

  ntripClientStop(true); //Do not allocate new wifiClient
  ntripServerStop(true); //Do not allocate new wifiClient

  if (wifiStartAP() == false) //Exits calling wifiConnect()
    return;

  incomingSettings = (char*)malloc(AP_CONFIG_SETTING_SIZE);
  memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

  //Pre-load settings CSV
  settingsCSV = (char*)malloc(AP_CONFIG_SETTING_SIZE);
  createSettingsString(settingsCSV);

  webserver = new AsyncWebServer(80);
  websocket = new AsyncWebSocket("/ws");

  websocket->onEvent(onWsEvent);
  webserver->addHandler(websocket);

  // * index.html (not gz'd)
  // * favicon.ico

  // * /src/bootstrap.bundle.min.js - Needed for popper
  // * /src/bootstrap.min.css
  // * /src/bootstrap.min.js
  // * /src/jquery-3.6.0.min.js
  // * /src/main.js (not gz'd)
  // * /src/rtk-setup.png
  // * /src/style.css

  // * /src/fonts/icomoon.eot
  // * /src/fonts/icomoon.svg
  // * /src/fonts/icomoon.ttf
  // * /src/fonts/icomoon.woof

  // * /listfiles responds with a CSV of files and sizes in root
  // * /file allows the download or deletion of a file

  webserver->onNotFound(notFound);

  webserver->onFileUpload(handleUpload); // Run handleUpload function when any file is uploaded. Must be before server.on() calls.

  webserver->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  webserver->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", favicon_ico, sizeof(favicon_ico));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", bootstrap_bundle_min_js, sizeof(bootstrap_bundle_min_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", bootstrap_min_css, sizeof(bootstrap_min_css));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", bootstrap_min_js, sizeof(bootstrap_min_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", jquery_js, sizeof(jquery_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/main.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/javascript", main_js);
  });

  webserver->on("/src/rtk-setup.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", rtkSetup_png, sizeof(rtkSetup_png));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", style_css, sizeof(style_css));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/fonts/icomoon.eot", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_eot, sizeof(icomoon_eot));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/fonts/icomoon.svg", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_svg, sizeof(icomoon_svg));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/fonts/icomoon.ttf", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_ttf, sizeof(icomoon_ttf));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver->on("/src/fonts/icomoon.woof", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_woof, sizeof(icomoon_woof));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  //Handler for the /upload form POST
  webserver->on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  }, handleFirmwareFileUpload);

  //Handlers for file manager
  webserver->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    systemPrintln(logmessage);
    request->send(200, "text/plain", getFileList());
  });

  //Handler for the filemanager
  webserver->on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleFirmwareFileDownload(request);
  });

  webserver->begin();

  log_d("Web Server Started");
  reportHeapNow();

#endif //COMPILE_AP
#endif //COMPILE_WIFI
}

void stopWebServer()
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

  if (webserver != NULL)
  {
    webserver->end();
    free(webserver);
    webserver = NULL;

    if (websocket != NULL)
    {
      delete websocket;
      websocket = NULL;
    }

    if (settingsCSV != NULL)
    {
      free(settingsCSV);
      settingsCSV = NULL;
    }

    if (incomingSettings != NULL)
    {
      free(incomingSettings);
      incomingSettings = NULL;
    }
  }

  log_d("Web Server Stopped");
  reportHeapNow();

#endif
#endif
}

#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  systemPrintln(logmessage);
  request->send(404, "text/plain", "Not found");
}
#endif
#endif

//Handler for firmware file downloads
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
static void handleFirmwareFileDownload(AsyncWebServerRequest *request)
{
  //This section does not tolerate semaphore transactions
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

  if (request->hasParam("name") && request->hasParam("action"))
  {
    const char *fileName = request->getParam("name")->value().c_str();
    const char *fileAction = request->getParam("action")->value().c_str();

    logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

    if (sd->exists(fileName) == false)
    {
      systemPrintln(logmessage + " ERROR: file does not exist");
      request->send(400, "text/plain", "ERROR: file does not exist");
    }
    else
    {
      systemPrintln(logmessage + " file exists");

      if (strcmp(fileAction, "download") == 0)
      {
        logmessage += " downloaded";

        if (managerFileOpen == false)
        {
          if (USE_SPI_MICROSD)
          {
            if (managerTempFile->open(fileName, O_READ) == true)
              managerFileOpen = true;
            else
              systemPrintln("Error: File Manager failed to open file");
          }
#ifdef COMPILE_SD_MMC
          else
          {
            *managerTempFile_SD_MMC = SD_MMC.open(fileName, FILE_READ);
            if (managerTempFile_SD_MMC)
              managerFileOpen = true;
            else
              systemPrintln("Error: File Manager failed to open file");
          }
#endif
        }
        else
        {
          //File is already in use. Wait your turn.
          request->send(202, "text/plain", "ERROR: File already downloading");
        }

        int dataAvailable;
        if (USE_SPI_MICROSD)
          dataAvailable = managerTempFile->size() - managerTempFile->position();
#ifdef COMPILE_SD_MMC
        else
          dataAvailable = managerTempFile_SD_MMC->size() - managerTempFile_SD_MMC->position();
#endif

        AsyncWebServerResponse *response = request->beginResponse("text/plain", dataAvailable,
                                           [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
        {
          uint32_t bytes = 0;
          uint32_t availableBytes;
          if (USE_SPI_MICROSD)
            availableBytes = managerTempFile->available();
#ifdef COMPILE_SD_MMC
          else
            availableBytes = managerTempFile_SD_MMC->available();
#endif

          if (availableBytes > maxLen)
          {
          if (USE_SPI_MICROSD)
            bytes = managerTempFile->read(buffer, maxLen);
#ifdef COMPILE_SD_MMC
          else
            bytes = managerTempFile_SD_MMC->read(buffer, maxLen);
#endif
          }
          else
          {
          if (USE_SPI_MICROSD)
          {
            bytes = managerTempFile->read(buffer, availableBytes);
            managerTempFile->close();
          }
#ifdef COMPILE_SD_MMC
          else
          {
            bytes = managerTempFile_SD_MMC->read(buffer, availableBytes);
            managerTempFile_SD_MMC->close();
          }
#endif
            managerFileOpen = false;

            websocket->textAll("fmNext,1,"); //Tell browser to send next file if needed
          }

          return bytes;
        });

        response->addHeader("Cache-Control", "no-cache");
        response->addHeader("Content-Disposition", "attachment; filename=" + String(fileName));
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
      }
      else if (strcmp(fileAction, "delete") == 0)
      {
        logmessage += " deleted";
        sd->remove(fileName);
        request->send(200, "text/plain", "Deleted File: " + String(fileName));
      }
      else
      {
        logmessage += " ERROR: invalid action param supplied";
        request->send(400, "text/plain", "ERROR: invalid action param supplied");
      }
      systemPrintln(logmessage);
    }
  }
  else
  {
    request->send(400, "text/plain", "ERROR: name and action params required");
  }
}
#endif
#endif

//Handler for firmware file upload
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
static void handleFirmwareFileUpload(AsyncWebServerRequest * request, String fileName, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    //Check file name against valid firmware names
    const char* BIN_EXT = "bin";
    const char* BIN_HEADER = "RTK_Surveyor_Firmware";

    char fname[50]; //Handle long file names
    fileName.toCharArray(fname, sizeof(fname));
    fname[fileName.length()] = '\0'; //Terminate array

    //Check 'bin' extension
    if (strcmp(BIN_EXT, &fname[strlen(fname) - strlen(BIN_EXT)]) == 0)
    {
      //Check for 'RTK_Surveyor_Firmware' start of file name
      if (strncmp(fname, BIN_HEADER, strlen(BIN_HEADER)) == 0)
      {
        //Begin update process
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
          Update.printError(Serial);
          return request->send(400, "text/plain", "OTA could not begin");
        }
      }
      else
      {
        systemPrintf("Unknown: %s\r\n", fname);
        return request->send(400, "text/html", "<b>Error:</b> Unknown file type");
      }
    }
    else
    {
      systemPrintf("Unknown: %s\r\n", fname);
      return request->send(400, "text/html", "<b>Error:</b> Unknown file type");
    }
  }

  // Write chunked data to the free sketch space
  if (len)
  {
    if (Update.write(data, len) != len)
      return request->send(400, "text/plain", "OTA could not begin");
    else
    {
      binBytesSent += len;

      //Send an update to browser every 100k
      if (binBytesSent - binBytesLastUpdate > 100000)
      {
        binBytesLastUpdate = binBytesSent;

        char bytesSentMsg[100];
        sprintf(bytesSentMsg, "%'d bytes sent", binBytesSent);

        systemPrintf("bytesSentMsg: %s\r\n", bytesSentMsg);

        char statusMsg[200] = {'\0'};
        stringRecord(statusMsg, "firmwareUploadStatus", bytesSentMsg); //Convert to "firmwareUploadMsg,11214 bytes sent,"

        systemPrintf("msg: %s\r\n", statusMsg);
        websocket->textAll(statusMsg);
      }

    }
  }

  if (final)
  {
    if (!Update.end(true))
    {
      Update.printError(Serial);
      return request->send(400, "text/plain", "Could not end OTA");
    }
    else
    {
      websocket->textAll("firmwareUploadComplete,1,");
      systemPrintln("Firmware update complete. Restarting");
      delay(500);
      ESP.restart();
    }
  }
}
#endif
#endif

//Events triggered by web sockets
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) {
    log_d("Websocket client connected");
    client->text(settingsCSV);
    lastCoordinateUpdate = millis();
    websocketConnected = true;
  }
  else if (type == WS_EVT_DISCONNECT) {
    log_d("Websocket client disconnected");

    //User has either refreshed the page or disconnected. Recompile the current settings.
    createSettingsString(settingsCSV);
    websocketConnected = false;
  }
  else if (type == WS_EVT_DATA) {
    for (int i = 0; i < len; i++) {
      incomingSettings[incomingSettingsSpot++] = data[i];
      incomingSettingsSpot %= AP_CONFIG_SETTING_SIZE;
    }
    timeSinceLastIncomingSetting = millis();
  }
}
#endif
#endif

//Create a csv string with current settings
void createSettingsString(char* newSettings)
{
#ifdef COMPILE_AP
  char tagText[32];
  char nameText[64];

  newSettings[0] = '\0'; //Erase current settings string

  //System Info
  stringRecord(newSettings, "platformPrefix", platformPrefix);

  char apRtkFirmwareVersion[86];
  sprintf(apRtkFirmwareVersion, "v%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
  stringRecord(newSettings, "rtkFirmwareVersion", apRtkFirmwareVersion);

  char apZedPlatform[50];
  if (zedModuleType == PLATFORM_F9P)
    strcpy(apZedPlatform, "ZED-F9P");
  else if (zedModuleType == PLATFORM_F9R)
    strcpy(apZedPlatform, "ZED-F9R");

  char apZedFirmwareVersion[80];
  sprintf(apZedFirmwareVersion, "%s Firmware: %s", apZedPlatform, zedFirmwareVersion);
  stringRecord(newSettings, "zedFirmwareVersion", apZedFirmwareVersion);

  char apDeviceBTID[30];
  sprintf(apDeviceBTID, "Device Bluetooth ID: %02X%02X", btMACAddress[4], btMACAddress[5]);
  stringRecord(newSettings, "deviceBTID", apDeviceBTID);

  //GNSS Config
  stringRecord(newSettings, "measurementRateHz", 1000.0 / settings.measurementRate, 2); //2 = decimals to print
  stringRecord(newSettings, "dynamicModel", settings.dynamicModel);
  stringRecord(newSettings, "ubxConstellationsGPS", settings.ubxConstellations[0].enabled); //GPS
  stringRecord(newSettings, "ubxConstellationsSBAS", settings.ubxConstellations[1].enabled); //SBAS
  stringRecord(newSettings, "ubxConstellationsGalileo", settings.ubxConstellations[2].enabled); //Galileo
  stringRecord(newSettings, "ubxConstellationsBeiDou", settings.ubxConstellations[3].enabled); //BeiDou
  stringRecord(newSettings, "ubxConstellationsGLONASS", settings.ubxConstellations[5].enabled); //GLONASS
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    stringRecord(newSettings, settings.ubxMessages[x].msgTextName, settings.ubxMessages[x].msgRate);

  //Base Config
  stringRecord(newSettings, "baseTypeSurveyIn", !settings.fixedBase);
  stringRecord(newSettings, "baseTypeFixed", settings.fixedBase);
  stringRecord(newSettings, "observationSeconds", settings.observationSeconds);
  stringRecord(newSettings, "observationPositionAccuracy", settings.observationPositionAccuracy, 2);

  if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
  {
    stringRecord(newSettings, "fixedBaseCoordinateTypeECEF", true);
    stringRecord(newSettings, "fixedBaseCoordinateTypeGeo", false);
  }
  else
  {
    stringRecord(newSettings, "fixedBaseCoordinateTypeECEF", false);
    stringRecord(newSettings, "fixedBaseCoordinateTypeGeo", true);
  }

  stringRecord(newSettings, "fixedEcefX", settings.fixedEcefX, 3);
  stringRecord(newSettings, "fixedEcefY", settings.fixedEcefY, 3);
  stringRecord(newSettings, "fixedEcefZ", settings.fixedEcefZ, 3);
  stringRecord(newSettings, "fixedLat", settings.fixedLat, haeNumberOfDecimals);
  stringRecord(newSettings, "fixedLong", settings.fixedLong, haeNumberOfDecimals);
  stringRecord(newSettings, "fixedAltitude", settings.fixedAltitude, 4);

  stringRecord(newSettings, "enableNtripServer", settings.enableNtripServer);
  stringRecord(newSettings, "ntripServer_CasterHost", settings.ntripServer_CasterHost);
  stringRecord(newSettings, "ntripServer_CasterPort", settings.ntripServer_CasterPort);
  stringRecord(newSettings, "ntripServer_CasterUser", settings.ntripServer_CasterUser);
  stringRecord(newSettings, "ntripServer_CasterUserPW", settings.ntripServer_CasterUserPW);
  stringRecord(newSettings, "ntripServer_MountPoint", settings.ntripServer_MountPoint);
  stringRecord(newSettings, "ntripServer_MountPointPW", settings.ntripServer_MountPointPW);

  stringRecord(newSettings, "enableNtripClient", settings.enableNtripClient);
  stringRecord(newSettings, "ntripClient_CasterHost", settings.ntripClient_CasterHost);
  stringRecord(newSettings, "ntripClient_CasterPort", settings.ntripClient_CasterPort);
  stringRecord(newSettings, "ntripClient_CasterUser", settings.ntripClient_CasterUser);
  stringRecord(newSettings, "ntripClient_CasterUserPW", settings.ntripClient_CasterUserPW);
  stringRecord(newSettings, "ntripClient_MountPoint", settings.ntripClient_MountPoint);
  stringRecord(newSettings, "ntripClient_MountPointPW", settings.ntripClient_MountPointPW);
  stringRecord(newSettings, "ntripClient_TransmitGGA", settings.ntripClient_TransmitGGA);

  //Sensor Fusion Config
  stringRecord(newSettings, "enableSensorFusion", settings.enableSensorFusion);
  stringRecord(newSettings, "autoIMUmountAlignment", settings.autoIMUmountAlignment);

  //System Config
  stringRecord(newSettings, "enableLogging", settings.enableLogging);
  stringRecord(newSettings, "maxLogTime_minutes", settings.maxLogTime_minutes);
  stringRecord(newSettings, "maxLogLength_minutes", settings.maxLogLength_minutes);

  char sdCardSizeChar[20];
  stringHumanReadableSize(sdCardSize).toCharArray(sdCardSizeChar, sizeof(sdCardSizeChar));
  char sdFreeSpaceChar[20];
  stringHumanReadableSize(sdFreeSpace).toCharArray(sdFreeSpaceChar, sizeof(sdFreeSpaceChar));

  stringRecord(newSettings, "sdFreeSpace", sdCardSizeChar);
  stringRecord(newSettings, "sdSize", sdFreeSpaceChar);

  stringRecord(newSettings, "enableResetDisplay", settings.enableResetDisplay);

  //Turn on SD display block last
  stringRecord(newSettings, "sdMounted", online.microSD);

  //Port Config
  stringRecord(newSettings, "dataPortBaud", settings.dataPortBaud);
  stringRecord(newSettings, "radioPortBaud", settings.radioPortBaud);
  stringRecord(newSettings, "dataPortChannel", settings.dataPortChannel);

  //L-Band
  char hardwareID[13];
  sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", lbandMACAddress[0], lbandMACAddress[1], lbandMACAddress[2], lbandMACAddress[3], lbandMACAddress[4], lbandMACAddress[5]); //Get ready for JSON
  stringRecord(newSettings, "hardwareID", hardwareID);

  char apDaysRemaining[20];
  if (strlen(settings.pointPerfectCurrentKey) > 0)
  {
#ifdef COMPILE_L_BAND
    uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
    sprintf(apDaysRemaining, "%d", daysRemaining);
#endif
  }
  else
    sprintf(apDaysRemaining, "No Keys");

  stringRecord(newSettings, "daysRemaining", apDaysRemaining);

  stringRecord(newSettings, "pointPerfectDeviceProfileToken", settings.pointPerfectDeviceProfileToken);
  stringRecord(newSettings, "enablePointPerfectCorrections", settings.enablePointPerfectCorrections);
  stringRecord(newSettings, "autoKeyRenewal", settings.autoKeyRenewal);

  //External PPS/Triggers
  stringRecord(newSettings, "enableExternalPulse", settings.enableExternalPulse);
  stringRecord(newSettings, "externalPulseTimeBetweenPulse_us", settings.externalPulseTimeBetweenPulse_us);
  stringRecord(newSettings, "externalPulseLength_us", settings.externalPulseLength_us);
  stringRecord(newSettings, "externalPulsePolarity", settings.externalPulsePolarity);
  stringRecord(newSettings, "enableExternalHardwareEventLogging", settings.enableExternalHardwareEventLogging);

  //Profiles
  stringRecord(newSettings, "profileName", profileNames[profileNumber]); //Must come before profile number so AP config page JS has name before number
  stringRecord(newSettings, "profileNumber", profileNumber);
  for (int index = 0; index < MAX_PROFILE_COUNT; index++)
  {
    sprintf(tagText, "profile%dName", index);
    sprintf(nameText, "%d: %s", index + 1, profileNames[index]);
    stringRecord(newSettings, tagText, nameText);
  }
  //stringRecord(newSettings, "activeProfiles", activeProfiles);

  //System state at power on. Convert various system states to either Rover or Base.
  int lastState = 0; //0 = Rover, 1 = Base
  if (settings.lastState >= STATE_BASE_NOT_STARTED && settings.lastState <= STATE_BASE_FIXED_TRANSMITTING) lastState = 1;
  stringRecord(newSettings, "baseRoverSetup", lastState);

  //Bluetooth radio type
  stringRecord(newSettings, "bluetoothRadioType", settings.bluetoothRadioType);

  //Current coordinates come from HPPOSLLH call back
  stringRecord(newSettings, "geodeticLat", latitude, haeNumberOfDecimals);
  stringRecord(newSettings, "geodeticLon", longitude, haeNumberOfDecimals);
  stringRecord(newSettings, "geodeticAlt", altitude, 3);

  double ecefX = 0;
  double ecefY = 0;
  double ecefZ = 0;

  geodeticToEcef(latitude, longitude, altitude, &ecefX, &ecefY, &ecefZ);

  stringRecord(newSettings, "ecefX", ecefX, 3);
  stringRecord(newSettings, "ecefY", ecefY, 3);
  stringRecord(newSettings, "ecefZ", ecefZ, 3);

  //Antenna height and ARP
  stringRecord(newSettings, "antennaHeight", settings.antennaHeight);
  stringRecord(newSettings, "antennaReferencePoint", settings.antennaReferencePoint, 1);

  //Radio / ESP-Now settings
  char radioMAC[18];   //Send radio MAC
  sprintf(radioMAC, "%02X:%02X:%02X:%02X:%02X:%02X",
          wifiMACAddress[0],
          wifiMACAddress[1],
          wifiMACAddress[2],
          wifiMACAddress[3],
          wifiMACAddress[4],
          wifiMACAddress[5]
         );
  stringRecord(newSettings, "radioMAC", radioMAC);
  stringRecord(newSettings, "radioType", settings.radioType);
  stringRecord(newSettings, "espnowPeerCount", settings.espnowPeerCount);
  for (int index = 0; index < settings.espnowPeerCount; index++)
  {
    sprintf(tagText, "peerMAC%d", index);
    sprintf(nameText, "%02X:%02X:%02X:%02X:%02X:%02X",
            settings.espnowPeers[index][0],
            settings.espnowPeers[index][1],
            settings.espnowPeers[index][2],
            settings.espnowPeers[index][3],
            settings.espnowPeers[index][4],
            settings.espnowPeers[index][5]
           );
    stringRecord(newSettings, tagText, nameText);
  }
  stringRecord(newSettings, "espnowBroadcast", settings.espnowBroadcast);

  //Add ECEF and Geodetic station data
  for (int index = 0; index < COMMON_COORDINATES_MAX_STATIONS ; index++) //Arbitrary 50 station limit
  {
    //stationInfo example: LocationA,-1280206.568,-4716804.403,4086665.484
    char stationInfo[100];

    //Try SD, then LFS
    if (getFileLineSD(stationCoordinateECEFFileName, index, stationInfo, sizeof(stationInfo)) == true) //fileName, lineNumber, array, arraySize
    {
      trim(stationInfo); //Remove trailing whitespace
      //log_d("ECEF SD station %d - found: %s", index, stationInfo);
      replaceCharacter(stationInfo, ',', ' '); //Change all , to ' ' for easier parsing on the JS side
      sprintf(tagText, "stationECEF%d", index);
      stringRecord(newSettings, tagText, stationInfo);
    }
    else if (getFileLineLFS(stationCoordinateECEFFileName, index, stationInfo, sizeof(stationInfo)) == true) //fileName, lineNumber, array, arraySize
    {
      trim(stationInfo); //Remove trailing whitespace
      //log_d("ECEF LFS station %d - found: %s", index, stationInfo);
      replaceCharacter(stationInfo, ',', ' '); //Change all , to ' ' for easier parsing on the JS side
      sprintf(tagText, "stationECEF%d", index);
      stringRecord(newSettings, tagText, stationInfo);
    }
    else
    {
      //We could not find this line
      break;
    }
  }

  for (int index = 0; index < COMMON_COORDINATES_MAX_STATIONS ; index++) //Arbitrary 50 station limit
  {
    //stationInfo example: LocationA,40.09029479,-105.18505761,1560.089
    char stationInfo[100];

    //Try SD, then LFS
    if (getFileLineSD(stationCoordinateGeodeticFileName, index, stationInfo, sizeof(stationInfo)) == true) //fileName, lineNumber, array, arraySize
    {
      trim(stationInfo); //Remove trailing whitespace
      //log_d("Geo SD station %d - found: %s", index, stationInfo);
      replaceCharacter(stationInfo, ',', ' '); //Change all , to ' ' for easier parsing on the JS side
      sprintf(tagText, "stationGeodetic%d", index);
      stringRecord(newSettings, tagText, stationInfo);
    }
    else if (getFileLineLFS(stationCoordinateGeodeticFileName, index, stationInfo, sizeof(stationInfo)) == true) //fileName, lineNumber, array, arraySize
    {
      trim(stationInfo); //Remove trailing whitespace
      //log_d("Geo LFS station %d - found: %s", index, stationInfo);
      replaceCharacter(stationInfo, ',', ' '); //Change all , to ' ' for easier parsing on the JS side
      sprintf(tagText, "stationGeodetic%d", index);
      stringRecord(newSettings, tagText, stationInfo);
    }
    else
    {
      //We could not find this line
      break;
    }
  }

  //Add WiFi credential table
  for (int x = 0 ; x < MAX_WIFI_NETWORKS ; x++)
  {
    sprintf(tagText, "wifiNetwork%dSSID", x);
    stringRecord(newSettings, tagText, settings.wifiNetworks[x].ssid);

    sprintf(tagText, "wifiNetwork%dPassword", x);
    stringRecord(newSettings, tagText, settings.wifiNetworks[x].password);
  }

  //Drop downs on the AP config page expect a value, whereas bools get stringRecord as true/false
  if (settings.wifiConfigOverAP == true)
    stringRecord(newSettings, "wifiConfigOverAP", 1); //1 = AP mode, 0 = WiFi
  else
    stringRecord(newSettings, "wifiConfigOverAP", 0); //1 = AP mode, 0 = WiFi

  stringRecord(newSettings, "wifiTcpPort", settings.wifiTcpPort);
  stringRecord(newSettings, "enableRCFirmware", enableRCFirmware);

  //New settings not yet integrated
  //...

  strcat(newSettings, "\0");
  systemPrintf("newSettings len: %d\r\n", strlen(newSettings));
  systemPrintf("newSettings: %s\r\n", newSettings);
#endif
}

//Create a csv string with the current coordinates
void createCoordinateString(char* settingsCSV)
{
#ifdef COMPILE_AP
  settingsCSV[0] = '\0'; //Erase current settings string

  //Current coordinates come from HPPOSLLH call back
  stringRecord(settingsCSV, "geodeticLat", latitude, haeNumberOfDecimals);
  stringRecord(settingsCSV, "geodeticLon", longitude, haeNumberOfDecimals);
  stringRecord(settingsCSV, "geodeticAlt", altitude, 3);

  double ecefX = 0;
  double ecefY = 0;
  double ecefZ = 0;

  geodeticToEcef(latitude, longitude, altitude, &ecefX, &ecefY, &ecefZ);

  stringRecord(settingsCSV, "ecefX", ecefX, 3);
  stringRecord(settingsCSV, "ecefY", ecefY, 3);
  stringRecord(settingsCSV, "ecefZ", ecefZ, 3);

  strcat(settingsCSV, "\0");
#endif
}

//Given a settingName, and string value, update a given setting
void updateSettingWithValue(const char *settingName, const char* settingValueStr)
{
#ifdef COMPILE_AP
  char* ptr;
  double settingValue = strtod(settingValueStr, &ptr);

  bool settingValueBool = false;
  if (strcmp(settingValueStr, "true") == 0) settingValueBool = true;

  if (strcmp(settingName, "maxLogTime_minutes") == 0)
    settings.maxLogTime_minutes = settingValue;
  else if (strcmp(settingName, "maxLogLength_minutes") == 0)
    settings.maxLogLength_minutes = settingValue;
  else if (strcmp(settingName, "measurementRateHz") == 0)
  {
    settings.measurementRate = (int)(1000.0 / settingValue);

    //This is one of the first settings to be received. If seen, remove the station files.
    removeFile(stationCoordinateECEFFileName);
    removeFile(stationCoordinateGeodeticFileName);
    log_d("Station coordinate files removed");
  }
  else if (strcmp(settingName, "dynamicModel") == 0)
    settings.dynamicModel = settingValue;
  else if (strcmp(settingName, "baseTypeFixed") == 0)
    settings.fixedBase = settingValueBool;
  else if (strcmp(settingName, "observationSeconds") == 0)
    settings.observationSeconds = settingValue;
  else if (strcmp(settingName, "observationPositionAccuracy") == 0)
    settings.observationPositionAccuracy = settingValue;
  else if (strcmp(settingName, "fixedBaseCoordinateTypeECEF") == 0)
    settings.fixedBaseCoordinateType = !settingValueBool; //When ECEF is true, fixedBaseCoordinateType = 0 (COORD_TYPE_ECEF)
  else if (strcmp(settingName, "fixedEcefX") == 0)
    settings.fixedEcefX = settingValue;
  else if (strcmp(settingName, "fixedEcefY") == 0)
    settings.fixedEcefY = settingValue;
  else if (strcmp(settingName, "fixedEcefZ") == 0)
    settings.fixedEcefZ = settingValue;
  else if (strcmp(settingName, "fixedLat") == 0)
    settings.fixedLat = settingValue;
  else if (strcmp(settingName, "fixedLong") == 0)
    settings.fixedLong = settingValue;
  else if (strcmp(settingName, "fixedAltitude") == 0)
    settings.fixedAltitude = settingValue;
  else if (strcmp(settingName, "dataPortBaud") == 0)
    settings.dataPortBaud = settingValue;
  else if (strcmp(settingName, "radioPortBaud") == 0)
    settings.radioPortBaud = settingValue;
  else if (strcmp(settingName, "enableLogging") == 0)
    settings.enableLogging = settingValueBool;
  else if (strcmp(settingName, "dataPortChannel") == 0)
    settings.dataPortChannel = (muxConnectionType_e)settingValue;
  else if (strcmp(settingName, "autoIMUmountAlignment") == 0)
    settings.autoIMUmountAlignment = settingValueBool;
  else if (strcmp(settingName, "enableSensorFusion") == 0)
    settings.enableSensorFusion = settingValueBool;
  else if (strcmp(settingName, "enableResetDisplay") == 0)
    settings.enableResetDisplay = settingValueBool;

  else if (strcmp(settingName, "enableExternalPulse") == 0)
    settings.enableExternalPulse = settingValueBool;
  else if (strcmp(settingName, "externalPulseTimeBetweenPulse_us") == 0)
    settings.externalPulseTimeBetweenPulse_us = settingValue;
  else if (strcmp(settingName, "externalPulseLength_us") == 0)
    settings.externalPulseLength_us = settingValue;
  else if (strcmp(settingName, "externalPulsePolarity") == 0)
    settings.externalPulsePolarity = (pulseEdgeType_e)settingValue;
  else if (strcmp(settingName, "enableExternalHardwareEventLogging") == 0)
    settings.enableExternalHardwareEventLogging = settingValueBool;
  else if (strcmp(settingName, "profileName") == 0)
  {
    strcpy(settings.profileName, settingValueStr);
    setProfileName(profileNumber);
  }
  else if (strcmp(settingName, "enableNtripServer") == 0)
    settings.enableNtripServer = settingValueBool;
  else if (strcmp(settingName, "ntripServer_CasterHost") == 0)
    strcpy(settings.ntripServer_CasterHost, settingValueStr);
  else if (strcmp(settingName, "ntripServer_CasterPort") == 0)
    settings.ntripServer_CasterPort = settingValue;
  else if (strcmp(settingName, "ntripServer_CasterUser") == 0)
    strcpy(settings.ntripServer_CasterUser, settingValueStr);
  else if (strcmp(settingName, "ntripServer_CasterUserPW") == 0)
    strcpy(settings.ntripServer_CasterUserPW, settingValueStr);
  else if (strcmp(settingName, "ntripServer_MountPoint") == 0)
    strcpy(settings.ntripServer_MountPoint, settingValueStr);
  else if (strcmp(settingName, "ntripServer_MountPointPW") == 0)
    strcpy(settings.ntripServer_MountPointPW, settingValueStr);

  else if (strcmp(settingName, "enableNtripClient") == 0)
    settings.enableNtripClient = settingValueBool;
  else if (strcmp(settingName, "ntripClient_CasterHost") == 0)
    strcpy(settings.ntripClient_CasterHost, settingValueStr);
  else if (strcmp(settingName, "ntripClient_CasterPort") == 0)
    settings.ntripClient_CasterPort = settingValue;
  else if (strcmp(settingName, "ntripClient_CasterUser") == 0)
    strcpy(settings.ntripClient_CasterUser, settingValueStr);
  else if (strcmp(settingName, "ntripClient_CasterUserPW") == 0)
    strcpy(settings.ntripClient_CasterUserPW, settingValueStr);
  else if (strcmp(settingName, "ntripClient_MountPoint") == 0)
    strcpy(settings.ntripClient_MountPoint, settingValueStr);
  else if (strcmp(settingName, "ntripClient_MountPointPW") == 0)
    strcpy(settings.ntripClient_MountPointPW, settingValueStr);
  else if (strcmp(settingName, "ntripClient_TransmitGGA") == 0)
    settings.ntripClient_TransmitGGA = settingValueBool;
  else if (strcmp(settingName, "serialTimeoutGNSS") == 0)
    settings.serialTimeoutGNSS = settingValue;
  else if (strcmp(settingName, "pointPerfectDeviceProfileToken") == 0)
    strcpy(settings.pointPerfectDeviceProfileToken, settingValueStr);
  else if (strcmp(settingName, "enablePointPerfectCorrections") == 0)
    settings.enablePointPerfectCorrections = settingValueBool;
  else if (strcmp(settingName, "autoKeyRenewal") == 0)
    settings.autoKeyRenewal = settingValueBool;
  else if (strcmp(settingName, "antennaHeight") == 0)
    settings.antennaHeight = settingValue;
  else if (strcmp(settingName, "antennaReferencePoint") == 0)
    settings.antennaReferencePoint = settingValue;
  else if (strcmp(settingName, "bluetoothRadioType") == 0)
    settings.bluetoothRadioType = (BluetoothRadioType_e)settingValue; //0 = SPP, 1 = BLE, 2 = Off
  else if (strcmp(settingName, "espnowBroadcast") == 0)
    settings.espnowBroadcast = settingValueBool;
  else if (strcmp(settingName, "radioType") == 0)
    settings.radioType = (RadioType_e)settingValue; //0 = Radio off, 1 = ESP-Now
  else if (strcmp(settingName, "baseRoverSetup") == 0)
  {
    settings.lastState = STATE_ROVER_NOT_STARTED; //Default
    if (settingValue == 1) settings.lastState = STATE_BASE_NOT_STARTED;
  }
  else if (strstr(settingName, "stationECEF") != NULL)
  {
    replaceCharacter((char *)settingValueStr, ' ', ','); //Replace all ' ' with ',' before recording to file
    recordLineToSD(stationCoordinateECEFFileName, settingValueStr);
    recordLineToLFS(stationCoordinateECEFFileName, settingValueStr);
    log_d("%s recorded", settingValueStr);
  }
  else if (strstr(settingName, "stationGeodetic") != NULL)
  {
    replaceCharacter((char *)settingValueStr, ' ', ','); //Replace all ' ' with ',' before recording to file
    recordLineToSD(stationCoordinateGeodeticFileName, settingValueStr);
    recordLineToLFS(stationCoordinateGeodeticFileName, settingValueStr);
    log_d("%s recorded", settingValueStr);
  }
  else if (strcmp(settingName, "wifiTcpPort") == 0)
    settings.wifiTcpPort = settingValue;
  else if (strcmp(settingName, "wifiConfigOverAP") == 0)
  {
    if (settingValue == 1) //Drop downs come back as a value
      settings.wifiConfigOverAP = true;
    else
      settings.wifiConfigOverAP = false;
  }

  else if (strcmp(settingName, "enableTcpClient") == 0)
    settings.enableTcpClient = settingValueBool;
  else if (strcmp(settingName, "enableTcpServer") == 0)
    settings.enableTcpServer = settingValueBool;
  else if (strcmp(settingName, "enableRCFirmware") == 0)
    enableRCFirmware = settingValueBool;

  //Unused variables - read to avoid errors
  else if (strcmp(settingName, "measurementRateSec") == 0) {}
  else if (strcmp(settingName, "baseTypeSurveyIn") == 0) {}
  else if (strcmp(settingName, "fixedBaseCoordinateTypeGeo") == 0) {}
  else if (strcmp(settingName, "saveToArduino") == 0) {}
  else if (strcmp(settingName, "enableFactoryDefaults") == 0) {}
  else if (strcmp(settingName, "enableFirmwareUpdate") == 0) {}
  else if (strcmp(settingName, "enableForgetRadios") == 0) {}
  else if (strcmp(settingName, "nicknameECEF") == 0) {}
  else if (strcmp(settingName, "nicknameGeodetic") == 0) {}
  else if (strcmp(settingName, "fileSelectAll") == 0) {}
  else if (strcmp(settingName, "fixedHAE_APC") == 0) {}

  //Special actions
  else if (strcmp(settingName, "firmwareFileName") == 0)
  {
    mountSDThenUpdate(settingValueStr);

    //If update is successful, it will force system reset and not get here.

    requestChangeState(STATE_ROVER_NOT_STARTED); //If update failed, return to Rover mode.
  }
  else if (strcmp(settingName, "factoryDefaultReset") == 0)
    factoryReset();
  else if (strcmp(settingName, "exitAndReset") == 0)
  {
    //Confirm receipt
    log_d("Sending reset confirmation");
    websocket->textAll("confirmReset,1,");
    delay(500); //Allow for delivery

    systemPrintln("Reset after AP Config");

    ESP.restart();
  }
  else if (strcmp(settingName, "setProfile") == 0)
  {
    //Change to new profile
    changeProfileNumber(settingValue);

    //Load new profile into system
    loadSettings();

    //Send new settings to browser. Re-use settingsCSV to avoid stack.
    if (settingsCSV == NULL)
      settingsCSV = (char*)malloc(AP_CONFIG_SETTING_SIZE);

    memset(settingsCSV, 0, AP_CONFIG_SETTING_SIZE); //Clear any garbage from settings array

    createSettingsString(settingsCSV);

    log_d("Sending profile %d: %s", settingValue, settingsCSV);
    websocket->textAll(settingsCSV);
  }
  else if (strcmp(settingName, "resetProfile") == 0)
  {
    settingsToDefaults(); //Overwrite our current settings with defaults

    recordSystemSettings(); //Overwrite profile file and NVM with these settings

    //Get bitmask of active profiles
    activeProfiles = loadProfileNames();

    //Send new settings to browser. Re-use settingsCSV to avoid stack.
    if (settingsCSV == NULL)
      settingsCSV = (char*)malloc(AP_CONFIG_SETTING_SIZE);

    memset(settingsCSV, 0, AP_CONFIG_SETTING_SIZE); //Clear any garbage from settings array

    createSettingsString(settingsCSV);

    log_d("Sending reset profile: %s", settingsCSV);
    websocket->textAll(settingsCSV);
  }
  else if (strcmp(settingName, "forgetEspNowPeers") == 0)
  {
    //Forget all ESP-Now Peers
    for (int x = 0 ; x < settings.espnowPeerCount ; x++)
      espnowRemovePeer(settings.espnowPeers[x]);
    settings.espnowPeerCount = 0;
  }
  else if (strcmp(settingName, "startNewLog") == 0)
  {
    if (settings.enableLogging == true && online.logging == true)
      endSD(false, true); //Close down file. A new one will be created at the next calling of updateLogs().
  }
  else if (strcmp(settingName, "checkNewFirmware") == 0)
  {
    log_d("Checking for new OTA Pull firmware");

    websocket->textAll("checkingNewFirmware,1,"); //Tell the config page we received their request

    char reportedVersion[20];
    char newVersionCSV[100];

    //Get firmware version from server
    if (otaCheckVersion(reportedVersion, sizeof(reportedVersion)))
    {
      //We got a version number, now determine if it's newer or not
      char currentVersion[20];
      if (enableRCFirmware == false)
        sprintf(currentVersion, "%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
      else
        sprintf(currentVersion, "%d.%d-%s", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

      if (isReportedVersionNewer(reportedVersion, currentVersion) == true)
      {
        log_d("New version detected");
        sprintf(newVersionCSV, "newFirmwareVersion,%s,", reportedVersion);
      }
      else
      {
        log_d("No new firmware available");
        sprintf(newVersionCSV, "newFirmwareVersion,CURRENT,");
      }
    }
    else
    {
      //Failed to get version number
      log_d("Sending error to AP config page");
      sprintf(newVersionCSV, "newFirmwareVersion,ERROR,");
    }

    websocket->textAll(newVersionCSV);
  }
  else if (strcmp(settingName, "getNewFirmware") == 0)
  {
    log_d("Getting new OTA Pull firmware");

    websocket->textAll("gettingNewFirmware,1,"); //Tell the config page we received their request

    apConfigFirmwareUpdateInProcess = true;
    otaUpdate();

    //We get here if WiFi failed to connect
    websocket->textAll("gettingNewFirmware,ERROR,");
  }

  //Check for bulk settings (constellations and message rates)
  //Must be last on else list
  else
  {
    bool knownSetting = false;

    //Scan for WiFi credentials
    if (knownSetting == false)
    {
      for (int x = 0 ; x < MAX_WIFI_NETWORKS ; x++)
      {
        char tempString[100]; //wifiNetwork0Password=parachutes
        sprintf(tempString, "wifiNetwork%dSSID", x);
        if (strcmp(settingName, tempString) == 0)
        {
          strcpy(settings.wifiNetworks[x].ssid, settingValueStr);
          knownSetting = true;
          break;
        }
        else
        {
          sprintf(tempString, "wifiNetwork%dPassword", x);
          if (strcmp(settingName, tempString) == 0)
          {
            strcpy(settings.wifiNetworks[x].password, settingValueStr);
            knownSetting = true;
            break;
          }
        }
      }
    }

    //Scan for constellation settings
    if (knownSetting == false)
    {
      for (int x = 0 ; x < MAX_CONSTELLATIONS ; x++)
      {
        char tempString[50]; //ubxConstellationsSBAS
        sprintf(tempString, "ubxConstellations%s", settings.ubxConstellations[x].textName);

        if (strcmp(settingName, tempString) == 0)
        {
          settings.ubxConstellations[x].enabled = settingValueBool;
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
        if (strcmp(settingName, settings.ubxMessages[x].msgTextName) == 0)
        {
          settings.ubxMessages[x].msgRate = settingValue;
          knownSetting = true;
          break;
        }
      }
    }

    //Last catch
    if (knownSetting == false)
    {
      systemPrintf("Unknown '%s': %0.3lf\r\n", settingName, settingValue);
    }
  } //End last strcpy catch
#endif
}

//Add record with int
void stringRecord(char* settingsCSV, const char *id, int settingValue)
{
  char record[100];
  sprintf(record, "%s,%d,", id, settingValue);
  strcat(settingsCSV, record);
}

//Add record with uint32_t
void stringRecord(char* settingsCSV, const char *id, uint32_t settingValue)
{
  char record[100];
  sprintf(record, "%s,%d,", id, settingValue);
  strcat(settingsCSV, record);
}

//Add record with double
void stringRecord(char* settingsCSV, const char *id, double settingValue, int decimalPlaces)
{
  char format[10];
  sprintf(format, "%%0.%dlf", decimalPlaces); //Create '%0.09lf'

  char formattedValue[20];
  sprintf(formattedValue, format, settingValue);

  char record[100];
  sprintf(record, "%s,%s,", id, formattedValue);
  strcat(settingsCSV, record);
}

//Add record with bool
void stringRecord(char* settingsCSV, const char *id, bool settingValue)
{
  char temp[10];
  if (settingValue == true)
    strcpy(temp, "true");
  else
    strcpy(temp, "false");

  char record[100];
  sprintf(record, "%s,%s,", id, temp);
  strcat(settingsCSV, record);
}

//Add record with string
void stringRecord(char* settingsCSV, const char *id, char* settingValue)
{
  char record[100];
  sprintf(record, "%s,%s,", id, settingValue);
  strcat(settingsCSV, record);
}

//Add record with uint64_t
void stringRecord(char* settingsCSV, const char *id, uint64_t settingValue)
{
  char record[100];
  sprintf(record, "%s,%lld,", id, settingValue);
  strcat(settingsCSV, record);
}

//Break CSV into setting constituents
//Can't use strtok because we may have two commas next to each other, ie measurementRateHz,4.00,measurementRateSec,,dynamicModel,0,
bool parseIncomingSettings()
{
  char settingName[100] = {'\0'};
  char valueStr[150] = {'\0'}; //stationGeodetic1,ANameThatIsTooLongToBeDisplayed 40.09029479 -105.18505761 1560.089

  char* commaPtr = incomingSettings;
  char* headPtr = incomingSettings;

  int counter = 0;
  int maxAttempts = 500;
  while (*headPtr) //Check if string is over
  {
    //Spin to first comma
    commaPtr = strstr(headPtr, ",");
    if (commaPtr != NULL) {
      *commaPtr = '\0';
      strcpy(settingName, headPtr);
      headPtr = commaPtr + 1;
    }

    commaPtr = strstr(headPtr, ",");
    if (commaPtr != NULL) {
      *commaPtr = '\0';
      strcpy(valueStr, headPtr);
      headPtr = commaPtr + 1;
    }

    //log_d("settingName: %s value: %s", settingName, valueStr);

    updateSettingWithValue(settingName, valueStr);

    //Avoid infinite loop if response is malformed
    counter++;
    if (counter == maxAttempts)
    {
      systemPrintln("Error: Incoming settings malformed.");
      break;
    }
  }

  if (counter < maxAttempts)
  {
    //Confirm receipt
    log_d("Sending receipt confirmation");
#ifdef COMPILE_AP
    websocket->textAll("confirmDataReceipt,1,");
#endif
  }

  return (true);
}

//When called, responds with the root folder list of files on SD card
//Name and size are formatted in CSV, formatted to html by JS
String getFileList()
{
  //settingsCSV[0] = '\'0; //Clear array
  String returnText = "";
  char fileName[50]; //Handle long file names

  //Attempt to gain access to the SD card
  if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
  {
    markSemaphore(FUNCTION_FILEMANAGER_UPLOAD1);

    if (USE_SPI_MICROSD)
    {
      SdFile dir;
      dir.open("/"); //Open root
      uint16_t fileCount = 0;
  
      while (managerTempFile->openNext(&dir, O_READ))
      {
        if (managerTempFile->isFile())
        {
          fileCount++;
  
          managerTempFile->getName(fileName, sizeof(fileName));
  
          returnText += "fmName," + String(fileName) + ",fmSize," + stringHumanReadableSize(managerTempFile->fileSize()) + ",";
        }
      }
  
      dir.close();
      managerTempFile->close();
    }
#ifdef COMPILE_SD_MMC
    else
    {
      File root = SD_MMC.open("/"); //Open root

      if (root && root.isDirectory())
      {
        uint16_t fileCount = 0;

        File file = root.openNextFile();
        while (file)
        {
          if (!file.isDirectory())
          {
            fileCount++;
    
            returnText += "fmName," + String(file.name()) + ",fmSize," + stringHumanReadableSize(file.size()) + ",";            
          }
          
          file = root.openNextFile();
        }
      }
      
      root.close();
      managerTempFile_SD_MMC->close();
    }
#endif

    xSemaphoreGive(sdCardSemaphore);
  }
  else
  {
    char semaphoreHolder[50];
    getSemaphoreFunction(semaphoreHolder);

    //This is an error because the current settings no longer match the settings
    //on the microSD card, and will not be restored to the expected settings!
    systemPrintf("sdCardSemaphore failed to yield, held by %s, Form.ino line %d\r\n", semaphoreHolder, __LINE__);
  }

  log_d("returnText (%d bytes): %s\r\n", returnText.length(), returnText.c_str());

  return returnText;
}

// Make size of files human readable
String stringHumanReadableSize(uint64_t bytes)
{
  char suffix[5] = {'\0'};
  char readableSize[50] = {'\0'};
  float cardSize = 0.0;

  if (bytes < 1024) strcpy(suffix, "B");
  else if (bytes < (1024 * 1024)) strcpy(suffix, "KB");
  else if (bytes < (1024 * 1024 * 1024)) strcpy(suffix, "MB");
  else strcpy(suffix, "GB");

  if (bytes < (1024 * 1024)) cardSize = bytes / 1024.0; //KB
  else if (bytes < (1024 * 1024 * 1024)) cardSize = bytes / 1024.0 / 1024.0; //MB
  else cardSize = bytes / 1024.0 / 1024.0 / 1024.0; //GB

  if (strcmp(suffix, "GB") == 0)
    sprintf(readableSize, "%0.1f %s", cardSize, suffix); //Print decimal portion
  else if (strcmp(suffix, "MB") == 0)
    sprintf(readableSize, "%0.1f %s", cardSize, suffix); //Print decimal portion
  else
    sprintf(readableSize, "%0.0f %s", cardSize, suffix); //Don't print decimal portion

  return String(readableSize);
}

#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

// Handles uploading of user files to SD
void handleUpload(AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String logmessage = "";

  if (!index)
  {
    logmessage = "Upload Start: " + String(filename);

    char tempFileName[50];
    filename.toCharArray(tempFileName, sizeof(tempFileName));

    //Attempt to gain access to the SD card
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      markSemaphore(FUNCTION_FILEMANAGER_UPLOAD1);

      if (USE_SPI_MICROSD)
        managerTempFile->open(tempFileName, O_CREAT | O_APPEND | O_WRITE);
#ifdef COMPILE_SD_MMC
      else
        *managerTempFile_SD_MMC = SD_MMC.open(tempFileName, FILE_APPEND);
#endif

      xSemaphoreGive(sdCardSemaphore);
    }

    systemPrintln(logmessage);
  }

  if (len)
  {
    //Attempt to gain access to the SD card
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      markSemaphore(FUNCTION_FILEMANAGER_UPLOAD2);

      if (USE_SPI_MICROSD)
        managerTempFile->write(data, len); // stream the incoming chunk to the opened file
#ifdef COMPILE_SD_MMC
      else
        managerTempFile_SD_MMC->write(data, len);
#endif

      xSemaphoreGive(sdCardSemaphore);
    }
  }

  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);

    //Attempt to gain access to the SD card
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      markSemaphore(FUNCTION_FILEMANAGER_UPLOAD3);

      if (USE_SPI_MICROSD)
        updateDataFileCreate(managerTempFile); // Update the file create time & date

      if (USE_SPI_MICROSD)
        managerTempFile->close();
#ifdef COMPILE_SD_MMC
      else
        managerTempFile_SD_MMC->close();
#endif

      xSemaphoreGive(sdCardSemaphore);
    }

    systemPrintln(logmessage);
    request->redirect("/");
  }
}

#endif
#endif
