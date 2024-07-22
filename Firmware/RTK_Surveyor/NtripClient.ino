/*------------------------------------------------------------------------------
NtripClient.ino

  The NTRIP client sits on top of the network layer and receives correction data
  from an NTRIP caster that is provided to the ZED (GNSS radio).

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

  NTRIP Client Testing:

     Using Ethernet on RTK Reference Station:

        1. Network failure - Disconnect Ethernet cable at RTK Reference Station,
           expecting retry NTRIP client connection after network restarts

     Using WiFi on RTK Express or RTK Reference Station:

        1. Internet link failure - Disconnect Ethernet cable between Ethernet
           switch and the firewall to simulate an internet failure, expecting
           retry NTRIP client connection after delay

        2. Internet outage - Disconnect Ethernet cable between Ethernet
           switch and the firewall to simulate an internet failure, expecting
           retries to exceed the connection limit causing the NTRIP client to
           shutdown after about 2 hours.  Restarting the NTRIP client may be
           done by rebooting the RTK or by using the configuration menus to
           turn off and on the NTRIP client.

  Test Setup:

          RTK Express     RTK Reference Station
               ^           ^                 ^
          WiFi |      WiFi |                 | Ethernet cable
               v           v                 v
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
  NTRIP Client States:
    NTRIP_CLIENT_OFF: Network off or using NTRIP server
    NTRIP_CLIENT_ON: WIFI_STATE_START state
    NTRIP_CLIENT_NETWORK_STARTED: Connecting to the network
    NTRIP_CLIENT_NETWORK_CONNECTED: Connected to the network
    NTRIP_CLIENT_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_WAIT_RESPONSE: Wait for a response from the NTRIP caster
    NTRIP_CLIENT_CONNECTED: Connected to the NTRIP caster

                               NTRIP_CLIENT_OFF
                                       |   ^
                      ntripClientStart |   | ntripClientShutdown()
                                       v   |
                               NTRIP_CLIENT_ON <--------------.
                                       |                      |
                                       |                      | ntripClientRestart()
                                       v                Fail  |
                         NTRIP_CLIENT_NETWORK_STARTED ------->+
                                       |                      ^
                                       |                      |
                                       v                      |
                        NTRIP_CLIENT_NETWORK_CONNECTED        |
                                       |                      |
                                       |                      |
                                       v                Fail  |
                           NTRIP_CLIENT_CONNECTING ---------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                          NTRIP_CLIENT_WAIT_RESPONSE -------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                            NTRIP_CLIENT_CONNECTED -----------'

  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#if COMPILE_NETWORK

//----------------------------------------
// Constants
//----------------------------------------

// Size of the credentials buffer in bytes
static const int CREDENTIALS_BUFFER_SIZE = 512;

// Give up connecting after this number of attempts
// Connection attempts are throttled to increase the time between attempts
// 30 attempts with 15 second increases will take almost two hours
static const int MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS = 30;

// NTRIP caster response timeout
static const uint32_t NTRIP_CLIENT_RESPONSE_TIMEOUT = 10 * 1000; // Milliseconds

// NTRIP client receive data timeout
static const uint32_t NTRIP_CLIENT_RECEIVE_DATA_TIMEOUT = 30 * 1000; // Milliseconds

// Most incoming data is around 500 bytes but may be larger
static const int RTCM_DATA_SIZE = 512 * 4;

// NTRIP client server request buffer size
static const int SERVER_BUFFER_SIZE = CREDENTIALS_BUFFER_SIZE + 3;

static const int NTRIPCLIENT_MS_BETWEEN_GGA = 5000; // 5s between transmission of GGA messages, if enabled

// NTRIP client connection delay before resetting the connect accempt counter
static const int NTRIP_CLIENT_CONNECTION_TIME = 5 * 60 * 1000;

// Define the NTRIP client states
enum NTRIPClientState
{
    NTRIP_CLIENT_OFF = 0,           // Using Bluetooth or NTRIP server
    NTRIP_CLIENT_ON,                // WIFI_STATE_START state
    NTRIP_CLIENT_NETWORK_STARTED,   // Connecting to WiFi access point or Ethernet
    NTRIP_CLIENT_NETWORK_CONNECTED, // Connected to an access point or Ethernet
    NTRIP_CLIENT_CONNECTING,        // Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_WAIT_RESPONSE,     // Wait for a response from the NTRIP caster
    NTRIP_CLIENT_CONNECTED,         // Connected to the NTRIP caster
    // Insert new states here
    NTRIP_CLIENT_STATE_MAX          // Last entry in the state list
};

const char * const ntripClientStateName[] =
{
    "NTRIP_CLIENT_OFF",
    "NTRIP_CLIENT_ON",
    "NTRIP_CLIENT_NETWORK_STARTED",
    "NTRIP_CLIENT_NETWORK_CONNECTED",
    "NTRIP_CLIENT_CONNECTING",
    "NTRIP_CLIENT_WAIT_RESPONSE",
    "NTRIP_CLIENT_CONNECTED"
};

const int ntripClientStateNameEntries = sizeof(ntripClientStateName) / sizeof(ntripClientStateName[0]);

const RtkMode_t ntripClientMode = RTK_MODE_ROVER
                                | RTK_MODE_BASE_SURVEY_IN;

//----------------------------------------
// Locals
//----------------------------------------

// The network connection to the NTRIP caster to obtain RTCM data.
static NetworkClient *ntripClient;
static volatile uint8_t ntripClientState = NTRIP_CLIENT_OFF;

// Throttle the time between connection attempts
// ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
static int ntripClientConnectionAttempts; // Count the number of connection attempts between restarts
static uint32_t ntripClientConnectionAttemptTimeout;
static int ntripClientConnectionAttemptsTotal; // Count the number of connection attempts absolutely

// NTRIP client timer usage:
//  * Reconnection delay
//  * Measure the connection response time
//  * Receive NTRIP data timeout
static uint32_t ntripClientTimer;
static uint32_t ntripClientStartTime;          // For calculating uptime

// Throttle GGA transmission to Caster to 1 report every 5 seconds
unsigned long lastGGAPush;

//----------------------------------------
// NTRIP Client Routines
//----------------------------------------

bool ntripClientConnect()
{
    if (!ntripClient)
        return false;

    // Remove any http:// or https:// prefix from host name
    char hostname[51];
    strncpy(hostname, settings.ntripClient_CasterHost,
            sizeof(hostname) - 1); // strtok modifies string to be parsed so we create a copy
    char *token = strtok(hostname, "//");
    if (token != nullptr)
    {
        token = strtok(nullptr, "//"); // Advance to data after //
        if (token != nullptr)
            strcpy(settings.ntripClient_CasterHost, token);
    }

    if (settings.debugNtripClientState)
        systemPrintf("NTRIP Client connecting to %s:%d\r\n",
                     settings.ntripClient_CasterHost,
                     settings.ntripClient_CasterPort);

    int connectResponse = ntripClient->connect(settings.ntripClient_CasterHost, settings.ntripClient_CasterPort);

    if (connectResponse < 1)
    {
        if (settings.debugNtripClientState)
            systemPrintf("NTRIP Client connection to NTRIP caster %s:%d failed\r\n",
                         settings.ntripClient_CasterHost,
                         settings.ntripClient_CasterPort);
        return false;
    }

    // Set up the server request (GET)
    char serverRequest[SERVER_BUFFER_SIZE];
    int length;
    snprintf(serverRequest, SERVER_BUFFER_SIZE, "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun_RTK_%s_",
             settings.ntripClient_MountPoint, platformPrefix);
    length = strlen(serverRequest);
    getFirmwareVersion(&serverRequest[length], SERVER_BUFFER_SIZE - 2 - length, false);
    length = strlen(serverRequest);
    serverRequest[length++] = '\r';
    serverRequest[length++] = '\n';
    serverRequest[length++] = 0;

    // Set up the credentials
    char credentials[CREDENTIALS_BUFFER_SIZE];
    if (strlen(settings.ntripClient_CasterUser) == 0)
    {
        strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", sizeof(credentials) - 1);
    }
    else
    {
        // Pass base64 encoded user:pw
        char userCredentials[sizeof(settings.ntripClient_CasterUser) + sizeof(settings.ntripClient_CasterUserPW) +
                             1]; // The ':' takes up a spot
        snprintf(userCredentials, sizeof(userCredentials), "%s:%s", settings.ntripClient_CasterUser,
                 settings.ntripClient_CasterUserPW);

        if (settings.debugNtripClientState)
        {
            systemPrint("NTRIP Client sending credentials: ");
            systemPrintln(userCredentials);
        }

        // Encode with ESP32 built-in library
        base64 b;
        String strEncodedCredentials = b.encode(userCredentials);
        char encodedCredentials[strEncodedCredentials.length() + 1];
        strEncodedCredentials.toCharArray(encodedCredentials,
                                          sizeof(encodedCredentials)); // Convert String to char array

        snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
    }

    // Add the encoded credentials to the server request
    strncat(serverRequest, credentials, SERVER_BUFFER_SIZE - 1);
    strncat(serverRequest, "\r\n", SERVER_BUFFER_SIZE - 1);

    if (settings.debugNtripClientState)
    {
        systemPrint("NTRIP Client serverRequest size: ");
        systemPrint(strlen(serverRequest));
        systemPrint(" of ");
        systemPrint(sizeof(serverRequest));
        systemPrintln(" bytes available");
        systemPrintln("NTRIP Client sending server request: ");
        systemPrintln(serverRequest);
    }

    // Send the server request
    ntripClient->write((const uint8_t *)serverRequest, strlen(serverRequest));
    ntripClientTimer = millis();
    return true;
}

// Determine if another connection is possible or if the limit has been reached
bool ntripClientConnectLimitReached()
{
    int seconds;

    // Retry the connection a few times
    bool limitReached = (ntripClientConnectionAttempts >= MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS);

    // Attempt to restart the network if possible
    if (settings.enableNtripClient && (!limitReached))
        networkRestart(NETWORK_USER_NTRIP_CLIENT);

    // Restart the NTRIP client
    ntripClientStop(limitReached || (!settings.enableNtripClient));

    ntripClientConnectionAttempts++;
    ntripClientConnectionAttemptsTotal++;
    if (settings.debugNtripClientState)
        ntripClientPrintStatus();

    if (limitReached == false)
    {
        if (ntripClientConnectionAttempts == 1)
            ntripClientConnectionAttemptTimeout = 15 * 1000L; // Wait 15s
        else if (ntripClientConnectionAttempts == 2)
            ntripClientConnectionAttemptTimeout = 30 * 1000L; // Wait 30s
        else if (ntripClientConnectionAttempts == 3)
            ntripClientConnectionAttemptTimeout = 1 * 60 * 1000L; // Wait 1 minute
        else if (ntripClientConnectionAttempts == 4)
            ntripClientConnectionAttemptTimeout = 2 * 60 * 1000L; // Wait 2 minutes
        else
            ntripClientConnectionAttemptTimeout =
                (ntripClientConnectionAttempts - 4) * 5 * 60 * 1000L; // Wait 5, 10, 15, etc minutes between attempts

        // Display the delay before starting the NTRIP client
        if (settings.debugNtripClientState && ntripClientConnectionAttemptTimeout)
        {
            seconds = ntripClientConnectionAttemptTimeout / 1000;
            if (seconds < 120)
                systemPrintf("NTRIP Client trying again in %d seconds.\r\n", seconds);
            else
                systemPrintf("NTRIP Client trying again in %d minutes.\r\n", seconds / 60);
        }
    }
    else
        // No more connection attempts, switching to Bluetooth
        systemPrintln("NTRIP Client connection attempts exceeded!");
    return limitReached;
}

// Print the NTRIP client state summary
void ntripClientPrintStateSummary()
{
    switch (ntripClientState)
    {
    default:
        systemPrintf("Unknown: %d", ntripClientState);
        break;
    case NTRIP_CLIENT_OFF:
        systemPrint("Disconnected");
        break;
    case NTRIP_CLIENT_ON:
    case NTRIP_CLIENT_NETWORK_STARTED:
    case NTRIP_CLIENT_NETWORK_CONNECTED:
    case NTRIP_CLIENT_CONNECTING:
    case NTRIP_CLIENT_WAIT_RESPONSE:
        systemPrint("Connecting");
        break;
    case NTRIP_CLIENT_CONNECTED:
        systemPrint("Connected");
        break;
    }
}

// Print the NTRIP Client status
void ntripClientPrintStatus()
{
    uint32_t days;
    byte hours;
    uint64_t milliseconds;
    byte minutes;
    byte seconds;

    // Display NTRIP Client status and uptime
    if (settings.enableNtripClient &&
        ((systemState >= STATE_ROVER_NOT_STARTED) && (systemState <= STATE_ROVER_RTK_FIX)))
    {
        systemPrint("NTRIP Client ");
        ntripClientPrintStateSummary();
        systemPrintf(" - %s/%s:%d", settings.ntripClient_CasterHost,
                     settings.ntripClient_MountPoint, settings.ntripClient_CasterPort);

        if (ntripClientState == NTRIP_CLIENT_CONNECTED)
            // Use ntripClientTimer since it gets reset after each successful data
            // receiption from the NTRIP caster
            milliseconds = ntripClientTimer - ntripClientStartTime;
        else
        {
            milliseconds = ntripClientStartTime;
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
                     days, hours, minutes, seconds, milliseconds, ntripClientConnectionAttemptsTotal);
    }
}

// Determine if NTRIP client data is available
int ntripClientReceiveDataAvailable()
{
    return ntripClient->available();
}

// Read the response from the NTRIP client
void ntripClientResponse(char *response, size_t maxLength)
{
    char *responseEnd;

    // Make sure that we can zero terminate the response
    responseEnd = &response[maxLength - 1];

    // Read bytes from the caster and store them
    while ((response < responseEnd) && (ntripClientReceiveDataAvailable() > 0))
    {
        *response++ = ntripClient->read();
    }

    // Zero terminate the response
    *response = '\0';
}

// Restart the NTRIP client
void ntripClientRestart()
{
    // Save the previous uptime value
    if (ntripClientState == NTRIP_CLIENT_CONNECTED)
        ntripClientStartTime = ntripClientTimer - ntripClientStartTime;
    ntripClientConnectLimitReached();
}

// Update the state of the NTRIP client state machine
// PERIODIC_DISPLAY(PD_NTRIP_CLIENT_STATE) is handled by ntripClientUpdate
void ntripClientSetState(uint8_t newState)
{
    if (settings.debugNtripClientState)
    {
        if (ntripClientState == newState)
            systemPrint("NTRIP client: *");
        else
            systemPrintf("NTRIP client: %s --> ", ntripClientStateName[ntripClientState]);
    }
    ntripClientState = newState;
    if (settings.debugNtripClientState)
    {
        if (ntripClientState >= NTRIP_CLIENT_STATE_MAX)
        {
            systemPrintf("Unknown client state %d\r\n", ntripClientState);
            reportFatalError("Unknown NTRIP Client state");
        }
        else
            systemPrintln(ntripClientStateName[ntripClientState]);
    }
}

// Shutdown the NTRIP client
void ntripClientShutdown()
{
    ntripClientStop(true);
}

// Start the NTRIP client
void ntripClientStart()
{
    // Display the heap state
    reportHeapNow(settings.debugNtripClientState);

    // Start the NTRIP client
    systemPrintln("NTRIP Client start");
    ntripClientStop(false);
}

// Shutdown or restart the NTRIP client
void ntripClientStop(bool shutdown)
{
    if (ntripClient)
    {
        // Break the NTRIP client connection if necessary
        if (ntripClient->connected())
            ntripClient->stop();

        // Free the NTRIP client resources
        delete ntripClient;
        ntripClient = nullptr;
        reportHeapNow(settings.debugNtripClientState);
    }

    // Increase timeouts if we started the network
    if (ntripClientState > NTRIP_CLIENT_ON)
    {
        // Mark the Client stop so that we don't immediately attempt re-connect to Caster
        ntripClientTimer = millis();

        // Done with the network
        if (networkGetUserNetwork(NETWORK_USER_NTRIP_CLIENT))
            networkUserClose(NETWORK_USER_NTRIP_CLIENT);
    }

    // Return the Main Talker ID to "GN".
    if (online.gnss)
    {
        theGNSS.setVal8(UBLOX_CFG_NMEA_MAINTALKERID, 3); // Return talker ID to GNGGA after NTRIP Client set to GPGGA
        theGNSS.setNMEAGPGGAcallbackPtr(nullptr);        // Remove callback
    }

    // Determine the next NTRIP client state
    online.ntripClient = false;
    netIncomingRTCM = false;
    if (shutdown)
    {
        ntripClientSetState(NTRIP_CLIENT_OFF);
        settings.enableNtripClient = false;
        ntripClientConnectionAttempts = 0;
        ntripClientConnectionAttemptTimeout = 0;
    }
    else
        ntripClientSetState(NTRIP_CLIENT_ON);
}

// Check for the arrival of any correction data. Push it to the GNSS.
// Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
    // Shutdown the NTRIP client when the mode or setting changes
    DMW_st(ntripClientSetState, ntripClientState);
    if (NEQ_RTK_MODE(ntripClientMode) || (!settings.enableNtripClient))
    {
        if (ntripClientState > NTRIP_CLIENT_OFF)
        {
            ntripClientStop(true);
            ntripClientConnectionAttempts = 0;
            ntripClientConnectionAttemptTimeout = 0;
            ntripClientSetState(NTRIP_CLIENT_OFF);
        }
    }

    // Enable the network and the NTRIP client if requested
    switch (ntripClientState)
    {
    case NTRIP_CLIENT_OFF:
        if (EQ_RTK_MODE(ntripClientMode) && settings.enableNtripClient)
            ntripClientStart();
        break;

    // Start the network
    case NTRIP_CLIENT_ON:
        if (networkUserOpen(NETWORK_USER_NTRIP_CLIENT, NETWORK_TYPE_ACTIVE))
            ntripClientSetState(NTRIP_CLIENT_NETWORK_STARTED);
        break;

    // Wait for a network media connection
    case NTRIP_CLIENT_NETWORK_STARTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            ntripClientStop(true); // Note: was ntripClientRestart();

        // Determine if the network is connected to the media
        else if (networkUserConnected(NETWORK_USER_NTRIP_CLIENT))
        {
            // Allocate the ntripClient structure
            ntripClient = new NetworkClient(NETWORK_USER_NTRIP_CLIENT);
            if (!ntripClient)
            {
                // Failed to allocate the ntripClient structure
                systemPrintln("ERROR: Failed to allocate the ntripClient structure!");
                ntripClientShutdown();
            }
            else
            {
                reportHeapNow(settings.debugNtripClientState);

                // The network is available for the NTRIP client
                ntripClientSetState(NTRIP_CLIENT_NETWORK_CONNECTED);
            }
        }
        break;

    case NTRIP_CLIENT_NETWORK_CONNECTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            ntripClientStop(true); // Note: was ntripClientRestart();

        // If GGA transmission is enabled, wait for GNSS lock before connecting to NTRIP Caster
        // If GGA transmission is not enabled, start connecting to NTRIP Caster
        else if ((!settings.ntripClient_TransmitGGA) || (fixType >= 3) && (fixType <= 5))
        {
            // Delay before opening the NTRIP client connection
            if ((millis() - ntripClientTimer) >= ntripClientConnectionAttemptTimeout)
            {
                // Open connection to NTRIP caster service
                if (!ntripClientConnect())
                {
                    // Assume service not available
                    if (ntripClientConnectLimitReached()) // Updates ntripClientConnectionAttemptTimeout
                        systemPrintln("NTRIP caster failed to connect. Do you have your caster address and port correct?");
                }
                else
                {
                    // Socket opened to NTRIP system
                    if (settings.debugNtripClientState)
                        systemPrintf("NTRIP Client waiting for response from %s:%d\r\n",
                                     settings.ntripClient_CasterHost,
                                     settings.ntripClient_CasterPort);
                    ntripClientSetState(NTRIP_CLIENT_WAIT_RESPONSE);
                }
            }
        }
        break;

    case NTRIP_CLIENT_WAIT_RESPONSE:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            ntripClientStop(true); // Note: was ntripClientRestart();

        // Check for no response from the caster service
        else if (ntripClientReceiveDataAvailable() <
                 strlen("ICY 200 OK")) // Wait until at least a few bytes have arrived
        {
            // Check for response timeout
            if (millis() - ntripClientTimer > NTRIP_CLIENT_RESPONSE_TIMEOUT)
            {
                // NTRIP web service did not respond
                if (ntripClientConnectLimitReached()) // Updates ntripClientConnectionAttemptTimeout
                    systemPrintln("NTRIP Caster failed to respond. Do you have your caster address and port correct?");
            }
        }
        else
        {
            // Caster web service responded
            char response[512];
            ntripClientResponse(&response[0], sizeof(response));

            if (settings.debugNtripClientState)
                systemPrintf("Caster Response: %s\r\n", response);
            else
                log_d("Caster Response: %s", response);

            // Look for various responses
            if (strstr(response, "200") != nullptr) //'200' found
            {
                // We got a response, now check it for possible errors
                if (strcasestr(response, "banned") != nullptr)
                {
                    systemPrintf("NTRIP Client connected to caster but caster responded with banned error: %s\r\n",
                                 response);

                    ntripClientConnectLimitReached(); //Re-attempted after a period of time. Shuts down NTRIP Client if limit reached.
                }
                else if (strcasestr(response, "sandbox") != nullptr)
                {
                    systemPrintf("NTRIP Client connected to caster but caster responded with sandbox error: %s\r\n",
                                 response);

                    ntripClientConnectLimitReached(); //Re-attempted after a period of time. Shuts down NTRIP Client if limit reached.
                }
                else if (strcasestr(response, "SOURCETABLE") != nullptr)
                {
                    systemPrintf("Caster may not have mountpoint %s. Caster responded with problem: %s\r\n",
                                 settings.ntripClient_MountPoint, response);

                    // Stop NTRIP client operations
                    ntripClientShutdown();
                }
                else
                {
                    // We successfully connected
                    // Timeout receiving NTRIP data, retry the NTRIP client connection
                    if (online.rtc && online.gnss)
                    {
                        int hours;
                        int minutes;
                        int seconds;

                        seconds = rtc.getLocalEpoch() % SECONDS_IN_A_DAY;
                        hours = seconds / SECONDS_IN_AN_HOUR;
                        seconds -= hours * SECONDS_IN_AN_HOUR;
                        minutes = seconds / SECONDS_IN_A_MINUTE;
                        seconds -= minutes * SECONDS_IN_A_MINUTE;
                        systemPrintf("NTRIP Client connected to %s:%d at %d:%02d:%02d\r\n",
                                     settings.ntripClient_CasterHost, settings.ntripClient_CasterPort, hours, minutes,
                                     seconds);
                    }
                    else
                        systemPrintf("NTRIP Client connected to %s:%d\r\n", settings.ntripClient_CasterHost,
                                     settings.ntripClient_CasterPort);

                    // Connection is now open, start the NTRIP receive data timer
                    ntripClientTimer = millis();

                    if (settings.ntripClient_TransmitGGA == true)
                    {
                        // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
                        theGNSS.setVal8(UBLOX_CFG_NMEA_MAINTALKERID, 1);
                        theGNSS.setNMEAGPGGAcallbackPtr(&pushGPGGA); // Set up the callback for GPGGA

                        float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;
                        if (measurementFrequency < 0.2)
                            measurementFrequency = 0.2; // 0.2Hz * 5 = 1 measurement every 5 seconds
                        log_d("Adjusting GGA setting to %f", measurementFrequency);
                        theGNSS.setVal8(
                            UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C,
                            measurementFrequency); // Enable GGA over I2C. Tell the module to output GGA every second

                        lastGGAPush =
                            millis() - NTRIPCLIENT_MS_BETWEEN_GGA; // Force immediate transmission of GGA message
                    }

                    // We don't use a task because we use I2C hardware (and don't have a semphore).
                    online.ntripClient = true;
                    ntripClientStartTime = millis();
                    ntripClientSetState(NTRIP_CLIENT_CONNECTED);
                }
            }
            else if (strstr(response, "401") != nullptr)
            {
                // Look for '401 Unauthorized'
                systemPrintf(
                    "NTRIP Caster responded with unauthorized error: %s. Are you sure your caster credentials are correct?\r\n",
                    response);

                // Stop NTRIP client operations
                ntripClientShutdown();
            }
            // Other errors returned by the caster
            else
            {
                systemPrintf("NTRIP Client connected but caster responded with problem: %s\r\n", response);

                // Stop NTRIP client operations
                ntripClientShutdown();
            }
        }
        break;

    case NTRIP_CLIENT_CONNECTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_NTRIP_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            ntripClientStop(true); // Note: was ntripClientRestart();

        // Check for a broken connection
        else if (!ntripClient->connected())
        {
            // Broken connection, retry the NTRIP client connection
            systemPrintln("NTRIP Client connection to caster was broken");
            ntripClientRestart();
        }
        else
        {
            // Handle other types of NTRIP connection failures to prevent
            // hammering the NTRIP caster with rapid connection attempts.
            // A fast reconnect is reasonable after a long NTRIP caster
            // connection.  However increasing backoff delays should be
            // added when the NTRIP caster fails after a short connection
            // interval.
            if (((millis() - ntripClientStartTime) > NTRIP_CLIENT_CONNECTION_TIME)
                && (ntripClientConnectionAttempts || ntripClientConnectionAttemptTimeout))
            {
                // After a long connection period, reset the attempt counter
                ntripClientConnectionAttempts = 0;
                ntripClientConnectionAttemptTimeout = 0;
                if (settings.debugNtripClientState)
                    systemPrintln("NTRIP Client resetting connection attempt counter and timeout");
            }

            // Check for timeout receiving NTRIP data
            if (ntripClientReceiveDataAvailable() == 0)
            {
                // Don't fail during retransmission attempts
                if ((millis() - ntripClientTimer) > NTRIP_CLIENT_RECEIVE_DATA_TIMEOUT)
                {
                    // Timeout receiving NTRIP data, retry the NTRIP client connection
                    if (online.rtc && online.gnss)
                    {
                        int hours;
                        int minutes;
                        int seconds;

                        seconds = rtc.getLocalEpoch() % SECONDS_IN_A_DAY;
                        hours = seconds / SECONDS_IN_AN_HOUR;
                        seconds -= hours * SECONDS_IN_AN_HOUR;
                        minutes = seconds / SECONDS_IN_A_MINUTE;
                        seconds -= minutes * SECONDS_IN_A_MINUTE;
                        systemPrintf("NTRIP Client timeout receiving data at %d:%02d:%02d\r\n",
                                     hours, minutes, seconds);
                    }
                    else
                        systemPrintln("NTRIP Client timeout receiving data");
                    ntripClientRestart();
                }
            }
            else
            {
                // Receive data from the NTRIP Caster
                uint8_t rtcmData[RTCM_DATA_SIZE];
                size_t rtcmCount = 0;

                // Collect any available RTCM data
                if (ntripClientReceiveDataAvailable() > 0)
                {
                    rtcmCount = ntripClient->read(rtcmData, sizeof(rtcmData));
                    if (rtcmCount)
                    {
                        // Restart the NTRIP receive data timer
                        ntripClientTimer = millis();

                        // Record the arrival of RTCM from the WiFi connection. This resets the RTCM timeout used on the L-Band.
                        rtcmLastPacketReceived = millis();

                        // Push RTCM to GNSS module over I2C / SPI
                        theGNSS.pushRawData(rtcmData, rtcmCount);
                        netIncomingRTCM = true;

                        if ((settings.debugNtripClientRtcm || PERIODIC_DISPLAY(PD_NTRIP_CLIENT_DATA))
                            && (!inMainMenu))
                        {
                            PERIODIC_CLEAR(PD_NTRIP_CLIENT_DATA);
                            systemPrintf("NTRIP Client received %d RTCM bytes, pushed to ZED\r\n", rtcmCount);
                        }
                    }
                }
            }
        }
        break;
    }

    // Periodically display the NTRIP client state
    if (PERIODIC_DISPLAY(PD_NTRIP_CLIENT_STATE))
    {
        systemPrintf("NTRIP client state: %s\r\n", ntripClientStateName[ntripClientState]);
        PERIODIC_CLEAR(PD_NTRIP_CLIENT_STATE);
    }
}

// Verify the NTRIP client tables
void ntripClientValidateTables()
{
    if (ntripClientStateNameEntries != NTRIP_CLIENT_STATE_MAX)
        reportFatalError("Fix ntripClientStateNameEntries to match NTRIPClientState");
}

void pushGPGGA(NMEA_GGA_data_t *nmeaData)
{
    // Provide the caster with our current position as needed
    if (ntripClient->connected() && settings.ntripClient_TransmitGGA == true)
    {
        if (millis() - lastGGAPush > NTRIPCLIENT_MS_BETWEEN_GGA)
        {
            lastGGAPush = millis();

            if (settings.debugNtripClientRtcm || PERIODIC_DISPLAY(PD_NTRIP_CLIENT_GGA))
            {
                PERIODIC_CLEAR(PD_NTRIP_CLIENT_GGA);
                systemPrintf("NTRIP Client pushing GGA to server: %s", (const char *)nmeaData->nmea);
            }

            // Push our current GGA sentence to caster
            ntripClient->print((const char *)nmeaData->nmea);
        }
    }
}

#endif  // COMPILE_NETWORK
