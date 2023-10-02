#ifdef COMPILE_ETHERNET

// Get the Ethernet parameters
void menuEthernet()
{
    if (!HAS_ETHERNET)
    {
        clearBuffer(); // Empty buffer of any newline chars
        return;
    }

    bool restartEthernet = false;

    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Ethernet");
        systemPrintln();

        systemPrint("1) Ethernet Config: ");
        if (settings.ethernetDHCP)
            systemPrintln("DHCP");
        else
            systemPrintln("Fixed IP");

        if (!settings.ethernetDHCP)
        {
            systemPrint("2) Fixed IP Address: ");
            systemPrintln(settings.ethernetIP.toString().c_str());
            systemPrint("3) DNS: ");
            systemPrintln(settings.ethernetDNS.toString().c_str());
            systemPrint("4) Gateway: ");
            systemPrintln(settings.ethernetGateway.toString().c_str());
            systemPrint("5) Subnet Mask: ");
            systemPrintln(settings.ethernetSubnet.toString().c_str());
        }

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 1)
        {
            settings.ethernetDHCP ^= 1;
            restartEthernet = true;
        }
        else if ((!settings.ethernetDHCP) && (incoming == 2))
        {
            systemPrint("Enter new IP Address: ");
            char tempStr[20];
            if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
            {
                String tempString = String(tempStr);
                settings.ethernetIP.fromString(tempString);
                restartEthernet = true;
            }
            else
                systemPrint("Error: invalid IP Address");
        }
        else if ((!settings.ethernetDHCP) && (incoming == 3))
        {
            systemPrint("Enter new DNS: ");
            char tempStr[20];
            if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
            {
                String tempString = String(tempStr);
                settings.ethernetDNS.fromString(tempString);
                restartEthernet = true;
            }
            else
                systemPrint("Error: invalid DNS");
        }
        else if ((!settings.ethernetDHCP) && (incoming == 4))
        {
            systemPrint("Enter new Gateway: ");
            char tempStr[20];
            if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
            {
                String tempString = String(tempStr);
                settings.ethernetGateway.fromString(tempString);
                restartEthernet = true;
            }
            else
                systemPrint("Error: invalid Gateway");
        }
        else if ((!settings.ethernetDHCP) && (incoming == 5))
        {
            systemPrint("Enter new Subnet Mask: ");
            char tempStr[20];
            if (getIPAddress(tempStr, sizeof(tempStr)) == INPUT_RESPONSE_VALID)
            {
                String tempString = String(tempStr);
                settings.ethernetSubnet.fromString(tempString);
                restartEthernet = true;
            }
            else
                systemPrint("Error: invalid Subnet Mask");
        }
        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    clearBuffer(); // Empty buffer of any newline chars

    if (restartEthernet) // Restart Ethernet to use the new ethernet settings
    {
        ethernetRestart();
    }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Ethernet routines
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Regularly called to update the Ethernet status
void ethernetBegin()
{
    if (HAS_ETHERNET == false)
        return;

    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping ethernetBegin");
        return;
    }

    if (!ethernetIsNeeded())
        return;

    if (PERIODIC_DISPLAY(PD_ETHERNET_STATE))
    {
        PERIODIC_CLEAR(PD_ETHERNET_STATE);
        ethernetDisplayState();
        systemPrintln();
    }
    switch (online.ethernetStatus)
    {
    case (ETH_NOT_STARTED):
        Ethernet.init(pin_Ethernet_CS);

        // First we start Ethernet without DHCP to detect if a cable is connected
        // DHCP causes system freeze for ~62 seconds so we avoid it until a cable is connected
        Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS, settings.ethernetGateway,
                       settings.ethernetSubnet);

        if (Ethernet.hardwareStatus() == EthernetNoHardware) // Check that a W5n00 has been detected
        {
            log_d("Ethernet hardware not found");
            online.ethernetStatus = ETH_CAN_NOT_BEGIN;
            return;
        }

        online.ethernetStatus = ETH_STARTED_CHECK_CABLE;
        lastEthernetCheck = millis(); // Wait a full second before checking the cable

        break;

    case (ETH_STARTED_CHECK_CABLE):
        if (millis() - lastEthernetCheck > 1000) // Check for cable every second
        {
            lastEthernetCheck = millis();

            if (Ethernet.linkStatus() == LinkON)
            {
                log_d("Ethernet cable detected");

                if (settings.ethernetDHCP)
                {
                    paintGettingEthernetIP();
                    online.ethernetStatus = ETH_STARTED_START_DHCP;
                }
                else
                {
                    systemPrintln("Ethernet started with static IP");
                    online.ethernetStatus = ETH_CONNECTED;
                }
            }
            else
            {
                // log_d("No cable detected");
            }
        }
        break;

    case (ETH_STARTED_START_DHCP):
        if (millis() - lastEthernetCheck > 1000) // Try DHCP every second
        {
            lastEthernetCheck = millis();

            if (Ethernet.begin(ethernetMACAddress, 20000)) // Restart Ethernet with DHCP. Use 20s timeout
            {
              log_d("Ethernet started with DHCP");
              online.ethernetStatus = ETH_CONNECTED;
            }
        }
        break;

    case (ETH_CONNECTED):
        if (Ethernet.linkStatus() == LinkOFF)
        {
            log_d("Ethernet cable disconnected!");
            online.ethernetStatus = ETH_STARTED_CHECK_CABLE;
        }
        break;

    case (ETH_CAN_NOT_BEGIN):
        break;

    default:
        log_d("Unknown status");
        break;
    }
}

