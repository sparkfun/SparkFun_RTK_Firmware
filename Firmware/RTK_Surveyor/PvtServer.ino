/*
pvtServer.ino

  The PVT (position, velocity and time) server sits on top of the network layer
  and sends position data to one or more computers or cell phones for display.

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

*/

#ifdef COMPILE_WIFI

//----------------------------------------
// Constants
//----------------------------------------

#define PVT_SERVER_MAX_CLIENTS 4
#define PVT_SERVER_CLIENT_DATA_TIMEOUT  (15 * 1000)

// Define the PVT server states
enum PvtServerStates
{
    PVT_SERVER_STATE_OFF = 0,
    PVT_SERVER_STATE_NETWORK_STARTED,
    PVT_SERVER_STATE_RUNNING,
    // Insert new states here
    PVT_SERVER_STATE_MAX            // Last entry in the state list
};

const char * const pvtServerStateName[] =
{
    "PVT_SERVER_STATE_OFF",
    "PVT_SERVER_STATE_NETWORK_STARTED",
    "PVT_SERVER_STATE_RUNNING",
};

const int pvtServerStateNameEntries = sizeof(pvtServerStateName) / sizeof(pvtServerStateName[0]);

//----------------------------------------
// Locals
//----------------------------------------

// PVT server
static WiFiServer * pvtServer = nullptr;
static uint8_t pvtServerState;
static uint32_t pvtServerTimer;

// PVT server clients
static volatile uint8_t pvtServerClientConnected;
static volatile uint8_t pvtServerClientDataSent;
static volatile uint8_t pvtServerClientWriteError;
static NetworkClient * pvtServerClient[PVT_SERVER_MAX_CLIENTS];
static IPAddress pvtServerClientIpAddress[PVT_SERVER_MAX_CLIENTS];
static volatile RING_BUFFER_OFFSET pvtServerClientTails[PVT_SERVER_MAX_CLIENTS];

//----------------------------------------
// PVT Server handleGnssDataTask Support Routines
//----------------------------------------

// Send data to the PVT clients
int32_t pvtServerClientSendData(int index, uint8_t *data, uint16_t length)
{

    length = pvtServerClient[index]->write(data, length);
    if (length >= 0)
    {
        // Update the data sent flag when data successfully sent
        if (length > 0)
            pvtServerClientDataSent |= 1 << index;
        if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_CLIENT_DATA)) && (!inMainMenu))
        {
            PERIODIC_CLEAR(PD_PVT_SERVER_CLIENT_DATA);
            systemPrintf("PVT server wrote %d bytes to %d.%d.%d.%d\r\n",
                         length,
                         pvtServerClientIpAddress[index][0],
                         pvtServerClientIpAddress[index][1],
                         pvtServerClientIpAddress[index][2],
                         pvtServerClientIpAddress[index][3]);
        }
    }

    // Failed to write the data
    else
    {
        // Done with this client connection
        if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_CLIENT_DATA)) && (!inMainMenu))
        {
            PERIODIC_CLEAR(PD_PVT_SERVER_CLIENT_DATA);
            systemPrintf("PVT server breaking connection %d with client %d.%d.%d.%d\r\n",
                         index,
                         pvtServerClientIpAddress[index][0],
                         pvtServerClientIpAddress[index][1],
                         pvtServerClientIpAddress[index][2],
                         pvtServerClientIpAddress[index][3]);
        }

        pvtServerClient[index]->stop();
        pvtServerClientConnected &= ~(1 << index);
        pvtServerClientWriteError |= 1 << index;
        length = 0;
    }
    return length;
}

