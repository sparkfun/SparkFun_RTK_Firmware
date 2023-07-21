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
// Locals
//----------------------------------------

#ifdef COMPILE_WIFI
WiFiClient pvtClient;
bool pvtClientConnected;
IPAddress pvtClientIpAddress;
#endif // COMPILE_WIFI

//----------------------------------------
// PVT Client Routines
//----------------------------------------

// Send PVT data to the NMEA server
uint16_t pvtClientSendData(uint16_t dataHead)
{
    bool connected;
    int bytesToSend;
    static uint16_t tail;

#ifdef COMPILE_WIFI
    // Determine if a client is connected
    bytesToSend = 0;
    connected = settings.enableTcpClient && online.pvtClient && pvtClientConnected;

    // Determine if the client is connected
    if ((!connected) || (!pvtClientConnected))
        tail = dataHead;
    else
    {
        // Determine the amount of data in the buffer
        bytesToSend = dataHead - tail;
        if (bytesToSend < 0)
            bytesToSend += settings.gnssHandlerBufferSize;
        if (bytesToSend > 0)
        {
            // Reduce bytes to send if we have more to send then the end of the buffer
            // We'll wrap next loop
            if ((tail + bytesToSend) > settings.gnssHandlerBufferSize)
                bytesToSend = settings.gnssHandlerBufferSize - tail;

            // Send the data to the NMEA server
            bytesToSend = pvtClient.write(&ringBuffer[tail], bytesToSend);
            if (bytesToSend >= 0)
            {
                if ((settings.enablePrintTcpStatus) && (!inMainMenu))
                    systemPrintf("PVT client sent %d bytes\r\n", bytesToSend);

                // Assume all data was sent, wrap the buffer pointer
                tail += bytesToSend;
                if (tail >= settings.gnssHandlerBufferSize)
                    tail -= settings.gnssHandlerBufferSize;

                // Update space available for use in UART task
                bytesToSend = dataHead - tail;
                if (bytesToSend < 0)
                    bytesToSend += settings.gnssHandlerBufferSize;
            }

            // Failed to write the data
            else
            {
                // Done with this client connection
                if (!inMainMenu)
                {
                    systemPrint("PVT client breaking connection with ");
                    systemPrint(pvtClientIpAddress);
                    systemPrint(":");
                    systemPrintln(settings.wifiTcpPort);
                }

                pvtClient.stop();
                pvtClientConnected = false;
                bytesToSend = 0;
            }
        }
    }
#endif // COMPILE_WIFI

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

#ifdef COMPILE_WIFI
    if (settings.enableTcpClient == false)
        return; // Nothing to do

    if (wifiInConfigMode())
        return; // Do not service the PVT client during WiFi config

    // If the PVT client is enabled, but WiFi is not connected, start WiFi
    if (settings.enableTcpClient && (!wifiIsConnected()))
    {
        log_d("tpcUpdate starting WiFi");
        wifiStart();
    }

    // Start the PVT client if enabled
    if (settings.enableTcpClient && (!online.pvtClient) && wifiIsConnected())
    {
        online.pvtClient = true;
        pvtClientIpAddress = WiFi.gatewayIP();
        systemPrint("PVT client connecting to ");
        systemPrint(pvtClientIpAddress);
        systemPrint(":");
        systemPrintln(settings.wifiTcpPort);
    }

    // Connect the PVT client to the NMEA server if enabled
    if (online.pvtClient)
    {
        if (((!pvtClient) || (!pvtClient.connected())) && ((millis() - lastConnectAttempt) >= 1000))
        {
            lastConnectAttempt = millis();
            if (settings.enablePrintTcpStatus)
            {
                systemPrint("PVT client connecting to ");
                systemPrint(pvtClientIpAddress);
                systemPrint(":");
                systemPrintln(settings.wifiTcpPort);
            }
            if (pvtClient.connect(pvtClientIpAddress, settings.wifiTcpPort))
            {
                online.pvtClient = true;
                systemPrint("PVT client connected to ");
                systemPrint(pvtClientIpAddress);
                systemPrint(":");
                systemPrintln(settings.wifiTcpPort);
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
                systemPrintln(settings.wifiTcpPort);
            }

            pvtClient.stop();
            pvtClientConnected = false;
        }
    }

#endif  // COMPILE_WIFI
}
