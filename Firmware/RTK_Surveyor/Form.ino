//Once connected to the access point for WiFi Config, the ESP32 sends current setting values in one long string to websocket
//After user clicks 'save', data is validated via main.js and a long string of values is returned.

//Start webserver in AP mode
void startConfigAP()
{
  stopBluetooth();
  stopUART2Tasks(); //Delete F9 serial tasks if running

#ifdef COMPILE_WIFI

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_init_config); //Restart WiFi resources

  //Check SD Size
  if (online.microSD)
  {
    csd_t csd;
    sd.card()->readCSD(&csd); //Card Specific Data
    sdCardSizeMB = 0.000512 * sdCardCapacity(&csd);
    sd.volumeBegin();

    //Find available cluster/space
    sdFreeSpaceMB = sd.vol()->freeClusterCount(); //This takes a few seconds to complete
    sdFreeSpaceMB *= sd.vol()->sectorsPerCluster() / 2;
    sdFreeSpaceMB /= 1024;

    sdUsedSpaceMB = sdCardSizeMB - sdFreeSpaceMB; //Don't think of it as used, think of it as unusable

    //  Serial.print("Card Size(MB): ");
    //  Serial.println(sdCardSizeMB);
    Serial.print("Free space(MB): ");
    Serial.println(sdFreeSpaceMB);
    Serial.print("Used space(MB): ");
    Serial.println(sdUsedSpaceMB);
  }

  //When testing, operate on local WiFi instead of AP
  //#define LOCAL_WIFI_TESTING 1

#ifndef LOCAL_WIFI_TESTING
  //Start in AP mode
  WiFi.mode(WIFI_AP);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  if (WiFi.softAP("RTK Config") == false) //Must be short enough to fit OLED Width
  {
    Serial.println(F("AP failed to start"));
    return;
  }
  Serial.print(F("AP Started with IP: "));
  Serial.println(WiFi.softAPIP());
#endif

#ifdef LOCAL_WIFI_TESTING
  //Connect to local router
#define WIFI_SSID "TRex"
#define WIFI_PASSWORD "parachutes"
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
#endif

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

  //Handler for the /update form POST
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  }, handleFirmwareFileUpload);

  server.begin();
#endif

  radioState = WIFI_ON_NOCONNECTION;
}

//Handler for firmware file upload
#ifdef COMPILE_WIFI
static void handleFirmwareFileUpload(AsyncWebServerRequest *request, String fileName, size_t index, uint8_t *data, size_t len, bool final)
{
  if (online.microSD == false)
  {
    Serial.println(F("No SD card available"));
    return;
  }

  //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
  if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) != pdPASS) {
    Serial.println(F("Failed to get file system lock on firmware file"));
    return;
  }

  if (!index) {
    //Convert string to array
    char tempFileName[100];
    fileName.toCharArray(tempFileName, sizeof(tempFileName));

    if (sd.exists(tempFileName))
    {
      sd.remove(tempFileName);
      Serial.printf("Removed old firmware file: %s\n\r", tempFileName);
    }

    Serial.printf("Start Firmware Upload: %s\n\r", tempFileName);

    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (newFirmwareFile.open(tempFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.printf("Failed to create firmware file: %s\n\r", tempFileName);
      xSemaphoreGive(xFATSemaphore);
      return;
    }
  }

  //Record to file
  if (newFirmwareFile.write(data, len) != len)
    log_e("Error writing to firmware file");
  else
    log_d("Recorded %d bytes to file\n\r", len);

  if (final)
  {
    updateDataFileCreate(&newFirmwareFile); // Update the file create time & date
    newFirmwareFile.close();

    Serial.print("Upload complete: ");
    Serial.println(fileName);

    binCount = 0;
    xSemaphoreGive(xFATSemaphore); //Must release semaphore before scanning for firmware
    scanForFirmware(); //Update firmware file list

    //Reload page upon success - this will show all available firmware files
    request->send(200, "text/html", "<meta http-equiv=\"Refresh\" content=\"0; url='/'\" />");
  }

  xSemaphoreGive(xFATSemaphore);
}
#endif

//Events triggered by web sockets
#ifdef COMPILE_WIFI
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) {
    char settingsCSV[3000];
    memset(settingsCSV, 0, sizeof(settingsCSV));
    createSettingsString(settingsCSV);
    log_d("Sending command: %s\n\r", settingsCSV);
    client->text(settingsCSV);
    radioState = WIFI_CONNECTED;
  }
  else if (type == WS_EVT_DISCONNECT) {
    log_d("Websocket client disconnected");
    radioState = WIFI_ON_NOCONNECTION;
  }
  else if (type == WS_EVT_DATA) {
    for (int i = 0; i < len; i++) {
      incomingSettings[incomingSettingsSpot++] = data[i];
    }
    timeSinceLastIncomingSetting = millis();
  }
}
#endif

