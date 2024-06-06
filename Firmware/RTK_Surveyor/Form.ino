/*------------------------------------------------------------------------------
Form.ino

  Start and stop the web-server, provide the form and handle browser input.
------------------------------------------------------------------------------*/

#ifdef COMPILE_AP

// Once connected to the access point for WiFi Config, the ESP32 sends current setting values in one long string to
// websocket After user clicks 'save', data is validated via main.js and a long string of values is returned.

bool websocketConnected = false;

class CaptiveRequestHandler : public AsyncWebHandler
{
  public:
    // https://en.wikipedia.org/wiki/Captive_portal
    String urls[5] = {"/hotspot-detect.html", "/library/test/success.html", "/generate_204", "/ncsi.txt",
                      "/check_network_status.txt"};
    CaptiveRequestHandler()
    {
    }
    virtual ~CaptiveRequestHandler()
    {
    }

    bool canHandle(AsyncWebServerRequest *request)
    {
        for (int i = 0; i < 5; i++)
        {
            if (request->url().equals(urls[i]))
                return true;
        }
        return false;
    }

    // Provide a custom small site for redirecting the user to the config site
    // HTTP redirect does not work and the relative links on the default config site do not work, because the phone is
    // requesting a different server
    void handleRequest(AsyncWebServerRequest *request)
    {
        String logmessage = "Captive Portal Client:" + request->client()->remoteIP().toString() + " " + request->url();
        systemPrintln(logmessage);
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>RTK Config</title></head><body>");
        response->print("<div class='container'>");
        response->printf("<div align='center' class='col-sm-12'><img src='http://%s/src/rtk-setup.png' alt='SparkFun "
                         "RTK WiFi Setup'></div>",
                         WiFi.softAPIP().toString().c_str());
        response->printf("<div align='center'><h3>Configure your RTK receiver <a href='http://%s/'>here</a></h3></div>",
                         WiFi.softAPIP().toString().c_str());
        response->print("</div></body></html>");
        request->send(response);
    }
};

