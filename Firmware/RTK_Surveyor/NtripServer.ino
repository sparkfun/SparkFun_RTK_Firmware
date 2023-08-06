/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  NTRIP Server States:
    NTRIP_SERVER_OFF: Network off or using NTRIP Client
    NTRIP_SERVER_ON: WIFI_START state
    NTRIP_SERVER_NETWORK_STARTED: Connecting to the network
    NTRIP_SERVER_NETWORK_CONNECTED: Connected to the network
    NTRIP_SERVER_WAIT_GNSS_DATA: Waiting for correction data from GNSS
    NTRIP_SERVER_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_SERVER_AUTHORIZATION: Validate the credentials
    NTRIP_SERVER_CASTING: Sending correction data to the NTRIP caster

                               NTRIP_SERVER_OFF
                                       |   ^
                      ntripServerStart |   | ntripServerShutdown()
                                       v   |
                    .---------> NTRIP_SERVER_ON <-------------------.
                    |                  |                            |
                    |                  |                            | ntripServerRestart()
                    |                  v                      Fail  |
                    |    NTRIP_SERVER_NETWORK_STARTED ------------->+
                    |                  |                            ^
                    |                  |                            |
                    |                  v                      Fail  |
                    |    NTRIP_SERVER_NETWORK_CONNECTED ----------->+
                    |                  |                            ^
                    |                  |                   Network  |
                    |                  v                      Fail  |
                    |    NTRIP_SERVER_WAIT_GNSS_DATA -------------->+
                    |                  |                            ^
                    |                  | Discard Data      Network  |
                    |                  v                      Fail  |
                    |      NTRIP_SERVER_CONNECTING ---------------->+
                    |                  |                            ^
                    |                  | Discard Data      Network  |
                    |                  v                      Fail  |
                    |     NTRIP_SERVER_AUTHORIZATION -------------->+
                    |                  |                            ^
                    |                  | Discard Data      Network  |
                    |                  v                      Fail  |
                    |         NTRIP_SERVER_CASTING -----------------'
                    |                  |
                    |                  | Data timeout
                    |                  |
                    |                  | Close Server connection
                    '------------------'

  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#if COMPILE_NETWORK

//----------------------------------------
// Constants
//----------------------------------------

// Give up connecting after this number of attempts
// Connection attempts are throttled to increase the time between attempts
// 30 attempts with 5 minute increases will take over 38 hours
static const int MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS = 30;

// Define the NTRIP server states
enum NTRIPServerState
{
    NTRIP_SERVER_OFF = 0,           // Using Bluetooth or NTRIP client
    NTRIP_SERVER_ON,                // WIFI_START state
    NTRIP_SERVER_NETWORK_STARTED,   // Connecting to WiFi access point
    NTRIP_SERVER_NETWORK_CONNECTED, // WiFi connected to an access point
    NTRIP_SERVER_WAIT_GNSS_DATA,    // Waiting for correction data from GNSS
    NTRIP_SERVER_CONNECTING,        // Attempting a connection to the NTRIP caster
    NTRIP_SERVER_AUTHORIZATION,     // Validate the credentials
    NTRIP_SERVER_CASTING,           // Sending correction data to the NTRIP caster
    // Insert new states here
    NTRIP_SERVER_STATE_MAX          // Last entry in the state list
};

const char * const ntripServerStateName[] =
{
    "NTRIP_SERVER_OFF",
    "NTRIP_SERVER_ON",
    "NTRIP_SERVER_NETWORK_STARTED",
    "NTRIP_SERVER_NETWORK_CONNECTED",
    "NTRIP_SERVER_WAIT_GNSS_DATA",
    "NTRIP_SERVER_CONNECTING",
    "NTRIP_SERVER_AUTHORIZATION",
    "NTRIP_SERVER_CASTING"
};

const int ntripServerStateNameEntries = sizeof(ntripServerStateName) / sizeof(ntripServerStateName[0]);

//----------------------------------------
// Locals
//----------------------------------------

// Network connection used to push RTCM to NTRIP caster
static NetworkClient *ntripServer;
static volatile uint8_t ntripServerState = NTRIP_SERVER_OFF;

