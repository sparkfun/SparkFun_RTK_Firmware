/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  NTRIP Client States:
    NTRIP_CLIENT_OFF: Network off or using NTRIP server
    NTRIP_CLIENT_ON: WIFI_START state
    NTRIP_CLIENT_NETWORK_STARTED: Connecting to the network
    NTRIP_CLIENT_NETWORK_CONNECTED: Connected to the network
    NTRIP_CLIENT_CONNECTING: Attempting a connection to the NTRIP caster
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
                                       v                Fail  |
                        NTRIP_CLIENT_NETWORK_CONNECTED ------>+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                           NTRIP_CLIENT_CONNECTING ---------->+
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

// Define the NTRIP client states
enum NTRIPClientState
{
    NTRIP_CLIENT_OFF = 0,           // Using Bluetooth or NTRIP server
    NTRIP_CLIENT_ON,                // WIFI_START state
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
        return false;

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
    // Retry the connection a few times
    bool limitReached = (ntripClientConnectionAttempts >= MAX_NTRIP_CLIENT_CONNECTION_ATTEMPTS);

    // Restart the NTRIP client
    ntripClientStop(limitReached);

    ntripClientConnectionAttempts++;
    ntripClientConnectionAttemptsTotal++;

    if (limitReached == false)
    {
        ntripClientConnectionAttemptTimeout =
            ntripClientConnectionAttempts * 15 * 1000L; // Wait 15, 30, 45, etc seconds between attempts

        reportHeapNow(settings.debugNtripClientState);
    }
    else
        // No more connection attempts, switching to Bluetooth
        systemPrintln("NTRIP Client connection attempts exceeded!");
    return limitReached;
}

// Determine if NTRIP Client is needed
bool ntripClientIsNeeded()
{
    if (settings.enableNtripClient == false)
    {
        // If user turns off NTRIP Client via settings, stop server
        if (ntripClientState > NTRIP_CLIENT_OFF)
            ntripClientShutdown();
        return (false);
    }

    if (wifiInConfigMode())
        return (false); // Do not service NTRIP during network config

    // Allow NTRIP Client to run during Survey-In,
    // but do not allow NTRIP Client to run during Base
    if (systemState == STATE_BASE_TEMP_TRANSMITTING)
        return (false);

    return (true);
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
    uint64_t milliseconds;
    uint32_t days;
    byte hours;
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
    ntripClientStop(false);
}

