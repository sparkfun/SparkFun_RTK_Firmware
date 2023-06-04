// Ethernet

// Determine if Ethernet is needed. Saves RAM...
bool ethernetIsNeeded()
{
    // Does NTP need Ethernet?
    if (HAS_ETHERNET && systemState >= STATE_NTPSERVER_NOT_STARTED && systemState <= STATE_NTPSERVER_SYNC)
        return true;

    // Does Base mode NTRIP Server need Ethernet?
    if (HAS_ETHERNET && settings.enableNtripServer == true &&
        (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
        //&& !settings.ntripServerUseWiFiNotEthernet //For future expansion
    )
        return true;

    // Does Base mode NTRIP Server need Ethernet?
    if (HAS_ETHERNET && settings.enableNtripClient == true &&
        (systemState >= STATE_ROVER_NOT_STARTED && systemState <= STATE_ROVER_RTK_FIX)
        //&& !settings.ntripClientUseWiFiNotEthernet //For future expansion
    )
        return true;

    // Does TCP need Ethernet?
    if (HAS_ETHERNET && settings.enableTcpClientEthernet)
        return true;

    return false;
}

// Regularly called to update the Ethernet status
void beginEthernet()
{
    if (HAS_ETHERNET == false)
        return;

    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping beginEthernet");
        return;
    }

#ifdef COMPILE_ETHERNET

    if (!ethernetIsNeeded())
        return;

    switch (online.ethernetStatus)
    {
    case (ETH_NOT_STARTED):
        Ethernet.init(pin_Ethernet_CS);

        // First we start Ethernet without DHCP to detect if a cable is connected
        // DHCP causes system freeze for ~62 seconds so we avoid it until a cable is conncted
        Ethernet.begin(ethernetMACAddress, settings.ethernetIP, settings.ethernetDNS, settings.ethernetGateway,
                       settings.ethernetSubnet);

        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            log_d("Ethernet hardware not found");
            online.ethernetStatus = ETH_CAN_NOT_BEGIN;
            return;
        }

        online.ethernetStatus = ETH_STARTED_CHECK_CABLE;
        break;

    case (ETH_STARTED_CHECK_CABLE):
        if (millis() - lastEthernetCheck > 1000) // Don't check for cable but once a second
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
        Ethernet.begin(ethernetMACAddress);

        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            log_d("Ethernet hardware not found");
            online.ethernetStatus = ETH_CAN_NOT_BEGIN;
            return;
        }

        log_d("Ethernet started with DHCP");
        online.ethernetStatus = ETH_CONNECTED;
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
#endif /// COMPILE_ETHERNET
}

void beginEthernetNTPServer()
{
    if (!HAS_ETHERNET)
        return;

    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping beginNTPServer");
        return;
    }

    // Skip if not in NTPSERVER mode
    if (systemState < STATE_NTPSERVER_NOT_STARTED || systemState > STATE_NTPSERVER_SYNC)
        return;

#ifdef COMPILE_ETHERNET
    if ((online.ethernetStatus == ETH_CONNECTED) && (online.ethernetNTPServer == false))
    {
        if (ethernetNTPServer == nullptr)
            ethernetNTPServer = new derivedEthernetUDP;
        ethernetNTPServer->begin(settings.ethernetNtpPort);
        ntpSockIndex = ethernetNTPServer->getSockIndex(); // Get the socket index
        w5500ClearSocketInterrupts();                     // Clear all interrupts
        w5500EnableSocketInterrupt(ntpSockIndex);         // Enable the RECV interrupt for the desired socket index
        online.ethernetNTPServer = true;
    }
#endif  // COMPILE_ETHERNET
}

void endEthernetNTPServer()
{
}

void updateEthernet()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping updateEthernet");
        return;
    }

    if (!HAS_ETHERNET)
        return;

#ifdef COMPILE_ETHERNET

    if (online.ethernetStatus == ETH_CAN_NOT_BEGIN)
        return;

    beginEthernet(); // This updates the link status

    if (w5500CheckSocketInterrupt(ntpSockIndex))
        w5500ClearSocketInterrupt(ntpSockIndex); // Clear the socket interrupt here

    // Maintain the ethernet connection
    switch (Ethernet.maintain())
    {
    case 1:
        // renewed fail
        if (settings.enablePrintEthernetDiag && (!inMainMenu))
            systemPrintln("Ethernet: Error: renewed fail");
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
#endif  // COMPILE_ETHERNET
}

void updateEthernetNTPServer()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping updateEthernetNTPServer");
        return;
    }

    if (!HAS_ETHERNET)
        return;

