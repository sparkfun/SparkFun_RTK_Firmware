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
                               | NTRIP Client receeives correction data
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

//----------------------------------------
// PVT Server Routines - Compiled out
//----------------------------------------

#ifdef COMPILE_WIFI

bool pvtServerActive() {return false;}
int pvtServerSendData(uint16_t dataHead) {return 0;}
void pvtServerUpdate() {}

#else // COMPILE_WIFI

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

//----------------------------------------
// PVT Server Routines
//----------------------------------------

// Send data to the PVT clients
uint16_t pvtServerClientSendData(int index, uint8_t *data, uint16_t length)
{

    length = pvtServerClient[index].write(data, length);
    if (length >= 0)
    {
        if ((settings.enablePrintTcpStatus) && (!inMainMenu))
            systemPrintf("%d bytes written over WiFi TCP\r\n", length);
    }
    // Failed to write the data
    else
    {
        // Done with this client connection
        if (!inMainMenu)
        {
            systemPrintf("Breaking WiFi TCP client %d connection\r\n", index);
        }

        pvtServerClient[index].stop();
        pvtServerClientConnected &= ~(1 << index);
        length = 0;
    }
    return length;
}

// Send PVT data to the clients
int pvtServerSendData(uint16_t dataHead)
{
    int usedSpace = 0;

    bool connected;
    int bytesToSend;
    int index;
    static uint16_t pvtTails[PVT_SERVER_MAX_CLIENTS]; // PVT client tails
    uint16_t tail;

    // Determine if a client is connected
    connected = settings.enableTcpServer && online.pvtServer && pvtServerClientConnected;

    // Update each of the clients
    for (index = 0; index < PVT_SERVER_MAX_CLIENTS; index++)
    {
        tail = pvtTails[index];

        // Determine the amount of TCP data in the buffer
        bytesToSend = dataHead - tail;
        if (bytesToSend < 0)
            bytesToSend += settings.gnssHandlerBufferSize;

        // Determine if the client is connected
        if ((!connected) || ((pvtServerClientConnected & (1 << index)) == 0))
            tail = dataHead;
        else
        {
            if (bytesToSend > 0)
            {
                // Reduce bytes to send if we have more to send then the end of the buffer
                // We'll wrap next loop
                if ((tail + bytesToSend) > settings.gnssHandlerBufferSize)
                    bytesToSend = settings.gnssHandlerBufferSize - tail;

                // Send the data to the TCP clients
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
        pvtTails[index] = tail;
    }

    // Shutdown the TCP server if necessary
    if (settings.enableTcpServer || online.pvtServer)
        pvtServerActive();

    // Return the amount of space that WiFi is using in the buffer
    return usedSpace;
}

// Check for TCP server active
bool pvtServerActive()
{
    if ((settings.enableTcpServer && online.pvtServer) || pvtServerClientConnected)
        return true;

    log_d("Stopping WiFi TCP Server");

    // Shutdown the TCP server
    online.pvtServer = false;

    // Stop the TCP server
    pvtServer->stop();

    if (pvtServer != nullptr)
        delete pvtServer;
    return false;
}

// Update the PVT server state
void pvtServerUpdate()
{
    int index;

    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping tcpUpdate");
        return;
    }

    if (settings.enableTcpServer == false)
        return; // Nothing to do

    if (wifiInConfigMode())
        return; // Do not service TCP during WiFi config

    // If TCP is enabled, but WiFi is not connected, start WiFi
    if (settings.enableTcpServer && (wifiIsConnected() == false))
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

    // Start the TCP server if enabled
    if (settings.enableTcpServer && (pvtServer == nullptr) && wifiIsConnected())
    {
        pvtServer = new WiFiServer(settings.wifiTcpPort);

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

                // Shutdown the TCP server if necessary
                if (settings.enableTcpServer || online.pvtServer)
                    pvtServerActive();
            }
        }
    }
}

#endif // COMPILE_WIFI