//Create a csv string with current settings
void createSettingsString(char* settingsCSV)
{
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
  stringRecord(settingsCSV, "fixedBaseCoordinateTypeECEF", !settings.fixedBaseCoordinateType); //COORD_TYPE_ECEF = 0
  stringRecord(settingsCSV, "fixedEcefX", settings.fixedEcefX, 3);
  stringRecord(settingsCSV, "fixedEcefY", settings.fixedEcefY, 3);
  stringRecord(settingsCSV, "fixedEcefZ", settings.fixedEcefZ, 3);
  stringRecord(settingsCSV, "fixedBaseCoordinateTypeGeo", settings.fixedBaseCoordinateType);
  stringRecord(settingsCSV, "fixedLat", settings.fixedLat, 9);
  stringRecord(settingsCSV, "fixedLong", settings.fixedLong, 9);
  stringRecord(settingsCSV, "fixedAltitude", settings.fixedAltitude, 4);

  stringRecord(settingsCSV, "enableNtripServer", settings.enableNtripServer);
  stringRecord(settingsCSV, "casterHost", settings.casterHost);
  stringRecord(settingsCSV, "casterPort", settings.casterPort);
  stringRecord(settingsCSV, "casterUser", settings.casterUser);
  stringRecord(settingsCSV, "casterUserPW", settings.casterUserPW);
  stringRecord(settingsCSV, "mountPointUpload", settings.mountPointUpload);
  stringRecord(settingsCSV, "mountPointUploadPW", settings.mountPointUploadPW);
  stringRecord(settingsCSV, "mountPointDownload", settings.mountPointDownload);
  stringRecord(settingsCSV, "mountPointDownloadPW", settings.mountPointDownloadPW);
  stringRecord(settingsCSV, "casterTransmitGGA", settings.casterTransmitGGA);
  stringRecord(settingsCSV, "wifiSSID", settings.wifiSSID);
  stringRecord(settingsCSV, "wifiPW", settings.wifiPW);

  //Sensor Fusion Config
  stringRecord(settingsCSV, "enableSensorFusion", settings.enableSensorFusion);
  stringRecord(settingsCSV, "autoIMUmountAlignment", settings.autoIMUmountAlignment);

  //System Config
  stringRecord(settingsCSV, "enableLogging", settings.enableLogging);
  stringRecord(settingsCSV, "maxLogTime_minutes", settings.maxLogTime_minutes);

  stringRecord(settingsCSV, "sdFreeSpaceMB", sdFreeSpaceMB);
  stringRecord(settingsCSV, "sdUsedSpaceMB", sdUsedSpaceMB);

  stringRecord(settingsCSV, "enableResetDisplay", settings.enableResetDisplay);

  //Pass any available firmware file names
  if (binCount > 0)
  {
    for (int x = 0 ; x < binCount ; x++)
    {
      char firmwareFileID[50];
      sprintf(firmwareFileID, "firmwareFileName%d", x);
      stringRecord(settingsCSV, firmwareFileID, binFileNames[x]);
    }
  }

  //Turn on SD display block last
  stringRecord(settingsCSV, "sdMounted", online.microSD);

  //Port Config
  stringRecord(settingsCSV, "dataPortBaud", settings.dataPortBaud);
  stringRecord(settingsCSV, "radioPortBaud", settings.radioPortBaud);
  stringRecord(settingsCSV, "dataPortChannel", settings.dataPortChannel);

  strcat(settingsCSV, "\0");
  Serial.printf("settingsCSV len: %d\n\r", strlen(settingsCSV));

  //Upon AP load, Survey In is always checked.
  //Sometimes, (perhaps if fixed should be checked) fixed area is left enabled. Check main.js for correct disable of baseTypeFixed if fixedBase = false


  //Is baseTypeSurveyIn 1 or 0
  Serial.printf("settingsCSV: %s\n\r", settingsCSV);
}

//Given a settingName, and string value, update a given setting
void updateSettingWithValue(const char *settingName, const char* settingValueStr)
{
  char* ptr;
  double settingValue = strtod(settingValueStr, &ptr);

  bool settingValueBool = false;
  if (strcmp(settingValueStr, "true") == 0) settingValueBool = true;

  if (strcmp(settingName, "maxLogTime_minutes") == 0)
    settings.maxLogTime_minutes = settingValue;
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
    settings.fixedBaseCoordinateType = settingValueBool;
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
  else if (strcmp(settingName, "enableNtripServer") == 0)
    settings.enableNtripServer = settingValueBool;
  else if (strcmp(settingName, "casterHost") == 0)
    strcpy(settings.casterHost, settingValueStr);
  else if (strcmp(settingName, "casterPort") == 0)
    settings.casterPort = settingValue;
  else if (strcmp(settingName, "casterUser") == 0)
    strcpy(settings.casterUser, settingValueStr);
  else if (strcmp(settingName, "casterUserPW") == 0)
    strcpy(settings.casterUserPW, settingValueStr);
  else if (strcmp(settingName, "mountPointUpload") == 0)
    strcpy(settings.mountPointUpload, settingValueStr);
  else if (strcmp(settingName, "mountPointUploadPW") == 0)
    strcpy(settings.mountPointUploadPW, settingValueStr);
  else if (strcmp(settingName, "mountPointDownload") == 0)
    strcpy(settings.mountPointDownload, settingValueStr);
  else if (strcmp(settingName, "mountPointDownloadPW") == 0)
    strcpy(settings.mountPointDownloadPW, settingValueStr);
  else if (strcmp(settingName, "casterTransmitGGA") == 0)
    settings.casterTransmitGGA = settingValueBool;
  else if (strcmp(settingName, "wifiSSID") == 0)
    strcpy(settings.wifiSSID, settingValueStr);
  else if (strcmp(settingName, "wifiPW") == 0)
    strcpy(settings.wifiPW, settingValueStr);
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
    updateFromSD(settingValueStr);
    requestChangeState(STATE_ROVER_NOT_STARTED); //If update failed, return to Rover mode.
  }
  else if (strcmp(settingName, "factoryDefaultReset") == 0)
    factoryReset();
  else if (strcmp(settingName, "exitToRoverMode") == 0)
  {
    ESP.restart();
    //requestChangeState(STATE_ROVER_NOT_STARTED);
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

    Serial.printf("settingName: %s value: %s\n\r", settingName, valueStr);

    //Ignore zero length values (measurementRateSec) received from browser
    if (strlen(valueStr) > 0)
      updateSettingWithValue(settingName, valueStr);
  }

  return (true);
}