#ifdef COMPILE_ETHERNET

    if (online.ethernetNTPServer == false)
        beginEthernetNTPServer();

    if (online.ethernetNTPServer == false)
        return;

    char ntpDiag[512]; // Char array to hold diagnostic messages

    // Check for new NTP requests - if the time has been sync'd
    bool processed = processOneNTPRequest(systemState == STATE_NTPSERVER_SYNC, (const timeval *)&ethernetNtpTv,
                                          (const timeval *)&gnssSyncTv, ntpDiag, sizeof(ntpDiag));

    if (processed)
    {
        // Print the diagnostics - if enabled
        if (settings.enablePrintNTPDiag && (!inMainMenu))
            systemPrint(ntpDiag);

        // Log the NTP request to file - if enabled
        if (settings.enableNTPFile)
        {
            // Gain access to the SPI controller for the microSD card
            if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
            {
                markSemaphore(FUNCTION_NTPEVENT);

                // Get the marks file name
                char fileName[32];
                bool fileOpen = false;
                bool sdCardWasOnline;
                int year;
                int month;
                int day;

                // Get the date
                year = rtc.getYear();
                month = rtc.getMonth() + 1;
                day = rtc.getDay();

                // Build the file name
                snprintf(fileName, sizeof(fileName), "/NTP_Requests_%04d_%02d_%02d.txt", year, month, day);

                // Try to gain access the SD card
                sdCardWasOnline = online.microSD;
                if (online.microSD != true)
                    beginSD();

                if (online.microSD == true)
                {
                    // Check if the NTP file already exists
                    bool ntpFileExists = false;
                    if (USE_SPI_MICROSD)
                    {
                        ntpFileExists = sd->exists(fileName);
                    }
#ifdef COMPILE_SD_MMC
                    else
                    {
                        ntpFileExists = SD_MMC.exists(fileName);
                    }
#endif  // COMPILE_SD_MMC

                    // Open the NTP file
                    FileSdFatMMC ntpFile;

                    if (ntpFileExists)
                    {
                        if (ntpFile && ntpFile.open(fileName, O_APPEND | O_WRITE))
                        {
                            fileOpen = true;
                            ntpFile.updateFileCreateTimestamp();
                        }
                    }
                    else
                    {
                        if (ntpFile && ntpFile.open(fileName, O_CREAT | O_WRITE))
                        {
                            fileOpen = true;
                            ntpFile.updateFileAccessTimestamp();

                            // If you want to add a file header, do it here
                        }
                    }

                    if (fileOpen)
                    {
                        // Write the NTP request to the file
                        ntpFile.write((const uint8_t *)ntpDiag, strlen(ntpDiag));

                        // Update the file to create time & date
                        ntpFile.updateFileCreateTimestamp();

                        // Close the mark file
                        ntpFile.close();
                    }

                    // Dismount the SD card
                    if (!sdCardWasOnline)
                        endSD(true, false);
                }

                // Done with the SPI controller
                xSemaphoreGive(sdCardSemaphore);

                lastLoggedNTPRequest = millis();
                ntpLogIncreasing = true;
            } // End sdCardSemaphore
        }
    }

    if (millis() > (lastLoggedNTPRequest + 5000))
        ntpLogIncreasing = false;

#endif  // COMPILE_ETHERNET
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Send data to the Ethernet TCP server as client
void ethernetSendTcpData(uint8_t *data, uint16_t length)
{
#ifdef COMPILE_ETHERNET

    if (!length) // Nothing to write
        return;

    if (online.tcpClientEthernet && ethernetTcpConnected)
    {
        if (ethernetTcpClient->write(data, length) == length)
        {
            if (settings.enablePrintTcpStatus && (!inMainMenu))
                systemPrintf("%d bytes written over Ethernet TCP\r\n", length);
        }
        // Failed to write the data
        else
        {
            if (!inMainMenu)
            {
                systemPrintln("Breaking Ethernet TCP client connection to host");
            }

            // Done with this client connection
            ethernetTcpClient->stop();
            ethernetTcpConnected = false;
        }
    }

#endif // COMPILE_ETHERNET
}

void tcpUpdateEthernet()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping tcpUpdateEthernet");
        return;
    }

    if (!HAS_ETHERNET)
        return;

