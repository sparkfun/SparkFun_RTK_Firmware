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

enum OtaState
{
    OTA_STATE_OFF = 0,
    OTA_STATE_START_NETWORK,
    // Insert new states before this line
    OTA_STATE_MAX
};

const char * const otaStateNames[] =
{
    "OTA_STATE_OFF",
    "OTA_STATE_START_NETWORK",
};
const int otaStateEntries = sizeof(otaStateNames) / sizeof(otaStateNames[0]);

//----------------------------------------
// Locals
//----------------------------------------

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

//----------------------------------------
// Over-The-Air (OTA) firmware update state machine
//----------------------------------------

// Perform the over-the-air (OTA) firmware updates
void otaClientUpdate()
{
    int32_t checkIntervalMillis;

    // Perform the firmware update
    if (!inMainMenu)
    {
        // Walk the state machine to do the firmware update
        switch (otaState)
        {
            // Handle invalid OTA states
            default: {
                systemPrintf("ERROR: Unknown OTA state (%d)\r\n", otaState);
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
                        otaSetState(OTA_STATE_START_NETWORK);
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
