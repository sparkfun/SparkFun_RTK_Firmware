/*
PvtClient.ino

  The (position, velocity and time) client sits on top of the network layer and
  sends position data to one
  or more computers or cell phones for display.

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
                               |
                               V
            Bluetooth         RTK                 Network: PVT Client
           .---------------- Rover ----------------------------------.
           |                   |                                     |
           |                   | Network: PVT Server                 |
           | PVT data          | Position, velocity & time data      | PVT data
           |                   V                                     |
           |              Computer or                                |
           '------------> Cell Phone <-------------------------------'
                          for display

  PVT Client Testing:

     Using Ethernet on RTK Reference Station and specifying a PVT Client host:

        1. Network failure - Disconnect Ethernet cable at RTK Reference Station,
           expecting retry PVT client connection after network restarts

        2. Internet link failure - Disconnect Ethernet cable between Ethernet
           switch and the firewall to simulate an internet failure, expecting
           retry PVT client connection after delay

        3. Internet outage - Disconnect Ethernet cable between Ethernet
           switch and the firewall to simulate an internet failure, expecting
           PVT client retry interval to increase from 5 seconds to 5 minutes
           and 20 seconds, and the PVT client to reconnect following the outage.

     Using WiFi on RTK Express or RTK Reference Station, no PVT Client host
     specified, running Vespucci on the cell phone:

        Vespucci Setup:
            * Click on the gear icon
            * Scroll down and click on Advanced preferences
            * Click on Location settings
            * Click on GPS/GNSS source
            * Set to NMEA from TCP server
            * Click on NMEA network source
            * Set IP address to 127.0.0.1:1958
            * Uncheck Leave GPS/GNSS turned off
            * Check Fallback to network location
            * Click on Stale location after
            * Set the value 5 seconds
            * Exit the menus

        1. Verify connection to the Vespucci application on the cell phone
           (gateway IP address).

        2. Vespucci not running: Stop the Vespucci application, expecting PVT
           client retry interval to increase from 5 seconds to 5 minutes and
           20 seconds.  The PVT client connects once the Vespucci application
           is restarted on the phone.

  Test Setups:

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
                                       NMEA Server
                                       NTRIP Caster


          RTK Express     RTK Reference Station
               ^            ^
          WiFi |       WiFi |
               \            /
                \          /
                 v        v
          Cell Phone (NMEA Server)
                      ^
                      |
                      v
                NTRIP Caster

  Possible NTRIP Casters

    * https://emlid.com/ntrip-caster/
    * http://rtk2go.com/
    * private SNIP NTRIP caster
*/

#if COMPILE_NETWORK

//----------------------------------------
// Constants
//----------------------------------------

#define PVT_MAX_CONNECTIONS             6
#define PVT_DELAY_BETWEEN_CONNECTIONS   (5 * 1000)

// Define the PVT client states
enum PvtClientStates
{
    PVT_CLIENT_STATE_OFF = 0,
    PVT_CLIENT_STATE_NETWORK_STARTED,
    PVT_CLIENT_STATE_CLIENT_STARTING,
    PVT_CLIENT_STATE_CONNECTED,
    // Insert new states here
    PVT_CLIENT_STATE_MAX            // Last entry in the state list
};

const char * const pvtClientStateName[] =
{
    "PVT_CLIENT_STATE_OFF",
    "PVT_CLIENT_STATE_NETWORK_STARTED",
    "PVT_CLIENT_STATE_CLIENT_STARTING",
    "PVT_CLIENT_STATE_CONNECTED"
};

const int pvtClientStateNameEntries = sizeof(pvtClientStateName) / sizeof(pvtClientStateName[0]);

const RtkMode_t pvtClientMode = RTK_MODE_BASE_FIXED
                              | RTK_MODE_BASE_SURVEY_IN
                              | RTK_MODE_ROVER;

//----------------------------------------
// Locals
//----------------------------------------

static NetworkClient * pvtClient;
static IPAddress pvtClientIpAddress;
static uint8_t pvtClientState;
static volatile RING_BUFFER_OFFSET pvtClientTail;
static volatile bool pvtClientWriteError;

