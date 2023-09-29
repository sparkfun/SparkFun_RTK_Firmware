/*------------------------------------------------------------------------------
NtripServer.ino

  The NTRIP server sits on top of the network layer and sends correction data
  from the ZED (GNSS radio) to an NTRIP caster.

                Satellite     ...    Satellite
                     |         |          |
                     |         |          |
                     |         V          |
                     |        RTK         |
                     '------> Base <------'
                            Station
                               |
                               | NTRIP Server sends correction data
                               V
                         NTRIP Caster
                               |
                               | NTRIP Client receives correction data
                               V
            Bluetooth         RTK        Network: NMEA Client
           .---------------- Rover --------------------------.
           |                   |                             |
           | NMEA              | Network: NEMA Server        | NMEA
           | position          | NEMA position data          | position
           | data              V                             | data
           |              Computer or                        |
           '------------> Cell Phone <-----------------------'
                          for display

  NTRIP Server Testing:

     Using Ethernet on RTK Reference Station:

        1. Network failure - Disconnect Ethernet cable at RTK Reference Station,
           expecting retry NTRIP client connection after network restarts

     Using WiFi on RTK Express or RTK Reference Station:

        1. Internet link failure - Disconnect Ethernet cable between Ethernet
           switch and the firewall to simulate an internet failure, expecting
           retry NTRIP server connection after delay

        2. Internet outage - Disconnect Ethernet cable between Ethernet
           switch and the firewall to simulate an internet failure, expecting
           retries to exceed the connection limit causing the NTRIP server to
           shutdown after about 25 hours and 18 minutes.  Restarting the NTRIP
           server may be done by rebooting the RTK or by using the configuration
           menus to turn off and on the NTRIP server.

  Test Setup:

                          RTK Reference Station
                           ^                 ^
                      WiFi |                 | Ethernet cable
                           v                 v
            WiFi Access Point <-----------> Ethernet Switch
                                Ethernet     ^
                                 Cable       | Ethernet cable
                                             v
                                     Internet Firewall
                                             ^
                                             | Ethernet cable
                                             v
                                           Modem
                                             ^
                                             |
                                             v
                                         Internet
                                             ^
                                             |
                                             v
                                       NTRIP Caster

  Possible NTRIP Casters

    * https://emlid.com/ntrip-caster/
    * http://rtk2go.com/
    * private SNIP NTRIP caster
------------------------------------------------------------------------------*/

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
// Connection attempts are throttled by increasing the time between attempts by
// 5 minutes. The NTRIP server stops retrying after 25 hours and 18 minutes
static const int MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS = 28;

// NTRIP client connection delay before resetting the connect accempt counter
static const int NTRIP_SERVER_CONNECTION_TIME = 5 * 60 * 1000;

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
//  * Reconnection delay
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
    {
        if (settings.debugNtripServerState)
            systemPrintf("NTRIP Server connection to NTRIP caster %s:%d failed\r\n",
                         settings.ntripServer_CasterHost,
                         settings.ntripServer_CasterPort);
        return false;
    }

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
    int seconds;

    // Retry the connection a few times
    bool limitReached = (ntripServerConnectionAttempts >= MAX_NTRIP_SERVER_CONNECTION_ATTEMPTS);

    // Shutdown the NTRIP server
    ntripServerStop(limitReached || (!settings.enableNtripServer));

    ntripServerConnectionAttempts++;
    ntripServerConnectionAttemptsTotal++;
    if (settings.debugNtripServerState)
        ntripServerPrintStatus();

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

        // Display the delay before starting the NTRIP server
        if (settings.debugNtripServerState && ntripServerConnectionAttemptTimeout)
        {
            seconds = ntripServerConnectionAttemptTimeout / 1000;
            if (seconds < 120)
                systemPrintf("NTRIP Server trying again in %d seconds.\r\n", seconds);
            else
                systemPrintf("NTRIP Server trying again in %d minutes.\r\n", seconds / 60);
        }
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
    static uint32_t zedBytesSent;

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
                systemPrintf("    Tx RTCM: %s.%03ld, %d bytes sent\r\n", timestamp, rtc.getMillis(), zedBytesSent);
                zedBytesSent = 0;
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
            zedBytesSent++;
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
    ntripServerConnectLimitReached();
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

    // Display the heap state
    reportHeapNow(settings.debugNtripServerState);

    // Start the NTRIP server if enabled
    if ((settings.ntripServer_StartAtSurveyIn == true) || (settings.enableNtripServer == true))
    {
        systemPrintln ("NTRIP Server start");
        ntripServerSetState(NTRIP_SERVER_ON);
    }

    ntripServerConnectionAttempts = 0;
}

