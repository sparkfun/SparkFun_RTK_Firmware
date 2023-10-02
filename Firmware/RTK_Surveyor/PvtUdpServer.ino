/*
pvtUdpServer.ino

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
           |                   | Network: PVT Server                  |
           | PVT data          | Position, velocity & time data      | PVT data
           |                   V                                     |
           |              Computer or                                |
           '------------> Cell Phone <-------------------------------'
                          for display

    PVT UDP Server Testing:

     RTK Express(+) with WiFi enabled, PvtUdpServer enabled, PvtUdpPort setup,
     Smartphone with QField:
        
        Network Setup:
            Connect the Smartphone and the RTK Express to the same Network (e.g. the Smartphones HotSpot). 

        QField Setup:
            * Open a project
            * Open the left menu
            * Click on the gear icon
            * Click on Settings
            * Click on Positioning
            * Add a new Positioning device
            * Set Connection type to UDP (NMEA)
            * Set the Adress to <broadcast>
            * Set the Port to the value of the specified PvtUdpPort (default 10110)
            * Optional: give it a name (e.g. RTK Express UDP)
            * Click on the Checkmark
            * Make sure the new device is set as the Postioning device in use
            * Exit the menus

        1. Long press on the location icon in the lower right corner
        
        2. Enable Show Position Information

        3. Verify that the displayed coordinates, fix tpe etc. are valid   
*/

#ifdef COMPILE_WIFI

//----------------------------------------
// Constants
//----------------------------------------

// Define the PVT server states
enum PvtUdpServerStates
{
    PVT_UDP_SERVER_STATE_OFF = 0,
    PVT_UDP_SERVER_STATE_NETWORK_STARTED,
    PVT_UDP_SERVER_STATE_RUNNING,
    // Insert new states here
    PVT_UDP_SERVER_STATE_MAX            // Last entry in the state list
};

const char * const pvtUdpServerStateName[] =
{
    "PVT_UDP_SERVER_STATE_OFF",
    "PVT_UDP_SERVER_STATE_NETWORK_STARTED",
    "PVT_UDP_SERVER_STATE_RUNNING",
};

const int pvtUdpServerStateNameEntries = sizeof(pvtUdpServerStateName) / sizeof(pvtUdpServerStateName[0]);

//----------------------------------------
// Locals
//----------------------------------------

// PVT UDP server
static NetworkUDP *pvtUdpServer = nullptr;
static uint8_t pvtUdpServerState;
static uint32_t pvtUdpServerTimer;
static volatile RING_BUFFER_OFFSET pvtUdpServerTail;
//----------------------------------------
// PVT UDP Server handleGnssDataTask Support Routines
//----------------------------------------

// Send data as broadcast
int32_t pvtUdpServerSendDataBroadcast(uint8_t *data, uint16_t length)
{
     if (!length)
        return 0;

    // Send the data as broadcast
    if (settings.enablePvtUdpServer && online.pvtUdpServer && wifiIsConnected())
    {
        pvtUdpServer->beginPacket(WiFi.broadcastIP(), settings.pvtUdpServerPort);
        pvtUdpServer->write(data, length);
        if(pvtUdpServer->endPacket()){
            if ((settings.debugPvtUdpServer || PERIODIC_DISPLAY(PD_PVT_UDP_SERVER_BROADCAST_DATA)) && (!inMainMenu))
            {
              systemPrintf("PVT UDP Server wrote %d bytes as broadcast on port %d\r\n", length, settings.pvtUdpServerPort);
              PERIODIC_CLEAR(PD_PVT_UDP_SERVER_BROADCAST_DATA);
            }
        }
        // Failed to write the data
        else if ((settings.debugPvtUdpServer || PERIODIC_DISPLAY(PD_PVT_UDP_SERVER_BROADCAST_DATA)) && (!inMainMenu))
        {
            PERIODIC_CLEAR(PD_PVT_UDP_SERVER_BROADCAST_DATA);
            systemPrintf("PVT UDP Server failed to write %d bytes as broadcast\r\n", length);
            length = 0;
        }       
    }
    return length;
}

// Send PVT data as broadcast
int32_t pvtUdpServerSendData(uint16_t dataHead)
{
    int32_t usedSpace = 0;

    int32_t bytesToSend;

    uint16_t tail;

    tail = pvtUdpServerTail;

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

        // Send the data 
        bytesToSend = pvtUdpServerSendDataBroadcast(&ringBuffer[tail], bytesToSend);

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

    pvtUdpServerTail = tail;

    // Return the amount of space that PVT server client is using in the buffer
    return usedSpace;
}

// Remove previous messages from the ring buffer
void discardPvtUdpServerBytes(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail)
{
    int index;
    uint16_t tail;

    tail = pvtUdpServerTail;
    if (previousTail < newTail)
    {
        // No buffer wrap occurred
        if ((tail >= previousTail) && (tail < newTail))
            pvtUdpServerTail = newTail;
    }
    else
    {
        // Buffer wrap occurred
        if ((tail >= previousTail) || (tail < newTail))
            pvtUdpServerTail = newTail;
    }
}

//----------------------------------------
// PVT Server Routines
//----------------------------------------

