/*------------------------------------------------------------------------------
OtaClient.ino

  The Over-The-Air (OTA) client sits on top of the network layer and requests
  a JSON file from the GitHub server that describes the version and URL of
  the released firmware.  If the released firmware is more recent then the OTA
  client downloads and flashes the released firmware.

                              RTK
                             Device
                               ^
                               | OTA client
                               V
                             GitHub

------------------------------------------------------------------------------*/

#if COMPILE_NETWORK

//----------------------------------------
// Constants
//----------------------------------------

#define HTTPS_TRANSPORT         (OTA_USE_SSL ? "https://" : "http://")

#define OTA_JSON_FILE_URL       \
    "/sparkfun/SparkFun_RTK_Firmware_Binaries/main/RTK-Firmware.json"
#define OTA_NO_PROGRESS_TIMEOUT (3 * 60 * 1000) // 3 minutes
#define OTA_SERVER              "raw.githubusercontent.com"
#define OTA_SERVER_PORT         443
#define OTA_USE_SSL             1

enum OtaState
{
    OTA_STATE_OFF = 0,
    OTA_STATE_START_NETWORK,
    OTA_STATE_WAIT_FOR_NETWORK,
    OTA_STATE_JSON_FILE_REQUEST,
    OTA_STATE_JSON_FILE_PARSE_HTTP_STATUS,
    OTA_STATE_JSON_FILE_PARSE_LENGTH,
    OTA_STATE_JSON_FILE_SKIP_HEADERS,
    OTA_STATE_JSON_FILE_READ_DATA,
    OTA_STATE_PARSE_JSON_DATA,
    OTA_STATE_BIN_FILE_REQUEST,
    OTA_STATE_BIN_FILE_PARSE_HTTP_STATUS,
    OTA_STATE_BIN_FILE_PARSE_LENGTH,
    OTA_STATE_BIN_FILE_SKIP_HEADERS,
    OTA_STATE_BIN_FILE_READ_DATA,
    // Insert new states before this line
    OTA_STATE_MAX
};

const char * const otaStateNames[] =
{
    "OTA_STATE_OFF",
    "OTA_STATE_START_NETWORK",
    "OTA_STATE_WAIT_FOR_NETWORK",
    "OTA_STATE_JSON_FILE_REQUEST",
    "OTA_STATE_JSON_FILE_PARSE_HTTP_STATUS",
    "OTA_STATE_JSON_FILE_PARSE_LENGTH",
    "OTA_STATE_JSON_FILE_SKIP_HEADERS",
    "OTA_STATE_JSON_FILE_READ_DATA",
    "OTA_STATE_PARSE_JSON_DATA",
    "OTA_STATE_BIN_FILE_REQUEST",
    "OTA_STATE_BIN_FILE_PARSE_HTTP_STATUS",
    "OTA_STATE_BIN_FILE_PARSE_LENGTH",
    "OTA_STATE_BIN_FILE_SKIP_HEADERS",
    "OTA_STATE_BIN_FILE_READ_DATA"
};
const int otaStateEntries = sizeof(otaStateNames) / sizeof(otaStateNames[0]);

const RtkMode_t otaClientMode = RTK_MODE_BASE_FIXED
                              | RTK_MODE_BASE_SURVEY_IN
                              | RTK_MODE_BUBBLE_LEVEL
                              | RTK_MODE_NTP
                              | RTK_MODE_ROVER;

//----------------------------------------
// Locals
//----------------------------------------

static byte otaBluetoothState = BT_OFF;
static char otaBuffer[1379];
static int otaBufferData;
static NetworkClient * otaClient;
static int otaFileBytes;
static int otaFileSize;
static String otaJsonFileData;
static String otaReleasedFirmwareURL;
static uint8_t otaState;
static uint32_t otaTimer;

//----------------------------------------
// Over-The-Air (OTA) firmware update support routines
//----------------------------------------

// Get the OTA state name
const char * otaGetStateName(uint8_t state, char * string)
{
    if (state < OTA_STATE_MAX)
        return otaStateNames[state];
    sprintf(string, "Unknown state (%d)", state);
    return string;
}

// Get the file length from the HTTP header
int otaParseFileLength()
{
    int fileLength;

    // Parse the file length from the HTTP header
    otaBufferData = 0;
    if (sscanf(otaBuffer, "Content-Length: %d", &fileLength) == 1)
        return fileLength;
    return -1;
}

