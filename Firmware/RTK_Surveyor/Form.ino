//Once connected to the access point for WiFi Config, the ESP32 sends current setting values in one long string to websocket
//After user clicks 'save', data is validated via main.js and a long string of values is returned.

//Start webserver in AP mode
void startWebServer()
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

  //Check SD Size
  if (online.microSD)
  {
    csd_t csd;
    sd->card()->readCSD(&csd); //Card Specific Data
    sdCardSizeMB = 0.000512 * sdCardCapacity(&csd);
    sd->volumeBegin();

    //Find available cluster/space
    sdFreeSpaceMB = sd->vol()->freeClusterCount(); //This takes a few seconds to complete
    sdFreeSpaceMB *= sd->vol()->sectorsPerCluster() / 2;
    sdFreeSpaceMB /= 1024;

    sdUsedSpaceMB = sdCardSizeMB - sdFreeSpaceMB; //Don't think of it as used, think of it as unusable

    //  Serial.print("Card Size(MB): ");
    //  Serial.println(sdCardSizeMB);
    Serial.print("Free space(MB): ");
    Serial.println(sdFreeSpaceMB);
    Serial.print("Used space(MB): ");
    Serial.println(sdUsedSpaceMB);
  }

  sdCardServerEnd();
  ntripClientStop(true);
  wifiStartAP();

  //Clear any garbage from settings array
  memset(incomingSettings, 0, sizeof(incomingSettings));

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", favicon_ico, sizeof(favicon_ico));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", bootstrap_bundle_min_js, sizeof(bootstrap_bundle_min_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", bootstrap_min_css, sizeof(bootstrap_min_css));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", bootstrap_min_js, sizeof(bootstrap_min_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", jquery_js, sizeof(jquery_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/main.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/javascript", main_js);
  });

  server.on("/src/rtk-setup.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", rtkSetup_png, sizeof(rtkSetup_png));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", style_css, sizeof(style_css));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/fonts/icomoon.eot", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_eot, sizeof(icomoon_eot));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/fonts/icomoon.svg", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_svg, sizeof(icomoon_svg));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/fonts/icomoon.ttf", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_ttf, sizeof(icomoon_ttf));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/fonts/icomoon.woof", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", icomoon_woof, sizeof(icomoon_woof));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  //Handler for the /upload form POST
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  }, handleFirmwareFileUpload);

  sdCardServerBegin(&server, false, false);

  log_d("Web Server Started");
  reportHeapNow();

#endif
#endif

  wifiState = WIFI_NOTCONNECTED;
}

void stopWebServer()
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

  //server.reset();
  server.end();

  log_d("Web Server Stopped");
  reportHeapNow();

#endif
#endif
}

//Handler for firmware file upload
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
static void handleFirmwareFileUpload(AsyncWebServerRequest *request, String fileName, size_t index, uint8_t *data, size_t len, bool final)
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
        Serial.printf("Unknown: %s\n\r", fname);
        return request->send(400, "text/html", "<b>Error:</b> Unknown file type");
      }
    }
    else
    {
      Serial.printf("Unknown: %s\n\r", fname);
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

        Serial.printf("bytesSentMsg: %s\n\r", bytesSentMsg);

        char statusMsg[200] = {'\0'};
        stringRecord(statusMsg, "firmwareUploadStatus", bytesSentMsg); //Convert to "firmwareUploadMsg,11214 bytes sent,"

        Serial.printf("msg: %s\n\r", statusMsg);
        ws.textAll(statusMsg);
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
      ws.textAll("firmwareUploadComplete,1,");
      Serial.println("Firmware update complete. Restarting");
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
    char settingsCSV[AP_CONFIG_SETTING_SIZE];
    memset(settingsCSV, 0, sizeof(settingsCSV));
    createSettingsString(settingsCSV);
    log_d("Sending command: %s\n\r", settingsCSV);
    client->text(settingsCSV);
    wifiState = WIFI_CONNECTED;
  }
  else if (type == WS_EVT_DISCONNECT) {
    log_d("Websocket client disconnected");
    wifiState = WIFI_NOTCONNECTED;
  }
  else if (type == WS_EVT_DATA) {
    for (int i = 0; i < len; i++) {
      incomingSettings[incomingSettingsSpot++] = data[i];
    }
    timeSinceLastIncomingSetting = millis();
  }
}
#endif
#endif

