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

#define OTA_USE_SSL             1

enum OtaState
{
    OTA_STATE_OFF = 0,
    OTA_STATE_START_NETWORK,
    OTA_STATE_WAIT_FOR_NETWORK,
    OTA_STATE_JSON_FILE_REQUEST,
    // Insert new states before this line
    OTA_STATE_MAX
};

const char * const otaStateNames[] =
{
    "OTA_STATE_OFF",
    "OTA_STATE_START_NETWORK",
    "OTA_STATE_WAIT_FOR_NETWORK",
    "OTA_STATE_JSON_FILE_REQUEST",
};
const int otaStateEntries = sizeof(otaStateNames) / sizeof(otaStateNames[0]);

//----------------------------------------
// Locals
//----------------------------------------

static byte otaBluetoothState = BT_OFF;
static int otaBufferData;
static NetworkClient * otaClient;
static String otaJsonFileData;
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

//----------------------------------------
// Over-The-Air (OTA) firmware update state machine
//----------------------------------------

// Perform the over-the-air (OTA) firmware updates
void otaClientUpdate()
{
    int32_t checkIntervalMillis;
    NETWORK_DATA * network;

    // Perform the firmware update
    if (!inMainMenu)
    {
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
                if (settings.enableAutoFirmwareUpdate)
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