// Update the state of the PVT server state machine
void pvtUdpServerSetState(uint8_t newState)
{
    if ((settings.debugPvtUdpServer || PERIODIC_DISPLAY(PD_PVT_UDP_SERVER_STATE)) && (!inMainMenu))
    {
        if (pvtUdpServerState == newState)
            systemPrint("*");
        else
            systemPrintf("%s --> ", pvtUdpServerStateName[pvtUdpServerState]);
    }
    pvtUdpServerState = newState;
    if ((settings.debugPvtUdpServer || PERIODIC_DISPLAY(PD_PVT_UDP_SERVER_STATE)) && (!inMainMenu))
    {
        PERIODIC_CLEAR(PD_PVT_UDP_SERVER_STATE);
        if (newState >= PVT_UDP_SERVER_STATE_MAX)
        {
            systemPrintf("Unknown PVT UDP Server state: %d\r\n", pvtUdpServerState);
            reportFatalError("Unknown PVT UDP Server state");
        }
        else
            systemPrintln(pvtUdpServerStateName[pvtUdpServerState]);
    }
}

// Start the PVT server
bool pvtUdpServerStart()
{
    IPAddress localIp;

    if (settings.debugPvtUdpServer && (!inMainMenu))
        systemPrintln("PVT UDP server starting");

    // Start the PVT server
    pvtUdpServer = new NetworkUDP(NETWORK_USER_PVT_UDP_SERVER);
    if (!pvtUdpServer)
        return false;

    pvtUdpServer->begin(settings.pvtUdpServerPort);
    online.pvtUdpServer = true;
    systemPrintf("PVT UDP server online, broadcasting to port %d\r\n", settings.pvtUdpServerPort);
    return true;
}

// Stop the PVT UDP server
void pvtUdpServerStop()
{
    int index;

    // Notify the rest of the system that the PVT server is shutting down
    if (online.pvtUdpServer)
    {
        // Notify the UART2 tasks of the PVT server shutdown
        online.pvtUdpServer = false;
        delay(5);
    }

    // Shutdown the PVT server
    if (pvtUdpServer != nullptr)
    {
        // Stop the PVT server
        if (settings.debugPvtUdpServer && (!inMainMenu))
            systemPrintln("PVT UDP server stopping");
        pvtUdpServer->stop();
        delete pvtUdpServer;
        pvtUdpServer = nullptr;
    }

    // Stop using the network
    if (pvtUdpServerState != PVT_UDP_SERVER_STATE_OFF)
    {
        networkUserClose(NETWORK_USER_PVT_UDP_SERVER);

        // The PVT server is now off
        pvtUdpServerSetState(PVT_UDP_SERVER_STATE_OFF);
        pvtUdpServerTimer = millis();
    }
}

// Update the PVT server state
void pvtUdpServerUpdate()
{
    bool connected;
    bool dataSent;
    int index;
    IPAddress ipAddress;

    /*
        PVT UDP Server state machine

                .--------------->PVT_UDP_SERVER_STATE_OFF
                |                           |
                | pvtUdpServerStop             | settings.enablePvtUdpServer
                |                           |
                |                           V
                +<---------PVT_UDP_SERVER_STATE_NETWORK_STARTED
                ^                           |
                |                           | networkUserConnected
                |                           |
                |                           V
                '--------------PVT_UDP_SERVER_STATE_RUNNING
    */

    DMW_st(pvtUdpServerSetState, pvtUdpServerState);
    switch (pvtUdpServerState)
    {
    default:
        break;

    // Wait until the PVT server is enabled
    case PVT_UDP_SERVER_STATE_OFF:
        // Determine if the PVT server should be running
        if (settings.enablePvtUdpServer && (!wifiInConfigMode()) && (!wifiIsConnected()))
        {
            if (networkUserOpen(NETWORK_USER_PVT_UDP_SERVER, NETWORK_TYPE_ACTIVE))
            {
                if (settings.debugPvtUdpServer && (!inMainMenu))
                    systemPrintln("PVT UDP server starting the network");
                pvtUdpServerSetState(PVT_UDP_SERVER_STATE_NETWORK_STARTED);
            }
        }
        break;

    // Wait until the network is connected
    case PVT_UDP_SERVER_STATE_NETWORK_STARTED:
        // Determine if the network has failed
        if (networkIsShuttingDown(NETWORK_USER_PVT_UDP_SERVER))
            // Failed to connect to to the network, attempt to restart the network
            pvtUdpServerStop();

        // Wait for the network to connect to the media
        else if (networkUserConnected(NETWORK_USER_PVT_UDP_SERVER))
        {
            // Delay before starting the PVT server
            if ((millis() - pvtUdpServerTimer) >= (1 * 1000))
            {
                // The network type and host provide a valid configuration
                pvtUdpServerTimer = millis();

                // Start the PVT UDP server
                if (pvtUdpServerStart())
                    pvtUdpServerSetState(PVT_UDP_SERVER_STATE_RUNNING);
            }
        }
        break;

    // Handle client connections and link failures
    case PVT_UDP_SERVER_STATE_RUNNING:
        // Determine if the network has failed
        if ((!settings.enablePvtUdpServer) || networkIsShuttingDown(NETWORK_USER_PVT_UDP_SERVER))
        {
            if ((settings.debugPvtUdpServer || PERIODIC_DISPLAY(PD_PVT_UDP_SERVER_DATA)) && (!inMainMenu))
            {
                PERIODIC_CLEAR(PD_PVT_UDP_SERVER_DATA);
                systemPrintln("PVT server initiating shutdown");
            }

            // Network connection failed, attempt to restart the network
            pvtUdpServerStop();
            break;
        }
        break;
    }

    // Periodically display the PVT state
    if (PERIODIC_DISPLAY(PD_PVT_UDP_SERVER_STATE) && (!inMainMenu))
        pvtUdpServerSetState(pvtUdpServerState);
}

// Zero the PVT server client tails
void pvtUdpServerZeroTail()
{
    pvtUdpServerTail = 0;
}

#endif // COMPILE_WIFI
