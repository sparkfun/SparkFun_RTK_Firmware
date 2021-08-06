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
    sdFreeSpaceMB *= sd.vol()->blocksPerCluster() / 2;
    sdFreeSpaceMB /= 1024;

    sdUsedSpaceMB = sdCardSizeMB - sdFreeSpaceMB; //Don't think of it as used, think of it as unusable

    //  Serial.print("Card Size(MB): ");
    //  Serial.println(sdCardSizeMB);
    Serial.print("Free space(MB): ");
    Serial.println(sdFreeSpaceMB);
    Serial.print("Used space(MB): ");
    Serial.println(sdUsedSpaceMB);
  }
  
  WiFi.mode(WIFI_AP);

  IPAddress local_IP(192, 168, 1, 1); //Set static IP to match OLED width
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

  //Clear any garbage from settings array
  memset(incomingSettings, 0, sizeof(incomingSettings));

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/src/main.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/javascript", main_js);
  });

  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", bootstrap_css, sizeof(bootstrap_css));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", bootstrap_min_js, sizeof(bootstrap_min_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/src/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", jquery_js, sizeof(jquery_js));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", favicon_ico, sizeof(favicon_ico));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.begin();
#endif
}

//Events triggered by web sockets
#ifdef COMPILE_WIFI
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) {
    char settingsCSV[2500];
    memset(settingsCSV, 0, sizeof(settingsCSV));
    createSettingsString(settingsCSV);
    //Serial.printf("Sending command: %s\n\r", settingsCSV);
    client->text(settingsCSV);
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Websocket client disconnected");
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
  //System Type/Page Title
  stringRecord(settingsCSV, "platformPrefix", platformPrefix);

  //GNSS Config
  stringRecord(settingsCSV, "measurementRateHz", 1000.0 / settings.measurementRate, 2); //2 = decimals to print
  stringRecord(settingsCSV, "dynamicModel", settings.dynamicModel);
  stringRecord(settingsCSV, "ubxConstellationsGPS", ubxConstellations[0].enabled); //GPS
  stringRecord(settingsCSV, "ubxConstellationsSBAS", ubxConstellations[1].enabled); //SBAS
  stringRecord(settingsCSV, "ubxConstellationsGalileo", ubxConstellations[2].enabled); //Galileo
  stringRecord(settingsCSV, "ubxConstellationsBeiDou", ubxConstellations[3].enabled); //BeiDou
  stringRecord(settingsCSV, "ubxConstellationsGLONASS", ubxConstellations[5].enabled); //GLONASS
  for (int x = 0 ; x < MAX_UBX_MSG ; x++)
    stringRecord(settingsCSV, ubxMessages[x].msgTextName, ubxMessages[x].msgRate);

  //Base Config
  stringRecord(settingsCSV, "baseTypeSurveyIn", !settings.fixedBase);
  stringRecord(settingsCSV, "baseTypeFixed", settings.fixedBase);
  stringRecord(settingsCSV, "observationSeconds", settings.observationSeconds);
  stringRecord(settingsCSV, "observationPositionAccuracy", settings.observationPositionAccuracy, 2);
  stringRecord(settingsCSV, "fixedBaseCoordinateTypeECEF", settings.fixedBaseCoordinateType);
  stringRecord(settingsCSV, "fixedEcefX", settings.fixedEcefX, 3);
  stringRecord(settingsCSV, "fixedEcefY", settings.fixedEcefY, 3);
  stringRecord(settingsCSV, "fixedEcefZ", settings.fixedEcefZ, 3);
  stringRecord(settingsCSV, "fixedBaseCoordinateTypeGeo", !settings.fixedBaseCoordinateType);
  stringRecord(settingsCSV, "fixedLat", settings.fixedLat, 9);
  stringRecord(settingsCSV, "fixedLong", settings.fixedLong, 9);
  stringRecord(settingsCSV, "fixedAltitude", settings.fixedAltitude, 4);

  stringRecord(settingsCSV, "enableNtripServer", settings.enableNtripServer);
  stringRecord(settingsCSV, "wifiSSID", settings.wifiSSID);
  stringRecord(settingsCSV, "wifiPW", settings.wifiPW);
  stringRecord(settingsCSV, "casterHost", settings.casterHost);
  stringRecord(settingsCSV, "casterPort", settings.casterPort);
  stringRecord(settingsCSV, "mountPoint", settings.mountPoint);
  stringRecord(settingsCSV, "mountPointPW", settings.mountPointPW);

  //System Config
  stringRecord(settingsCSV, "enableLogging", settings.enableLogging);
  stringRecord(settingsCSV, "maxLogTime_minutes", settings.maxLogTime_minutes);

  stringRecord(settingsCSV, "sdFreeSpaceMB", sdFreeSpaceMB);
  stringRecord(settingsCSV, "sdUsedSpaceMB", sdUsedSpaceMB);

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
  else if (strcmp(settingName, "mountPoint") == 0)
    strcpy(settings.mountPoint, settingValueStr);
  else if (strcmp(settingName, "mountPointPW") == 0)
    strcpy(settings.mountPointPW, settingValueStr);
  else if (strcmp(settingName, "wifiSSID") == 0)
    strcpy(settings.wifiSSID, settingValueStr);
  else if (strcmp(settingName, "wifiPW") == 0)
    strcpy(settings.wifiPW, settingValueStr);
  else if (strcmp(settingName, "enableLogging") == 0)
    settings.enableLogging = settingValueBool;
  else if (strcmp(settingName, "dataPortChannel") == 0)
    settings.dataPortChannel = (muxConnectionType_e)settingValue;

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
  }
  else if (strcmp(settingName, "factoryDefaultReset") == 0)
  {
    factoryReset();
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
        sprintf(tempString, "ubxConstellations%s", ubxConstellations[x].textName);

        if (strcmp(settingName, tempString) == 0)
        {
          ubxConstellations[x].enabled = settingValueBool;
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
        if (strcmp(settingName, ubxMessages[x].msgTextName) == 0)
        {
          ubxMessages[x].msgRate = settingValue;
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
bool parseIncomingSettings()
{
  char settingName[50];
  char valueStr[50]; //firmwareFileName,RTK_Surveyor_Firmware_v14.bin,
  char *ptr = strtok(incomingSettings, ","); //measurementRateHz,2.00,
  for (int x = 0 ; ptr != NULL ; x++)
  {
    strcpy(settingName, ptr);
    ptr = strtok(NULL, ","); //Move to next comma
    if (ptr == NULL) break;

    strcpy(valueStr, ptr);
    ptr = strtok(NULL, ","); //Move to next comma

    //Serial.printf("settingName: %s value: %s\n\r", settingName, valueStr);
    updateSettingWithValue(settingName, valueStr);
  }

  return (true);
}
