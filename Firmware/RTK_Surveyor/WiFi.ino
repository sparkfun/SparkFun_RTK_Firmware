/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  WiFi Status Values:
    WL_CONNECTED: assigned when connected to a WiFi network
    WL_CONNECTION_LOST: assigned when the connection is lost
    WL_CONNECT_FAILED: assigned when the connection fails for all the attempts
    WL_DISCONNECTED: assigned when disconnected from a network
    WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and
                    remains active until the number of attempts expires (resulting in
                    WL_CONNECT_FAILED) or a connection is established (resulting in
                    WL_CONNECTED)
    WL_NO_SHIELD: assigned when no WiFi shield is present
    WL_NO_SSID_AVAIL: assigned when no SSID are available
    WL_SCAN_COMPLETED: assigned when the scan networks is completed

  WiFi Station States:

                                  WIFI_OFF<--------------------.
                                    |                          |
                       wifiStart()  |                          |
                                    |                          | WL_CONNECT_FAILED (Bad password)
                                    |                          | WL_NO_SSID_AVAIL (Out of range)
                                    v                 Fail     |
                                  WIFI_CONNECTING------------->+
                                    |    ^                     ^
                     wifiConnect()  |    |                     | wifiStop()
                                    |    | WL_CONNECTION_LOST  |
                                    |    | WL_DISCONNECTED     |
                                    v    |                     |
                                  WIFI_CONNECTED --------------'
  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Globals
//----------------------------------------

int wifiConnectionAttempts; // Count the number of connection attempts between restarts

#ifdef COMPILE_WIFI

//----------------------------------------
// Constants
//----------------------------------------


//----------------------------------------
// Locals
//----------------------------------------

static uint32_t wifiLastConnectionAttempt;

// Throttle the time between connection attempts
// ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
static uint32_t wifiConnectionAttemptsTotal; // Count the number of connection attempts absolutely
static uint32_t wifiConnectionAttemptTimeout;

// WiFi Timer usage:
//  * Measure interval to display IP address
static unsigned long wifiDisplayTimer;

// Last time the WiFi state was displayed
static uint32_t lastWifiState;

//----------------------------------------
// WiFi Routines
//----------------------------------------

void wifiDisplayIpAddress()
{
    systemPrint("WiFi IP address: ");
    systemPrint(WiFi.localIP());
    systemPrintf(" RSSI: %d\r\n", WiFi.RSSI());

    wifiDisplayTimer = millis();
}

byte wifiGetStatus()
{
    return WiFi.status();
}

// Update the state of the WiFi state machine
void wifiSetState(byte newState)
{
    if ((settings.enablePrintWifiState || PERIODIC_DISPLAY(PD_WIFI_STATE))
        && (wifiState == newState))
        systemPrint("*");
    wifiState = newState;

    if (settings.enablePrintWifiState || PERIODIC_DISPLAY(PD_WIFI_STATE))
    {
        PERIODIC_CLEAR(PD_WIFI_STATE);
        switch (newState)
        {
        default:
            systemPrintf("Unknown WiFi state: %d\r\n", newState);
            break;
        case WIFI_OFF:
            systemPrintln("WIFI_OFF");
            break;
        case WIFI_START:
            systemPrintln("WIFI_START");
            break;
        case WIFI_CONNECTING:
            systemPrintln("WIFI_CONNECTING");
            break;
        case WIFI_CONNECTED:
            systemPrintln("WIFI_CONNECTED");
            break;
        }
    }
}

//----------------------------------------
// WiFi Config Support Routines - compiled out
//----------------------------------------