//----------------------------------------
// PVT Client handleGnssDataTask Support Routines
//----------------------------------------

// Send PVT data to the NMEA server
int32_t pvtClientSendData(uint16_t dataHead)
{
    bool connected;
    int32_t bytesToSend;
    int32_t bytesSent;

    // Determine if a client is connected
    bytesToSend = 0;
    connected = settings.enablePvtClient && online.pvtClient;

    // Determine if the client is connected
    if ((!connected) || (!online.pvtClient))
        pvtClientTail = dataHead;
    else
    {
        // Determine the amount of data in the buffer
        bytesToSend = dataHead - pvtClientTail;
        if (bytesToSend < 0)
            bytesToSend += settings.gnssHandlerBufferSize;
        if (bytesToSend > 0)
        {
            // Reduce bytes to send if we have more to send then the end of the buffer
            // We'll wrap next loop
            if ((pvtClientTail + bytesToSend) > settings.gnssHandlerBufferSize)
                bytesToSend = settings.gnssHandlerBufferSize - pvtClientTail;

            // Send the data to the NMEA server
            bytesSent = pvtClient->write(&ringBuffer[pvtClientTail], bytesToSend);
            if (bytesSent >= 0)
            {
                if ((settings.debugPvtClient || PERIODIC_DISPLAY(PD_PVT_CLIENT_DATA)) && (!inMainMenu))
                {
                    PERIODIC_CLEAR(PD_PVT_CLIENT_DATA);
                    systemPrintf("PVT client sent %d bytes, %d remaining\r\n", bytesSent, bytesToSend - bytesSent);
                }

                // Assume all data was sent, wrap the buffer pointer
                pvtClientTail += bytesSent;
                if (pvtClientTail >= settings.gnssHandlerBufferSize)
                    pvtClientTail -= settings.gnssHandlerBufferSize;

                // Update space available for use in UART task
                bytesToSend = dataHead - pvtClientTail;
                if (bytesToSend < 0)
                    bytesToSend += settings.gnssHandlerBufferSize;
            }

            // Failed to write the data
            else
            {
                // Done with this client connection
                if (!inMainMenu)
                    systemPrintf("PVT client breaking connection with %s:%d\r\n",
                         pvtClientIpAddress.toString().c_str(), settings.pvtClientPort);

                pvtClientWriteError = true;
                bytesToSend = 0;
            }
        }
    }

    // Return the amount of space that WiFi is using in the buffer
    return bytesToSend;
}

// Update the state of the PVT client state machine
void pvtClientSetState(uint8_t newState)
{
    if ((settings.debugPvtClient || PERIODIC_DISPLAY(PD_PVT_CLIENT_STATE)) && (!inMainMenu))
    {
        if (pvtClientState == newState)
            systemPrint("*");
        else
            systemPrintf("%s --> ", pvtClientStateName[pvtClientState]);
    }
    pvtClientState = newState;
    if ((settings.debugPvtClient || PERIODIC_DISPLAY(PD_PVT_CLIENT_STATE)) && (!inMainMenu))
    {
        PERIODIC_CLEAR(PD_PVT_CLIENT_STATE);
        if (newState >= PVT_CLIENT_STATE_MAX)
        {
            systemPrintf("Unknown PVT Client state: %d\r\n", pvtClientState);
            reportFatalError("Unknown PVT Client state");
        }
        else
            systemPrintln(pvtClientStateName[pvtClientState]);
    }
}

// Remove previous messages from the ring buffer
void discardPvtClientBytes(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail)
{
    if (previousTail < newTail)
    {
        // No buffer wrap occurred
        if ((pvtClientTail >= previousTail) && (pvtClientTail < newTail))
            pvtClientTail = newTail;
    }
    else
    {
        // Buffer wrap occurred
        if ((pvtClientTail >= previousTail) || (pvtClientTail < newTail))
            pvtClientTail = newTail;
    }
}

//----------------------------------------
// PVT Client Routines
//----------------------------------------