#ifdef COMPILE_ETHERNET
    // Start the TCP client if enabled
    if (settings.enableTcpClientEthernet && (!online.tcpClientEthernet) && (online.ethernetStatus == ETH_CONNECTED))
    {
        if (ethernetTcpClient == nullptr)
            ethernetTcpClient = new EthernetClient;

        online.tcpClientEthernet = true;
        ethernetTcpConnected = false;
        systemPrint("Ethernet TCP client online: Local IP: ");
        systemPrintln(Ethernet.localIP());
    }

    static unsigned long lastTcpConnectAttempt = 0;

    if (online.tcpClientEthernet)
    {
        // Try to connect to the TCP Host
        if (((!ethernetTcpClient) || (!ethernetTcpClient->connected())) && ((millis() - lastTcpConnectAttempt) >= 1000))
        {
            lastTcpConnectAttempt = millis();

            // Remove any http:// or https:// prefix from host name
            char hostname[51];
            strncpy(hostname, settings.hostForTCPClient,
                    sizeof(hostname) - 1); // strtok modifies string to be parsed so we create a copy
            char *token = strtok(hostname, "//");
            if (token != nullptr)
            {
                token = strtok(nullptr, "//"); // Advance to data after //
                if (token != nullptr)
                    strcpy(settings.hostForTCPClient, token);
            }

            if (settings.enablePrintTcpStatus && (!inMainMenu))
            {
                systemPrint("Trying to connect Ethernet TCP client to ");
                systemPrint(settings.hostForTCPClient);
                systemPrint(" on port ");
                systemPrintln(settings.ethernetTcpPort);
            }

            if (ethernetTcpClient->connect(settings.hostForTCPClient, settings.ethernetTcpPort))
            {
                online.tcpClientEthernet = true;
                ethernetTcpConnected = true;
                if (!inMainMenu)
                {
                    systemPrint("Ethernet TCP client connected to ");
                    systemPrintln(ethernetTcpClient->remoteIP());
                }
            }
            else
            {
                // Release any allocated resources
                // if (ethernetTcpClient)
                ethernetTcpClient->stop();
                ethernetTcpConnected = false;
            }
        }
    }

    if (ethernetTcpConnected)
    {
        // Check for a broken connection
        if ((!ethernetTcpClient) || (!ethernetTcpClient->connected()))
        {
            if (!inMainMenu)
            {
                systemPrintln("TCP client disconnected from host");
            }

            // Done with this client connection
            ethernetTcpClient->stop();
            ethernetTcpConnected = false;
        }
    }

    if (ethernetTcpConnected && (online.ethernetStatus < ETH_CONNECTED))
    {
        // Check for Ethernet disconnection
        if (!inMainMenu)
        {
            systemPrintln("Ethernet is disconnected. TCP client offline");
        }
        ethernetTcpClient->stop();
        ethernetTcpConnected = false;
        online.tcpClientEthernet = false;
    }

#endif  // COMPILE_ETHERNET
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Start Ethernet WebServer ESP32 W5500 - needs exclusive access to WiFi, SPI and Interrupts
void startEthernerWebServerESP32W5500()
{
#ifdef COMPILE_ETHERNET
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
#endif  // COMPILE_ETHERNET
}

void endEthernerWebServerESP32W5500()
{
#ifdef COMPILE_ETHERNET
    ETH.end(); // This is _really_ important. It undoes the low-level changes to SPI and interrupts
#endif  // COMPILE_ETHERNET
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Ethernet (W5500) ISR
// Triggered by the falling edge of the W5500 interrupt signal - indicates the arrival of a packet
// Record the time the packet arrived

#ifdef COMPILE_ETHERNET
void ethernetISR()
{
    // Don't check or clear the interrupt here -
    // it may clash with a GNSS SPI transaction and cause a wdt timeout.
    // Do it in updateEthernet
    gettimeofday((timeval *)&ethernetNtpTv, NULL); // Record the time of the NTP interrupt
}
#endif  // COMPILE_ETHERNET

void menuEthernet()
{
#ifdef COMPILE_ETHERNET
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

        systemPrint("c) Ethernet TCP Client: ");
        systemPrintf("%s\r\n", settings.enableTcpClientEthernet ? "Enabled" : "Disabled");

        if (settings.enableTcpClientEthernet == true)
        {
            systemPrintf("p) Ethernet TCP Port: %ld\r\n", settings.ethernetTcpPort);
            systemPrint("h) Host for Ethernet TCP: ");
            systemPrintln(settings.hostForTCPClient);
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
        else if (incoming == 'c')
        {
            // Toggle TCP client
            settings.enableTcpClientEthernet ^= 1;
            restartEthernet = true;
        }
        else if ((incoming == 'p') && (settings.enableTcpClientEthernet == true))
        {
            systemPrint("Enter the TCP port to use (0 to 65535): ");
            int ethernetTcpPort = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((ethernetTcpPort != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (ethernetTcpPort != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (ethernetTcpPort < 0 || ethernetTcpPort > 65535)
                    systemPrintln("Error: TCP Port out of range");
                else
                {
                    settings.ethernetTcpPort = ethernetTcpPort; // Recorded to NVM and file at main menu exit
                    restartEthernet = true;
                }
            }
        }
        else if ((incoming == 'h') && (settings.enableTcpClientEthernet == true))
        {
            systemPrint("Enter new host name / address: ");
            getString(settings.hostForTCPClient, sizeof(settings.hostForTCPClient));
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

#endif // COMPILE_ETHERNET
}

void ethernetRestart()
{
#ifdef COMPILE_ETHERNET
    // Reset online.ethernetStatus so beginEthernet will call Ethernet.begin to use the new settings
    online.ethernetStatus = ETH_NOT_STARTED;

    // Ethernet TCP Client
    if (ethernetTcpClient)
        ethernetTcpClient->stop();
    ethernetTcpConnected = false;
    online.tcpClientEthernet = false;

    // NTP Server
    if (ethernetNTPServer)
        ethernetNTPServer->stop();
    online.ethernetNTPServer = false;

    // NTRIP?
#endif // COMPILE_ETHERNET
}