// Send PVT data to the clients
int32_t pvtServerSendData(uint16_t dataHead)
{
    int32_t usedSpace = 0;

    int32_t bytesToSend;
    int index;
    uint16_t tail;

    // Update each of the clients
    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
    {
        tail = pvtServerClientTails[index];

        // Determine if the client is connected
        if (!(pvtServerClientConnected & (1 << index)))
            tail = dataHead;
        else
        {
            // Determine the amount of PVT data in the buffer
            bytesToSend = dataHead - tail;
            if (bytesToSend < 0)
                bytesToSend += settings.gnssHandlerBufferSize;
            if (bytesToSend > 0)
            {
                // Reduce bytes to send if we have more to send then the end of the buffer
                // We'll wrap next loop
                if ((tail + bytesToSend) > settings.gnssHandlerBufferSize)
                    bytesToSend = settings.gnssHandlerBufferSize - tail;

                // Send the data to the PVT server clients
                bytesToSend = pvtServerClientSendData(index, &ringBuffer[tail], bytesToSend);

                // Assume all data was sent, wrap the buffer pointer
                tail += bytesToSend;
                if (tail >= settings.gnssHandlerBufferSize)
                    tail -= settings.gnssHandlerBufferSize;

                // Update space available for use in UART task
                bytesToSend = dataHead - tail;
                if (bytesToSend < 0)
                    bytesToSend += settings.gnssHandlerBufferSize;
                if (usedSpace < bytesToSend)
                    usedSpace = bytesToSend;
            }
        }
        pvtServerClientTails[index] = tail;
    }

    // Return the amount of space that PVT server client is using in the buffer
    return usedSpace;
}

// Remove previous messages from the ring buffer
void discardPvtServerBytes(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail)
{
    int index;
    uint16_t tail;

    // Update each of the clients
    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
    {
        tail = pvtServerClientTails[index];
        if (previousTail < newTail)
        {
            // No buffer wrap occurred
            if ((tail >= previousTail) && (tail < newTail))
                pvtServerClientTails[index] = newTail;
        }
        else
        {
            // Buffer wrap occurred
            if ((tail >= previousTail) || (tail < newTail))
                pvtServerClientTails[index] = newTail;
        }
    }
}

//----------------------------------------
// PVT Server Routines
//----------------------------------------

// Update the state of the PVT server state machine
void pvtServerSetState(uint8_t newState)
{
    if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_STATE)) && (!inMainMenu))
    {
        if (pvtServerState == newState)
            systemPrint("*");
        else
            systemPrintf("%s --> ", pvtServerStateName[pvtServerState]);
    }
    pvtServerState = newState;
    if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_STATE)) && (!inMainMenu))
    {
        PERIODIC_CLEAR(PD_PVT_SERVER_STATE);
        if (newState >= PVT_SERVER_STATE_MAX)
        {
            systemPrintf("Unknown PVT Server state: %d\r\n", pvtServerState);
            reportFatalError("Unknown PVT Server state");
        }
        else
            systemPrintln(pvtServerStateName[pvtServerState]);
    }
}

// Start the PVT server
bool pvtServerStart()
{
    IPAddress localIp;

    if (settings.debugPvtServer && (!inMainMenu))
        systemPrintln("PVT server starting the server");

    // Start the PVT server
    pvtServer = new WiFiServer(settings.pvtServerPort);
    if (!pvtServer)
        return false;

    pvtServer->begin();
    online.pvtServer = true;
    localIp = wifiGetIpAddress();
    systemPrintf("PVT server online, IP address %d.%d.%d.%d:%d\r\n",
                 localIp[0], localIp[1], localIp[2], localIp[3],
                 settings.pvtServerPort);
    return true;
}

// Stop the PVT server
void pvtServerStop()
{
    int index;

    // Notify the rest of the system that the PVT server is shutting down
    if (online.pvtServer)
    {
        // Notify the UART2 tasks of the PVT server shutdown
        online.pvtServer = false;
        delay(5);
    }

    // Determine if PVT server clients are active
    if (pvtServerClientConnected)
    {
        // Shutdown the PVT server client links
        for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
            pvtServerStopClient(index);
    }

    // Shutdown the PVT server
    if (pvtServer != nullptr)
    {
        // Stop the PVT server
        if (settings.debugPvtServer && (!inMainMenu))
            systemPrintln("PVT server stopping");
        pvtServer->stop();
        delete pvtServer;
        pvtServer = nullptr;
    }

    // Stop using the network
    if (pvtServerState != PVT_SERVER_STATE_OFF)
    {
        networkUserClose(NETWORK_USER_PVT_SERVER);

        // The PVT server is now off
        pvtServerSetState(PVT_SERVER_STATE_OFF);
        pvtServerTimer = millis();
    }
}