// Update the state of the NTRIP client state machine
void ntripClientSetState(uint8_t newState)
{
    if (settings.debugNtripClientState || PERIODIC_DISPLAY(PD_NTRIP_CLIENT_STATE))
    {
        if (ntripClientState == newState)
            systemPrint("*");
        else
            systemPrintf("%s --> ", ntripClientStateName[ntripClientState]);
    }
    ntripClientState = newState;
    if (settings.debugNtripClientState || PERIODIC_DISPLAY(PD_NTRIP_CLIENT_STATE))
    {
        PERIODIC_CLEAR(PD_NTRIP_CLIENT_STATE);
        if (newState >= NTRIP_CLIENT_STATE_MAX)
        {
            systemPrintf("Unknown NTRIP Client state: %d\r\n", newState);
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
    // Stop NTRIP client
    ntripClientShutdown();

    // Display the heap state
    reportHeapNow(settings.debugNtripClientState);

    // Start the NTRIP client if enabled
    if (settings.enableNtripClient == true)
    {
        systemPrintln("NTRIP Client start");
        ntripClientSetState(NTRIP_CLIENT_ON);
    }

    ntripClientConnectionAttempts = 0;
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
        ntripClientConnectionAttemptTimeout =
            15 * 1000L; // Wait 15s between stopping and the first re-connection attempt.
    }

    // Return the Main Talker ID to "GN".
    if (online.gnss)
    {
        theGNSS.setVal8(UBLOX_CFG_NMEA_MAINTALKERID, 3); // Return talker ID to GNGGA after NTRIP Client set to GPGGA
        theGNSS.setNMEAGPGGAcallbackPtr(nullptr);        // Remove callback
    }

    // Determine the next NTRIP client state
    ntripClientSetState(shutdown ? NTRIP_CLIENT_OFF : NTRIP_CLIENT_ON);
    online.ntripClient = false;
    netIncomingRTCM = false;
}

// Check for the arrival of any correction data. Push it to the GNSS.
// Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
    if (ntripClientIsNeeded() == false)
        return;

    // Enable the network and the NTRIP client if requested
    switch (ntripClientState)
    {
    case NTRIP_CLIENT_OFF:
        break;

    // Start the network
    case NTRIP_CLIENT_ON:
        if (HAS_ETHERNET)
        {
            if (online.ethernetStatus == ETH_NOT_STARTED)
            {
                systemPrintln("Ethernet not started. Can not start NTRIP Client");
                ntripClientRestart();
            }
            else if ((online.ethernetStatus >= ETH_STARTED_CHECK_CABLE) && (online.ethernetStatus <= ETH_CONNECTED))
            {
                // Pause until connection timeout has passed
                if (millis() - ntripClientTimer > ntripClientConnectionAttemptTimeout)
                {
                    ntripClientTimer = millis();
                    log_d("NTRIP Client starting on Ethernet");
                    ntripClientTimer = millis();
                    ntripClientSetState(NTRIP_CLIENT_NETWORK_STARTED);
                }
            }
            else
            {
                systemPrintln("Error: Please connect Ethernet before starting NTRIP Client");
                ntripClientShutdown();
            }
        }
        else
        {
            if (wifiNetworkCount() == 0)
            {
                systemPrintln("Error: Please enter at least one SSID before starting NTRIP Client");
                ntripClientShutdown();
            }
            else
            {
                // Pause until connection timeout has passed
                if (millis() - ntripClientTimer > ntripClientConnectionAttemptTimeout)
                {
                    ntripClientTimer = millis();
                    log_d("NTRIP Client starting WiFi");
                    wifiStart();
                    ntripClientTimer = millis();
                    ntripClientSetState(NTRIP_CLIENT_NETWORK_STARTED);
                }
            }
        }
        break;

    case NTRIP_CLIENT_NETWORK_STARTED:
        // Determine if the RTK timed out connecting to the network
        if ((millis() - ntripClientTimer) > (1 * 60 * 1000))
        {
            // Failed to connect to to the network, attempt to restart the network
            ntripClientRestart();
            break;
        }

        // Determine if the RTK has connected to the network
        if (wifiIsConnected() || (HAS_ETHERNET && (online.ethernetStatus != ETH_CONNECTED)))
        {
            // Allocate the ntripClient structure
            ntripClient = new NetworkClient(false);
            if (ntripClient)
            {
                reportHeapNow(settings.debugNtripClientState);

                // The network is available for the NTRIP client
                ntripClientSetState(NTRIP_CLIENT_NETWORK_CONNECTED);
            }
            else
            {
                // Failed to allocate the ntripClient structure
                systemPrintln("ERROR: Failed to allocate the ntripClient structure!");
                ntripClientShutdown();
            }
        }
        break;

    case NTRIP_CLIENT_NETWORK_CONNECTED:
        // If GGA transmission is enabled, wait for GNSS lock before connecting to NTRIP Caster
        // If GGA transmission is not enabled, start connecting to NTRIP Caster
        if ((settings.ntripClient_TransmitGGA == true && (fixType == 3 || fixType == 4 || fixType == 5)) ||
            settings.ntripClient_TransmitGGA == false)
        {
            // Open connection to caster service
            if (!ntripClientConnect())
            {
                // Assume service not available
                if (ntripClientConnectLimitReached()) // Updates ntripClientConnectionAttemptTimeout
                    systemPrintln("NTRIP caster failed to connect. Do you have your caster address and port correct?");
                else
                {
                    if (ntripClientConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("NTRIP Client failed to connect to caster. Trying again in %d seconds.\r\n",
                                     ntripClientConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("NTRIP Client failed to connect to caster. Trying again in %d minutes.\r\n",
                                     ntripClientConnectionAttemptTimeout / 1000 / 60);
                }
            }
            else
                // Socket opened to NTRIP system
                ntripClientSetState(NTRIP_CLIENT_CONNECTING);
        }
        break;

    case NTRIP_CLIENT_CONNECTING:
        // Check for no response from the caster service
        if (ntripClientReceiveDataAvailable() < strlen("ICY 200 OK")) // Wait until at least a few bytes have arrived
        {
            // Check for response timeout
            if (millis() - ntripClientTimer > NTRIP_CLIENT_RESPONSE_TIMEOUT)
            {
                // NTRIP web service did not respond
                if (ntripClientConnectLimitReached()) // Updates ntripClientConnectionAttemptTimeout
                {
                    systemPrintln("NTRIP Caster failed to respond. Do you have your caster address and port correct?");

                    // Stop NTRIP client operations
                    ntripClientShutdown();
                }
                else
                {
                    if (ntripClientConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("NTRIP Client failed to connect to caster. Trying again in %d seconds.\r\n",
                                     ntripClientConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("NTRIP Client failed to connect to caster. Trying again in %d minutes.\r\n",
                                     ntripClientConnectionAttemptTimeout / 1000 / 60);

                    // Restart network operation after delay
                    ntripClientRestart();
                }
            }
        }
        else
        {
            // Caster web service responded
            char response[512];
            ntripClientResponse(&response[0], sizeof(response));

            //log_d("Caster Response: %s", response);

            // Look for various responses
            if (strstr(response, "200") != nullptr) //'200' found
            {
                systemPrintf("NTRIP Client connected to %s:%d\r\n",
                             settings.ntripClient_CasterHost,
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

                    lastGGAPush = millis() - NTRIPCLIENT_MS_BETWEEN_GGA; // Force immediate transmission of GGA message
                }

                // We don't use a task because we use I2C hardware (and don't have a semphore).
                online.ntripClient = true;
                ntripClientStartTime = millis();
                ntripClientConnectionAttempts = 0;
                ntripClientSetState(NTRIP_CLIENT_CONNECTED);
            }
            else if (strstr(response, "401") != nullptr)
            {
                // Look for '401 Unauthorized'
                systemPrintf(
                    "NTRIP Caster responded with bad news: %s. Are you sure your caster credentials are correct?\r\n",
                    response);

                // Stop NTRIP client operations
                ntripClientShutdown();
            }
            else if (strstr(response, "banned") != nullptr)
            {
                // Look for 'HTTP/1.1 200 OK' and banned IP information
                systemPrintf("NTRIP Client connected to caster but caster responded with problem: %s\r\n", response);

                // Stop NTRIP client operations
                ntripClientShutdown();
            }
            else if (strstr(response, "SOURCETABLE") != nullptr)
            {
                // Look for 'SOURCETABLE 200 OK'
                systemPrintf("Caster may not have mountpoint %s. Caster responded with problem: %s\r\n",
                             settings.ntripClient_MountPoint, response);

                // Stop NTRIP client operations
                ntripClientShutdown();
            }
            // Other errors returned by the caster
            else
            {
                systemPrintf("NTRIP Client connected but caster responded with problem: %s\r\n", response);

                // Check for connection limit
                if (ntripClientConnectLimitReached())
                    systemPrintln("NTRIP Client retry limit reached; do you have your caster address and port correct?");

                // Attempt to reconnect after throttle controlled timeout
                else
                {
                    if (ntripClientConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("NTRIP Client attempting connection in %d seconds.\r\n",
                                     ntripClientConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("NTRIP Client attempting connection in %d minutes.\r\n",
                                     ntripClientConnectionAttemptTimeout / 1000 / 60);
                }
            }
        }
        break;

    case NTRIP_CLIENT_CONNECTED:
        // Check for a broken connection
        if (!ntripClient->connected())
        {
            // Broken connection, retry the NTRIP client connection
            systemPrintln("NTRIP Client connection to caster was broken");
            ntripClientRestart();
        }
        else
        {
            // Check for timeout receiving NTRIP data
            if (ntripClientReceiveDataAvailable() == 0)
            {
                if ((millis() - ntripClientTimer) > NTRIP_CLIENT_RECEIVE_DATA_TIMEOUT)
                {
                    // Timeout receiving NTRIP data, retry the NTRIP client connection
                    systemPrintln("NTRIP Client: No data received timeout");
                    ntripClientRestart();
                }
            }
            else
            {
                // Receive data from the NTRIP Caster
                uint8_t rtcmData[RTCM_DATA_SIZE];
                size_t rtcmCount = 0;

                // Collect any available RTCM data
                while (ntripClientReceiveDataAvailable() > 0)
                {
                    rtcmData[rtcmCount++] = ntripClient->read();
                    if (rtcmCount == sizeof(rtcmData))
                        break;
                }

                // Restart the NTRIP receive data timer
                ntripClientTimer = millis();

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
        break;
    }

    // Periodically display the NTRIP client state
    if (PERIODIC_DISPLAY(PD_NTRIP_CLIENT_STATE))
        ntripClientSetState(ntripClientState);
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