// Start the access point for user to connect to and configure device
// We can also start as a WiFi station and attempt to connect to local WiFi for config
bool wifiStartAP()
{
    if (settings.wifiConfigOverAP == true)
    {
        // Stop any current WiFi activity
        wifiStop();

        // Start in AP mode
        WiFi.mode(WIFI_AP);

        // Before starting AP mode, be sure we have default WiFi protocols enabled.
        // esp_wifi_set_protocol requires WiFi to be started
        esp_err_t response =
            esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
        if (response != ESP_OK)
            systemPrintf("wifiStartAP: Error setting WiFi protocols: %s\r\n", esp_err_to_name(response));
        else
            log_d("WiFi protocols set");

        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        WiFi.softAPConfig(local_IP, gateway, subnet);
        if (WiFi.softAP("RTK Config") == false) // Must be short enough to fit OLED Width
        {
            systemPrintln("WiFi AP failed to start");
            return (false);
        }
        systemPrint("WiFi AP Started with IP: ");
        systemPrintln(WiFi.softAPIP());
    }
    else
    {
        // Start webserver on local WiFi instead of AP

        // Attempt to connect to local WiFi with increasing timeouts
        int timeout = 0;
        int x = 0;
        const int maxTries = 2;
        for (; x < maxTries; x++)
        {
            timeout += 5000;
            if (wifiConnect(timeout) == true) // Attempt to connect to any SSID on settings list
            {
                wifiPrintNetworkInfo();
                break;
            }
        }
        if (x == maxTries)
        {
            displayNoWiFi(2000);
            if (productVariant == REFERENCE_STATION)
                requestChangeState(STATE_BASE_NOT_STARTED); // If WiFi failed, return to Base mode.
            else
                requestChangeState(STATE_ROVER_NOT_STARTED); // If WiFi failed, return to Rover mode.
            return (false);
        }
    }

    return (true);
}

//----------------------------------------
// Global WiFi Routines
//----------------------------------------