// Start the PVT client
bool pvtClientStart()
{
    NetworkClient * client;

    // Allocate the PVT client
    client = new NetworkClient(NETWORK_USER_PVT_CLIENT);
    if (client)
    {
        // Get the host name
        char hostname[sizeof(settings.pvtClientHost)];
        strcpy(hostname, settings.pvtClientHost);
        if (!strlen(hostname))
        {
            // No host was specified, assume we are using WiFi and using a phone
            // running the application and a WiFi hot spot.  The IP address of
            // the phone is provided to the RTK during the DHCP handshake as the
            // gateway IP address.

            // Attempt the PVT client connection
            pvtClientIpAddress = wifiGetGatewayIpAddress();
            sprintf(hostname, "%s", pvtClientIpAddress.toString().c_str());
        }

        // Display the PVT client connection attempt
        if (settings.debugPvtClient)
            systemPrintf("PVT client connecting to %s:%d\r\n", hostname, settings.pvtClientPort);

        // Attempt the PVT client connection
        if (client->connect(hostname, settings.pvtClientPort))
        {
            // Get the client IP address
            pvtClientIpAddress = client->remoteIP();

            // Display the PVT client connection
            systemPrintf("PVT client connected to %s:%d\r\n",
                         pvtClientIpAddress.toString().c_str(), settings.pvtClientPort);

            // The PVT client is connected
            pvtClient = client;
            pvtClientWriteError = false;
            online.pvtClient = true;
            return true;
        }
        else
        {
            // Release any allocated resources
            client->stop();
            delete client;
        }
    }
    return false;
}

// Stop the PVT client
void pvtClientStop()
{
    NetworkClient * client;
    IPAddress ipAddress;

    client = pvtClient;
    if (client)
    {
        // Delay to allow the UART task to finish with the pvtClient
        online.pvtClient = false;
        delay(5);

        // Done with the PVT client connection
        ipAddress = pvtClientIpAddress;
        pvtClientIpAddress = IPAddress((uint32_t)0);
        pvtClient->stop();
        delete pvtClient;
        pvtClient = nullptr;

        // Notify the user of the PVT client shutdown
        if (!inMainMenu)
            systemPrintf("PVT client disconnected from %s:%d\r\n",
                         ipAddress.toString().c_str(), settings.pvtClientPort);
    }

    // Done with the network
    if (pvtClientState != PVT_CLIENT_STATE_OFF)
        networkUserClose(NETWORK_USER_PVT_CLIENT);

    // Initialize the PVT client
    pvtClientWriteError = false;
    if (settings.debugPvtClient)
        systemPrintln("PVT client offline");
    pvtClientSetState(PVT_CLIENT_STATE_OFF);
}