//Create a csv string with current settings
void createSettingsString(char* settingsCSV)
{
#ifdef COMPILE_AP
  //System Info
  stringRecord(settingsCSV, "platformPrefix", platformPrefix);

  char apRtkFirmwareVersion[50];
  sprintf(apRtkFirmwareVersion, "RTK %s Firmware: v%d.%d-%s", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
  stringRecord(settingsCSV, "rtkFirmwareVersion", apRtkFirmwareVersion);

  char apZedPlatform[50];
  if (zedModuleType == PLATFORM_F9P)
    strcpy(apZedPlatform, "ZED-F9P");
  else if (zedModuleType == PLATFORM_F9R)
    strcpy(apZedPlatform, "ZED-F9R");

  char apZedFirmwareVersion[50];
  sprintf(apZedFirmwareVersion, "%s Firmware: %s", apZedPlatform, zedFirmwareVersion);
  stringRecord(settingsCSV, "zedFirmwareVersion", apZedFirmwareVersion);

  //GNSS Config
  stringRecord(settingsCSV, "measurementRateHz", 1000.0 / settings.measurementRate, 2); //2 = decimals to print
  stringRecord(settingsCSV, "dynamicModel", settings.dynamicModel);
  stringRecord(settingsCSV, "ubxConstellationsGPS", settings.ubxConstellations[0].enabled); //GPS
  stringRecord(settingsCSV, "ubxConstellationsSBAS", settings.ubxConstellations[1].enabled); //SBAS
  stringRecord(settingsCSV, "ubxConstellationsGalileo", settings.ubxConstellations[2].enabled); //Galileo
  stringRecord(settingsCSV, "ubxConstellationsBeiDou", settings.ubxConstellations[3].enabled); //BeiDou
  stringRecord(settingsCSV, "ubxConstellationsGLONASS", settings.ubxConstellations[5].enabled); //GLONASS
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    stringRecord(settingsCSV, settings.ubxMessages[x].msgTextName, settings.ubxMessages[x].msgRate);

  //Base Config
  stringRecord(settingsCSV, "baseTypeSurveyIn", !settings.fixedBase);
  stringRecord(settingsCSV, "baseTypeFixed", settings.fixedBase);
  stringRecord(settingsCSV, "observationSeconds", settings.observationSeconds);
  stringRecord(settingsCSV, "observationPositionAccuracy", settings.observationPositionAccuracy, 2);

  if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
  {
    stringRecord(settingsCSV, "fixedBaseCoordinateTypeECEF", true);
    stringRecord(settingsCSV, "fixedBaseCoordinateTypeGeo", false);
  }
  else
  {
    stringRecord(settingsCSV, "fixedBaseCoordinateTypeECEF", false);
    stringRecord(settingsCSV, "fixedBaseCoordinateTypeGeo", true);
  }

  stringRecord(settingsCSV, "fixedEcefX", settings.fixedEcefX, 3);
  stringRecord(settingsCSV, "fixedEcefY", settings.fixedEcefY, 3);
  stringRecord(settingsCSV, "fixedEcefZ", settings.fixedEcefZ, 3);
  stringRecord(settingsCSV, "fixedLat", settings.fixedLat, 9);
  stringRecord(settingsCSV, "fixedLong", settings.fixedLong, 9);
  stringRecord(settingsCSV, "fixedAltitude", settings.fixedAltitude, 4);

  stringRecord(settingsCSV, "enableNtripServer", settings.enableNtripServer);
  stringRecord(settingsCSV, "ntripServer_CasterHost", settings.ntripServer_CasterHost);
  stringRecord(settingsCSV, "ntripServer_CasterPort", settings.ntripServer_CasterPort);
  stringRecord(settingsCSV, "ntripServer_CasterUser", settings.ntripServer_CasterUser);
  stringRecord(settingsCSV, "ntripServer_CasterUserPW", settings.ntripServer_CasterUserPW);
  stringRecord(settingsCSV, "ntripServer_MountPoint", settings.ntripServer_MountPoint);
  stringRecord(settingsCSV, "ntripServer_MountPointPW", settings.ntripServer_MountPointPW);
  stringRecord(settingsCSV, "ntripServer_wifiSSID", settings.ntripServer_wifiSSID);
  stringRecord(settingsCSV, "ntripServer_wifiPW", settings.ntripServer_wifiPW);

  stringRecord(settingsCSV, "enableNtripClient", settings.enableNtripClient);
  stringRecord(settingsCSV, "ntripClient_CasterHost", settings.ntripClient_CasterHost);
  stringRecord(settingsCSV, "ntripClient_CasterPort", settings.ntripClient_CasterPort);
  stringRecord(settingsCSV, "ntripClient_CasterUser", settings.ntripClient_CasterUser);
  stringRecord(settingsCSV, "ntripClient_CasterUserPW", settings.ntripClient_CasterUserPW);
  stringRecord(settingsCSV, "ntripClient_MountPoint", settings.ntripClient_MountPoint);
  stringRecord(settingsCSV, "ntripClient_MountPointPW", settings.ntripClient_MountPointPW);
  stringRecord(settingsCSV, "ntripClient_wifiSSID", settings.ntripClient_wifiSSID);
  stringRecord(settingsCSV, "ntripClient_wifiPW", settings.ntripClient_wifiPW);
  stringRecord(settingsCSV, "ntripClient_TransmitGGA", settings.ntripClient_TransmitGGA);

  //Sensor Fusion Config
  stringRecord(settingsCSV, "enableSensorFusion", settings.enableSensorFusion);
  stringRecord(settingsCSV, "autoIMUmountAlignment", settings.autoIMUmountAlignment);

  //System Config
  stringRecord(settingsCSV, "enableLogging", settings.enableLogging);
  stringRecord(settingsCSV, "maxLogTime_minutes", settings.maxLogTime_minutes);
  stringRecord(settingsCSV, "maxLogLength_minutes", settings.maxLogLength_minutes);

  stringRecord(settingsCSV, "sdFreeSpaceMB", sdFreeSpaceMB);
  stringRecord(settingsCSV, "sdUsedSpaceMB", sdUsedSpaceMB);

  stringRecord(settingsCSV, "enableResetDisplay", settings.enableResetDisplay);

  //Turn on SD display block last
  stringRecord(settingsCSV, "sdMounted", online.microSD);

  //Port Config
  stringRecord(settingsCSV, "dataPortBaud", settings.dataPortBaud);
  stringRecord(settingsCSV, "radioPortBaud", settings.radioPortBaud);
  stringRecord(settingsCSV, "dataPortChannel", settings.dataPortChannel);

  //L-Band
  char hardwareID[13];
  sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", unitMACAddress[0], unitMACAddress[1], unitMACAddress[2], unitMACAddress[3], unitMACAddress[4], unitMACAddress[5]); //Get ready for JSON
  stringRecord(settingsCSV, "hardwareID", hardwareID);

  char apDaysRemaining[20];
  if (strlen(settings.pointPerfectCurrentKey) > 0)
  {
    uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
    sprintf(apDaysRemaining, "%d", daysRemaining);
  }
  else
    sprintf(apDaysRemaining, "No Keys");

  stringRecord(settingsCSV, "daysRemaining", apDaysRemaining);

  stringRecord(settingsCSV, "pointPerfectDeviceProfileToken", settings.pointPerfectDeviceProfileToken);
  stringRecord(settingsCSV, "enablePointPerfectCorrections", settings.enablePointPerfectCorrections);
  stringRecord(settingsCSV, "home_wifiSSID", settings.home_wifiSSID);
  stringRecord(settingsCSV, "home_wifiPW", settings.home_wifiPW);
  stringRecord(settingsCSV, "autoKeyRenewal", settings.autoKeyRenewal);

  //External PPS/Triggers
  stringRecord(settingsCSV, "enableExternalPulse", settings.enableExternalPulse);
  stringRecord(settingsCSV, "externalPulseTimeBetweenPulse_us", settings.externalPulseTimeBetweenPulse_us);
  stringRecord(settingsCSV, "externalPulseLength_us", settings.externalPulseLength_us);
  stringRecord(settingsCSV, "externalPulsePolarity", settings.externalPulsePolarity);
  stringRecord(settingsCSV, "enableExternalHardwareEventLogging", settings.enableExternalHardwareEventLogging);

  //Profiles
  char apProfileName[50];
  sprintf(apProfileName, "Profile Name: %s", settings.profileName);
  stringRecord(settingsCSV, "profileName", apProfileName);

  //New settings not yet integrated
  //...

  strcat(settingsCSV, "\0");
  Serial.printf("settingsCSV len: %d\n\r", strlen(settingsCSV));
  Serial.printf("settingsCSV: %s\n\r", settingsCSV);
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
  {
    newAPSettings = true; //Mark settings as new to force record before reset
    settings.maxLogTime_minutes = settingValue;
  }

  else if (strcmp(settingName, "maxLogLength_minutes") == 0)
    settings.maxLogLength_minutes = settingValue;
  else if (strcmp(settingName, "measurementRateHz") == 0)
    settings.measurementRate = (int)(1000.0 / settingValue);
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
    strcpy(settings.profileName, settingValueStr);

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
  else if (strcmp(settingName, "ntripServer_wifiSSID") == 0)
    strcpy(settings.ntripServer_wifiSSID, settingValueStr);
  else if (strcmp(settingName, "ntripServer_wifiPW") == 0)
    strcpy(settings.ntripServer_wifiPW, settingValueStr);

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
  else if (strcmp(settingName, "ntripClient_wifiSSID") == 0)
    strcpy(settings.ntripClient_wifiSSID, settingValueStr);
  else if (strcmp(settingName, "ntripClient_wifiPW") == 0)
    strcpy(settings.ntripClient_wifiPW, settingValueStr);
  else if (strcmp(settingName, "ntripClient_TransmitGGA") == 0)
    settings.ntripClient_TransmitGGA = settingValueBool;
  else if (strcmp(settingName, "serialTimeoutGNSS") == 0)
    settings.serialTimeoutGNSS = settingValue;
  else if (strcmp(settingName, "pointPerfectDeviceProfileToken") == 0)
    strcpy(settings.pointPerfectDeviceProfileToken, settingValueStr);
  else if (strcmp(settingName, "enablePointPerfectCorrections") == 0)
    settings.enablePointPerfectCorrections = settingValueBool;
  else if (strcmp(settingName, "home_wifiSSID") == 0)
    strcpy(settings.home_wifiSSID, settingValueStr);
  else if (strcmp(settingName, "home_wifiPW") == 0)
    strcpy(settings.home_wifiPW, settingValueStr);
  else if (strcmp(settingName, "autoKeyRenewal") == 0)
    settings.autoKeyRenewal = settingValueBool;

  //Unused variables - read to avoid errors
  else if (strcmp(settingName, "measurementRateSec") == 0) {}
  else if (strcmp(settingName, "baseTypeSurveyIn") == 0) {}
  else if (strcmp(settingName, "fixedBaseCoordinateTypeGeo") == 0) {}
  else if (strcmp(settingName, "saveToArduino") == 0) {}
  else if (strcmp(settingName, "enableFactoryDefaults") == 0) {}
  else if (strcmp(settingName, "enableFirmwareUpdate") == 0) {}

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
    if (newAPSettings == true) recordSystemSettings(); //If we've recieved settings, record before restart

    ESP.restart();
  }

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
      Serial.printf("Unknown '%s': %0.3lf\n\r", settingName, settingValue);
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
  sprintf(record, "%s,%ld,", id, settingValue);
  strcat(settingsCSV, record);
}

//Break CSV into setting constituents
//Can't use strtok because we may have two commas next to each other, ie measurementRateHz,4.00,measurementRateSec,,dynamicModel,0,
bool parseIncomingSettings()
{
  char settingName[50] = {'\0'};
  char valueStr[50] = {'\0'}; //firmwareFileName,RTK_Surveyor_Firmware_v14.bin,

  char* commaPtr = incomingSettings;
  char* headPtr = incomingSettings;
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

    log_d("settingName: %s value: %s", settingName, valueStr);

    updateSettingWithValue(settingName, valueStr);
  }

  return (true);
}
