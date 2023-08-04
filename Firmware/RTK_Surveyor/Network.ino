/*------------------------------------------------------------------------------
Network.ino

 This module implements the network layer.
------------------------------------------------------------------------------*/

#if COMPILE_NETWORK

//----------------------------------------
// Constants
//----------------------------------------

// List of network names
const char * const networkName[] =
{
    "WiFi",              // NETWORK_TYPE_WIFI
    "Ethernet",          // NETWORK_TYPE_ETHERNET
    "Hardware Default",  // NETWORK_TYPE_DEFAULT
};
const int networkNameEntries = sizeof(networkName) / sizeof(networkName[0]);

//----------------------------------------
// Locals
//----------------------------------------

static uint32_t networkLastIpAddressDisplayMillis[NETWORK_TYPE_MAX];

//----------------------------------------
// Menu to get the common network settings
//----------------------------------------
void menuNetwork()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Network");
        systemPrintln();

        //------------------------------
        // Display the PVT client menu items
        //------------------------------

        // Display the menu
        systemPrintf("1) PVT Client: %s\r\n", settings.enablePvtClient ? "Enabled" : "Disabled");
        systemPrintf("2) PVT Client Host: %s\r\n", settings.pvtClientHost);
        systemPrintf("3) PVT Client Port: %ld\r\n", settings.pvtClientPort);

        //------------------------------
        // Display the PVT server menu items
        //------------------------------

        systemPrintf("4) PVT Server: %s\r\n", settings.enablePvtServer ? "Enabled" : "Disabled");

        if (settings.enablePvtServer == true || settings.enablePvtClient == true)
            systemPrintf("5) PVT Server Port: %ld\r\n", settings.pvtServerPort);

        //------------------------------
        // Display the Ethernet PVT client menu items
        //------------------------------

        systemPrintf("6) PVT Client (Ethernet): %s\r\n", settings.enableTcpClientEthernet ? "Enabled" : "Disabled");

        //------------------------------
        // Finish the menu and get the input
        //------------------------------

        systemPrintln("x) Exit");
        byte incoming = getCharacterNumber();

        //------------------------------
        // Get the PVT client parameters
        //------------------------------

        // Toggle PVT client enable
        if (incoming == 1)
            settings.enablePvtClient ^= 1;

        // Get the PVT client host
        else if ((incoming == 2) && (settings.enablePvtClient || settings.enableTcpClientEthernet))
        {
            char hostname[sizeof(settings.pvtClientHost)];

            systemPrint("Enter PVT client host name / address: ");

            // Get the host name or IP address
            memset(hostname, 0, sizeof(hostname));
            getString(hostname, sizeof(hostname) - 1);
            strcpy(settings.pvtClientHost, hostname);

            // Remove any http:// or https:// prefix from host name
            // strtok modifies string to be parsed so we create a copy
            strncpy(hostname, settings.pvtClientHost, sizeof(hostname) - 1);
            char *token = strtok(hostname, "//");
            if (token != nullptr)
            {
                token = strtok(nullptr, "//"); // Advance to data after //
                if (token != nullptr)
                    strcpy(settings.pvtClientHost, token);
            }
        }

        // Get the PVT client port number
        else if ((incoming == 3) && (settings.enablePvtClient || settings.enableTcpClientEthernet))
        {
            systemPrint("Enter the PVT client port number to use (0 to 65535): ");
            int portNumber = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((portNumber != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (portNumber != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if ((portNumber < 0) || (portNumber > 65535))
                    systemPrintln("Error: Port number out of range");
                else
                {
                    settings.pvtClientPort = portNumber; // Recorded to NVM and file at main menu exit
                }
            }
        }

        //------------------------------
        // Get the PVT server parameters
        //------------------------------

        else if (incoming == 4)
        {
            // Toggle WiFi NEMA server
            settings.enablePvtServer ^= 1;
            if ((!settings.enablePvtServer) && online.pvtServer)
            {
                // Tell the UART2 tasks that the TCP server is shutting down
                online.pvtServer = false;

                // Wait for the UART2 tasks to close the TCP client connections
                while (pvtServerActive())
                    delay(5);
                systemPrintln("TCP Server offline");
            }
        }
        else if (incoming == 5)
        {
            systemPrint("Enter the TCP port to use (0 to 65535): ");
            int portNumber = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((portNumber != INPUT_RESPONSE_GETNUMBER_EXIT) && (portNumber != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (portNumber < 0 || portNumber > 65535)
                    systemPrintln("Error: TCP Port out of range");
                else
                    settings.pvtServerPort = portNumber; // Recorded to NVM and file at main menu exit
            }
        }

        //------------------------------
        // Get the Ethernet PVT client parameters
        //------------------------------

        // Toggle PVT client (Ethernet) enable
        else if (incoming == 6)
            settings.enableTcpClientEthernet ^= 1;

        //------------------------------
        // Handle exit and invalid input
        //------------------------------

        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }
}

//----------------------------------------
// Display the IP address
//----------------------------------------
void networkDisplayIpAddress(uint8_t networkType)
{
//    if (settings.debugNetworkLayer || settings.printNetworkStatus)
    {
        systemPrintf("%s IP address: ", networkName[networkType]);
        systemPrint(networkGetIpAddress(networkType));
        if (networkType == NETWORK_TYPE_WIFI)
            systemPrintf(" RSSI: %d", wifiGetRssi());
        systemPrintln();

        // The address was just displayed
        networkLastIpAddressDisplayMillis[networkType] = millis();
    }
}

//----------------------------------------
// Get the IP address
//----------------------------------------
IPAddress networkGetIpAddress(uint8_t networkType)
{
    if (networkType == NETWORK_TYPE_ETHERNET)
        return ethernetGetIpAddress();
    if (networkType == NETWORK_TYPE_WIFI)
        return wifiGetIpAddress();
    return IPAddress((uint32_t)0);
}

//----------------------------------------
// Periodically display the IP address
//----------------------------------------
void networkPeriodicallyDisplayIpAddress()
{
    if (PERIODIC_DISPLAY(PD_ETHERNET_IP_ADDRESS))
    {
        PERIODIC_CLEAR(PD_ETHERNET_IP_ADDRESS);
        networkDisplayIpAddress(NETWORK_TYPE_ETHERNET);
    }
    if (PERIODIC_DISPLAY(PD_WIFI_IP_ADDRESS))
    {
        PERIODIC_CLEAR(PD_WIFI_IP_ADDRESS);
        networkDisplayIpAddress(NETWORK_TYPE_WIFI);
    }
}

//----------------------------------------
// Print the name associated with a network type
//----------------------------------------
void networkPrintName(uint8_t networkType)
{
    if (networkType > NETWORK_TYPE_USE_DEFAULT)
        systemPrint("Unknown");
    else if (HAS_ETHERNET)
        systemPrint(networkName[networkType]);
    else
        systemPrint(networkName[NETWORK_TYPE_WIFI]);
}

//----------------------------------------
// Update the network device state
//----------------------------------------
void networkTypeUpdate(uint8_t networkType)
{
    // Update the physical network connections
    switch (networkType)
    {
        case NETWORK_TYPE_WIFI:
            wifiUpdate();
            break;

        case NETWORK_TYPE_ETHERNET:
            ethernetUpdate();
            break;
    }
}

//----------------------------------------
// Maintain the network connections
//----------------------------------------
void networkUpdate()
{
    uint8_t networkType;

    // Update the network layer
    for (networkType = 0; networkType < NETWORK_TYPE_MAX; networkType++)
        networkTypeUpdate(networkType);

    // Skip updates if in configure-via-ethernet mode
    if (!configureViaEthernet)
    {
        ntripClientUpdate(); // Check the NTRIP client connection and move data NTRIP --> ZED
        ntripServerUpdate(); // Check the NTRIP server connection and move data ZED --> NTRIP
        pvtClientUpdate();   // Turn on the PVT Client as needed
        pvtServerUpdate();   // Turn on the PVT Server as needed
        ethernetPvtClientUpdate(); // Turn on TCP Client as needed
    }

    // Update the network applicaiton connections
    ethernetNTPServerUpdate(); // Process any received NTP requests

    // Display the IP addresses
    networkPeriodicallyDisplayIpAddress();
}

#endif  // COMPILE_NETWORK
