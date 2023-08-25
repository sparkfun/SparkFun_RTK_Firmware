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

//----------------------------------------
// Locals
//----------------------------------------

// PVT server
static WiFiServer *pvtServer = nullptr;

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

// Update the PVT server state
void pvtServerUpdate()
{
    int index;

    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping pvtServerUpdate");
        return;
    }

    if (settings.enablePvtServer == false)
        return; // Nothing to do

    if (wifiInConfigMode())
        return; // Do not service PVT server during WiFi config

    // If PVT server is enabled, but WiFi is not connected, start WiFi
    if (settings.enablePvtServer && (wifiIsConnected() == false))
    {
        // Verify PVT_SERVER_MAX_CLIENTS
        if ((sizeof(pvtServerClientConnected) * 8) < PVT_SERVER_MAX_CLIENTS)
        {
            systemPrintf("Please set PVT_SERVER_MAX_CLIENTS <= %d or increase size of pvtServerClientConnected\r\n",
                         sizeof(pvtServerClientConnected) * 8);
            while (true)
                ; // Freeze
        }

        log_d("PVT server starting WiFi");
        wifiStart();
    }

    // Start the PVT server if enabled
    if (settings.enablePvtServer && (pvtServer == nullptr) && wifiIsConnected())
    {
        pvtServer = new WiFiServer(settings.pvtServerPort);

        pvtServer->begin();
        online.pvtServer = true;
        systemPrint("PVT server online, IP Address ");
        systemPrintln(WiFi.localIP());
    }

    // Check for another PVT server client
    if (online.pvtServer)
    {
        for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
            if (!(pvtServerClientConnected & (1 << index)))
            {
                if ((!pvtServerClient[index]) || (!pvtServerClient[index].connected()))
                {
                    pvtServerClient[index] = pvtServer->available();
                    if (!pvtServerClient[index])
                        break;
                    pvtServerClientIpAddress[index] = pvtServerClient[index].remoteIP();
                    systemPrintf("PVT server client %d connected to ", index);
                    systemPrintln(pvtServerClientIpAddress[index]);
                    pvtServerClientConnected |= 1 << index;
                }
            }
    }

    // Walk the list of PVT server clients
    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
    {
        if (pvtServerClientConnected & (1 << index))
        {
            // Check for a broken connection
            if ((!pvtServerClient[index]) || (!pvtServerClient[index].connected()))
            {
                // Done with this client connection
                if (!inMainMenu)
                {
                    systemPrintf("PVT server client %d Disconnected from ", index);
                    systemPrintln(pvtServerClientIpAddress[index]);
                }

                pvtServerClient[index].stop();
                pvtServerClientConnected &= ~(1 << index);

                // Shutdown the PVT server if necessary
                if (settings.enablePvtServer || online.pvtServer)
                    pvtServerActive();
            }
        }
    }
}

// Zero the PVT server client tails
void pvtServerZeroTail()
{
    int index;

    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
        pvtServerClientTails[index] = 0;
}

#endif // COMPILE_WIFI