// Update the PVT client state
void pvtClientUpdate()
{
    static uint8_t connectionAttempt;
    static uint32_t connectionDelay;
    uint32_t days;
    byte hours;
    uint64_t milliseconds;
    byte minutes;
    byte seconds;
    static uint32_t timer;

    // Shutdown the PVT client when the mode or setting changes
    DMW_st(pvtClientSetState, pvtClientState);
    if (NEQ_RTK_MODE(pvtClientMode) || (!settings.enablePvtClient))
    {
        if (pvtClientState > PVT_CLIENT_STATE_OFF)
            pvtClientStop();
    }

    /*
        PVT Client state machine

                .---------------->PVT_CLIENT_STATE_OFF
                |                           |
                | pvtClientStop             | settings.enablePvtClient
                |                           |
                |                           V
                +<----------PVT_CLIENT_STATE_NETWORK_STARTED
                ^                           |
                |                           | networkUserConnected
                |                           |
                |                           V
                +<----------PVT_CLIENT_STATE_CLIENT_STARTING
                ^                           |
                |                           | pvtClientStart = true
                |                           |
                |                           V
                '--------------PVT_CLIENT_STATE_CONNECTED
    */

    switch (pvtClientState)
    {
    default:
        systemPrintf("PVT client state: %d\r\n", pvtClientState);
        reportFatalError("Invalid PVT client state");
        break;

    // Wait until the PVT client is enabled
    case PVT_CLIENT_STATE_OFF:
        // Determine if the PVT client should be running
        if (EQ_RTK_MODE(pvtClientMode) && settings.enablePvtClient)
        {
            if (networkUserOpen(NETWORK_USER_PVT_CLIENT, NETWORK_TYPE_ACTIVE))
            {
                timer = 0;
                pvtClientSetState(PVT_CLIENT_STATE_NETWORK_STARTED);
            }
        }
        break;

    // Wait until the network is connected
    case PVT_CLIENT_STATE_NETWORK_STARTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_PVT_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            pvtClientStop();

        // Determine if WiFi is required
        else if ((!strlen(settings.pvtClientHost)) && (networkGetType(NETWORK_TYPE_ACTIVE) != NETWORK_TYPE_WIFI))
        {
            // Wrong network type, WiFi is required but another network is being used
            if ((millis() - timer) >= (15 * 1000))
            {
                timer = millis();
                systemPrintln("PVT Client must connect via WiFi when no host is specified");
            }
        }

        // Wait for the network to connect to the media
        else if (networkUserConnected(NETWORK_USER_PVT_CLIENT))
        {
            // The network type and host provide a valid configuration
            timer = millis();
            pvtClientSetState(PVT_CLIENT_STATE_CLIENT_STARTING);
        }
        break;

    // Attempt the connection ot the PVT server
    case PVT_CLIENT_STATE_CLIENT_STARTING:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_PVT_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            pvtClientStop();

        // Delay before connecting to the network
        else if ((millis() - timer) >= connectionDelay)
        {
            timer = millis();

            // Start the PVT client
            if (!pvtClientStart())
            {
                // Connection failure
                if (settings.debugPvtClient)
                    systemPrintln("PVT Client connection failed");
                connectionDelay = PVT_DELAY_BETWEEN_CONNECTIONS << connectionAttempt;
                if (connectionAttempt < PVT_MAX_CONNECTIONS)
                    connectionAttempt += 1;

                // Display the uptime
                milliseconds = connectionDelay;
                days = milliseconds / MILLISECONDS_IN_A_DAY;
                milliseconds %= MILLISECONDS_IN_A_DAY;
                hours = milliseconds / MILLISECONDS_IN_AN_HOUR;
                milliseconds %= MILLISECONDS_IN_AN_HOUR;
                minutes = milliseconds / MILLISECONDS_IN_A_MINUTE;
                milliseconds %= MILLISECONDS_IN_A_MINUTE;
                seconds = milliseconds / MILLISECONDS_IN_A_SECOND;
                milliseconds %= MILLISECONDS_IN_A_SECOND;
                if (settings.debugPvtClient)
                    systemPrintf("PVT Client delaying %d %02d:%02d:%02d.%03lld\r\n",
                                 days, hours, minutes, seconds, milliseconds);
            }
            else
            {
                // Successful connection
                connectionAttempt = 0;
                pvtClientSetState(PVT_CLIENT_STATE_CONNECTED);
            }
        }
        break;

    // Wait for the PVT client to shutdown or a PVT client link failure
    case PVT_CLIENT_STATE_CONNECTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_PVT_CLIENT))
            // Failed to connect to to the network, attempt to restart the network
            pvtClientStop();

        // Determine if the PVT client link is broken
        else if ((!*pvtClient) || (!pvtClient->connected()) || pvtClientWriteError)
            // Stop the PVT client
            pvtClientStop();
        break;
    }

    // Periodically display the PVT client state
    if (PERIODIC_DISPLAY(PD_PVT_CLIENT_STATE))
        pvtClientSetState(pvtClientState);
}

// Verify the PVT client tables
void pvtClientValidateTables()
{
    if (pvtClientStateNameEntries != PVT_CLIENT_STATE_MAX)
        reportFatalError("Fix pvtClientStateNameEntries to match PvtClientStates");
}

// Zero the PVT client tail
void pvtClientZeroTail()
{
    pvtClientTail = 0;
}

#endif  // COMPILE_NETWORK