// Count of bytes sent by the NTRIP server to the NTRIP caster
uint32_t ntripServerBytesSent;

// Throttle the time between connection attempts
// ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
static uint32_t ntripServerConnectionAttemptTimeout;
static uint32_t ntripServerLastConnectionAttempt;
static int ntripServerConnectionAttempts; // Count the number of connection attempts between restarts

// Last time the NTRIP server state was displayed
static uint32_t ntripServerStateLastDisplayed;

// NTRIP server timer usage:
//  * Measure the connection response time
//  * Receive RTCM correction data timeout
//  * Monitor last RTCM byte received for frame counting
static uint32_t ntripServerTimer;
static uint32_t ntripServerStartTime;
static int ntripServerConnectionAttemptsTotal; // Count the number of connection attempts absolutely

//----------------------------------------
// NTRIP Server Routines
//----------------------------------------

// Initiate a connection to the NTRIP caster
bool ntripServerConnectCaster()
{
    const int SERVER_BUFFER_SIZE = 512;
    char serverBuffer[SERVER_BUFFER_SIZE];

    // Remove any http:// or https:// prefix from host name
    char hostname[51];
    strncpy(hostname, settings.ntripServer_CasterHost,
            sizeof(hostname) - 1); // strtok modifies string to be parsed so we create a copy
    char *token = strtok(hostname, "//");
    if (token != nullptr)
    {
        token = strtok(nullptr, "//"); // Advance to data after //
        if (token != nullptr)
            strcpy(settings.ntripServer_CasterHost, token);
    }

    if (settings.debugNtripServerState)
        systemPrintf("NTRIP Server connecting to %s:%d\r\n",
                     settings.ntripServer_CasterHost,
                     settings.ntripServer_CasterPort);

    // Attempt a connection to the NTRIP caster
    if (!ntripServer->connect(settings.ntripServer_CasterHost, settings.ntripServer_CasterPort))
        return false;

    if (settings.debugNtripServerState)
        systemPrintln("NTRIP Server sending authorization credentials");

    // Build the authorization credentials message
    //  * Mount point
    //  * Password
    //  * Agent
    snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/\r\n\r\n",
             settings.ntripServer_MountPointPW, settings.ntripServer_MountPoint, platformPrefix);
    int length = strlen(serverBuffer);
    getFirmwareVersion(&serverBuffer[length], sizeof(serverBuffer) - length, false);

    // Send the authorization credentials to the NTRIP caster
    ntripServer->write((const uint8_t *)serverBuffer, strlen(serverBuffer));
    return true;
}

// Determine if the connection limit has been reached
bool ntripServerConnectLimitReached()
{
    // Retry the connection a few times
    bool limitReached = (ntripServerConnectionAttempts >= MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS);

    // Shutdown the NTRIP server
    ntripServerStop(limitReached);

    ntripServerConnectionAttempts++;
    ntripServerConnectionAttemptsTotal++;

    if (limitReached == false)
    {
        if (ntripServerConnectionAttempts == 1)
            ntripServerConnectionAttemptTimeout = 15 * 1000L; // Wait 15s
        else if (ntripServerConnectionAttempts == 2)
            ntripServerConnectionAttemptTimeout = 30 * 1000L; // Wait 30s
        else if (ntripServerConnectionAttempts == 3)
            ntripServerConnectionAttemptTimeout = 1 * 60 * 1000L; // Wait 1 minute
        else if (ntripServerConnectionAttempts == 4)
            ntripServerConnectionAttemptTimeout = 2 * 60 * 1000L; // Wait 2 minutes
        else
            ntripServerConnectionAttemptTimeout =
                (ntripServerConnectionAttempts - 4) * 5 * 60 * 1000L; // Wait 5, 10, 15, etc minutes between attempts

        reportHeapNow();
    }
    else
        // No more connection attempts
        systemPrintln("NTRIP Server connection attempts exceeded!");
    return limitReached;
}

// Determine if the NTRIP server is casting
bool ntripServerIsCasting()
{
    return (ntripServerState == NTRIP_SERVER_CASTING);
}

