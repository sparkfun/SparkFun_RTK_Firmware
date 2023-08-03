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

        systemPrintf("1) PVT Client: %s\r\n", settings.enableTcpClientEthernet ? "Enabled" : "Disabled");
        systemPrintf("2) Host for Ethernet TCP: %s\r\n", settings.hostForTCPClient);
        systemPrintf("3) Ethernet TCP Port: %ld\r\n", settings.ethernetTcpPort);

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
            settings.enableTcpClientEthernet ^= 1;

        // Get the PVT client host
        else if ((incoming == 2) && settings.enableTcpClientEthernet)
        {
            systemPrint("Enter PVT client host name / address: ");
            getString(settings.hostForTCPClient, sizeof(settings.hostForTCPClient));
        }

        // Get the PVT client port number
        else if ((incoming == 3) && settings.enableTcpClientEthernet)
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
                    settings.ethernetTcpPort = portNumber; // Recorded to NVM and file at main menu exit
                }
            }
        }

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