// Stop the PVT server client
void pvtServerStopClient(int index)
{
    bool connected;
    bool dataSent;

    // Done with this client connection
    if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_DATA)) && (!inMainMenu))
    {
        PERIODIC_CLEAR(PD_PVT_SERVER_DATA);

        // Determine the shutdown reason
        connected = pvtServerClient[index]->connected()
                    && (!(pvtServerClientWriteError & (1 << index)));
        dataSent = ((millis() - pvtServerTimer) < PVT_SERVER_CLIENT_DATA_TIMEOUT)
                 || (pvtServerClientDataSent & (1 << index));
        if (!dataSent)
            systemPrintf("PVT Server: No data sent over %d seconds\r\n",
                         PVT_SERVER_CLIENT_DATA_TIMEOUT / 1000);
        if (!connected)
            systemPrintf("PVT Server: Link to client broken\r\n");
        systemPrintf("PVT server client %d disconnected from %d.%d.%d.%d\r\n",
                     index,
                     pvtServerClientIpAddress[index][0],
                     pvtServerClientIpAddress[index][1],
                     pvtServerClientIpAddress[index][2],
                     pvtServerClientIpAddress[index][3]);
    }

    // Shutdown the PVT server client link
    pvtServerClient[index]->stop();
    pvtServerClientConnected &= ~(1 << index);
    pvtServerClientWriteError &= ~(1 << index);
}