// Get the server file status from the HTTP header
int otaParseJsonStatus()
{
    int status;

    // Parse the status from the HTTP header
    if (sscanf(otaBuffer, "HTTP/1.1 %d", &status) == 1)
        return status;
    return -1;
}

// Read data from the JSON or firmware file
void otaReadFileData(int bufferLength)
{
    int bytesToRead;

    // Determine how much data is available
    otaBufferData = 0;
    bytesToRead = otaClient->available();
    if (bytesToRead)
    {
        // Determine the number of bytes to read
        if (bytesToRead > bufferLength)
            bytesToRead = bufferLength;

        // Read in the file data
        otaBufferData = otaClient->read((uint8_t *)otaBuffer, bytesToRead);
    }
}

// Read a line from the HTTP header
int otaReadHeaderLine()
{
    int bytesToRead;
    String otaReleasedFirmwareVersion;
    int status;

    // Determine if the network is shutting down
    if (networkIsShuttingDown(NETWORK_USER_OTA_FIRMWARE_UPDATE))
    {
        systemPrintln("OTA: Network is shutting down!");
        otaStop();
        return -1;
    }

    // Determine if the network is connected to the media
    if (!networkUserConnected(NETWORK_USER_OTA_FIRMWARE_UPDATE))
    {
        systemPrintln("OTA: Network has failed!");
        otaStop();
        return -1;
    }

    // Verify the connection to the HTTP server
    if (!otaClient->connected())
    {
        systemPrintln("OTA: HTTP connection broken!");
        otaStop();
        return -1;
    }

    // Read in the released firmware data
    while (otaClient->available())
    {
        otaBuffer[otaBufferData] = otaClient->read();

        // Drop the carriage return
        if (otaBuffer[otaBufferData] != '\r')
        {
            // Build the header line
            if (otaBuffer[otaBufferData] != '\n')
                otaBufferData += 1;
            else
            {
                // Zero-terminate the header line
                otaBuffer[otaBufferData] = 0;
                return 0;
            }
        }
    }
    return 1;
}

// Set the next OTA state
void otaSetState(uint8_t newState)
{
    char string1[40];
    char string2[40];
    const char * arrow;
    const char * asterisk;
    const char * initialState;
    const char * endingState;
    bool pd;

    // Display the state transition
    pd = PERIODIC_DISPLAY(PD_OTA_CLIENT_STATE);
    if ((settings.debugFirmwareUpdate) || pd)
    {
        arrow = "";
        asterisk = "";
        initialState = "";
        if (newState == otaState)
            asterisk = "*";
        else
        {
            initialState = otaGetStateName(otaState, string1);
            arrow = " --> ";
        }
    }

    // Set the new state
    otaState = newState;
    if ((settings.debugFirmwareUpdate) || pd)
    {
        // Display the new firmware update state
        PERIODIC_CLEAR(PD_OTA_CLIENT_STATE);
        endingState = otaGetStateName(newState, string2);
        if (!online.rtc)
            systemPrintf("%s%s%s%s\r\n", asterisk, initialState, arrow, endingState);
        else
        {
            // Timestamp the state change
            //          1         2
            // 12345678901234567890123456
            // YYYY-mm-dd HH:MM:SS.xxxrn0
            struct tm timeinfo = rtc.getTimeStruct();
            char s[30];
            strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &timeinfo);
            systemPrintf("%s%s%s%s, %s.%03ld\r\n", asterisk, initialState, arrow, endingState, s, rtc.getMillis());
        }
    }

    // Display the starting percentage
    if (otaState == OTA_STATE_BIN_FILE_READ_DATA)
        otaDisplayPercentage(otaFileBytes, otaFileSize, pd);

    // Validate the firmware update state
    if (newState >= OTA_STATE_MAX)
        reportFatalError("Invalid OTA state");
}

// Stop the OTA firmware update
void otaStop()
{
    if (settings.debugFirmwareUpdate)
        systemPrintln("otaStop called");

    if (otaState != OTA_STATE_OFF)
    {
        // Stop WiFi
        systemPrintln("OTA stopping WiFi");
        online.otaFirmwareUpdate = false;

        // Stop writing to flash
        if (Update.isRunning())
            Update.abort();

        // Close the SSL connection
        if (otaClient)
        {
            delete otaClient;
            otaClient = nullptr;
        }

        // Close the network connection
        if (networkGetUserNetwork(NETWORK_USER_OTA_FIRMWARE_UPDATE))
            networkUserClose(NETWORK_USER_OTA_FIRMWARE_UPDATE);

        // Stop the firmware update
        otaBufferData = 0;
        otaJsonFileData = String("");
        otaSetState(OTA_STATE_OFF);
        otaTimer = millis();

        // Restart bluetooth if necessary
        if (otaBluetoothState == BT_CONNECTED)
        {
            otaBluetoothState = BT_OFF;
            if (settings.debugFirmwareUpdate)
                systemPrintln("Firmware update restarting Bluetooth");
            bluetoothStart(); // Restart BT according to settings
        }
    }
};