// Shutdown or restart the NTRIP server
void ntripServerStop(bool shutdown)
{
    if (ntripServer)
    {
        // Break the NTRIP server connection if necessary
        if (ntripServer->connected())
            ntripServer->stop();

        // Attempt to restart the network if possible
        if (!shutdown)
            networkRestart(NETWORK_USER_NTRIP_SERVER);

        // Done with the network
        if (networkGetUserNetwork(NETWORK_USER_NTRIP_SERVER))
            networkUserClose(NETWORK_USER_NTRIP_SERVER);

        // Free the NTRIP server resources
        delete ntripServer;
        ntripServer = nullptr;
        reportHeapNow(settings.debugNtripServerState);
    }

    // Increase timeouts if we started the network
    if (ntripServerState > NTRIP_SERVER_ON)
        // Mark the Server stop so that we don't immediately attempt re-connect to Caster
        ntripServerTimer = millis();

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
    DMW_st(ntripServerSetState, ntripServerState);
    switch (ntripServerState)
    {
    case NTRIP_SERVER_OFF:
        break;

    // Start the network
    case NTRIP_SERVER_ON:
        if (networkUserOpen(NETWORK_USER_NTRIP_SERVER, NETWORK_TYPE_ACTIVE))
            ntripServerSetState(NTRIP_SERVER_NETWORK_STARTED);
        break;

    // Wait for a network media connection
    case NTRIP_SERVER_NETWORK_STARTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();

        // Determine if the network is connected to the media
        else if (networkUserConnected(NETWORK_USER_NTRIP_SERVER))
        {
            // Allocate the ntripServer structure
            ntripServer = new NetworkClient(NETWORK_USER_NTRIP_SERVER);
            if (!ntripServer)
            {
                // Failed to allocate the ntripServer structure
                systemPrintln("ERROR: Failed to allocate the ntripServer structure!");
                ntripServerShutdown();
            }
            else
            {
                reportHeapNow(settings.debugNtripServerState);

                // The network is available for the NTRIP server
                ntripServerSetState(NTRIP_SERVER_NETWORK_CONNECTED);
            }
        }
        break;

    // Network available
    case NTRIP_SERVER_NETWORK_CONNECTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();

        else if (settings.enableNtripServer
            && (millis() - ntripServerLastConnectionAttempt > ntripServerConnectionAttemptTimeout))
        {
            // No RTCM correction data sent yet
            rtcmPacketsSent = 0;

            // Open socket to NTRIP caster
            ntripServerSetState(NTRIP_SERVER_WAIT_GNSS_DATA);
        }
        break;

    // Wait for GNSS correction data
    case NTRIP_SERVER_WAIT_GNSS_DATA:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();

        // State change handled in ntripServerProcessRTCM
        break;

    // Initiate the connection to the NTRIP caster
    case NTRIP_SERVER_CONNECTING:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();

        // Delay before opening the NTRIP server connection
        else if ((millis() - ntripServerTimer) >= ntripServerConnectionAttemptTimeout)
        {
            // Attempt a connection to the NTRIP caster
            if (!ntripServerConnectCaster())
            {
                // Assume service not available
                if (ntripServerConnectLimitReached()) // Update ntripServerConnectionAttemptTimeout
                    systemPrintln("NTRIP Server failed to connect! Do you have your caster address and port correct?");
            }
            else
            {
                // Connection open to NTRIP caster, wait for the authorization response
                ntripServerTimer = millis();
                ntripServerSetState(NTRIP_SERVER_AUTHORIZATION);
            }
        }
        break;

    // Wait for authorization response
    case NTRIP_SERVER_AUTHORIZATION:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();

        // Check if caster service responded
        else if (ntripServer->available() < strlen("ICY 200 OK")) // Wait until at least a few bytes have arrived
        {
            // Check for response timeout
            if (millis() - ntripServerTimer > 10000)
            {
                if (ntripServerConnectLimitReached())
                    systemPrintln("Caster failed to respond. Do you have your caster address and port correct?");
            }
        }
        else
        {
            // NTRIP caster's authorization response received
            char response[512];
            ntripServerResponse(response, sizeof(response));

            if (settings.debugNtripServerState)
                systemPrintf("Server Response: %s\r\n", response);
            else
                log_d("Server Response: %s", response);

            // Look for various responses
            if (strstr(response, "200") != nullptr) //'200' found
            {
                // We got a response, now check it for possible errors
                if (strcasestr(response, "banned") != nullptr)
                {
                    systemPrintf("NTRIP Server connected to caster but caster responded with banned error: %s\r\n",
                                 response);

                    // Stop NTRIP Server operations
                    ntripServerShutdown();
                }
                else if (strcasestr(response, "sandbox") != nullptr)
                {
                    systemPrintf("NTRIP Server connected to caster but caster responded with sandbox error: %s\r\n",
                                 response);

                    // Stop NTRIP Server operations
                    ntripServerShutdown();
                }

                systemPrintf("NTRIP Server connected to %s:%d %s\r\n", settings.ntripServer_CasterHost,
                             settings.ntripServer_CasterPort, settings.ntripServer_MountPoint);

                // Connection is now open, start the RTCM correction data timer
                ntripServerTimer = millis();

                // We don't use a task because we use I2C hardware (and don't have a semphore).
                online.ntripServer = true;
                ntripServerStartTime = millis();
                ntripServerSetState(NTRIP_SERVER_CASTING);
            }

            // Look for '401 Unauthorized'
            else if (strstr(response, "401") != nullptr)
            {
                systemPrintf(
                    "NTRIP Caster responded with unauthorized error: %s. Are you sure your caster credentials are correct?\r\n",
                    response);

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
            }
        }
        break;

    // NTRIP server authorized to send RTCM correction data to NTRIP caster
    case NTRIP_SERVER_CASTING:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            ntripServerRestart();

        // Check for a broken connection
        else if (!ntripServer->connected())
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
            // Handle other types of NTRIP connection failures to prevent
            // hammering the NTRIP caster with rapid connection attempts.
            // A fast reconnect is reasonable after a long NTRIP caster
            // connection.  However increasing backoff delays should be
            // added when the NTRIP caster fails after a short connection
            // interval.
            if (((millis() - ntripServerStartTime) > NTRIP_SERVER_CONNECTION_TIME)
                && (ntripServerConnectionAttempts || ntripServerConnectionAttemptTimeout))
            {
                // After a long connection period, reset the attempt counter
                ntripServerConnectionAttempts = 0;
                ntripServerConnectionAttemptTimeout = 0;
                if (settings.debugNtripServerState)
                    systemPrintln("NTRIP Server resetting connection attempt counter and timeout");
            }

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