// Print the NTRIP server state summary
void ntripServerPrintStateSummary()
{
    switch (ntripServerState)
    {
    default:
        systemPrintf("Unknown: %d", ntripServerState);
        break;
    case NTRIP_SERVER_OFF:
        systemPrint("Disconnected");
        break;
    case NTRIP_SERVER_ON:
    case NTRIP_SERVER_NETWORK_STARTED:
    case NTRIP_SERVER_NETWORK_CONNECTED:
    case NTRIP_SERVER_WAIT_GNSS_DATA:
    case NTRIP_SERVER_CONNECTING:
    case NTRIP_SERVER_AUTHORIZATION:
        systemPrint("Connecting");
        break;
    case NTRIP_SERVER_CASTING:
        systemPrint("Connected");
        break;
    }
}

// Print the NTRIP server status
void ntripServerPrintStatus ()
{
    uint64_t milliseconds;
    uint32_t days;
    byte hours;
    byte minutes;
    byte seconds;

    if (settings.enableNtripServer == true &&
        (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING))
    {
        systemPrint("NTRIP Server ");
        ntripServerPrintStateSummary();
        systemPrintf(" - %s/%s:%d", settings.ntripServer_CasterHost, settings.ntripServer_MountPoint,
                     settings.ntripServer_CasterPort);

        if (ntripServerState == NTRIP_SERVER_CASTING)
            // Use ntripServerTimer since it gets reset after each successful data
            // receiption from the NTRIP caster
            milliseconds = ntripServerTimer - ntripServerStartTime;
        else
        {
            milliseconds = ntripServerStartTime;
            systemPrint(" Last");
        }

        // Display the uptime
        days = milliseconds / MILLISECONDS_IN_A_DAY;
        milliseconds %= MILLISECONDS_IN_A_DAY;

        hours = milliseconds / MILLISECONDS_IN_AN_HOUR;
        milliseconds %= MILLISECONDS_IN_AN_HOUR;

        minutes = milliseconds / MILLISECONDS_IN_A_MINUTE;
        milliseconds %= MILLISECONDS_IN_A_MINUTE;

        seconds = milliseconds / MILLISECONDS_IN_A_SECOND;
        milliseconds %= MILLISECONDS_IN_A_SECOND;

        systemPrint(" Uptime: ");
        systemPrintf("%d %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
                     days, hours, minutes, seconds, milliseconds, ntripServerConnectionAttemptsTotal);
    }
}

// This function gets called as each RTCM byte comes in
void ntripServerProcessRTCM(uint8_t incoming)
{
    if (ntripServerState == NTRIP_SERVER_CASTING)
    {
        // Generate and print timestamp if needed
        uint32_t currentMilliseconds;
        static uint32_t previousMilliseconds = 0;
        if (online.rtc)
        {
            // Timestamp the RTCM messages
            currentMilliseconds = millis();
            if (((settings.debugNtripServerRtcm && ((currentMilliseconds - previousMilliseconds) > 5))
                || PERIODIC_DISPLAY(PD_NTRIP_SERVER_DATA)) && (!settings.enableRtcmMessageChecking)
                && (!inMainMenu) && ntripServerBytesSent)
            {
                PERIODIC_CLEAR(PD_NTRIP_SERVER_DATA);
                printTimeStamp();
                //         1         2         3
                // 123456789012345678901234567890
                // YYYY-mm-dd HH:MM:SS.xxxrn0
                struct tm timeinfo = rtc.getTimeStruct();
                char timestamp[30];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
                systemPrintf("    Tx RTCM: %s.%03ld, %d bytes sent\r\n", timestamp, rtc.getMillis(), ntripServerBytesSent);
            }
            previousMilliseconds = currentMilliseconds;
        }

        // If we have not gotten new RTCM bytes for a period of time, assume end of frame
        if (((millis() - ntripServerTimer) > 100) && (ntripServerBytesSent > 0))
        {
            if ((!inMainMenu) && settings.debugNtripServerState)
                systemPrintf("NTRIP Server transmitted %d RTCM bytes to Caster\r\n", ntripServerBytesSent);

            ntripServerBytesSent = 0;
        }

        if (ntripServer->connected())
        {
            ntripServer->write(incoming); // Send this byte to socket
            ntripServerBytesSent++;
            ntripServerTimer = millis();
            netOutgoingRTCM = true;
        }
    }

    // Indicate that the GNSS is providing correction data
    else if (ntripServerState == NTRIP_SERVER_WAIT_GNSS_DATA)
    {
        ntripServerSetState(NTRIP_SERVER_CONNECTING);
        rtcmParsingState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;
    }
}