// Advance the WiFi state from off to connected
// Throttle connection attempts as needed
void wifiUpdate()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping wifiUpdate");
        return;
    }

    // Periodically display the WiFi state
    if (settings.enablePrintWifiState && ((millis() - lastWifiState) > 15000))
    {
        wifiSetState(wifiState);
        lastWifiState = millis();
    }

    switch (wifiState)
    {
    default:
        systemPrintf("Unknown wifiState: %d\r\n", wifiState);
        break;

    case WIFI_OFF:
        // Any service that needs WiFi will call wifiStart()
        break;

    case WIFI_CONNECTING:
        // Pause until connection timeout has passed
        if (millis() - wifiLastConnectionAttempt > wifiConnectionAttemptTimeout)
        {
            wifiLastConnectionAttempt = millis();

            if (wifiConnect(10000) == true) // Attempt to connect to any SSID on settings list
            {
                if (espnowState > ESPNOW_OFF)
                    espnowStart();

                wifiSetState(WIFI_CONNECTED);
            }
            else
            {
                // We failed to connect
                if (wifiConnectLimitReached() == false) // Increases wifiConnectionAttemptTimeout
                {
                    if (wifiConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("Next WiFi attempt in %d seconds.\r\n", wifiConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("Next WiFi attempt in %d minutes.\r\n", wifiConnectionAttemptTimeout / 1000 / 60);
                }
                else
                {
                    systemPrintln("WiFi connection failed. Giving up.");
                    displayNoWiFi(2000);
                    wifiStop(); // Move back to WIFI_OFF
                }
            }
        }

        break;

    case WIFI_CONNECTED:
        // Verify link is still up
        if (wifiIsConnected() == false)
        {
            systemPrintln("WiFi link lost");
            wifiConnectionAttempts = 0; // Reset the timeout
            wifiSetState(WIFI_CONNECTING);
        }

        // If WiFi is connected, and no services require WiFi, shut it off
        else if (wifiIsNeeded() == false)
            wifiStop();
        break;
    }
}

// Starts the WiFi connection state machine (moves from WIFI_OFF to WIFI_CONNECTING)
// Sets the appropriate protocols (WiFi + ESP-Now)
// If radio is off entirely, start WiFi
// If ESP-Now is active, only add the LR protocol
void wifiStart()
{
    if (wifiNetworkCount() == 0)
    {
        systemPrintln("Error: Please enter at least one SSID before using WiFi");
        // paintNoWiFi(2000); //TODO
        wifiStop();
        return;
    }

    if (wifiIsConnected() == true)
        return; // We don't need to do anything

    if (wifiState > WIFI_OFF)
        return; // We're in the midst of connecting

    log_d("Starting WiFi");

    wifiSetState(WIFI_CONNECTING); // This starts the state machine running

    // Display the heap state
    reportHeapNow();
}

// Stop WiFi and release all resources
// If ESP NOW is active, leave WiFi on enough for ESP NOW
void wifiStop()
{
    stopWebServer();

    // Shutdown the PVT client
    if (online.pvtClient)
    {
        // Tell the UART2 tasks that the TCP client is shutting down
        online.pvtClient = false;
        delay(5);
        systemPrintln("PVT client offline");
    }

    // Shutdown the TCP server connection
    if (online.pvtServer)
    {
        // Tell the UART2 tasks that the TCP server is shutting down
        online.pvtServer = false;

        // Wait for the UART2 tasks to close the TCP client connections
        while (pvtServerActive())
            delay(5);
        systemPrintln("TCP Server offline");
    }

    if (settings.mdnsEnable == true)
        MDNS.end();

    wifiSetState(WIFI_OFF);

    wifiConnectionAttempts = 0; // Reset the timeout

    // If ESP-Now is active, change protocol to only Long Range and re-start WiFi
    if (espnowState > ESPNOW_OFF)
    {
        if (WiFi.getMode() == WIFI_OFF)
            WiFi.mode(WIFI_STA);

        // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
        // esp_wifi_set_protocol requires WiFi to be started
        esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
        if (response != ESP_OK)
            systemPrintf("wifiStop: Error setting ESP-Now lone protocol: %s\r\n", esp_err_to_name(response));
        else
            log_d("WiFi disabled, ESP-Now left in place");
    }
    else
    {
        WiFi.mode(WIFI_OFF);
        log_d("WiFi Stopped");
    }

    // Display the heap state
    reportHeapNow();
}

bool wifiIsConnected()
{
    return (wifiGetStatus() == WL_CONNECTED);
}

// Attempts a connection to all provided SSIDs
// Returns true if successful
// Gives up if no SSID detected or connection times out
bool wifiConnect(unsigned long timeout)
{
    if (wifiIsConnected())
        return (true); // Nothing to do

    displayWiFiConnect();

    // Before we can issue esp_wifi_() commands WiFi must be started
    if (WiFi.getMode() == WIFI_OFF)
        WiFi.mode(WIFI_STA);

    // Verify that the necessary protocols are set
    uint8_t protocols = 0;
    esp_err_t response = esp_wifi_get_protocol(WIFI_IF_STA, &protocols);
    if (response != ESP_OK)
        systemPrintf("wifiConnect: Failed to get protocols: %s\r\n", esp_err_to_name(response));

    // If ESP-NOW is running, blend in ESP-NOW protocol.
    if (espnowState > ESPNOW_OFF)
    {
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR))
        {
            esp_err_t response =
                esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N |
                                                       WIFI_PROTOCOL_LR); // Enable WiFi + ESP-Now.
            if (response != ESP_OK)
                systemPrintf("wifiConnect: Error setting WiFi + ESP-NOW protocols: %s\r\n", esp_err_to_name(response));
        }
    }
    else
    {
        // Make sure default WiFi protocols are in place
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N))
        {
            esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                                                                        WIFI_PROTOCOL_11N); // Enable WiFi.
            if (response != ESP_OK)
                systemPrintf("wifiConnect: Error setting WiFi + ESP-NOW protocols: %s\r\n", esp_err_to_name(response));
        }
    }

    WiFiMulti wifiMulti;

    // Load SSIDs
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) > 0)
            wifiMulti.addAP(settings.wifiNetworks[x].ssid, settings.wifiNetworks[x].password);
    }

    systemPrint("Connecting WiFi... ");

    int wifiResponse = wifiMulti.run(timeout);
    if (wifiResponse == WL_CONNECTED)
    {
        if (settings.enableTcpClient == true || settings.enableTcpServer == true)
        {
            if (settings.mdnsEnable == true)
            {
                if (MDNS.begin("rtk") == false) // This should make the module findable from 'rtk.local' in browser
                    log_d("Error setting up MDNS responder!");
                else
                    MDNS.addService("http", "tcp", settings.wifiTcpPort); // Add service to MDNS
            }
        }

        systemPrintln();
        return true;
    }
    else if (wifiResponse == WL_DISCONNECTED)
        systemPrint("No friendly WiFi networks detected. ");
    else
        systemPrintf("WiFi failed to connect: error #%d. ", wifiResponse);

    return false;
}