// Update the PVT server state
void pvtServerUpdate()
{
    bool connected;
    bool dataSent;
    int index;
    IPAddress ipAddress;

    /*
        PVT Server state machine

                .---------------->PVT_SERVER_STATE_OFF
                |                           |
                | pvtServerStop             | settings.enablePvtServer
                |                           |
                |                           V
                +<----------PVT_SERVER_STATE_NETWORK_STARTED
                ^                           |
                |                           | networkUserConnected
                |                           |
                |                           V
                +<--------------PVT_SERVER_STATE_RUNNING
                ^                           |
                |                           | network failure
                |                           |
                |                           V
                '-----------PVT_SERVER_STATE_WAIT_NO_CLIENTS
    */

    DMW_st(pvtServerSetState, pvtServerState);
    switch (pvtServerState)
    {
    default:
        break;

    // Wait until the PVT server is enabled
    case PVT_SERVER_STATE_OFF:
        // Determine if the PVT server should be running
        if (settings.enablePvtServer && (!wifiInConfigMode()) && (!wifiIsConnected()))
        {
            if (networkUserOpen(NETWORK_USER_PVT_SERVER, NETWORK_TYPE_ACTIVE))
            {
                if (settings.debugPvtServer && (!inMainMenu))
                    systemPrintln("PVT server starting the network");
                pvtServerSetState(PVT_SERVER_STATE_NETWORK_STARTED);
            }
        }
        break;

    // Wait until the network is connected
    case PVT_SERVER_STATE_NETWORK_STARTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_PVT_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            pvtServerStop();

        // Wait for the network to connect to the media
        else if (networkUserConnected(NETWORK_USER_PVT_SERVER))
        {
            // Delay before starting the PVT server
            if ((millis() - pvtServerTimer) >= (1 * 1000))
            {
                // The network type and host provide a valid configuration
                pvtServerTimer = millis();

                // Start the PVT server
                if (pvtServerStart())
                    pvtServerSetState(PVT_SERVER_STATE_RUNNING);
            }
        }
        break;

    // Handle client connections and link failures
    case PVT_SERVER_STATE_RUNNING:
        // Determine if the network has failed
        if ((!settings.enablePvtServer) || networkIsShuttingDown(NETWORK_USER_PVT_SERVER))
        {
            if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_DATA)) && (!inMainMenu))
            {
                PERIODIC_CLEAR(PD_PVT_SERVER_DATA);
                systemPrintln("PVT server initiating shutdown");
            }

            // Network connection failed, attempt to restart the network
            pvtServerStop();
            break;
        }

        // Walk the list of PVT server clients
        for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
        {
            // Determine if the client data structure is in use
            if (pvtServerClientConnected & (1 << index))
            {
                // Data structure in use
                // Check for a working PVT server client connection
                connected = pvtServerClient[index]->connected();
                dataSent = ((millis() - pvtServerTimer) < PVT_SERVER_CLIENT_DATA_TIMEOUT)
                         || (pvtServerClientDataSent & (1 << index));
                if (connected && dataSent)
                {
                    // Display this client connection
                    if (PERIODIC_DISPLAY(PD_PVT_SERVER_DATA) && (!inMainMenu))
                    {
                        PERIODIC_CLEAR(PD_PVT_SERVER_DATA);
                        systemPrintf("PVT server client %d connected to %d.%d.%d.%d\r\n",
                                     index,
                                     pvtServerClientIpAddress[index][0],
                                     pvtServerClientIpAddress[index][1],
                                     pvtServerClientIpAddress[index][2],
                                     pvtServerClientIpAddress[index][3]);
                    }
                }

                // Shutdown the PVT server client link
                else
                    pvtServerStopClient(index);
            }
        }

        // Walk the list of PVT server clients
        for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
        {
            // Determine if the client data structure is in use
            if (!(pvtServerClientConnected & (1 << index)))
            {
                WiFiClient client;

                // Data structure not in use
                // Check for another PVT server client
                client = pvtServer->available();

                // Done if no PVT server client found
                if (!client)
                    break;

                // Start processing the new PVT server client connection
                pvtServerClient[index] = new NetworkWiFiClient(client);
                pvtServerClientIpAddress[index] = pvtServerClient[index]->remoteIP();
                pvtServerClientConnected |= 1 << index;
                pvtServerClientDataSent |= 1 << index;
                if ((settings.debugPvtServer || PERIODIC_DISPLAY(PD_PVT_SERVER_DATA)) && (!inMainMenu))
                {
                    PERIODIC_CLEAR(PD_PVT_SERVER_DATA);
                    systemPrintf("PVT server client %d connected to %d.%d.%d.%d\r\n",
                                 index,
                                 pvtServerClientIpAddress[index][0],
                                 pvtServerClientIpAddress[index][1],
                                 pvtServerClientIpAddress[index][2],
                                 pvtServerClientIpAddress[index][3]);
                }
            }
        }

        // Check for data moving across the connections
        if ((millis() - pvtServerTimer) >= PVT_SERVER_CLIENT_DATA_TIMEOUT)
        {
            // Restart the data verification
            pvtServerTimer = millis();
            pvtServerClientDataSent = 0;
        }
        break;
    }

    // Periodically display the PVT state
    if (PERIODIC_DISPLAY(PD_PVT_SERVER_STATE) && (!inMainMenu))
        pvtServerSetState(pvtServerState);
}

// Verify the PVT server tables
void pvtServerValidateTables()
{
    char line[128];

    // Verify PVT_SERVER_MAX_CLIENTS
    if ((sizeof(pvtServerClientConnected) * 8) < PVT_SERVER_MAX_CLIENTS)
    {
        snprintf(line, sizeof(line),
                 "Please set PVT_SERVER_MAX_CLIENTS <= %d or increase size of pvtServerClientConnected",
                 sizeof(pvtServerClientConnected) * 8);
        reportFatalError(line);
    }
    if (pvtServerStateNameEntries != PVT_SERVER_STATE_MAX)
        reportFatalError("Fix pvtServerStateNameEntries to match PvtServerStates");
}

// Zero the PVT server client tails
void pvtServerZeroTail()
{
    int index;

    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
        pvtServerClientTails[index] = 0;
}

#endif // COMPILE_WIFI