// Read the authorization response from the NTRIP caster
void ntripServerResponse(char *response, size_t maxLength)
{
    char *responseEnd;

    // Make sure that we can zero terminate the response
    responseEnd = &response[maxLength - 1];

    // Read bytes from the caster and store them
    while ((response < responseEnd) && ntripServer->available())
        *response++ = ntripServer->read();

    // Zero terminate the response
    *response = '\0';
}

// Restart the NTRIP server
void ntripServerRestart()
{
    // Save the previous uptime value
    if (ntripServerState == NTRIP_SERVER_CASTING)
        ntripServerStartTime = ntripServerTimer - ntripServerStartTime;
    ntripServerStop(false);
}

// Update the state of the NTRIP server state machine
void ntripServerSetState(uint8_t newState)
{
    if (settings.debugNtripServerState || PERIODIC_DISPLAY(PD_NTRIP_SERVER_STATE))
    {
        if (ntripServerState == newState)
            systemPrint("*");
        else
            systemPrintf("%s --> ", ntripServerStateName[ntripServerState]);
    }
    ntripServerState = newState;
    if (settings.debugNtripServerState || PERIODIC_DISPLAY(PD_NTRIP_SERVER_STATE))
    {
        PERIODIC_CLEAR(PD_NTRIP_SERVER_STATE);
        if (newState >= NTRIP_SERVER_STATE_MAX)
        {
            systemPrintf("Unknown NTRIP Server state: %d\r\n", newState);
            reportFatalError("Unknown NTRIP Server state");
        }
        else
            systemPrintln(ntripServerStateName[ntripServerState]);
    }
}

// Shutdown the NTRIP server
void ntripServerShutdown()
{
    ntripServerStop(true);
}

// Start the NTRIP server
void ntripServerStart()
{
    // Stop the NTRIP server and network
    ntripServerShutdown();

    // Start the NTRIP server if enabled
    if ((settings.ntripServer_StartAtSurveyIn == true) || (settings.enableNtripServer == true))
    {
        // Display the heap state
        reportHeapNow();
        systemPrintln ("NTRIP Server start");
        ntripServerSetState(NTRIP_SERVER_ON);
    }

    ntripServerConnectionAttempts = 0;
}

// Stop the NTRIP server
void ntripServerStop(bool shutdown)
{
    if (ntripServer)
    {
        // Break the NTRIP server connection if necessary
        if (ntripServer->connected())
            ntripServer->stop();

        // Free the NTRIP server resources
        delete ntripServer;
        ntripServer = nullptr;
    }

    // Increase timeouts if we started the network
    if (ntripServerState > NTRIP_SERVER_ON)
    {
        // Mark the Server stop so that we don't immediately attempt re-connect to Caster
        ntripServerTimer = millis();
        ntripServerConnectionAttemptTimeout =
            15 * 1000L; // Wait 15s between stopping and the first re-connection attempt. 5 is too short for Emlid.
    }

    // Determine the next NTRIP server state
    ntripServerSetState(shutdown ? NTRIP_SERVER_OFF : NTRIP_SERVER_ON);
    online.ntripServer = false;
}