int otaWriteDataToFlash(int bytesToWrite)
{
    int bytesWritten;

    bytesWritten = 0;
    if (bytesToWrite)
    {
        // Write the data to flash
        bytesWritten = Update.write((uint8_t *)otaBuffer, bytesToWrite);
        if (bytesWritten)
        {
            otaFileBytes += bytesWritten;
            if (bytesWritten != bytesToWrite)
            {
                // Only a portion of the data was written, move the rest of
                // the data to the beginning of the buffer
                memcpy(otaBuffer, &otaBuffer[bytesWritten], bytesToWrite - bytesWritten);
                if (settings.debugFirmwareUpdate)
                    systemPrintf("OTA: Wrote only %d of %d bytes to flash\r\n", bytesWritten, otaBufferData);
            }

            // Display the percentage written
            otaPullCallback(otaFileBytes, otaFileSize);
        }
    }

    // Return the number of bytes to write
    return bytesToWrite - bytesWritten;
}

//----------------------------------------
// Over-The-Air (OTA) firmware update state machine
//----------------------------------------

// Perform the over-the-air (OTA) firmware updates
void otaClientUpdate()
{
    int bytesWritten;
    int32_t checkIntervalMillis;
    NETWORK_DATA * network;
    String otaReleasedFirmwareVersion;
    int status;

    // Perform the firmware update
    if (!inMainMenu)
    {
        // Shutdown the OTA client when the mode or setting changes
        DMW_st(otaSetState, otaState);
        if (NEQ_RTK_MODE(otaClientMode) || (!settings.enableAutoFirmwareUpdate))
        {
            if (otaState > OTA_STATE_OFF)
            {
                otaStop();

                // Due to the interruption, enable a fast retry
                otaTimer = millis() - checkIntervalMillis + OTA_NO_PROGRESS_TIMEOUT;
            }
        }

        // Walk the state machine to do the firmware update
        switch (otaState)
        {
            // Handle invalid OTA states
            default: {
                systemPrintf("ERROR: Unknown OTA state (%d)\r\n", otaState);
                otaStop();
                break;
            }

            // Over-the-air firmware updates are not active
            case OTA_STATE_OFF: {
                // Determine if the user enabled automatic firmware updates
                if (EQ_RTK_MODE(otaClientMode) && settings.enableAutoFirmwareUpdate)
                {
                    // Wait until it is time to check for a firmware update
                    checkIntervalMillis = settings.autoFirmwareCheckMinutes * 60 * 1000;
                    if ((int32_t)(millis() - otaTimer) >= checkIntervalMillis)
                    {
                        otaTimer = millis();
                        online.otaFirmwareUpdate = true;
                        otaSetState(OTA_STATE_START_NETWORK);
                    }
                }
                break;
            }

            // Start the network
            case OTA_STATE_START_NETWORK: {
                if (settings.debugFirmwareUpdate)
                    systemPrintln("OTA starting network");
                if (!networkUserOpen(NETWORK_USER_OTA_FIRMWARE_UPDATE, NETWORK_TYPE_ACTIVE))
                {
                    systemPrintln("OTA: Firmware update failed, unable to start network");
                    otaStop();
                }
                else
                    otaSetState(OTA_STATE_WAIT_FOR_NETWORK);
                break;
            }

            // Wait for connection to the network
            case OTA_STATE_WAIT_FOR_NETWORK: {
                // Determine if the network has failed
                if (networkIsShuttingDown(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintln("OTA: Network is shutting down!");
                    otaStop();
                }

                // Determine if the network is connected to the media
                else if (networkUserConnected(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintln("OTA connected to network");

                    // Allocate the OTA firmware update client
                    otaClient = networkClient(NETWORK_USER_OTA_FIRMWARE_UPDATE, OTA_USE_SSL);
                    if (!otaClient)
                    {
                        systemPrintln("ERROR: Failed to allocate OTA client!");
                        otaStop();
                    }
                    else
                    {
                        // Stop Bluetooth
                        otaBluetoothState = bluetoothGetState();
                        if (otaBluetoothState != BT_OFF)
                        {
                            if (settings.debugFirmwareUpdate)
                                systemPrintln("OTA stopping Bluetooth");
                            bluetoothStop();
                        }

                        // Connect to GitHub
                        otaSetState(OTA_STATE_JSON_FILE_REQUEST);
                    }
                }
                break;
            }

            // Issue the HTTP request to get the JSON file
            case OTA_STATE_JSON_FILE_REQUEST: {
                // Determine if the network is shutting down
                if (networkIsShuttingDown(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintln("OTA: Network is shutting down!");
                    otaStop();
                }

                // Determine if the network is connected to the media
                else if (!networkUserConnected(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintln("OTA: Network has failed!");
                    otaStop();
                }

                // Attempt to connect to the server using HTTPS
                else if (!otaClient->connect(OTA_SERVER, OTA_SERVER_PORT))
                {
                    // if you didn't get a connection to the server:
                    Serial.println("OTA: Connection failed");
                    otaStop();
                }
                else
                {
                    Serial.println("OTA: Requesting JSON file");

                    // Make the HTTP request:
                    otaClient->print("GET ");
                    otaClient->print(OTA_FIRMWARE_JSON_URL);
                    otaClient->println(" HTTP/1.1");
                    otaClient->println("User-Agent: RTK OTA Client");
                    otaClient->print("Host: ");
                    otaClient->println(OTA_SERVER);
                    otaClient->println("Connection: close");
                    otaClient->println();
                    otaBufferData = 0;
                    otaSetState(OTA_STATE_JSON_FILE_PARSE_HTTP_STATUS);
                }
                break;
            }

            // Locate the HTTP status header
            case OTA_STATE_JSON_FILE_PARSE_HTTP_STATUS: {
                status = otaReadHeaderLine();
                if (status)
                    break;

                // Verify that the server found the file
                status = otaParseJsonStatus();
                if (settings.debugFirmwareUpdate)
                    systemPrintf("OTA: Server file status: %d\r\n", status);
                if (status != 200)
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintln("OTA: Server failed to locate the JSON file");
                     otaStop();
                }
                else
                {
                    otaBufferData = 0;
                    otaSetState(OTA_STATE_JSON_FILE_PARSE_LENGTH);
                }
                break;
            }

            // Locate the file length header
            case OTA_STATE_JSON_FILE_PARSE_LENGTH: {
                status = otaReadHeaderLine();
                if (status)
                    break;

                // Verify the header line length
                if (!otaBufferData)
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintln("OTA: JSON file length not found");
                    otaStop();
                    break;
                }

                // Get the file length
                otaFileSize = otaParseFileLength();
                if (otaFileSize >= 0)
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintf("OTA: JSON file length %d bytes\r\n", otaFileSize);
                    otaBufferData = 0;
                    otaFileBytes = 0;
                    otaSetState(OTA_STATE_JSON_FILE_SKIP_HEADERS);
                }
                break;
            }

            // Skip over the rest of the HTTP headers
            case OTA_STATE_JSON_FILE_SKIP_HEADERS: {
                status = otaReadHeaderLine();
                if (status)
                    break;

                // Determine if this is the separater between the HTTP headers
                // and the file data
                if (!otaBufferData)
                    otaSetState(OTA_STATE_JSON_FILE_READ_DATA);
                otaBufferData = 0;
                break;
            }

            // Receive the JSON data from the HTTP server
            case OTA_STATE_JSON_FILE_READ_DATA: {
                // Determine if the network is shutting down
                if (networkIsShuttingDown(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintf("OTA: Network is shutting down after %d bytes!\r\n", otaFileBytes);
                    otaStop();
                }

                // Determine if the network is connected to the media
                else if (!networkUserConnected(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintf("OTA: Network has failed after %d bytes!\r\n", otaFileBytes);
                    otaStop();
                }

                // Verify the connection to the HTTP server
                else if (!otaClient->connected())
                {
                    systemPrintf("OTA: HTTP connection broken after %d bytes!\r\n", otaFileBytes);
                    otaStop();
                }
                else
                {
                    // Read data from the JSON file
                    otaReadFileData(sizeof(otaBuffer) - 1);
                    if (otaBufferData)
                    {
                        // Zero terminate the file data in the buffer
                        otaBuffer[otaBufferData] = 0;

                        // Append the JSON file data to the string
                        otaJsonFileData += String(&otaBuffer[0]);
                        otaBufferData = 0;
                    }

                    // Done if not at the end-of-file
                    if (otaJsonFileData.length() != otaFileSize)
                        break;

                    // Reached end-of-file
                    // Parse the JSON file
                    if (settings.debugFirmwareUpdate)
                        systemPrintf("OTA: JSON data: %s\r\n", otaJsonFileData.c_str());
                    otaSetState(OTA_STATE_PARSE_JSON_DATA);
                }
                break;
            }

            // Parse the JSON data and determine if new firmware is available
            case OTA_STATE_PARSE_JSON_DATA: {
                // Locate the fields in the JSON file
                DynamicJsonDocument doc(1000);
                if (deserializeJson(doc, otaJsonFileData.c_str()) != DeserializationError::Ok)
                {
                    systemPrintln("OTA: Failed to parse the JSON file data");
                    otaStop();
                }
                else
                {
                    char versionString[9];

                    // Get the current version
                    getFirmwareVersion(versionString, sizeof(versionString), false);

                    // Step through the configurations looking for a match
                    for (auto config : doc["Configurations"].as<JsonArray>())
                    {
                        // Get the latest released version
                        otaReleasedFirmwareVersion = config["Version"].isNull() ? "" : (const char *)config["Version"];
                        otaReleasedFirmwareURL = config["URL"].isNull() ? "" : (const char *)config["URL"];
                        if ((tolower(versionString[0]) != 'd')
                            && (FIRMWARE_VERSION_MAJOR != 99)
                            && (String(&versionString[1]) >= otaReleasedFirmwareVersion))
                        {
                            if (settings.debugFirmwareUpdate)
                                systemPrintf("OTA: Current firmware %s is beyond released %s, no change necessary.\r\n",
                                             versionString, otaReleasedFirmwareVersion.c_str());
                            otaStop();
                        }
                        else
                        {
                            if (settings.debugFirmwareUpdate)
                                systemPrintf("OTA: Firmware URL %s\r\n", otaReleasedFirmwareURL.c_str());
                            if ((strncasecmp(HTTPS_TRANSPORT,
                                             otaReleasedFirmwareURL.c_str(),
                                             strlen(HTTPS_TRANSPORT)) != 0)
                                || (strncasecmp(OTA_SERVER,
                                                &otaReleasedFirmwareURL.c_str()[strlen(HTTPS_TRANSPORT)],
                                                strlen(OTA_SERVER)) != 0))
                            {
                                if (settings.debugFirmwareUpdate)
                                    systemPrintln("OTA: Invalid firmware URL");
                                otaStop();
                            }
                            else
                            {
                                // Break the connection with the HTTP server
                                otaClient->stop();
                                if (settings.debugFirmwareUpdate)
                                    systemPrintf("OTA: Upgrading firmware from %s to %s\r\n",
                                                 versionString, otaReleasedFirmwareVersion.c_str());
                                otaReleasedFirmwareURL.remove(0, strlen(HTTPS_TRANSPORT) + strlen(OTA_SERVER));
                                otaSetState(OTA_STATE_BIN_FILE_REQUEST);
                            }
                        }
                        break;
                    }
                }
                break;
            }

            // Issue the HTTP request to get the released firmware file
            case OTA_STATE_BIN_FILE_REQUEST: {
                // Determine if the network is shutting down
                if (networkIsShuttingDown(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintln("OTA: Network is shutting down!");
                    otaStop();
                }

                // Determine if the network is connected to the media
                else if (!networkUserConnected(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                {
                    systemPrintln("OTA: Network has failed!");
                    otaStop();
                }

                // Attempt to connect to the server using HTTPS
                else if (!otaClient->connect(OTA_SERVER, OTA_SERVER_PORT))
                {
                    // if you didn't get a connection to the server:
                    Serial.println("OTA: Connection failed");
                    otaStop();
                }
                else
                {
                    Serial.println("OTA: Requesting BIN file");

                    // Make the HTTP request:
                    otaClient->print("GET ");
                    otaClient->print(otaReleasedFirmwareURL.c_str());
                    otaClient->println(" HTTP/1.1");
                    otaClient->println("User-Agent: RTK OTA Client");
                    otaClient->print("Host: ");
                    otaClient->println(OTA_SERVER);
                    otaClient->println("Connection: close");
                    otaClient->println();
                    otaBufferData = 0;
                    otaSetState(OTA_STATE_BIN_FILE_PARSE_HTTP_STATUS);
                }
                break;
            }

            // Locate the HTTP status header
            case OTA_STATE_BIN_FILE_PARSE_HTTP_STATUS: {
                status = otaReadHeaderLine();
                if (status)
                    break;

                // Verify that the server found the file
                status = otaParseJsonStatus();
                if (settings.debugFirmwareUpdate)
                    systemPrintf("OTA: Server file status: %d\r\n", status);
                if (status != 200)
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintln("OTA: Server failed to locate the JSON file");
                     otaStop();
                }
                else
                {
                    otaBufferData = 0;
                    otaSetState(OTA_STATE_BIN_FILE_PARSE_LENGTH);
                }
                break;
            }

            // Locate the file length header
            case OTA_STATE_BIN_FILE_PARSE_LENGTH: {
                status = otaReadHeaderLine();
                if (status)
                    break;

                // Verify the header line length
                if (!otaBufferData)
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintln("OTA: BIN file length not found");
                    otaStop();
                    break;
                }

                // Get the file length
                otaFileSize = otaParseFileLength();
                if (otaFileSize >= 0)
                {
                    if (settings.debugFirmwareUpdate)
                        systemPrintf("OTA: BIN file length %d bytes\r\n", otaFileSize);
                    otaBufferData = 0;
                    otaFileBytes = 0;
                    otaSetState(OTA_STATE_BIN_FILE_SKIP_HEADERS);
                }
                break;
            }

            // Skip over the rest of the HTTP headers
            case OTA_STATE_BIN_FILE_SKIP_HEADERS: {
                status = otaReadHeaderLine();
                if (status)
                    break;

                // Determine if this is the separater between the HTTP headers
                // and the file data
                if (!otaBufferData)
                {
                    // Start the firmware update process
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                        otaStop();
                    else
                    {
                        otaTimer = millis();
                        otaSetState(OTA_STATE_BIN_FILE_READ_DATA);
                    }
                }
                otaBufferData = 0;
                break;
            }

            // Receive the bin file from the HTTP server
            case OTA_STATE_BIN_FILE_READ_DATA: {
                do
                {
                    bytesWritten = 0;

                    // Determine if the network is shutting down
                    if (networkIsShuttingDown(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                    {
                        systemPrintln("OTA: Network is shutting down!");
                        otaStop();
                    }

                    // Determine if the network is connected to the media
                    else if (!networkUserConnected(NETWORK_USER_OTA_FIRMWARE_UPDATE))
                    {
                        systemPrintln("OTA: Network has failed!");
                        otaStop();
                    }

                    // Verify the connection to the HTTP server
                    else if (!otaClient->connected())
                    {
                        systemPrintf("OTA: HTTP connection broken after %d bytes!\r\n", otaFileBytes);
                        otaStop();
                    }

                    // Determine if progress is being made
                    else if ((millis() - otaTimer) >= OTA_NO_PROGRESS_TIMEOUT)
                    {
                        systemPrintln("OTA: No progress being made, link broken!");
                        otaStop();
                        checkIntervalMillis = settings.autoFirmwareCheckMinutes * 60 * 1000;

                        // Delay for OTA_NO_PROGRESS_TIMEOUT
                        otaTimer = millis() - checkIntervalMillis + OTA_NO_PROGRESS_TIMEOUT;
                    }

                    // Read data and write it to the flash
                    else
                    {
                        // Read data from the binary file
                        if (!otaBufferData)
                            otaReadFileData(sizeof(otaBuffer));

                        // Write the data to the flash
                        if (otaBufferData)
                        {
                            bytesWritten = otaBufferData;
                            otaBufferData = otaWriteDataToFlash(otaBufferData);
                            bytesWritten -= otaBufferData;
                            otaTimer = millis();

                            // Check for end-of-file
                            if (otaFileBytes == otaFileSize)
                            {
                                // The end-of-file was reached
                                if (settings.debugFirmwareUpdate)
                                    systemPrintf("OTA: Downloaded %d bytes\r\n", otaFileBytes);
                                Update.end(true);

                                // Reset the system
                                systemPrintln("OTA: Starting the new firmware");
                                delay(1000);
                                ESP.restart();
                                break;
                            }
                        }
                    }
                } while (bytesWritten);
                break;
            }
        }

        // Periodically display the PVT client state
        if (PERIODIC_DISPLAY(PD_OTA_CLIENT_STATE))
            otaSetState(otaState);
    }
}

// Verify the firmware update tables
void otaVerifyTables()
{
    // Verify the table lengths
    if (otaStateEntries != OTA_STATE_MAX)
        reportFatalError("Fix otaStateNames table to match OtaState");
}

#endif  // COMPILE_NETWORK
