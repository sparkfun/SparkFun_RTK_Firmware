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

// Define the PVT server states
enum PvtServerStates
{
    PVT_SERVER_STATE_OFF = 0,
    PVT_SERVER_STATE_NETWORK_STARTED,
    PVT_SERVER_STATE_RUNNING,
    PVT_SERVER_STATE_WAIT_NO_CLIENTS,
    // Insert new states here
    PVT_SERVER_STATE_MAX            // Last entry in the state list
};

const char * const pvtServerStateName[] =
{
    "PVT_SERVER_STATE_OFF",
    "PVT_SERVER_STATE_NETWORK_STARTED",
    "PVT_SERVER_STATE_RUNNING",
    "PVT_SERVER_STATE_WAIT_NO_CLIENTS"
};

const int pvtServerStateNameEntries = sizeof(pvtServerStateName) / sizeof(pvtServerStateName[0]);

//----------------------------------------
// Locals
//----------------------------------------

// PVT server
static WiFiServer * pvtServer = nullptr;
static uint8_t pvtServerState;

// PVT server clients
static volatile uint8_t pvtServerClientConnected;
static WiFiClient pvtServerClient[PVT_SERVER_MAX_CLIENTS];
static IPAddress pvtServerClientIpAddress[PVT_SERVER_MAX_CLIENTS];
static volatile uint16_t pvtServerClientTails[PVT_SERVER_MAX_CLIENTS];

//----------------------------------------
// PVT Server Routines
//----------------------------------------

// Send data to the PVT clients
int32_t pvtServerClientSendData(int index, uint8_t *data, uint16_t length)
{

    length = pvtServerClient[index].write(data, length);
    if (length >= 0)
    {
        if ((settings.debugPvtServer) && (!inMainMenu))
            systemPrintf("PVT server wrote %d bytes\r\n", length);
    }
    // Failed to write the data
    else
    {
        // Done with this client connection
        if (!inMainMenu)
        {
            systemPrintf("PVT server breaking connection %d with client\r\n", index);
        }

        pvtServerClient[index].stop();
        pvtServerClientConnected &= ~(1 << index);
        length = 0;
    }
    return length;
}

// Send PVT data to the clients
int32_t pvtServerSendData(uint16_t dataHead)
{
    int32_t usedSpace = 0;

    bool connected;
    int32_t bytesToSend;
    int index;
    uint16_t tail;

    // Determine if a client is connected
    connected = settings.enablePvtServer && online.pvtServer && pvtServerClientConnected;

    // Update each of the clients
    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
    {
        tail = pvtServerClientTails[index];

        // Determine if the client is connected
        if ((!connected) || ((pvtServerClientConnected & (1 << index)) == 0))
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

    // Shutdown the PVT server if necessary
    if (settings.enablePvtServer || online.pvtServer)
        pvtServerActive();

    // Return the amount of space that WiFi is using in the buffer
    return usedSpace;
}

// Check for PVT server active
bool pvtServerActive()
{
    if ((settings.enablePvtServer && online.pvtServer) || pvtServerClientConnected)
        return true;

    log_d("Stopping PVT Server");

    // Shutdown the PVT server
    online.pvtServer = false;

    if (pvtServer != nullptr)
    {
        // Stop the PVT server
        pvtServer->stop();
        delete pvtServer;
        pvtServer = nullptr;
    }
    return false;
}

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

// Update the PVT server state
void pvtServerUpdate()
{
    int index;

    if (settings.enablePvtServer == false)
        return; // Nothing to do

    if (wifiInConfigMode())
        return; // Do not service PVT server during WiFi config

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

    switch (pvtServerState)
    {
    default:
        break;

    // Wait until the PVT server is enabled
    case PVT_SERVER_STATE_OFF:
        // Determine if the PVT server should be running
        if (settings.enablePvtServer && (!wifiIsConnected()))
        {
            // Verify PVT_SERVER_MAX_CLIENTS
            if ((sizeof(pvtServerClientConnected) * 8) < PVT_SERVER_MAX_CLIENTS)
            {
                systemPrintf("Please set PVT_SERVER_MAX_CLIENTS <= %d or increase size of pvtServerClientConnected\r\n",
                             sizeof(pvtServerClientConnected) * 8);
                reportFatalError("Please adjust PVT_SERVER_MAX_CLIENTS  or increase size of pvtServerClientConnected");
            }

            log_d("PVT server starting WiFi");
            wifiStart();
            pvtServerSetState(PVT_SERVER_STATE_NETWORK_STARTED);
        }
        break;

    // Wait until the network is connected
    case PVT_SERVER_STATE_NETWORK_STARTED:
        if (wifiIsConnected())
        {
            // Start the PVT server if enabled
            pvtServer = new WiFiServer(settings.pvtServerPort);
            pvtServer->begin();
            online.pvtServer = true;
            systemPrint("PVT server online, IP Address ");
            systemPrintln(WiFi.localIP());
            pvtServerSetState(PVT_SERVER_STATE_RUNNING);
        }
        break;

    // Handle client connections and link failures
    case PVT_SERVER_STATE_RUNNING:
        if ((!settings.enablePvtServer) || (!wifiIsConnected()))
        {
            // Notify the rest of the system that the PVT server is shutting down
            online.pvtServer = false;
            pvtServerSetState(PVT_SERVER_STATE_WAIT_NO_CLIENTS);
            break;
        }

        // Walk the list of PVT server clients
        for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
        {
            // Determine if the client data structure is in use
            if (pvtServerClientConnected & (1 << index))
            {
                // Data structure in use
                // Check for a broken connection
                if ((!pvtServerClient[index]) || (!pvtServerClient[index].connected()))
                {
                    // Done with this client connection
                    if (!inMainMenu)
                    {
                        systemPrintf("PVT server client %d Disconnected from ", index);
                        systemPrintln(pvtServerClientIpAddress[index]);
                    }

                    // Shutdown the PVT server if necessary
                    if (pvtServerClient[index])
                        pvtServerClient[index].stop();
                    pvtServerClientConnected &= ~(1 << index);
                    if (settings.enablePvtServer || online.pvtServer)
                        pvtServerActive();
                }
            }
        }

        // Walk the list of PVT server clients
        for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
        {
            // Determine if the client data structure is in use
            if (!(pvtServerClientConnected & (1 << index)))
            {
                // Data structure not in use
                // Check for another PVT server client
                pvtServerClient[index] = pvtServer->available();
                if (!pvtServerClient[index])
                    break;
                pvtServerClientIpAddress[index] = pvtServerClient[index].remoteIP();
                systemPrintf("PVT server client %d connected to ", index);
                systemPrintln(pvtServerClientIpAddress[index]);
                pvtServerClientConnected |= 1 << index;
            }
        }
        break;

    // Wait until all the clients are closed
    case PVT_SERVER_STATE_WAIT_NO_CLIENTS:
        if (!pvtServerClientConnected)
        {
            log_d("Stopping WiFi PVT Server");

            // Stop the PVT server
            pvtServer->stop();
            delete pvtServer;
            pvtServer = nullptr;
            pvtServerSetState(PVT_SERVER_STATE_OFF);
        }
        break;
    }
}

// Verify the PVT server tables
void pvtServerValidateTables()
{
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
