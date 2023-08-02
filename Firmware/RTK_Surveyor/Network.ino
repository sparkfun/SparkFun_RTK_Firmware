/*------------------------------------------------------------------------------
Network.ino

 This module implements the network layer.
------------------------------------------------------------------------------*/

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
            updateEthernet();
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
        tcpUpdateEthernet(); // Turn on TCP Client as needed
    }

    // Update the network applicaiton connections
    updateEthernetNTPServer(); // Process any received NTP requests

    // Display the IP addresses
    networkPeriodicallyDisplayIpAddress();
}