// Display the Ethernet state
void ethernetDisplayState()
{
    if (online.ethernetStatus >= ethernetStateEntries)
        systemPrint("UNKNOWN");
    else
        systemPrint(ethernetStates[online.ethernetStatus]);
}

// Return the IP address for the Ethernet controller
IPAddress ethernetGetIpAddress()
{
    return Ethernet.localIP();
}

// Determine if Ethernet is needed. Saves RAM...
bool ethernetIsNeeded()
{
    // Does NTP need Ethernet?
    if (systemState >= STATE_NTPSERVER_NOT_STARTED && systemState <= STATE_NTPSERVER_SYNC)
        return true;

    // Does Base mode NTRIP Server need Ethernet?
    if (settings.enableNtripServer == true &&
        (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
    )
        return true;

    // Does Rover mode NTRIP Client need Ethernet?
    if (settings.enableNtripClient == true &&
        (systemState >= STATE_ROVER_NOT_STARTED && systemState <= STATE_ROVER_RTK_FIX)
    )
        return true;

    // Does PVT client or server need Ethernet?
    if (settings.enablePvtClient || settings.enablePvtServer || settings.enablePvtUdpServer)
        return true;

    return false;
}

// Ethernet (W5500) ISR
// Triggered by the falling edge of the W5500 interrupt signal - indicates the arrival of a packet
// Record the time the packet arrived
void ethernetISR()
{
    // Don't check or clear the interrupt here -
    // it may clash with a GNSS SPI transaction and cause a wdt timeout.
    // Do it in updateEthernet
    gettimeofday((timeval *)&ethernetNtpTv, nullptr); // Record the time of the NTP interrupt
}

// Restart the Ethernet controller
void ethernetRestart()
{
    // Reset online.ethernetStatus so ethernetBegin will call Ethernet.begin to use the new settings
    online.ethernetStatus = ETH_NOT_STARTED;

    // NTP Server
    ntpServerStop();

    // NTRIP?
}

// Update the Ethernet state
void ethernetUpdate()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping updateEthernet");
        return;
    }

    if (!HAS_ETHERNET)
        return;

    if (online.ethernetStatus == ETH_CAN_NOT_BEGIN)
        return;

    ethernetBegin(); // This updates the link status

    // Maintain the ethernet connection
    if ((online.ethernetStatus >= ETH_STARTED_CHECK_CABLE) && (online.ethernetStatus <= ETH_CONNECTED))
        switch (Ethernet.maintain())
        {
        case 1:
            // renewed fail
            if (settings.enablePrintEthernetDiag && (!inMainMenu))
                systemPrintln("Ethernet: Error: renewed fail");
            ethernetRestart(); // Restart Ethernet
            break;

        case 2:
            // renewed success
            if (settings.enablePrintEthernetDiag && (!inMainMenu))
            {
                systemPrint("Ethernet: Renewed success. IP address: ");
                systemPrintln(Ethernet.localIP());
            }
            break;

        case 3:
            // rebind fail
            if (settings.enablePrintEthernetDiag && (!inMainMenu))
                systemPrintln("Ethernet: Error: rebind fail");
            ethernetRestart(); // Restart Ethernet
            break;

        case 4:
            // rebind success
            if (settings.enablePrintEthernetDiag && (!inMainMenu))
            {
                systemPrint("Ethernet: Rebind success. IP address: ");
                systemPrintln(Ethernet.localIP());
            }
            break;

        default:
            // nothing happened
            break;
        }
}

// Verify the Ethernet tables
void ethernetVerifyTables()
{
    // Verify the table lengths
    if (ethernetStateEntries != ETH_MAX_STATE)
        reportFatalError("Please fix ethernetStates table to match ethernetStatus_e");
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Web server routines
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Start Ethernet WebServer ESP32 W5500 - needs exclusive access to WiFi, SPI and Interrupts
void ethernetWebServerStartESP32W5500()
{
    // Configure the W5500
    // To be called before ETH.begin()
    ESP32_W5500_onEvent();

    // start the ethernet connection and the server:
    // Use DHCP dynamic IP
    // bool begin(int POCI_GPIO, int PICO_GPIO, int SCLK_GPIO, int CS_GPIO, int INT_GPIO, int SPI_CLOCK_MHZ,
    //            int SPI_HOST, uint8_t *W5500_Mac = W5500_Default_Mac, bool installIsrService = true);
    ETH.begin(pin_POCI, pin_PICO, pin_SCK, pin_Ethernet_CS, pin_Ethernet_Interrupt, 25, SPI3_HOST, ethernetMACAddress);

    if (!settings.ethernetDHCP)
        ETH.config(settings.ethernetIP, settings.ethernetGateway, settings.ethernetSubnet, settings.ethernetDNS);

    if (ETH.linkUp())
        ESP32_W5500_waitForConnect();
}

// Stop the Ethernet web server
void ethernetWebServerStopESP32W5500()
{
    ETH.end(); // This is _really_ important. It undoes the low-level changes to SPI and interrupts
}

#endif // COMPILE_ETHERNET