// Update the NTRIP server state machine
void ntripServerUpdate()
{
    // For Ref Stn, process any RTCM data waiting in the u-blox library RTCM Buffer
    // This causes the state change from NTRIP_SERVER_WAIT_GNSS_DATA to NTRIP_SERVER_CONNECTING
    processRTCMBuffer();

    if (settings.enableNtripServer == false)
    {
        // If user turns off NTRIP Server via settings, stop server
        if (ntripServerState > NTRIP_SERVER_OFF)
            ntripServerShutdown();
        return;
    }

    if (wifiInConfigMode())
        return; // Do not service NTRIP during WiFi config

    // Periodically display the NTRIP server state
    if (settings.debugNtripServerState && ((millis() - ntripServerStateLastDisplayed) > 15000))
    {
        ntripServerSetState(ntripServerState);
        ntripServerStateLastDisplayed = millis();
    }

    // Enable the network and the NTRIP server if requested
    switch (ntripServerState)
    {
    case NTRIP_SERVER_OFF:
        break;

    // Start the network
    case NTRIP_SERVER_ON:
        if (HAS_ETHERNET)
        {
            if (online.ethernetStatus == ETH_NOT_STARTED)
            {
                systemPrintln("Ethernet not started. Can not start NTRIP Server");
                ntripServerRestart();
            }
            else if ((online.ethernetStatus >= ETH_STARTED_CHECK_CABLE) && (online.ethernetStatus <= ETH_CONNECTED))
            {
                // Pause until connection timeout has passed
                if ((millis() - ntripServerTimer) > ntripServerConnectionAttemptTimeout)
                {
                    log_d("NTRIP Server starting on Ethernet");
                    ntripServerTimer = millis();
                    ntripServerSetState(NTRIP_SERVER_NETWORK_STARTED);
                }
            }
            else
            {
                systemPrintln("Error: Please connect Ethernet before starting NTRIP Server");
                ntripServerShutdown();
            }
        }
        else
        {
            if (wifiNetworkCount() == 0)
            {
                systemPrintln("Error: Please enter at least one SSID before starting NTRIP Server");
                ntripServerShutdown();
            }
            else
            {
                // Pause until connection timeout has passed
                if ((millis() - ntripServerLastConnectionAttempt) > ntripServerConnectionAttemptTimeout)
                {
                    log_d("NTRIP Server starting WiFi");
                    wifiStart();
                    ntripServerTimer = millis();
                    ntripServerSetState(NTRIP_SERVER_NETWORK_STARTED);
                }
            }
        }
        break;

    // Wait for connection to an access point
    case NTRIP_SERVER_NETWORK_STARTED:
        // Determine if the RTK timed out connecting to the network
        if ((millis() - ntripServerTimer) > (1 * 60 * 1000))
        {
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();
            break;
        }

        // Determine if the RTK has connected to the network
        if (wifiIsConnected() || (HAS_ETHERNET && (online.ethernetStatus != ETH_CONNECTED)))
        {
            // Allocate the ntripServer structure
            ntripServer = new NetworkClient(false);
            if (ntripServer)
                ntripServerSetState(NTRIP_SERVER_NETWORK_CONNECTED);
            else
            {
                // Failed to allocate the ntripServer structure
                systemPrintln("ERROR: Failed to allocate the ntripServer structure!");
                ntripServerShutdown();
            }
        }
        break;

    // Network available
    case NTRIP_SERVER_NETWORK_CONNECTED:
        if (settings.enableNtripServer)
        {
            // No RTCM correction data sent yet
            rtcmPacketsSent = 0;

            // Open socket to NTRIP caster
            ntripServerSetState(NTRIP_SERVER_WAIT_GNSS_DATA);
        }
        break;

    // Wait for GNSS correction data
    case NTRIP_SERVER_WAIT_GNSS_DATA:
        // State change handled in ntripServerProcessRTCM
        break;

    // Initiate the connection to the NTRIP caster
    case NTRIP_SERVER_CONNECTING:
        // Attempt a connection to the NTRIP caster
        if (!ntripServerConnectCaster())
        {
            // Assume service not available
            if (ntripServerConnectLimitReached()) // Update ntripServerConnectionAttemptTimeout
                systemPrintln("NTRIP Server failed to connect! Do you have your caster address and port correct?");
            else
            {
                if (ntripServerConnectionAttemptTimeout / 1000 < 120)
                    systemPrintf("NTRIP Server failed to connect to caster. Trying again in %d seconds.\r\n",
                                 ntripServerConnectionAttemptTimeout / 1000);
                else
                    systemPrintf("NTRIP Server failed to connect to caster. Trying again in %d minutes.\r\n",
                                 ntripServerConnectionAttemptTimeout / 1000 / 60);
            }
        }
        else
        {
            // Connection open to NTRIP caster, wait for the authorization response
            ntripServerTimer = millis();
            ntripServerSetState(NTRIP_SERVER_AUTHORIZATION);
        }
        break;

    // Wait for authorization response
    case NTRIP_SERVER_AUTHORIZATION:
        // Check if caster service responded
        if (ntripServer->available() < strlen("ICY 200 OK")) // Wait until at least a few bytes have arrived
        {
            // Check for response timeout
            if (millis() - ntripServerTimer > 10000)
            {
                if (ntripServerConnectLimitReached())
                    systemPrintln("Caster failed to respond. Do you have your caster address and port correct?");
                else
                {
                    if (ntripServerConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("NTRIP caster failed to respond. Trying again in %d seconds.\r\n",
                                     ntripServerConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("NTRIP caster failed to respond. Trying again in %d minutes.\r\n",
                                     ntripServerConnectionAttemptTimeout / 1000 / 60);

                    // Restart network operation after delay
                    ntripServerRestart();
                }
            }
        }
        else
        {
            // NTRIP caster's authorization response received
            char response[512];
            ntripServerResponse(response, sizeof(response));

            // Look for various responses
            if (strstr(response, "200") != nullptr) //'200' found
            {
                systemPrintf("NTRIP Server connected to %s:%d %s\r\n", settings.ntripServer_CasterHost,
                             settings.ntripServer_CasterPort, settings.ntripServer_MountPoint);

                // Connection is now open, start the RTCM correction data timer
                ntripServerTimer = millis();

                // We don't use a task because we use I2C hardware (and don't have a semphore).
                online.ntripServer = true;
                ntripServerConnectionAttempts = 0;
                ntripServerSetState(NTRIP_SERVER_CASTING);
            }

            // Look for '401 Unauthorized'
            else if (strstr(response, "401") != nullptr)
            {
                systemPrintf(
                    "NTRIP Caster responded with bad news: %s. Are you sure your caster credentials are correct?\r\n",
                    response);

                // Give up - Shutdown NTRIP server, no further retries
                ntripServerShutdown();
            }

            // Look for banned IP information
            else if (strstr(response, "banned") != nullptr) //'Banned' found
            {
                systemPrintf("NTRIP Server connected to caster but caster responded with problem: %s\r\n", response);

                // Give up - Shutdown NTRIP server, no further retries
                ntripServerShutdown();
            }

            // Other errors returned by the caster
            else
            {
                systemPrintf("NTRIP Server connected but caster responded with problem: %s\r\n", response);

                // Check for connection limit
                if (ntripServerConnectLimitReached())
                    systemPrintln("NTRIP Server retry limit reached; do you have your caster address and port correct?");

                // Attempt to reconnect after throttle controlled timeout
                else
                {
                    if (ntripServerConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("NTRIP Server attempting connection in %d seconds.\r\n",
                                     ntripServerConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("NTRIP Server attempting connection in %d minutes.\r\n",
                                     ntripServerConnectionAttemptTimeout / 1000 / 60);
                }
            }
        }
        break;

    // NTRIP server authorized to send RTCM correction data to NTRIP caster
    case NTRIP_SERVER_CASTING:
        // Check for a broken connection
        if (!ntripServer->connected())
        {
            // Broken connection, retry the NTRIP connection
            systemPrintln("Connection to NTRIP Caster was lost");
            ntripServerRestart();
        }
        else if ((millis() - ntripServerTimer) > (3 * 1000))
        {
            // GNSS stopped sending RTCM correction data
            systemPrintln("NTRIP Server breaking connection to caster due to lack of RTCM data!");
            ntripServerRestart();
        }
        else
        {
            // All is well
            cyclePositionLEDs();
        }
        break;
    }

    // Periodically display the state
    if (PERIODIC_DISPLAY(PD_NTRIP_SERVER_STATE))
        ntripServerSetState(ntripServerState);
}

// Verify the NTRIP server tables
void ntripServerValidateTables()
{
    if (ntripServerStateNameEntries != NTRIP_SERVER_STATE_MAX)
        reportFatalError("Fix ntripServerStateNameEntries to match NTRIPServerState");
}

#endif  // COMPILE_NETWORK
