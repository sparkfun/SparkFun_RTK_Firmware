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

*/

#ifdef COMPILE_WIFI

//----------------------------------------
// Locals
//----------------------------------------

WiFiClient pvtClient;
bool pvtClientConnected;
IPAddress pvtClientIpAddress;
static volatile uint16_t pvtClientTail;

//----------------------------------------
// PVT Client Routines
//----------------------------------------

// Send PVT data to the NMEA server
int32_t pvtClientSendData(uint16_t dataHead)
{
    bool connected;
    int32_t bytesToSend;
    int32_t bytesSent;

    // Determine if a client is connected
    bytesToSend = 0;
    connected = settings.enablePvtClient && online.pvtClient && pvtClientConnected;

    // Determine if the client is connected
    if ((!connected) || (!pvtClientConnected))
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
            bytesSent = pvtClient.write(&ringBuffer[pvtClientTail], bytesToSend);
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

                pvtClient.stop();
                pvtClientConnected = false;
                bytesToSend = 0;
            }
        }
    }

    // Return the amount of space that WiFi is using in the buffer
    return bytesToSend;
}

// Update the PVT client state
void pvtClientUpdate()
{
    static uint32_t lastConnectAttempt;

    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping pvtClientUpdate");
        return;
    }

    if (settings.enablePvtClient == false)
        return; // Nothing to do

    if (wifiInConfigMode())
        return; // Do not service the PVT client during WiFi config

    // If the PVT client is enabled, but WiFi is not connected, start WiFi
    if (settings.enablePvtClient && (!wifiIsConnected()))
    {
        log_d("tpcUpdate starting WiFi");
        wifiStart();
    }

    // Start the PVT client if enabled
    if (settings.enablePvtClient && (!online.pvtClient) && wifiIsConnected())
    {
        online.pvtClient = true;
        pvtClientIpAddress = wifiGetGatewayIpAddress();
        systemPrint("PVT client connecting to ");
        systemPrint(pvtClientIpAddress);
        systemPrint(":");
        systemPrintln(settings.pvtClientPort);
    }

    // Connect the PVT client to the NMEA server if enabled
    if (online.pvtClient)
    {
        if (((!pvtClient) || (!pvtClient.connected())) && ((millis() - lastConnectAttempt) >= 1000))
        {
            lastConnectAttempt = millis();
            if (settings.debugPvtClient)
            {
                systemPrint("PVT client connecting to ");
                systemPrint(pvtClientIpAddress);
                systemPrint(":");
                systemPrintln(settings.pvtClientPort);
            }
            if (pvtClient.connect(pvtClientIpAddress, settings.pvtClientPort))
            {
                online.pvtClient = true;
                systemPrint("PVT client connected to ");
                systemPrint(pvtClientIpAddress);
                systemPrint(":");
                systemPrintln(settings.pvtClientPort);
                pvtClientConnected = true;
            }
            else
            {
                // Release any allocated resources
                // if (pvtClient)
                pvtClient.stop();
            }
        }
    }

    // Check for a broken connection
    if (pvtClientConnected)
    {
        if ((!pvtClient) || (!pvtClient.connected()))
        {
            // Done with this client connection
            if (!inMainMenu)
            {
                systemPrintf("PVT client disconnected from ");
                systemPrint(pvtClientIpAddress);
                systemPrint(":");
                systemPrintln(settings.pvtClientPort);
            }

            pvtClient.stop();
            pvtClientConnected = false;
        }
    }
}

// Zero the PVT client tail
void pvtClientZeroTail()
{
    pvtClientTail = 0;
}

#endif  // COMPILE_WIFI
