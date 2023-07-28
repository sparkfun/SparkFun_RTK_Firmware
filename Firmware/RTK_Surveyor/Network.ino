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
    "WiFi",     // NETWORK_TYPE_WIFI
    "Ethernet", // NETWORK_TYPE_ETHERNET
    "Default",  // NETWORK_TYPE_DEFAULT
};
const int networkNameEntries = sizeof(networkName) / sizeof(networkName[0]);

//----------------------------------------
// Print the name associated with a network type
//----------------------------------------
void networkPrintName(uint8_t networkType)
{
    if (networkType > NETWORK_TYPE_USE_DEFAULT)
        systemPrint("Unknown");
    else if (HAS_ETHERNET && (networkType != NETWORK_TYPE_WIFI))
        systemPrint(networkName[NETWORK_TYPE_ETHERNET]);
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
}