// Start webserver in AP mode
bool startWebServer(bool startWiFi = true, int httpPort = 80)
{
    do
    {
        ntripClientStop(true); // Do not allocate new wifiClient
        for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            ntripServerStop(serverIndex, true); // Do not allocate new wifiClient

        if (startWiFi)
            if (wifiStartAP() == false) // Exits calling wifiConnect()
                break;

        if (settings.mdnsEnable == true)
        {
            if (MDNS.begin("rtk") == false) // This should make the module findable from 'rtk.local' in browser
                systemPrintln("Error setting up MDNS responder!");
            else
                MDNS.addService("http", "tcp", 80); // Add service to MDNS-SD
        }

        incomingSettings = (char *)malloc(AP_CONFIG_SETTING_SIZE);
        if (!incomingSettings)
        {
            systemPrintln("ERROR: Failed to allocate incomingSettings");
            break;
        }
        memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

        // Pre-load settings CSV
        settingsCSV = (char *)malloc(AP_CONFIG_SETTING_SIZE);
        if (!settingsCSV)
        {
            systemPrintln("ERROR: Failed to allocate settingsCSV");
            break;
        }
        createSettingsString(settingsCSV);

        webserver = new AsyncWebServer(httpPort);
        if (!webserver)
        {
            systemPrintln("ERROR: Failed to allocate webserver");
            break;
        }
        websocket = new AsyncWebSocket("/ws");
        if (!websocket)
        {
            systemPrintln("ERROR: Failed to allocate websocket");
            break;
        }

        if (settings.enableCaptivePortal == true)
            webserver->addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP

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
        // * /listMessages responds with a CSV of messages supported by this platform
        // * /listMessagesBase responds with a CSV of RTCM Base messages supported by this platform
        // * /file allows the download or deletion of a file

        webserver->onNotFound(notFound);

        webserver->onFileUpload(
            handleUpload); // Run handleUpload function when any file is uploaded. Must be before server.on() calls.

        webserver->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/html", index_html, sizeof(index_html));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/plain", favicon_ico, sizeof(favicon_ico));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", bootstrap_bundle_min_js,
                                                                        sizeof(bootstrap_bundle_min_js));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/css", bootstrap_min_css, sizeof(bootstrap_min_css));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/javascript", bootstrap_min_js, sizeof(bootstrap_min_js));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/javascript", jquery_js, sizeof(jquery_js));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/javascript", main_js, sizeof(main_js));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/rtk-setup.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response;
            if (productVariant == REFERENCE_STATION)
                response = request->beginResponse_P(200, "image/png", rtkSetup_png, sizeof(rtkSetup_png));
            else
                response = request->beginResponse_P(200, "image/png", rtkSetupWiFi_png, sizeof(rtkSetupWiFi_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        // Battery icons
        webserver->on("/src/BatteryBlank.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", batteryBlank_png, sizeof(batteryBlank_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery0.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery0_png, sizeof(battery0_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery1.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery1_png, sizeof(battery1_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery2.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery2_png, sizeof(battery2_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery3.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery3_png, sizeof(battery3_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/Battery0_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery0_Charging_png, sizeof(battery0_Charging_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery1_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery1_Charging_png, sizeof(battery1_Charging_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery2_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery2_Charging_png, sizeof(battery2_Charging_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
        webserver->on("/src/Battery3_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "image/png", battery3_Charging_png, sizeof(battery3_Charging_png));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", style_css, sizeof(style_css));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/fonts/icomoon.eot", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/plain", icomoon_eot, sizeof(icomoon_eot));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/fonts/icomoon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/plain", icomoon_svg, sizeof(icomoon_svg));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/fonts/icomoon.ttf", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/plain", icomoon_ttf, sizeof(icomoon_ttf));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        webserver->on("/src/fonts/icomoon.woof", HTTP_GET, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse_P(200, "text/plain", icomoon_woof, sizeof(icomoon_woof));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

        // Handler for the /upload form POST
        webserver->on(
            "/upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, handleFirmwareFileUpload);

        // Handler for file manager
        webserver->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest *request) {
            String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
            systemPrintln(logmessage);
            String files;
            getFileList(files);
            request->send(200, "text/plain", files);
        });

        // Handler for supported messages list
        webserver->on("/listMessages", HTTP_GET, [](AsyncWebServerRequest *request) {
            String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
            systemPrintln(logmessage);
            String messages;
            createMessageList(messages);
            request->send(200, "text/plain", messages);
        });

        // Handler for supported RTCM/Base messages list
        webserver->on("/listMessagesBase", HTTP_GET, [](AsyncWebServerRequest *request) {
            String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
            systemPrintln(logmessage);
            String messageList;
            createMessageListBase(messageList);
            request->send(200, "text/plain", messageList);
        });

        // Handler for file manager
        webserver->on("/file", HTTP_GET, [](AsyncWebServerRequest *request) { handleFileManager(request); });

        webserver->begin();

        if (settings.debugWiFiConfig == true)
            systemPrintln("Web Server Started");
        reportHeapNow(false);
        return true;
    } while (0);

    // Release the resources
    stopWebServer();
    return false;
}

void stopWebServer()
{
    if (webserver != nullptr)
    {
        webserver->end();
        free(webserver);
        webserver = nullptr;
    }

    if (websocket != nullptr)
    {
        delete websocket;
        websocket = nullptr;
    }

    if (settingsCSV != nullptr)
    {
        free(settingsCSV);
        settingsCSV = nullptr;
    }

    if (incomingSettings != nullptr)
    {
        free(incomingSettings);
        incomingSettings = nullptr;
    }

    if (settings.debugWiFiConfig == true)
        systemPrintln("Web Server Stopped");
    reportHeapNow(false);
}

void notFound(AsyncWebServerRequest *request)
{
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    systemPrintln(logmessage);
    request->send(404, "text/plain", "Not found");
}

// Handler for firmware file downloads
static void handleFileManager(AsyncWebServerRequest *request)
{
    // This section does not tolerate semaphore transactions
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (request->hasParam("name") && request->hasParam("action"))
    {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() +
                     "?name=" + String(fileName) + "&action=" + String(fileAction);

        char slashFileName[60];
        snprintf(slashFileName, sizeof(slashFileName), "/%s", request->getParam("name")->value().c_str());

        bool fileExists;
        if (USE_SPI_MICROSD)
        {
            fileExists = sd->exists(slashFileName);
        }
#ifdef COMPILE_SD_MMC
        else
        {
            fileExists = SD_MMC.exists(slashFileName);
        }
#endif // COMPILE_SD_MMC

        if (fileExists == false)
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
                    // Allocate the managerTempFile
                    if (!managerTempFile)
                    {
                        managerTempFile = new FileSdFatMMC;
                        if (!managerTempFile)
                        {
                            systemPrintln("Failed to allocate managerTempFile!");
                            return;
                        }
                    }

                    if (managerTempFile->open(slashFileName, O_READ) == true)
                        managerFileOpen = true;
                    else
                        systemPrintln("Error: File Manager failed to open file");
                }
                else
                {
                    // File is already in use. Wait your turn.
                    request->send(202, "text/plain", "ERROR: File already downloading");
                }

                int dataAvailable;
                dataAvailable = managerTempFile->size() - managerTempFile->position();

                AsyncWebServerResponse *response = request->beginResponse(
                    "text/plain", dataAvailable, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                        uint32_t bytes = 0;
                        uint32_t availableBytes;
                        availableBytes = managerTempFile->available();

                        if (availableBytes > maxLen)
                        {
                            bytes = managerTempFile->read(buffer, maxLen);
                        }
                        else
                        {
                            bytes = managerTempFile->read(buffer, availableBytes);
                            managerTempFile->close();

                            managerFileOpen = false;

                            websocket->textAll("fmNext,1,"); // Tell browser to send next file if needed
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
                if (USE_SPI_MICROSD)
                    sd->remove(slashFileName);
#ifdef COMPILE_SD_MMC
                else
                    SD_MMC.remove(slashFileName);
#endif // COMPILE_SD_MMC
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

// Handler for firmware file upload
static void handleFirmwareFileUpload(AsyncWebServerRequest *request, String fileName, size_t index, uint8_t *data,
                                     size_t len, bool final)
{
    if (!index)
    {
        // Check file name against valid firmware names
        const char *BIN_EXT = "bin";
        const char *BIN_HEADER = "RTK_Surveyor_Firmware";

        int fnameLen = fileName.length();
        char fname[fnameLen + 2] = {'/'}; // Filename must start with / or VERY bad things happen on SD_MMC
        fileName.toCharArray(&fname[1], fnameLen + 1);
        fname[fnameLen + 1] = '\0'; // Terminate array

        // Check 'bin' extension
        if (strcmp(BIN_EXT, &fname[strlen(fname) - strlen(BIN_EXT)]) == 0)
        {
            // Check for 'RTK_Surveyor_Firmware' start of file name
            if (strncmp(fname, BIN_HEADER, strlen(BIN_HEADER)) == 0)
            {
                // Begin update process
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

            // Send an update to browser every 100k
            if (binBytesSent - binBytesLastUpdate > 100000)
            {
                binBytesLastUpdate = binBytesSent;

                char bytesSentMsg[100];
                snprintf(bytesSentMsg, sizeof(bytesSentMsg), "%'d bytes sent", binBytesSent);

                systemPrintf("bytesSentMsg: %s\r\n", bytesSentMsg);

                char statusMsg[200] = {'\0'};
                stringRecord(statusMsg, "firmwareUploadStatus",
                             bytesSentMsg); // Convert to "firmwareUploadMsg,11214 bytes sent,"

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

// Events triggered by web sockets
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data,
               size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        if (settings.debugWiFiConfig == true)
            systemPrintln("Websocket client connected");
        client->text(settingsCSV);
        lastDynamicDataUpdate = millis();
        websocketConnected = true;
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        if (settings.debugWiFiConfig == true)
            systemPrintln("Websocket client disconnected");

        // User has either refreshed the page or disconnected. Recompile the current settings.
        createSettingsString(settingsCSV);
        websocketConnected = false;
    }
    else if (type == WS_EVT_DATA)
    {
        if (currentlyParsingData == false)
        {
            for (int i = 0; i < len; i++)
            {
                incomingSettings[incomingSettingsSpot++] = data[i];
                incomingSettingsSpot %= AP_CONFIG_SETTING_SIZE;
            }
            timeSinceLastIncomingSetting = millis();
        }
    }
    else
    {
        if (settings.debugWiFiConfig == true)
            systemPrintf("onWsEvent: unrecognised AwsEventType %d\r\n", type);
    }
}
// Create a csv string with current settings
void createSettingsString(char *newSettings)
{
    char tagText[32];
    char nameText[64];

    newSettings[0] = '\0'; // Erase current settings string

    // System Info
    char apPlatformPrefix[80];
    strncpy(apPlatformPrefix, platformPrefixTable[productVariant], sizeof(apPlatformPrefix));
    stringRecord(newSettings, "platformPrefix", apPlatformPrefix);

    char apRtkFirmwareVersion[86];
    getFirmwareVersion(apRtkFirmwareVersion, sizeof(apRtkFirmwareVersion), true);
    stringRecord(newSettings, "rtkFirmwareVersion", apRtkFirmwareVersion);

    if (!configureViaEthernet) // ZED type is unknown if we are in configure-via-ethernet mode
    {
        char apZedPlatform[50];
        if (zedModuleType == PLATFORM_F9P)
            strcpy(apZedPlatform, "ZED-F9P");
        else if (zedModuleType == PLATFORM_F9R)
            strcpy(apZedPlatform, "ZED-F9R");

        char apZedFirmwareVersion[80];
        snprintf(apZedFirmwareVersion, sizeof(apZedFirmwareVersion), "%s Firmware: %s ID: %s", apZedPlatform,
                 zedFirmwareVersion, zedUniqueId);
        stringRecord(newSettings, "zedFirmwareVersion", apZedFirmwareVersion);
        stringRecord(newSettings, "zedFirmwareVersionInt", zedFirmwareVersionInt);
    }
    else
    {
        char apZedFirmwareVersion[80];
        snprintf(apZedFirmwareVersion, sizeof(apZedFirmwareVersion), "ZED-F9: Unknown");
        stringRecord(newSettings, "zedFirmwareVersion", apZedFirmwareVersion);
    }

    char apDeviceBTID[30];
    snprintf(apDeviceBTID, sizeof(apDeviceBTID), "Device Bluetooth ID: %02X%02X", btMACAddress[4], btMACAddress[5]);
    stringRecord(newSettings, "deviceBTID", apDeviceBTID);

    // GNSS Config
    stringRecord(newSettings, "measurementRateHz", 1000.0 / settings.measurementRate, 2); // 2 = decimals to print
    stringRecord(newSettings, "dynamicModel", settings.dynamicModel);
    stringRecord(newSettings, "ubxConstellationsGPS", settings.ubxConstellations[0].enabled);     // GPS
    stringRecord(newSettings, "ubxConstellationsSBAS", settings.ubxConstellations[1].enabled);    // SBAS
    stringRecord(newSettings, "ubxConstellationsGalileo", settings.ubxConstellations[2].enabled); // Galileo
    stringRecord(newSettings, "ubxConstellationsBeiDou", settings.ubxConstellations[3].enabled);  // BeiDou
    stringRecord(newSettings, "ubxConstellationsGLONASS", settings.ubxConstellations[5].enabled); // GLONASS

    // Base Config
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
    for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
    {
        char name[50];
        sprintf(name, "ntripServer_%s_%d", "CasterHost", serverIndex);
        stringRecord(newSettings, name, &settings.ntripServer_CasterHost[serverIndex][0]);
        sprintf(name, "ntripServer_%s_%d", "CasterPort", serverIndex);
        stringRecord(newSettings, name, settings.ntripServer_CasterPort[serverIndex]);
        sprintf(name, "ntripServer_%s_%d", "CasterUser", serverIndex);
        stringRecord(newSettings, name, &settings.ntripServer_CasterUser[serverIndex][0]);
        sprintf(name, "ntripServer_%s_%d", "CasterUserPW", serverIndex);
        stringRecord(newSettings, name, &settings.ntripServer_CasterUserPW[serverIndex][0]);
        sprintf(name, "ntripServer_%s_%d", "MountPoint", serverIndex);
        stringRecord(newSettings, name, &settings.ntripServer_MountPoint[serverIndex][0]);
        sprintf(name, "ntripServer_%s_%d", "MountPointPW", serverIndex);
        stringRecord(newSettings, name, &settings.ntripServer_MountPointPW[serverIndex][0]);
    }

    stringRecord(newSettings, "enableNtripClient", settings.enableNtripClient);
    stringRecord(newSettings, "ntripClient_CasterHost", settings.ntripClient_CasterHost);
    stringRecord(newSettings, "ntripClient_CasterPort", settings.ntripClient_CasterPort);
    stringRecord(newSettings, "ntripClient_CasterUser", settings.ntripClient_CasterUser);
    stringRecord(newSettings, "ntripClient_CasterUserPW", settings.ntripClient_CasterUserPW);
    stringRecord(newSettings, "ntripClient_MountPoint", settings.ntripClient_MountPoint);
    stringRecord(newSettings, "ntripClient_MountPointPW", settings.ntripClient_MountPointPW);
    stringRecord(newSettings, "ntripClient_TransmitGGA", settings.ntripClient_TransmitGGA);

    // Sensor Fusion Config
    stringRecord(newSettings, "enableSensorFusion", settings.enableSensorFusion);
    stringRecord(newSettings, "autoIMUmountAlignment", settings.autoIMUmountAlignment);

    // System Config
    stringRecord(newSettings, "enableUART2UBXIn", settings.enableUART2UBXIn);
    stringRecord(newSettings, "enableLogging", settings.enableLogging);
    stringRecord(newSettings, "enableARPLogging", settings.enableARPLogging);
    stringRecord(newSettings, "ARPLoggingInterval", settings.ARPLoggingInterval_s);
    stringRecord(newSettings, "maxLogTime_minutes", settings.maxLogTime_minutes);
    stringRecord(newSettings, "maxLogLength_minutes", settings.maxLogLength_minutes);

    char sdCardSizeChar[20];
    String cardSize;
    stringHumanReadableSize(cardSize, sdCardSize);
    cardSize.toCharArray(sdCardSizeChar, sizeof(sdCardSizeChar));
    char sdFreeSpaceChar[20];
    String freeSpace;
    stringHumanReadableSize(freeSpace, sdFreeSpace);
    freeSpace.toCharArray(sdFreeSpaceChar, sizeof(sdFreeSpaceChar));

    stringRecord(newSettings, "sdFreeSpace", sdFreeSpaceChar);
    stringRecord(newSettings, "sdSize", sdCardSizeChar);

    stringRecord(newSettings, "enableResetDisplay", settings.enableResetDisplay);

    // Ethernet
    stringRecord(newSettings, "ethernetDHCP", settings.ethernetDHCP);
    char ipAddressChar[20];
    snprintf(ipAddressChar, sizeof(ipAddressChar), "%s", settings.ethernetIP.toString().c_str());
    stringRecord(newSettings, "ethernetIP", ipAddressChar);
    snprintf(ipAddressChar, sizeof(ipAddressChar), "%s", settings.ethernetDNS.toString().c_str());
    stringRecord(newSettings, "ethernetDNS", ipAddressChar);
    snprintf(ipAddressChar, sizeof(ipAddressChar), "%s", settings.ethernetGateway.toString().c_str());
    stringRecord(newSettings, "ethernetGateway", ipAddressChar);
    snprintf(ipAddressChar, sizeof(ipAddressChar), "%s", settings.ethernetSubnet.toString().c_str());
    stringRecord(newSettings, "ethernetSubnet", ipAddressChar);
    stringRecord(newSettings, "httpPort", settings.httpPort);
    stringRecord(newSettings, "ethernetNtpPort", settings.ethernetNtpPort);
    stringRecord(newSettings, "pvtClientPort", settings.pvtClientPort);
    stringRecord(newSettings, "pvtClientHost", settings.pvtClientHost);

    // Network layer
    stringRecord(newSettings, "defaultNetworkType", settings.defaultNetworkType);
    stringRecord(newSettings, "enableNetworkFailover", settings.enableNetworkFailover);

    // NTP
    stringRecord(newSettings, "ntpPollExponent", settings.ntpPollExponent);
    stringRecord(newSettings, "ntpPrecision", settings.ntpPrecision);
    stringRecord(newSettings, "ntpRootDelay", settings.ntpRootDelay);
    stringRecord(newSettings, "ntpRootDispersion", settings.ntpRootDispersion);
    stringRecord(newSettings, "ntpPollExponent", settings.ntpPollExponent);
    char ntpRefId[5];
    snprintf(ntpRefId, sizeof(ntpRefId), "%s", settings.ntpReferenceId);
    stringRecord(newSettings, "ntpReferenceId", ntpRefId);

    // Automatic firmware update settings
    stringRecord(newSettings, "enableAutoFirmwareUpdate", settings.enableAutoFirmwareUpdate);
    stringRecord(newSettings, "autoFirmwareCheckMinutes", settings.autoFirmwareCheckMinutes);

    // Turn on SD display block last
    stringRecord(newSettings, "sdMounted", online.microSD);

    // Port Config
    stringRecord(newSettings, "dataPortBaud", settings.dataPortBaud);
    stringRecord(newSettings, "radioPortBaud", settings.radioPortBaud);
    stringRecord(newSettings, "dataPortChannel", settings.dataPortChannel);

    // L-Band
    char hardwareID[13];
    snprintf(hardwareID, sizeof(hardwareID), "%02X%02X%02X%02X%02X%02X", lbandMACAddress[0], lbandMACAddress[1],
             lbandMACAddress[2], lbandMACAddress[3], lbandMACAddress[4], lbandMACAddress[5]);
    stringRecord(newSettings, "hardwareID", hardwareID);

    char apDaysRemaining[20];
    if (strlen(settings.pointPerfectCurrentKey) > 0)
    {
#ifdef COMPILE_L_BAND
        int daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
        snprintf(apDaysRemaining, sizeof(apDaysRemaining), "%d", daysRemaining);
#endif // COMPILE_L_BAND
    }
    else
        snprintf(apDaysRemaining, sizeof(apDaysRemaining), "No Keys");

    stringRecord(newSettings, "daysRemaining", apDaysRemaining);

    stringRecord(newSettings, "pointPerfectDeviceProfileToken", settings.pointPerfectDeviceProfileToken);
    stringRecord(newSettings, "enablePointPerfectCorrections", settings.enablePointPerfectCorrections);
    stringRecord(newSettings, "autoKeyRenewal", settings.autoKeyRenewal);
    stringRecord(newSettings, "geographicRegion", settings.geographicRegion);

    // External PPS/Triggers
    stringRecord(newSettings, "enableExternalPulse", settings.enableExternalPulse);
    stringRecord(newSettings, "externalPulseTimeBetweenPulse_us", settings.externalPulseTimeBetweenPulse_us);
    stringRecord(newSettings, "externalPulseLength_us", settings.externalPulseLength_us);
    stringRecord(newSettings, "externalPulsePolarity", settings.externalPulsePolarity);
    stringRecord(newSettings, "enableExternalHardwareEventLogging", settings.enableExternalHardwareEventLogging);

    // Profiles
    stringRecord(
        newSettings, "profileName",
        profileNames[profileNumber]); // Must come before profile number so AP config page JS has name before number
    stringRecord(newSettings, "profileNumber", profileNumber);
    for (int index = 0; index < MAX_PROFILE_COUNT; index++)
    {
        snprintf(tagText, sizeof(tagText), "profile%dName", index);
        snprintf(nameText, sizeof(nameText), "%d: %s", index + 1, profileNames[index]);
        stringRecord(newSettings, tagText, nameText);
    }
    // stringRecord(newSettings, "activeProfiles", activeProfiles);

    // System state at power on. Convert various system states to either Rover or Base or NTP.
    int lastState; // 0 = Rover, 1 = Base, 2 = NTP
    if (productVariant == REFERENCE_STATION)
    {
        lastState = 1; // Default Base
        if (settings.lastState >= STATE_ROVER_NOT_STARTED && settings.lastState <= STATE_ROVER_RTK_FIX)
            lastState = 0;
        if (settings.lastState >= STATE_NTPSERVER_NOT_STARTED && settings.lastState <= STATE_NTPSERVER_SYNC)
            lastState = 2;
    }
    else
    {
        lastState = 0; // Default Rover
        if (settings.lastState >= STATE_BASE_NOT_STARTED && settings.lastState <= STATE_BASE_FIXED_TRANSMITTING)
            lastState = 1;
    }
    stringRecord(newSettings, "baseRoverSetup", lastState);

    // Bluetooth radio type
    stringRecord(newSettings, "bluetoothRadioType", settings.bluetoothRadioType);

    // Current coordinates come from HPPOSLLH call back
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

    // Antenna height and ARP
    stringRecord(newSettings, "antennaHeight", settings.antennaHeight);
    stringRecord(newSettings, "antennaReferencePoint", settings.antennaReferencePoint, 1);

    // Radio / ESP-Now settings
    char radioMAC[18]; // Send radio MAC
    snprintf(radioMAC, sizeof(radioMAC), "%02X:%02X:%02X:%02X:%02X:%02X", wifiMACAddress[0], wifiMACAddress[1],
             wifiMACAddress[2], wifiMACAddress[3], wifiMACAddress[4], wifiMACAddress[5]);
    stringRecord(newSettings, "radioMAC", radioMAC);
    stringRecord(newSettings, "radioType", settings.radioType);
    stringRecord(newSettings, "espnowPeerCount", settings.espnowPeerCount);
    for (int index = 0; index < settings.espnowPeerCount; index++)
    {
        snprintf(tagText, sizeof(tagText), "peerMAC%d", index);
        snprintf(nameText, sizeof(nameText), "%02X:%02X:%02X:%02X:%02X:%02X", settings.espnowPeers[index][0],
                 settings.espnowPeers[index][1], settings.espnowPeers[index][2], settings.espnowPeers[index][3],
                 settings.espnowPeers[index][4], settings.espnowPeers[index][5]);
        stringRecord(newSettings, tagText, nameText);
    }
    stringRecord(newSettings, "espnowBroadcast", settings.espnowBroadcast);

    stringRecord(newSettings, "logFileName", logFileName);

    if (HAS_NO_BATTERY) // Ref Stn does not have a battery
    {
        stringRecord(newSettings, "batteryIconFileName", (char *)"src/BatteryBlank.png");
        stringRecord(newSettings, "batteryPercent", (char *)" ");
    }
    else
    {
        // Determine battery icon
        int iconLevel = 0;
        if (battLevel < 25)
            iconLevel = 0;
        else if (battLevel < 50)
            iconLevel = 1;
        else if (battLevel < 75)
            iconLevel = 2;
        else // batt level > 75
            iconLevel = 3;

        char batteryIconFileName[sizeof("src/Battery2_Charging.png__")]; // sizeof() includes 1 for \0 termination

        if (externalPowerConnected)
            snprintf(batteryIconFileName, sizeof(batteryIconFileName), "src/Battery%d_Charging.png", iconLevel);
        else
            snprintf(batteryIconFileName, sizeof(batteryIconFileName), "src/Battery%d.png", iconLevel);

        stringRecord(newSettings, "batteryIconFileName", batteryIconFileName);

        // Determine battery percent
        char batteryPercent[sizeof("+100%__")];
        int tempLevel = battLevel;
        if (tempLevel > 100)
            tempLevel = 100;

        if (externalPowerConnected)
            snprintf(batteryPercent, sizeof(batteryPercent), "+%d%%", tempLevel);
        else
            snprintf(batteryPercent, sizeof(batteryPercent), "%d%%", tempLevel);
        stringRecord(newSettings, "batteryPercent", batteryPercent);
    }

    stringRecord(newSettings, "minElev", settings.minElev);
    stringRecord(newSettings, "imuYaw", settings.imuYaw);
    stringRecord(newSettings, "imuPitch", settings.imuPitch);
    stringRecord(newSettings, "imuRoll", settings.imuRoll);
    stringRecord(newSettings, "sfDisableWheelDirection", settings.sfDisableWheelDirection);
    stringRecord(newSettings, "sfCombineWheelTicks", settings.sfCombineWheelTicks);
    stringRecord(newSettings, "rateNavPrio", settings.rateNavPrio);
    stringRecord(newSettings, "sfUseSpeed", settings.sfUseSpeed);
    stringRecord(newSettings, "coordinateInputType", settings.coordinateInputType);
    // stringRecord(newSettings, "lbandFixTimeout_seconds", settings.lbandFixTimeout_seconds);

    if (zedModuleType == PLATFORM_F9R)
        stringRecord(newSettings, "minCNO", settings.minCNO_F9R);
    else
        stringRecord(newSettings, "minCNO", settings.minCNO_F9P);

    stringRecord(newSettings, "mdnsEnable", settings.mdnsEnable);

    // Add ECEF and Geodetic station data to the end of settings
    for (int index = 0; index < COMMON_COORDINATES_MAX_STATIONS; index++) // Arbitrary 50 station limit
    {
        // stationInfo example: LocationA,-1280206.568,-4716804.403,4086665.484
        char stationInfo[100];

        // Try SD, then LFS
        if (getFileLineSD(stationCoordinateECEFFileName, index, stationInfo, sizeof(stationInfo)) ==
            true) // fileName, lineNumber, array, arraySize
        {
            trim(stationInfo); // Remove trailing whitespace

            if (settings.debugWiFiConfig == true)
                systemPrintf("ECEF SD station %d - found: %s\r\n", index, stationInfo);

            replaceCharacter(stationInfo, ',', ' '); // Change all , to ' ' for easier parsing on the JS side
            snprintf(tagText, sizeof(tagText), "stationECEF%d", index);
            stringRecord(newSettings, tagText, stationInfo);
        }
        else if (getFileLineLFS(stationCoordinateECEFFileName, index, stationInfo, sizeof(stationInfo)) ==
                 true) // fileName, lineNumber, array, arraySize
        {
            trim(stationInfo); // Remove trailing whitespace

            if (settings.debugWiFiConfig == true)
                systemPrintf("ECEF LFS station %d - found: %s\r\n", index, stationInfo);

            replaceCharacter(stationInfo, ',', ' '); // Change all , to ' ' for easier parsing on the JS side
            snprintf(tagText, sizeof(tagText), "stationECEF%d", index);
            stringRecord(newSettings, tagText, stationInfo);
        }
        else
        {
            // We could not find this line
            break;
        }
    }

    for (int index = 0; index < COMMON_COORDINATES_MAX_STATIONS; index++) // Arbitrary 50 station limit
    {
        // stationInfo example: LocationA,40.09029479,-105.18505761,1560.089
        char stationInfo[100];

        // Try SD, then LFS
        if (getFileLineSD(stationCoordinateGeodeticFileName, index, stationInfo, sizeof(stationInfo)) ==
            true) // fileName, lineNumber, array, arraySize
        {
            trim(stationInfo); // Remove trailing whitespace

            if (settings.debugWiFiConfig == true)
                systemPrintf("Geo SD station %d - found: %s\r\n", index, stationInfo);

            replaceCharacter(stationInfo, ',', ' '); // Change all , to ' ' for easier parsing on the JS side
            snprintf(tagText, sizeof(tagText), "stationGeodetic%d", index);
            stringRecord(newSettings, tagText, stationInfo);
        }
        else if (getFileLineLFS(stationCoordinateGeodeticFileName, index, stationInfo, sizeof(stationInfo)) ==
                 true) // fileName, lineNumber, array, arraySize
        {
            trim(stationInfo); // Remove trailing whitespace

            if (settings.debugWiFiConfig == true)
                systemPrintf("Geo LFS station %d - found: %s\r\n", index, stationInfo);

            replaceCharacter(stationInfo, ',', ' '); // Change all , to ' ' for easier parsing on the JS side
            snprintf(tagText, sizeof(tagText), "stationGeodetic%d", index);
            stringRecord(newSettings, tagText, stationInfo);
        }
        else
        {
            // We could not find this line
            break;
        }
    }

    // Add WiFi credential table
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        snprintf(tagText, sizeof(tagText), "wifiNetwork%dSSID", x);
        stringRecord(newSettings, tagText, settings.wifiNetworks[x].ssid);

        snprintf(tagText, sizeof(tagText), "wifiNetwork%dPassword", x);
        stringRecord(newSettings, tagText, settings.wifiNetworks[x].password);
    }

    // Drop downs on the AP config page expect a value, whereas bools get stringRecord as true/false
    if (settings.wifiConfigOverAP == true)
        stringRecord(newSettings, "wifiConfigOverAP", 1); // 1 = AP mode, 0 = WiFi
    else
        stringRecord(newSettings, "wifiConfigOverAP", 0); // 1 = AP mode, 0 = WiFi

    stringRecord(newSettings, "enablePvtServer", settings.enablePvtServer);
    stringRecord(newSettings, "enablePvtClient", settings.enablePvtClient);
    stringRecord(newSettings, "pvtServerPort", settings.pvtServerPort);
    stringRecord(newSettings, "enablePvtUdpServer", settings.enablePvtUdpServer);
    stringRecord(newSettings, "pvtUdpServerPort", settings.pvtUdpServerPort);
    stringRecord(newSettings, "enableRCFirmware", enableRCFirmware);

    // New settings not yet integrated
    //...

    strcat(newSettings, "\0");
    systemPrintf("newSettings len: %d\r\n", strlen(newSettings));
    systemPrintf("newSettings: %s\r\n", newSettings);
}

// Create a csv string with the dynamic data to update (current coordinates, battery level, etc)
void createDynamicDataString(char *settingsCSV)
{
    settingsCSV[0] = '\0'; // Erase current settings string

    // Current coordinates come from HPPOSLLH call back
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

    if (HAS_NO_BATTERY) // Ref Stn does not have a battery
    {
        stringRecord(settingsCSV, "batteryIconFileName", (char *)"src/BatteryBlank.png");
        stringRecord(settingsCSV, "batteryPercent", (char *)" ");
    }
    else
    {
        // Determine battery icon
        int iconLevel = 0;
        if (battLevel < 25)
            iconLevel = 0;
        else if (battLevel < 50)
            iconLevel = 1;
        else if (battLevel < 75)
            iconLevel = 2;
        else // batt level > 75
            iconLevel = 3;

        char batteryIconFileName[sizeof("src/Battery2_Charging.png__")]; // sizeof() includes 1 for \0 termination

        if (externalPowerConnected)
            snprintf(batteryIconFileName, sizeof(batteryIconFileName), "src/Battery%d_Charging.png", iconLevel);
        else
            snprintf(batteryIconFileName, sizeof(batteryIconFileName), "src/Battery%d.png", iconLevel);

        stringRecord(settingsCSV, "batteryIconFileName", batteryIconFileName);

        // Determine battery percent
        char batteryPercent[sizeof("+100%__")];
        if (externalPowerConnected)
            snprintf(batteryPercent, sizeof(batteryPercent), "+%d%%", battLevel);
        else
            snprintf(batteryPercent, sizeof(batteryPercent), "%d%%", battLevel);
        stringRecord(settingsCSV, "batteryPercent", batteryPercent);
    }

    strcat(settingsCSV, "\0");
}

// Given a settingName, and string value, update a given setting
void updateSettingWithValue(const char *settingName, const char *settingValueStr)
{
    char *ptr;
    double settingValue = strtod(settingValueStr, &ptr);

    bool settingValueBool = false;
    if (strcmp(settingValueStr, "true") == 0)
        settingValueBool = true;

    if (strcmp(settingName, "maxLogTime_minutes") == 0)
        settings.maxLogTime_minutes = settingValue;
    else if (strcmp(settingName, "maxLogLength_minutes") == 0)
        settings.maxLogLength_minutes = settingValue;
    else if (strcmp(settingName, "measurementRateHz") == 0)
    {
        settings.measurementRate = (int)(1000.0 / settingValue);

        // This is one of the first settings to be received. If seen, remove the station files.
        removeFile(stationCoordinateECEFFileName);
        removeFile(stationCoordinateGeodeticFileName);
        if (settings.debugWiFiConfig == true)
            systemPrintln("Station coordinate files removed");
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
        settings.fixedBaseCoordinateType =
            !settingValueBool; // When ECEF is true, fixedBaseCoordinateType = 0 (COORD_TYPE_ECEF)
    else if (strcmp(settingName, "fixedEcefX") == 0)
        settings.fixedEcefX = settingValue;
    else if (strcmp(settingName, "fixedEcefY") == 0)
        settings.fixedEcefY = settingValue;
    else if (strcmp(settingName, "fixedEcefZ") == 0)
        settings.fixedEcefZ = settingValue;
    else if (strcmp(settingName, "fixedLatText") == 0)
    {
        double newCoordinate = 0.0;
        CoordinateInputType newCoordinateInputType =
            coordinateIdentifyInputType((char *)settingValueStr, &newCoordinate);
        if (newCoordinateInputType == COORDINATE_INPUT_TYPE_INVALID_UNKNOWN)
            settings.fixedLat = 0.0;
        else
        {
            settings.fixedLat = newCoordinate;
            settings.coordinateInputType = newCoordinateInputType;
        }
    }
    else if (strcmp(settingName, "fixedLongText") == 0)
    {
        // Lat defines the settings.coordinateInputType. Don't update it here
        double newCoordinate = 0.0;
        if (coordinateIdentifyInputType((char *)settingValueStr, &newCoordinate) ==
            COORDINATE_INPUT_TYPE_INVALID_UNKNOWN)
            settings.fixedLong = 0.0;
        else
            settings.fixedLong = newCoordinate;
    }
    else if (strcmp(settingName, "fixedAltitude") == 0)
        settings.fixedAltitude = settingValue;
    else if (strcmp(settingName, "dataPortBaud") == 0)
        settings.dataPortBaud = settingValue;
    else if (strcmp(settingName, "radioPortBaud") == 0)
        settings.radioPortBaud = settingValue;
    else if (strcmp(settingName, "enableUART2UBXIn") == 0)
        settings.enableUART2UBXIn = settingValueBool;
    else if (strcmp(settingName, "enableLogging") == 0)
        settings.enableLogging = settingValueBool;
    else if (strcmp(settingName, "enableARPLogging") == 0)
        settings.enableARPLogging = settingValueBool;
    else if (strcmp(settingName, "ARPLoggingInterval") == 0)
        settings.ARPLoggingInterval_s = settingValue;
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
        setProfileName(profileNumber); // Copy the current settings.profileName into the array of profile names at
                                       // location profileNumber
    }
    else if (strcmp(settingName, "enableNtripServer") == 0)
        settings.enableNtripServer = settingValueBool;

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
        settings.bluetoothRadioType = (BluetoothRadioType_e)settingValue; // 0 = SPP, 1 = BLE, 2 = Off
    else if (strcmp(settingName, "espnowBroadcast") == 0)
        settings.espnowBroadcast = settingValueBool;
    else if (strcmp(settingName, "radioType") == 0)
        settings.radioType = (RadioType_e)settingValue; // 0 = Radio off, 1 = ESP-Now
    else if (strcmp(settingName, "baseRoverSetup") == 0)
    {
        // 0 = Rover, 1 = Base, 2 = NTP
        settings.lastState = STATE_ROVER_NOT_STARTED; // Default
        if (settingValue == 1)
            settings.lastState = STATE_BASE_NOT_STARTED;
        if (settingValue == 2)
            settings.lastState = STATE_NTPSERVER_NOT_STARTED;
    }
    else if (strstr(settingName, "stationECEF") != nullptr)
    {
        replaceCharacter((char *)settingValueStr, ' ', ','); // Replace all ' ' with ',' before recording to file
        recordLineToSD(stationCoordinateECEFFileName, settingValueStr);
        recordLineToLFS(stationCoordinateECEFFileName, settingValueStr);
        if (settings.debugWiFiConfig == true)
            systemPrintf("%s recorded\r\n", settingValueStr);
    }
    else if (strstr(settingName, "stationGeodetic") != nullptr)
    {
        replaceCharacter((char *)settingValueStr, ' ', ','); // Replace all ' ' with ',' before recording to file
        recordLineToSD(stationCoordinateGeodeticFileName, settingValueStr);
        recordLineToLFS(stationCoordinateGeodeticFileName, settingValueStr);
        if (settings.debugWiFiConfig == true)
            systemPrintf("%s recorded\r\n", settingValueStr);
    }
    else if (strcmp(settingName, "pvtServerPort") == 0)
        settings.pvtServerPort = settingValue;
    else if (strcmp(settingName, "pvtUdpServerPort") == 0)
        settings.pvtUdpServerPort = settingValue;
    else if (strcmp(settingName, "wifiConfigOverAP") == 0)
    {
        if (settingValue == 1) // Drop downs come back as a value
            settings.wifiConfigOverAP = true;
        else
            settings.wifiConfigOverAP = false;
    }

    else if (strcmp(settingName, "enablePvtClient") == 0)
        settings.enablePvtClient = settingValueBool;
    else if (strcmp(settingName, "enablePvtServer") == 0)
        settings.enablePvtServer = settingValueBool;
    else if (strcmp(settingName, "enablePvtUdpServer") == 0)
        settings.enablePvtUdpServer = settingValueBool;
    else if (strcmp(settingName, "enableRCFirmware") == 0)
        enableRCFirmware = settingValueBool;
    else if (strcmp(settingName, "minElev") == 0)
        settings.minElev = settingValue;
    else if (strcmp(settingName, "imuYaw") == 0)
        settings.imuYaw = settingValue * 100; // Comes in as 0 to 360.0 but stored as 0 to 36,000
    else if (strcmp(settingName, "imuPitch") == 0)
        settings.imuPitch = settingValue * 100; // Comes in as -90 to 90.0 but stored as -9000 to 9000
    else if (strcmp(settingName, "imuRoll") == 0)
        settings.imuRoll = settingValue * 100; // Comes in as -180 to 180.0 but stored as -18000 to 18000
    else if (strcmp(settingName, "sfDisableWheelDirection") == 0)
        settings.sfDisableWheelDirection = settingValueBool;
    else if (strcmp(settingName, "sfCombineWheelTicks") == 0)
        settings.sfCombineWheelTicks = settingValueBool;
    else if (strcmp(settingName, "rateNavPrio") == 0)
        settings.rateNavPrio = settingValue;
    else if (strcmp(settingName, "minCNO") == 0)
    {
        if (zedModuleType == PLATFORM_F9R)
            settings.minCNO_F9R = settingValue;
        else
            settings.minCNO_F9P = settingValue;
    }

    else if (strcmp(settingName, "ethernetDHCP") == 0)
        settings.ethernetDHCP = settingValueBool;
    else if (strcmp(settingName, "ethernetIP") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetIP.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetDNS") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetDNS.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetGateway") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetGateway.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetSubnet") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetSubnet.fromString(tempString);
    }
    else if (strcmp(settingName, "httpPort") == 0)
        settings.httpPort = settingValue;
    else if (strcmp(settingName, "ethernetNtpPort") == 0)
        settings.ethernetNtpPort = settingValue;
    else if (strcmp(settingName, "pvtClientPort") == 0)
        settings.pvtClientPort = settingValue;
    else if (strcmp(settingName, "pvtClientHost") == 0)
        strcpy(settings.pvtClientHost, settingValueStr);

    // Network layer
    else if (strcmp(settingName, "defaultNetworkType") == 0)
        settings.defaultNetworkType = settingValue;
    else if (strcmp(settingName, "enableNetworkFailover") == 0)
        settings.enableNetworkFailover = settingValue;

    // NTP
    else if (strcmp(settingName, "ntpPollExponent") == 0)
        settings.ntpPollExponent = settingValue;
    else if (strcmp(settingName, "ntpPrecision") == 0)
        settings.ntpPrecision = settingValue;
    else if (strcmp(settingName, "ntpRootDelay") == 0)
        settings.ntpRootDelay = settingValue;
    else if (strcmp(settingName, "ntpRootDispersion") == 0)
        settings.ntpRootDispersion = settingValue;
    else if (strcmp(settingName, "ntpReferenceId") == 0)
    {
        strcpy(settings.ntpReferenceId, settingValueStr);
        for (int i = strlen(settingValueStr); i < 5; i++)
            settings.ntpReferenceId[i] = 0;
    }
    else if (strcmp(settingName, "mdnsEnable") == 0)
        settings.mdnsEnable = settingValueBool;

    // Automatic firmware update settings
    else if (strcmp(settingName, "enableAutoFirmwareUpdate") == 0)
        settings.enableAutoFirmwareUpdate = settingValueBool;
    else if (strcmp(settingName, "autoFirmwareCheckMinutes") == 0)
        settings.autoFirmwareCheckMinutes = settingValueBool;

    else if (strcmp(settingName, "geographicRegion") == 0)
        settings.geographicRegion = settingValue;

    // Unused variables - read to avoid errors
    else if (strcmp(settingName, "measurementRateSec") == 0)
    {
    }
    else if (strcmp(settingName, "baseTypeSurveyIn") == 0)
    {
    }
    else if (strcmp(settingName, "fixedBaseCoordinateTypeGeo") == 0)
    {
    }
    else if (strcmp(settingName, "saveToArduino") == 0)
    {
    }
    else if (strcmp(settingName, "enableFactoryDefaults") == 0)
    {
    }
    else if (strcmp(settingName, "enableFirmwareUpdate") == 0)
    {
    }
    else if (strcmp(settingName, "enableForgetRadios") == 0)
    {
    }
    else if (strcmp(settingName, "nicknameECEF") == 0)
    {
    }
    else if (strcmp(settingName, "nicknameGeodetic") == 0)
    {
    }
    else if (strcmp(settingName, "fileSelectAll") == 0)
    {
    }
    else if (strcmp(settingName, "fixedHAE_APC") == 0)
    {
    }
    else if (strcmp(settingName, "measurementRateSecBase") == 0)
    {
    }

    // Special actions
    else if (strcmp(settingName, "firmwareFileName") == 0)
    {
        mountSDThenUpdate(settingValueStr);

        // If update is successful, it will force system reset and not get here.

        if (productVariant == REFERENCE_STATION)
            requestChangeState(STATE_BASE_NOT_STARTED); // If update failed, return to Base mode.
        else
            requestChangeState(STATE_ROVER_NOT_STARTED); // If update failed, return to Rover mode.
    }
    else if (strcmp(settingName, "factoryDefaultReset") == 0)
        factoryReset(false); // We do not have the sdSemaphore
    else if (strcmp(settingName, "exitAndReset") == 0)
    {
        // Confirm receipt
        if (settings.debugWiFiConfig == true)
            systemPrintln("Sending reset confirmation");

        websocket->textAll("confirmReset,1,");
        delay(500); // Allow for delivery

        if (configureViaEthernet)
            systemPrintln("Reset after Configure-Via-Ethernet");
        else
            systemPrintln("Reset after AP Config");

        if (configureViaEthernet)
        {
            ethernetWebServerStopESP32W5500();

            // We need to exit configure-via-ethernet mode.
            // But if the settings have not been saved then lastState will still be STATE_CONFIG_VIA_ETH_STARTED.
            // If that is true, then force exit to Base mode. I think it is the best we can do.
            //(If the settings have been saved, then the code will restart in NTP, Base or Rover mode as desired.)
            if (settings.lastState == STATE_CONFIG_VIA_ETH_STARTED)
            {
                systemPrintln("Settings were not saved. Resetting into Base mode.");
                settings.lastState = STATE_BASE_NOT_STARTED;
                recordSystemSettings();
            }
        }

        ESP.restart();
    }
    else if (strcmp(settingName, "setProfile") == 0)
    {
        // Change to new profile
        if (settings.debugWiFiConfig == true)
            systemPrintf("Changing to profile number %d\r\n", settingValue);
        changeProfileNumber(settingValue);

        // Load new profile into system
        loadSettings();

        // Send new settings to browser. Re-use settingsCSV to avoid stack.
        memset(settingsCSV, 0, AP_CONFIG_SETTING_SIZE); // Clear any garbage from settings array

        createSettingsString(settingsCSV);

        if (settings.debugWiFiConfig == true)
        {
            systemPrintf("Sending profile %d\r\n", settingValue);
            systemPrintf("Profile contents: %s\r\n", settingsCSV);
        }
        websocket->textAll(settingsCSV);
    }
    else if (strcmp(settingName, "resetProfile") == 0)
    {
        settingsToDefaults(); // Overwrite our current settings with defaults

        recordSystemSettings(); // Overwrite profile file and NVM with these settings

        // Get bitmask of active profiles
        activeProfiles = loadProfileNames();

        // Send new settings to browser. Re-use settingsCSV to avoid stack.
        memset(settingsCSV, 0, AP_CONFIG_SETTING_SIZE); // Clear any garbage from settings array

        createSettingsString(settingsCSV);

        if (settings.debugWiFiConfig == true)
        {
            systemPrintf("Sending reset profile %d\r\n", settingValue);
            systemPrintf("Profile contents: %s\r\n", settingsCSV);
        }
        websocket->textAll(settingsCSV);
    }
    else if (strcmp(settingName, "forgetEspNowPeers") == 0)
    {
        // Forget all ESP-Now Peers
        for (int x = 0; x < settings.espnowPeerCount; x++)
            espnowRemovePeer(settings.espnowPeers[x]);
        settings.espnowPeerCount = 0;
    }
    else if (strcmp(settingName, "startNewLog") == 0)
    {
        if (settings.enableLogging == true && online.logging == true)
        {
            endLogging(false, true); //(gotSemaphore, releaseSemaphore) Close file. Reset parser stats.
            beginLogging();          // Create new file based on current RTC.
            setLoggingType();        // Determine if we are standard, PPP, or custom. Changes logging icon accordingly.

            char newFileNameCSV[sizeof("logFileName,") + sizeof(logFileName) + 1];
            snprintf(newFileNameCSV, sizeof(newFileNameCSV), "logFileName,%s,", logFileName);

            websocket->textAll(newFileNameCSV); // Tell the config page the name of the file we just created
        }
    }
    else if (strcmp(settingName, "checkNewFirmware") == 0)
    {
        if (settings.debugWiFiConfig == true)
            systemPrintln("Checking for new OTA Pull firmware");

        websocket->textAll("checkingNewFirmware,1,"); // Tell the config page we received their request

        char reportedVersion[20];
        char newVersionCSV[100];

        // Get firmware version from server
        if (otaCheckVersion(reportedVersion, sizeof(reportedVersion)))
        {
            // We got a version number, now determine if it's newer or not
            char currentVersion[21];
            getFirmwareVersion(currentVersion, sizeof(currentVersion), enableRCFirmware);
            if (isReportedVersionNewer(reportedVersion, currentVersion) == true)
            {
                if (settings.debugWiFiConfig == true)
                    systemPrintln("New version detected");
                snprintf(newVersionCSV, sizeof(newVersionCSV), "newFirmwareVersion,%s,", reportedVersion);
            }
            else
            {
                if (settings.debugWiFiConfig == true)
                    systemPrintln("No new firmware available");
                snprintf(newVersionCSV, sizeof(newVersionCSV), "newFirmwareVersion,CURRENT,");
            }
        }
        else
        {
            // Failed to get version number
            if (settings.debugWiFiConfig == true)
                systemPrintln("Sending error to AP config page");
            snprintf(newVersionCSV, sizeof(newVersionCSV), "newFirmwareVersion,ERROR,");
        }

        websocket->textAll(newVersionCSV);
    }
    else if (strcmp(settingName, "getNewFirmware") == 0)
    {
        if (settings.debugWiFiConfig == true)
            systemPrintln("Getting new OTA Pull firmware");

        websocket->textAll("gettingNewFirmware,1,"); // Tell the config page we received their request

        apConfigFirmwareUpdateInProcess = true;
        otaUpdate();

        // We get here if WiFi failed to connect
        websocket->textAll("gettingNewFirmware,ERROR,");
    }

    // Check for bulk settings (constellations and message rates)
    // Must be last on else list
    else
    {
        bool knownSetting = false;

        // Scan for WiFi credentials
        if (knownSetting == false)
        {
            for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
            {
                char tempString[100]; // wifiNetwork0Password=parachutes
                snprintf(tempString, sizeof(tempString), "wifiNetwork%dSSID", x);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(settings.wifiNetworks[x].ssid, settingValueStr);
                    knownSetting = true;
                    break;
                }
                else
                {
                    snprintf(tempString, sizeof(tempString), "wifiNetwork%dPassword", x);
                    if (strcmp(settingName, tempString) == 0)
                    {
                        strcpy(settings.wifiNetworks[x].password, settingValueStr);
                        knownSetting = true;
                        break;
                    }
                }
            }
        }

        // Scan for constellation settings
        if (knownSetting == false)
        {
            for (int x = 0; x < MAX_CONSTELLATIONS; x++)
            {
                char tempString[50]; // ubxConstellationsSBAS
                snprintf(tempString, sizeof(tempString), "ubxConstellations%s", settings.ubxConstellations[x].textName);

                if (strcmp(settingName, tempString) == 0)
                {
                    settings.ubxConstellations[x].enabled = settingValueBool;
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for message settings
        if (knownSetting == false)
        {
            char tempString[50];

            for (int x = 0; x < MAX_UBX_MSG; x++)
            {
                snprintf(tempString, sizeof(tempString), "%s", ubxMessages[x].msgTextName); // UBX_RTCM_1074
                if (strcmp(settingName, tempString) == 0)
                {
                    settings.ubxMessageRates[x] = settingValue;
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for Base RTCM message settings
        if (knownSetting == false)
        {
            int firstRTCMRecord = getMessageNumberByName("UBX_RTCM_1005");

            char tempString[50];
            for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
            {
                snprintf(tempString, sizeof(tempString), "%sBase",
                         ubxMessages[firstRTCMRecord + x].msgTextName); // UBX_RTCM_1074Base
                if (strcmp(settingName, tempString) == 0)
                {
                    settings.ubxMessageRatesBase[x] = settingValue;
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for ntripServerCasterHost
        if (knownSetting == false)
        {
            for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            {
                char tempString[50];
                snprintf(tempString, sizeof(tempString), "ntripServer_CasterHost_%d", serverIndex);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(&settings.ntripServer_CasterHost[serverIndex][0], settingValueStr);
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for ntripServerCasterPort
        if (knownSetting == false)
        {
            for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            {
                char tempString[50];
                snprintf(tempString, sizeof(tempString), "ntripServer_CasterPort_%d", serverIndex);
                if (strcmp(settingName, tempString) == 0)
                {
                    settings.ntripServer_CasterPort[serverIndex] = settingValue;
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for ntripServerCasterUser
        if (knownSetting == false)
        {
            for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            {
                char tempString[50];
                snprintf(tempString, sizeof(tempString), "ntripServer_CasterUser_%d", serverIndex);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(&settings.ntripServer_CasterUser[serverIndex][0], settingValueStr);
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for ntripServerCasterUserPW
        if (knownSetting == false)
        {
            for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            {
                char tempString[50];
                snprintf(tempString, sizeof(tempString), "ntripServer_CasterUserPW_%d", serverIndex);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(&settings.ntripServer_CasterUserPW[serverIndex][0], settingValueStr);
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for ntripServerMountPoint
        if (knownSetting == false)
        {
            for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            {
                char tempString[50];
                snprintf(tempString, sizeof(tempString), "ntripServer_MountPoint_%d", serverIndex);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(&settings.ntripServer_MountPoint[serverIndex][0], settingValueStr);
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for ntripServerMountPointPW
        if (knownSetting == false)
        {
            for (int serverIndex = 0; serverIndex < NTRIP_SERVER_MAX; serverIndex++)
            {
                char tempString[50];
                snprintf(tempString, sizeof(tempString), "ntripServer_MountPointPW_%d", serverIndex);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(&settings.ntripServer_MountPointPW[serverIndex][0], settingValueStr);
                    knownSetting = true;
                    break;
                }
            }
        }

        // Last catch
        if (knownSetting == false)
        {
            systemPrintf("Unknown '%s': %0.3lf\r\n", settingName, settingValue);
        }
    } // End last strcpy catch
}

// Add record with int
void stringRecord(char *settingsCSV, const char *id, int settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%d,", id, settingValue);
    strcat(settingsCSV, record);
}

// Add record with uint32_t
void stringRecord(char *settingsCSV, const char *id, uint32_t settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%d,", id, settingValue);
    strcat(settingsCSV, record);
}

// Add record with double
void stringRecord(char *settingsCSV, const char *id, double settingValue, int decimalPlaces)
{
    char format[10];
    snprintf(format, sizeof(format), "%%0.%dlf", decimalPlaces); // Create '%0.09lf'

    char formattedValue[20];
    snprintf(formattedValue, sizeof(formattedValue), format, settingValue);

    char record[100];
    snprintf(record, sizeof(record), "%s,%s,", id, formattedValue);
    strcat(settingsCSV, record);
}

// Add record with bool
void stringRecord(char *settingsCSV, const char *id, bool settingValue)
{
    char temp[10];
    if (settingValue == true)
        strcpy(temp, "true");
    else
        strcpy(temp, "false");

    char record[100];
    snprintf(record, sizeof(record), "%s,%s,", id, temp);
    strcat(settingsCSV, record);
}

// Add record with string
void stringRecord(char *settingsCSV, const char *id, char *settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%s,", id, settingValue);
    strcat(settingsCSV, record);
}

// Add record with uint64_t
void stringRecord(char *settingsCSV, const char *id, uint64_t settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%lld,", id, settingValue);
    strcat(settingsCSV, record);
}

// Break CSV into setting constituents
// Can't use strtok because we may have two commas next to each other, ie
// measurementRateHz,4.00,measurementRateSec,,dynamicModel,0,
bool parseIncomingSettings()
{
    char settingName[100] = {'\0'};
    char valueStr[150] = {'\0'}; // stationGeodetic1,ANameThatIsTooLongToBeDisplayed 40.09029479 -105.18505761 1560.089

    char *commaPtr = incomingSettings;
    char *headPtr = incomingSettings;

    int counter = 0;
    int maxAttempts = 500;
    while (*headPtr) // Check if we've reached the end of the string
    {
        // Spin to first comma
        commaPtr = strstr(headPtr, ",");
        if (commaPtr != nullptr)
        {
            *commaPtr = '\0';
            strcpy(settingName, headPtr);
            headPtr = commaPtr + 1;
        }

        commaPtr = strstr(headPtr, ",");
        if (commaPtr != nullptr)
        {
            *commaPtr = '\0';
            strcpy(valueStr, headPtr);
            headPtr = commaPtr + 1;
        }

        // log_d("settingName: %s value: %s", settingName, valueStr);

        updateSettingWithValue(settingName, valueStr);

        // Avoid infinite loop if response is malformed
        counter++;
        if (counter == maxAttempts)
        {
            systemPrintln("Error: Incoming settings malformed.");
            break;
        }
    }

    if (counter < maxAttempts)
    {
        // Confirm receipt
        if (settings.debugWiFiConfig == true)
            systemPrintln("Sending receipt confirmation of settings");
        websocket->textAll("confirmDataReceipt,1,");
    }

    return (true);
}

// When called, responds with the root folder list of files on SD card
// Name and size are formatted in CSV, formatted to html by JS
void getFileList(String &returnText)
{
    returnText = "";

    // Update the SD Size and Free Space
    String cardSize;
    stringHumanReadableSize(cardSize, sdCardSize);
    returnText += "sdSize," + cardSize + ",";
    String freeSpace;
    stringHumanReadableSize(freeSpace, sdFreeSpace);
    returnText += "sdFreeSpace," + freeSpace + ",";

    char fileName[50]; // Handle long file names

    // Attempt to gain access to the SD card
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
        markSemaphore(FUNCTION_FILEMANAGER_UPLOAD1);

        if (USE_SPI_MICROSD)
        {
            SdFile root;
            root.open("/"); // Open root
            SdFile file;
            uint16_t fileCount = 0;

            while (file.openNext(&root, O_READ))
            {
                if (file.isFile())
                {
                    fileCount++;

                    file.getName(fileName, sizeof(fileName));

                    String fileSize;
                    stringHumanReadableSize(fileSize, file.fileSize());
                    returnText += "fmName," + String(fileName) + ",fmSize," + fileSize + ",";
                }
            }

            root.close();
            file.close();
        }
#ifdef COMPILE_SD_MMC
        else
        {
            File root = SD_MMC.open("/"); // Open root

            if (root && root.isDirectory())
            {
                uint16_t fileCount = 0;

                File file = root.openNextFile();
                while (file)
                {
                    if (!file.isDirectory())
                    {
                        fileCount++;

                        String fileSize;
                        stringHumanReadableSize(fileSize, file.size());
                        returnText += "fmName," + String(file.name()) + ",fmSize," + fileSize + ",";
                    }

                    file = root.openNextFile();
                }
            }

            root.close();
        }
#endif // COMPILE_SD_MMC

        xSemaphoreGive(sdCardSemaphore);
    }
    else
    {
        char semaphoreHolder[50];
        getSemaphoreFunction(semaphoreHolder);

        // This is an error because the current settings no longer match the settings
        // on the microSD card, and will not be restored to the expected settings!
        systemPrintf("sdCardSemaphore failed to yield, held by %s, Form.ino line %d\r\n", semaphoreHolder, __LINE__);
    }

    if (settings.debugWiFiConfig == true)
        systemPrintf("returnText (%d bytes): %s\r\n", returnText.length(), returnText.c_str());
}

// When called, responds with the messages supported on this platform
// Message name and current rate are formatted in CSV, formatted to html by JS
void createMessageList(String &returnText)
{
    returnText = "";

    for (int messageNumber = 0; messageNumber < MAX_UBX_MSG; messageNumber++)
    {
        if (messageSupported(messageNumber) == true)
            returnText += String(ubxMessages[messageNumber].msgTextName) + "," +
                          String(settings.ubxMessageRates[messageNumber]) + ","; // UBX_RTCM_1074,4,
    }

    if (settings.debugWiFiConfig == true)
        systemPrintf("returnText (%d bytes): %s\r\n", returnText.length(), returnText.c_str());
}

// When called, responds with the RTCM/Base messages supported on this platform
// Message name and current rate are formatted in CSV, formatted to html by JS
void createMessageListBase(String &returnText)
{
    returnText = "";

    int firstRTCMRecord = getMessageNumberByName("UBX_RTCM_1005");

    for (int messageNumber = 0; messageNumber < MAX_UBX_MSG_RTCM; messageNumber++)
    {
        if (messageSupported(firstRTCMRecord + messageNumber) == true)
            returnText += String(ubxMessages[messageNumber + firstRTCMRecord].msgTextName) + "Base," +
                          String(settings.ubxMessageRatesBase[messageNumber]) + ","; // UBX_RTCM_1074Base,4,
    }

    if (settings.debugWiFiConfig == true)
        systemPrintf("returnText (%d bytes): %s\r\n", returnText.length(), returnText.c_str());
}

// Handles uploading of user files to SD
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    String logmessage = "";

    if (!index)
    {
        logmessage = "Upload Start: " + String(filename);

        int fileNameLen = filename.length();
        char tempFileName[fileNameLen + 2] = {'/'}; // Filename must start with / or VERY bad things happen on SD_MMC
        filename.toCharArray(&tempFileName[1], fileNameLen + 1);
        tempFileName[fileNameLen + 1] = '\0'; // Terminate array

        // Allocate the managerTempFile
        if (!managerTempFile)
        {
            managerTempFile = new FileSdFatMMC;
            if (!managerTempFile)
            {
                systemPrintln("Failed to allocate managerTempFile!");
                return;
            }
        }
        // Attempt to gain access to the SD card
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
            markSemaphore(FUNCTION_FILEMANAGER_UPLOAD1);

            managerTempFile->open(tempFileName, O_CREAT | O_APPEND | O_WRITE);

            xSemaphoreGive(sdCardSemaphore);
        }

        systemPrintln(logmessage);
    }

    if (len)
    {
        // Attempt to gain access to the SD card
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
            markSemaphore(FUNCTION_FILEMANAGER_UPLOAD2);

            managerTempFile->write(data, len); // stream the incoming chunk to the opened file

            xSemaphoreGive(sdCardSemaphore);
        }
    }

    if (final)
    {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);

        // Attempt to gain access to the SD card
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
            markSemaphore(FUNCTION_FILEMANAGER_UPLOAD3);

            managerTempFile->updateFileCreateTimestamp(); // Update the file create time & date

            managerTempFile->close();

            xSemaphoreGive(sdCardSemaphore);
        }

        systemPrintln(logmessage);
        request->redirect("/");
    }
}

#endif // COMPILE_AP