// Based on the current settings and system states, determine if we need WiFi on or not
// This function does not start WiFi. Any service that needs it should call wifiStart().
// This function is used to turn WiFi off if nothing needs it.
bool wifiIsNeeded()
{
    bool needed = false;

    if (settings.enableTcpClient == true)
        needed = true;
    if (settings.enableTcpServer == true)
        needed = true;

    // Handle WiFi within systemStates
    if (systemState <= STATE_ROVER_RTK_FIX && settings.enableNtripClient == true)
        needed = true;

    if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING &&
        settings.enableNtripServer == true)
        needed = true;

    // If the user has enabled NTRIP Client for an Assisted Survey-In, and Survey-In is running, keep WiFi on.
    if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_TEMP_SURVEY_STARTED &&
        settings.enableNtripClient == true && settings.fixedBase == false)
        needed = true;

    // If WiFi is on while we are in the following states, allow WiFi to continue to operate
    if (systemState >= STATE_BUBBLE_LEVEL && systemState <= STATE_PROFILE)
    {
        // Keep WiFi on if user presses setup button, enters bubble level, is in AP config mode, etc
        needed = true;
    }

    if (systemState == STATE_KEYS_WIFI_STARTED || systemState == STATE_KEYS_WIFI_CONNECTED)
        needed = true;
    if (systemState == STATE_KEYS_PROVISION_WIFI_STARTED || systemState == STATE_KEYS_PROVISION_WIFI_CONNECTED)
        needed = true;

    return needed;
}

// Counts the number of entered SSIDs
int wifiNetworkCount()
{
    // Count SSIDs
    int networkCount = 0;
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) > 0)
            networkCount++;
    }
    return networkCount;
}

// Determine if another connection is possible or if the limit has been reached
bool wifiConnectLimitReached()
{
    // Retry the connection a few times
    bool limitReached = false;
    if (wifiConnectionAttempts++ >= wifiMaxConnectionAttempts)
        limitReached = true;

    wifiConnectionAttemptsTotal++;

    if (limitReached == false)
    {
        wifiConnectionAttemptTimeout =
            wifiConnectionAttempts * 15 * 1000L; // Wait 15, 30, 45, etc seconds between attempts

        reportHeapNow();
    }
    else
    {
        // No more connection attempts
        systemPrintln("WiFi connection attempts exceeded!");
    }
    return limitReached;
}

void wifiPrintNetworkInfo()
{
    systemPrintln("\nNetwork Configuration:");
    systemPrintln("----------------------");
    systemPrint("         SSID: ");
    systemPrintln(WiFi.SSID());
    systemPrint("  WiFi Status: ");
    systemPrintln(WiFi.status());
    systemPrint("WiFi Strength: ");
    systemPrint(WiFi.RSSI());
    systemPrintln(" dBm");
    systemPrint("          MAC: ");
    systemPrintln(WiFi.macAddress());
    systemPrint("           IP: ");
    systemPrintln(WiFi.localIP());
    systemPrint("       Subnet: ");
    systemPrintln(WiFi.subnetMask());
    systemPrint("      Gateway: ");
    systemPrintln(WiFi.gatewayIP());
    systemPrint("        DNS 1: ");
    systemPrintln(WiFi.dnsIP(0));
    systemPrint("        DNS 2: ");
    systemPrintln(WiFi.dnsIP(1));
    systemPrint("        DNS 3: ");
    systemPrintln(WiFi.dnsIP(2));
    systemPrintln();
}

// Returns true if unit is in config mode
// Used to disallow services (NTRIP, TCP, etc) from updating
bool wifiInConfigMode()
{
    if (systemState >= STATE_WIFI_CONFIG_NOT_STARTED && systemState <= STATE_WIFI_CONFIG)
        return true;
    return false;
}

IPAddress wifiGetIpAddress()
{
    return WiFi.localIP();
}

int wifiGetRssi()
{
    return WiFi.RSSI();
}

#endif // COMPILE_WIFI
