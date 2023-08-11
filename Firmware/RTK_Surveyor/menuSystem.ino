// Display current system status
void menuSystem()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: System");

        beginI2C();
        if (online.i2c == false)
            systemPrintln("I2C: Offline - Something is causing bus problems");

        systemPrint("GNSS: ");
        if (online.gnss == true)
        {
            systemPrint("Online - ");

            printZEDInfo();

            systemPrintf("Module unique chip ID: %s\r\n", zedUniqueId);

            printCurrentConditions();
        }
        else
            systemPrintln("Offline");

        systemPrint("Display: ");
        if (online.display == true)
            systemPrintln("Online");
        else
            systemPrintln("Offline");

        if (online.accelerometer == true)
            systemPrintln("Accelerometer: Online");

        systemPrint("Fuel Gauge: ");
        if (online.battery == true)
        {
            systemPrint("Online - ");

            battLevel = lipo.getSOC();
            battVoltage = lipo.getVoltage();

            systemPrintf("Batt (%d%%) / Voltage: %0.02fV", battLevel, battVoltage);
            systemPrintln();
        }
        else
            systemPrintln("Offline");

        systemPrint("microSD: ");
        if (online.microSD == true)
            systemPrintln("Online");
        else
            systemPrintln("Offline");

        if (online.lband == true)
        {
            systemPrint("L-Band: Online - ");

            if (online.lbandCorrections == true)
                systemPrint("Keys Good");
            else
                systemPrint("No Keys");

            systemPrint(" / Corrections Received");
            if (lbandCorrectionsReceived == false)
                systemPrint(" Failed");

            systemPrintf(" / Eb/N0[dB] (>9 is good): %0.2f", lBandEBNO);

            systemPrint(" - ");

            printNEOInfo();
        }

        // Display the Bluetooth status
        bluetoothTest(false);

#ifdef COMPILE_WIFI
        systemPrint("WiFi MAC Address: ");
        systemPrintf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", wifiMACAddress[0], wifiMACAddress[1], wifiMACAddress[2],
                     wifiMACAddress[3], wifiMACAddress[4], wifiMACAddress[5]);
        if (wifiState == WIFI_CONNECTED)
            wifiDisplayIpAddress();
#endif // COMPILE_WIFI

#ifdef COMPILE_ETHERNET
        if (HAS_ETHERNET)
        {
            systemPrint("Ethernet cable: ");
            if (Ethernet.linkStatus() == LinkON)
                systemPrintln("connected");
            else
                systemPrintln("disconnected");
            systemPrint("Ethernet MAC Address: ");
            systemPrintf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", ethernetMACAddress[0], ethernetMACAddress[1],
                         ethernetMACAddress[2], ethernetMACAddress[3], ethernetMACAddress[4], ethernetMACAddress[5]);
            systemPrint("Ethernet IP Address: ");
            systemPrintln(Ethernet.localIP());
            if (!settings.ethernetDHCP)
            {
                systemPrint("Ethernet DNS: ");
                systemPrintf("%s\r\n", settings.ethernetDNS.toString());
                systemPrint("Ethernet Gateway: ");
                systemPrintf("%s\r\n", settings.ethernetGateway.toString());
                systemPrint("Ethernet Subnet Mask: ");
                systemPrintf("%s\r\n", settings.ethernetSubnet.toString());
            }
        }
#endif // COMPILE_ETHERNET

        // Display the uptime
        uint64_t uptimeMilliseconds = millis();
        uint32_t uptimeDays = 0;
        byte uptimeHours = 0;
        byte uptimeMinutes = 0;
        byte uptimeSeconds = 0;

        uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
        uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

        uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
        uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

        uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
        uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

        uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
        uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

        systemPrint("System Uptime: ");
        systemPrintf("%d %02d:%02d:%02d.%03lld (Resets: %d)\r\n", uptimeDays, uptimeHours, uptimeMinutes, uptimeSeconds,
                     uptimeMilliseconds, settings.resetCount);

        // Display NTRIP Client status and uptime
        ntripClientPrintStatus();

        // Display NTRIP Server status and uptime
        ntripServerPrintStatus();

        if (settings.enableSD == true && online.microSD == true)
        {
            systemPrintln("f) Display microSD Files");
        }

        systemPrint("e) Echo User Input: ");
        if (settings.echoUserInput == true)
            systemPrintln("On");
        else
            systemPrintln("Off");

        systemPrintln("d) Configure Debug");

        systemPrintf("z) Set time zone offset: %02d:%02d:%02d\r\n", settings.timeZoneHours, settings.timeZoneMinutes,
                     settings.timeZoneSeconds);

        if (settings.shutdownNoChargeTimeout_s == 0)
            systemPrintln("C) Shutdown if not charging: Disabled");
        else
            systemPrintf("C) Shutdown if not charging after: %d seconds\r\n", settings.shutdownNoChargeTimeout_s);

        systemPrint("~) Setup button: ");
        if (settings.disableSetupButton == true)
            systemPrintln("Disabled");
        else
            systemPrintln("Enabled");

        systemPrint("b) Set Bluetooth Mode: ");
        if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
            systemPrintln("Classic");
        else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
            systemPrintln("BLE");
        else
            systemPrintln("Off");

        systemPrintln("r) Reset all settings to default");

        // Support mode switching
        systemPrintln("B) Switch to Base mode");
        if (HAS_ETHERNET)
            systemPrintln("N) Switch to NTP Server mode");
        systemPrintln("R) Switch to Rover mode");
        systemPrintln("W) Switch to WiFi Config mode");
        systemPrintln("S) Shut down");

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 'd')
            menuDebug();
        else if (incoming == 'z')
        {
                systemPrint("Enter time zone hour offset (-23 <= offset <= 23): ");
                int value = getNumber(); // Returns EXIT, TIMEOUT, or long
                if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                {
                    if (value < -23 || value > 23)
                        systemPrintln("Error: -24 < hours < 24");
                    else
                    {
                        settings.timeZoneHours = value;

                        systemPrint("Enter time zone minute offset (-59 <= offset <= 59): ");
                        int value = getNumber(); // Returns EXIT, TIMEOUT, or long
                        if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                        {
                            if (value < -59 || value > 59)
                                systemPrintln("Error: -60 < minutes < 60");
                            else
                            {
                                settings.timeZoneMinutes = value;

                                systemPrint("Enter time zone second offset (-59 <= offset <= 59): ");
                                int value = getNumber(); // Returns EXIT, TIMEOUT, or long
                                if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                                    (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                                {
                                    if (value < -59 || value > 59)
                                        systemPrintln("Error: -60 < seconds < 60");
                                    else
                                    {
                                        settings.timeZoneSeconds = value;
                                        online.rtc = false;
                                        syncRTCInterval =
                                            1000; // Reset syncRTCInterval to 1000ms (tpISR could have set it to 59000)
                                        rtcSyncd = false;
                                        updateRTC();
                                    } // Succesful seconds
                                }
                            } // Succesful minute
                        }
                    } // Succesful hours
                }
        }
        else if (incoming == 'C')
        {
                systemPrint("Enter time in seconds to shutdown unit if not charging (0 to disable): ");
                int shutdownNoChargeTimeout_s = getNumber(); // Returns EXIT, TIMEOUT, or long
                if ((shutdownNoChargeTimeout_s != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                    (shutdownNoChargeTimeout_s != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                {
                    if (shutdownNoChargeTimeout_s < 0 ||
                        shutdownNoChargeTimeout_s > 60 * 60 * 24 * 7) // Arbitrary 7 day limit
                        systemPrintln("Error: Time out of range");
                    else
                        settings.shutdownNoChargeTimeout_s =
                            shutdownNoChargeTimeout_s; // Recorded to NVM and file at main menu exit
                }
        }
        else if (incoming == '~')
        {
                settings.disableSetupButton ^= 1;
        }
        else if (incoming == 'e')
        {
                settings.echoUserInput ^= 1;
        }
        else if (incoming == 'b')
        {
                // Restart Bluetooth
                bluetoothStop();
                if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
                    settings.bluetoothRadioType = BLUETOOTH_RADIO_BLE;
                else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
                    settings.bluetoothRadioType = BLUETOOTH_RADIO_OFF;
                else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF)
                    settings.bluetoothRadioType = BLUETOOTH_RADIO_SPP;
                bluetoothStart();
        }
        else if (incoming == 'r')
        {
                systemPrintln("\r\nResetting to factory defaults. Press 'y' to confirm:");
                byte bContinue = getCharacterNumber();
                if (bContinue == 'y')
                {
                    factoryReset(false); // We do not have the SD semaphore
                }
                else
                    systemPrintln("Reset aborted");
        }
        else if ((incoming == 'f') && (settings.enableSD == true) && (online.microSD == true))
        {
                printFileList();
        }
        // Support mode switching
        else if (incoming == 'B')
        {
                forceSystemStateUpdate = true; // Imediately go to this new state
                changeState(STATE_BASE_NOT_STARTED);
        }
        else if ((incoming == 'N') && HAS_ETHERNET)
        {
            forceSystemStateUpdate = true; // Imediately go to this new state
            changeState(STATE_NTPSERVER_NOT_STARTED);
        }
        else if (incoming == 'R')
        {
                forceSystemStateUpdate = true; // Imediately go to this new state
                changeState(STATE_ROVER_NOT_STARTED);
        }
        else if (incoming == 'W')
        {
                forceSystemStateUpdate = true; // Imediately go to this new state
                changeState(STATE_WIFI_CONFIG_NOT_STARTED);
        }
        else if (incoming == 'S')
        {
                systemPrintln("Shutting down...");
                forceDisplayUpdate = true;
                powerDown(true);
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
}

// Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Debug");

        systemPrintf("Filtered by parser: %d NMEA / %d RTCM / %d UBX\r\n", failedParserMessages_NMEA,
                     failedParserMessages_RTCM, failedParserMessages_UBX);

        systemPrint("1) u-blox I2C Debugging Output: ");
        if (settings.enableI2Cdebug == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("2) Heap Reporting: ");
        if (settings.enableHeapReport == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("3) Task Highwater Reporting: ");
        if (settings.enableTaskReports == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("4) Set SPI/SD Interface Frequency: ");
        systemPrint(settings.spiFrequency);
        systemPrintln(" MHz");

        systemPrint("5) Set SPP RX Buffer Size: ");
        systemPrintln(settings.sppRxQueueSize);

        systemPrint("6) Set SPP TX Buffer Size: ");
        systemPrintln(settings.sppTxQueueSize);

        systemPrintf("8) Display Reset Counter: %d - ", settings.resetCount);
        if (settings.enableResetDisplay == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("9) GNSS Serial Timeout: ");
        systemPrintln(settings.serialTimeoutGNSS);

        systemPrint("10) Periodically print WiFi IP Address: ");
        systemPrintf("%s\r\n", PERIODIC_DISPLAY(PD_WIFI_IP_ADDRESS) ? "Enabled" : "Disabled");

        systemPrint("11) Periodically print state: ");
        systemPrintf("%s\r\n", settings.enablePrintState ? "Enabled" : "Disabled");

        systemPrint("12) Debug WiFi state: ");
        systemPrintf("%s\r\n", settings.debugWifiState ? "Enabled" : "Disabled");

        systemPrint("13) Debug NTRIP client state: ");
        systemPrintf("%s\r\n", settings.debugNtripClientState ? "Enabled" : "Disabled");

        systemPrint("14) Debug NTRIP server state: ");
        systemPrintf("%s\r\n", settings.debugNtripServerState ? "Enabled" : "Disabled");

        systemPrint("15) Periodically print position: ");
        systemPrintf("%s\r\n", settings.enablePrintPosition ? "Enabled" : "Disabled");

        systemPrint("16) Periodically print CPU idle time: ");
        systemPrintf("%s\r\n", settings.enablePrintIdleTime ? "Enabled" : "Disabled");

        systemPrintln("17) Mirror ZED-F9x's UART1 settings to USB");

        systemPrint("18) Print battery status messages: ");
        systemPrintf("%s\r\n", settings.enablePrintBatteryMessages ? "Enabled" : "Disabled");

        systemPrint("19) Print Rover accuracy messages: ");
        systemPrintf("%s\r\n", settings.enablePrintRoverAccuracy ? "Enabled" : "Disabled");

        systemPrint("20) Print messages with bad checksums or CRCs: ");
        systemPrintf("%s\r\n", settings.enablePrintBadMessages ? "Enabled" : "Disabled");

        systemPrint("21) Print log file messages: ");
        systemPrintf("%s\r\n", settings.enablePrintLogFileMessages ? "Enabled" : "Disabled");

        systemPrint("22) Print log file status: ");
        systemPrintf("%s\r\n", settings.enablePrintLogFileStatus ? "Enabled" : "Disabled");

        systemPrint("23) Print ring buffer offsets: ");
        systemPrintf("%s\r\n", settings.enablePrintRingBufferOffsets ? "Enabled" : "Disabled");

        systemPrint("24) Debug caster --> NTRIP server GNSS messages: ");
        systemPrintf("%s\r\n", settings.debugNtripServerRtcm ? "Enabled" : "Disabled");

        systemPrint("25) Debug NTRIP client --> caster GGA messages: ");
        systemPrintf("%s\r\n", settings.debugNtripClientRtcm ? "Enabled" : "Disabled");

        systemPrint("26) Print states: ");
        systemPrintf("%s\r\n", settings.enablePrintStates ? "Enabled" : "Disabled");

        systemPrint("27) Print duplicate states: ");
        systemPrintf("%s\r\n", settings.enablePrintDuplicateStates ? "Enabled" : "Disabled");

        systemPrint("28) RTCM message checking: ");
        systemPrintf("%s\r\n", settings.enableRtcmMessageChecking ? "Enabled" : "Disabled");

        systemPrint("29) Run Logging Test: ");
        systemPrintf("%s\r\n", settings.runLogTest ? "Enabled" : "Disabled");

        systemPrintln("30) Run Bluetooth Test");

        systemPrint("31) Debug PVT client: ");
        systemPrintf("%s\r\n", settings.debugPvtClient ? "Enabled" : "Disabled");

        systemPrint("32) ESP-Now Broadcast Override: ");
        systemPrintf("%s\r\n", settings.espnowBroadcast ? "Enabled" : "Disabled");

        systemPrint("33) Print buffer overruns: ");
        systemPrintf("%s\r\n", settings.enablePrintBufferOverrun ? "Enabled" : "Disabled");

        systemPrint("34) Set UART Receive Buffer Size: ");
        systemPrintln(settings.uartReceiveBufferSize);

        systemPrint("35) Set GNSS Handler Buffer Size: ");
        systemPrintln(settings.gnssHandlerBufferSize);

        systemPrint("36) Print SD and UART buffer sizes: ");
        systemPrintf("%s\r\n", settings.enablePrintSDBuffers ? "Enabled" : "Disabled");

        systemPrint("37) Print RTC resyncs: ");
        systemPrintf("%s\r\n", settings.enablePrintRtcSync ? "Enabled" : "Disabled");

        systemPrint("38) Debug NTP: ");
        systemPrintf("%s\r\n", settings.debugNtp ? "Enabled" : "Disabled");

        systemPrint("39) Print Ethernet diagnostics: ");
        systemPrintf("%s\r\n", settings.enablePrintEthernetDiag ? "Enabled" : "Disabled");

        systemPrint("40) Set L-Band RTK Fix Timeout (seconds): ");
        if (settings.lbandFixTimeout_seconds > 0)
            systemPrintln(settings.lbandFixTimeout_seconds);
        else
            systemPrintln("Disabled - no resets");

        systemPrint("41) Set BT Read Task Priority: ");
        systemPrintln(settings.btReadTaskPriority);

        systemPrint("42) Set GNSS Read Task Priority: ");
        systemPrintln(settings.gnssReadTaskPriority);

        systemPrint("43) Set GNSS Data Handler Task Priority: ");
        systemPrintln(settings.handleGnssDataTaskPriority);

        systemPrint("44) Set BT Read Task Core: ");
        systemPrintln(settings.btReadTaskCore);

        systemPrint("45) Set GNSS Read Task Core: ");
        systemPrintln(settings.gnssReadTaskCore);

        systemPrint("46) Set GNSS Data Handler Core: ");
        systemPrintln(settings.handleGnssDataTaskCore);

        systemPrint("47) Set Serial GNSS RX Full Threshold: ");
        systemPrintln(settings.serialGNSSRxFullThreshold);

        systemPrint("48) Set Core used for GNSS UART Interrupts: ");
        systemPrintln(settings.gnssUartInterruptsCore);

        systemPrint("49) Set Core used for Bluetooth Interrupts: ");
        systemPrintln(settings.bluetoothInterruptsCore);

        systemPrint("50) Set Core used for I2C Interrupts: ");
        systemPrintln(settings.i2cInterruptsCore);

        systemPrintf("51) Periodic print interval (seconds): %d\r\n", settings.periodicDisplayInterval / 1000);

        systemPrintf("52) Periodic print: %d (0x%08x)\r\n", settings.periodicDisplay, settings.periodicDisplay);

        systemPrint("53) Periodically print Bluetooth RX: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_BLUETOOTH_DATA_RX) ? "Enabled" : "Disabled");

        systemPrint("54) Periodically print Bluetooth TX: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_BLUETOOTH_DATA_TX) ? "Enabled" : "Disabled");

        systemPrint("55) Periodically print Ethernet IP address: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_ETHERNET_IP_ADDRESS) ? "Enabled" : "Disabled");

        systemPrint("56) Periodically print Ethernet state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_ETHERNET_STATE) ? "Enabled" : "Disabled");

        systemPrint("57) Periodically print network state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NETWORK_STATE) ? "Enabled" : "Disabled");

        systemPrint("58) Periodically print NTRIP client data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTRIP_CLIENT_DATA) ? "Enabled" : "Disabled");

        systemPrint("59) Periodically print NTRIP client state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTRIP_CLIENT_STATE) ? "Enabled" : "Disabled");

        systemPrint("60) Periodically print NTRIP server data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTRIP_SERVER_DATA) ? "Enabled" : "Disabled");

        systemPrint("61) Periodically print NTRIP server state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTRIP_SERVER_STATE) ? "Enabled" : "Disabled");

        systemPrint("62) Periodically print PVT client data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_PVT_CLIENT_DATA) ? "Enabled" : "Disabled");

        systemPrint("63) Periodically print PVT client state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_PVT_CLIENT_STATE) ? "Enabled" : "Disabled");

        systemPrint("64) Periodically print PVT server data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_PVT_SERVER_DATA) ? "Enabled" : "Disabled");

        systemPrint("65) Periodically print PVT server state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_PVT_SERVER_STATE) ? "Enabled" : "Disabled");

        systemPrint("66) Periodically print PVT server client data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_PVT_SERVER_CLIENT_DATA) ? "Enabled" : "Disabled");

        systemPrint("67) Periodically print SD log write data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_SD_LOG_WRITE) ? "Enabled" : "Disabled");

        systemPrint("68) Periodically print WiFi state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_WIFI_STATE) ? "Enabled" : "Disabled");

        systemPrint("69) Periodically print ZED RX data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_ZED_DATA_RX) ? "Enabled" : "Disabled");

        systemPrint("70) Periodically print ZED TX data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_ZED_DATA_TX) ? "Enabled" : "Disabled");

        systemPrint("71) Periodically print NTRIP client GGA writes: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTRIP_CLIENT_GGA) ? "Enabled" : "Disabled");

        systemPrint("72) Periodically print ring buffer consumer times: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_RING_BUFFER_MILLIS) ? "Enabled" : "Disabled");

        systemPrint("73) Debug PVT server: ");
        systemPrintf("%s\r\n", settings.debugPvtServer ? "Enabled" : "Disabled");

        systemPrint("74) Debug network layer: ");
        systemPrintf("%s\r\n", settings.debugNetworkLayer ? "Enabled" : "Disabled");

        systemPrint("75) Print network layer status: ");
        systemPrintf("%s\r\n", settings.printNetworkStatus ? "Enabled" : "Disabled");

        systemPrint("76) Periodically print NTP server data: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTP_SERVER_DATA) ? "Enabled" : "Disabled");

        systemPrint("77) Periodically print NTP server state: ");
        systemPrintf("%s\r\n", PERIODIC_SETTING(PD_NTP_SERVER_STATE) ? "Enabled" : "Disabled");

        systemPrintln("t) Enter Test Screen");

        systemPrintln("e) Erase LittleFS");

        systemPrintln("r) Force system reset");

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 1)
        {
            settings.enableI2Cdebug ^= 1;

            if (settings.enableI2Cdebug)
            {
#if defined(REF_STN_GNSS_DEBUG)
                if (ENABLE_DEVELOPER && productVariant == REFERENCE_STATION)
                    theGNSS.enableDebugging(serialGNSS); // Output all debug messages over serialGNSS
                else
#endif                                                     // REF_STN_GNSS_DEBUG
                    theGNSS.enableDebugging(Serial, true); // Enable only the critical debug messages over Serial
            }
            else
                theGNSS.disableDebugging();
        }
        else if (incoming == 2)
        {
            settings.enableHeapReport ^= 1;
        }
        else if (incoming == 3)
        {
            settings.enableTaskReports ^= 1;
        }
        else if (incoming == 4)
        {
            systemPrint("Enter SPI frequency in MHz (1 to 16): ");
            int freq = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((freq != INPUT_RESPONSE_GETNUMBER_EXIT) && (freq != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (freq < 1 || freq > 16) // Arbitrary 16MHz limit
                    systemPrintln("Error: SPI frequency out of range");
                else
                    settings.spiFrequency = freq; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 5)
        {
            systemPrint("Enter SPP RX Queue Size in Bytes (32 to 16384): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 16384) // Arbitrary 16k limit
                    systemPrintln("Error: Queue size out of range");
                else
                    settings.sppRxQueueSize = queSize; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 6)
        {
            systemPrint("Enter SPP TX Queue Size in Bytes (32 to 16384): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 16384) // Arbitrary 16k limit
                    systemPrintln("Error: Queue size out of range");
                else
                    settings.sppTxQueueSize = queSize; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 8)
        {
            settings.enableResetDisplay ^= 1;
            if (settings.enableResetDisplay == true)
            {
                settings.resetCount = 0;
                recordSystemSettings(); // Record to NVM
            }
        }
        else if (incoming == 9)
        {
            systemPrint("Enter GNSS Serial Timeout in milliseconds (0 to 1000): ");
            int serialTimeoutGNSS = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (serialTimeoutGNSS < 0 || serialTimeoutGNSS > 1000) // Arbitrary 1s limit
                    systemPrintln("Error: Timeout is out of range");
                else
                    settings.serialTimeoutGNSS = serialTimeoutGNSS; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 10)
        {
            PERIODIC_TOGGLE(PD_WIFI_IP_ADDRESS);
        }
        else if (incoming == 11)
        {
            settings.enablePrintState ^= 1;
        }
        else if (incoming == 12)
        {
            settings.debugWifiState ^= 1;
        }
        else if (incoming == 13)
        {
            settings.debugNtripClientState ^= 1;
        }
        else if (incoming == 14)
        {
            settings.debugNtripServerState ^= 1;
        }
        else if (incoming == 15)
        {
            settings.enablePrintPosition ^= 1;
        }
        else if (incoming == 16)
        {
            settings.enablePrintIdleTime ^= 1;
        }
        else if (incoming == 17)
        {
            bool response = setMessagesUSB(MAX_SET_MESSAGES_RETRIES);

            if (response == false)
                systemPrintln(F("Failed to enable USB messages"));
            else
                systemPrintln(F("USB messages successfully enabled"));
        }
        else if (incoming == 18)
        {
            settings.enablePrintBatteryMessages ^= 1;
        }
        else if (incoming == 19)
        {
            settings.enablePrintRoverAccuracy ^= 1;
        }
        else if (incoming == 20)
        {
            settings.enablePrintBadMessages ^= 1;
        }
        else if (incoming == 21)
        {
            settings.enablePrintLogFileMessages ^= 1;
        }
        else if (incoming == 22)
        {
            settings.enablePrintLogFileStatus ^= 1;
        }
        else if (incoming == 23)
        {
            settings.enablePrintRingBufferOffsets ^= 1;
        }
        else if (incoming == 24)
        {
            settings.debugNtripServerRtcm ^= 1;
        }
        else if (incoming == 25)
        {
            settings.debugNtripClientRtcm ^= 1;
        }
        else if (incoming == 26)
        {
            settings.enablePrintStates ^= 1;
        }
        else if (incoming == 27)
        {
            settings.enablePrintDuplicateStates ^= 1;
        }
        else if (incoming == 28)
        {
            settings.enableRtcmMessageChecking ^= 1;
        }
        else if (incoming == 29)
        {
            settings.runLogTest ^= 1;

            logTestState = LOGTEST_START; // Start test

            // Mark current log file as complete to force test start
            startCurrentLogTime_minutes = systemTime_minutes - settings.maxLogLength_minutes;
        }
        else if (incoming == 30)
        {
            bluetoothTest(true);
        }
        else if (incoming == 31)
        {
            settings.debugPvtClient ^= 1;
        }
        else if (incoming == 32)
        {
            settings.espnowBroadcast ^= 1;
        }
        else if (incoming == 33)
        {
            settings.enablePrintBufferOverrun ^= 1;
        }
        else if (incoming == 34)
        {
            systemPrintln("Warning: changing the Receive Buffer Size will restart the RTK. Enter 0 to abort");
            systemPrint("Enter UART Receive Buffer Size in Bytes (32 to 16384): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 16384) // Arbitrary 16k limit
                    systemPrintln("Error: Queue size out of range");
                else
                {
                    settings.uartReceiveBufferSize = queSize; // Recorded to NVM and file
                    recordSystemSettings();
                    ESP.restart();
                }
            }
        }
        else if (incoming == 35)
        {
            systemPrintln("Warning: changing the Handler Buffer Size will restart the RTK. Enter 0 to abort");
            systemPrint("Enter GNSS Handler Buffer Size in Bytes (32 to 65535): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 65535) // Arbitrary 64k limit
                    systemPrintln("Error: Queue size out of range");
                else
                {
                    // Stop the UART2 tssks to prevent the system from crashing
                    tasksStopUART2();

                    // Update the buffer size
                    settings.gnssHandlerBufferSize = queSize; // Recorded to NVM and file
                    recordSystemSettings();

                    // Reboot the system
                    ESP.restart();
                }
            }
        }
        else if (incoming == 36)
        {
            settings.enablePrintSDBuffers ^= 1;
        }
        else if (incoming == 37)
        {
            settings.enablePrintRtcSync ^= 1;
        }
        else if (incoming == 38)
        {
            settings.debugNtp ^= 1;
        }
        else if (incoming == 39)
        {
            settings.enablePrintEthernetDiag ^= 1;
        }
        else if (incoming == 40)
        {
            systemPrint("Enter number of seconds in RTK float before hot-start (0-disable to 3600): ");
            int timeout = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((timeout != INPUT_RESPONSE_GETNUMBER_EXIT) && (timeout != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (timeout < 0 || timeout > 3600) // Arbitrary 60 minute limit
                    systemPrintln("Error: Timeout out of range");
                else
                    settings.lbandFixTimeout_seconds = timeout; // Recorded to NVM and file at main menu exit
            }
        }

        else if (incoming == 41)
        {
            systemPrint("Enter BT Read Task Priority (0 to 3): ");
            int btReadTaskPriority = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((btReadTaskPriority != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (btReadTaskPriority != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (btReadTaskPriority < 0 || btReadTaskPriority > 3)
                    systemPrintln("Error: Task priority out of range");
                else
                {
                    settings.btReadTaskPriority = btReadTaskPriority; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 42)
        {
            systemPrint("Enter GNSS Read Task Priority (0 to 3): ");
            int gnssReadTaskPriority = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((gnssReadTaskPriority != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (gnssReadTaskPriority != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (gnssReadTaskPriority < 0 || gnssReadTaskPriority > 3)
                    systemPrintln("Error: Task priority out of range");
                else
                {
                    settings.gnssReadTaskPriority = gnssReadTaskPriority; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 43)
        {
            systemPrint("Enter GNSS Data Handle Task Priority (0 to 3): ");
            int handleGnssDataTaskPriority = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((handleGnssDataTaskPriority != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (handleGnssDataTaskPriority != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (handleGnssDataTaskPriority < 0 || handleGnssDataTaskPriority > 3)
                    systemPrintln("Error: Task priority out of range");
                else
                {
                    settings.handleGnssDataTaskPriority = handleGnssDataTaskPriority; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 44)
        {
            systemPrint("Enter BT Read Task Core (0 or 1): ");
            int btReadTaskCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((btReadTaskCore != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (btReadTaskCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (btReadTaskCore < 0 || btReadTaskCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.btReadTaskCore = btReadTaskCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 45)
        {
            systemPrint("Enter GNSS Read Task Core (0 or 1): ");
            int gnssReadTaskCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((gnssReadTaskCore != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (gnssReadTaskCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (gnssReadTaskCore < 0 || gnssReadTaskCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.gnssReadTaskCore = gnssReadTaskCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 46)
        {
            systemPrint("Enter GNSS Data Handler Task Core (0 or 1): ");
            int handleGnssDataTaskCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((handleGnssDataTaskCore != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (handleGnssDataTaskCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (handleGnssDataTaskCore < 0 || handleGnssDataTaskCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.handleGnssDataTaskCore = handleGnssDataTaskCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 47)
        {
            systemPrint("Enter Serial GNSS RX Full Threshold (1 to 127): ");
            int serialGNSSRxFullThreshold = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((serialGNSSRxFullThreshold != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (serialGNSSRxFullThreshold != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (serialGNSSRxFullThreshold < 1 || serialGNSSRxFullThreshold > 127)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.serialGNSSRxFullThreshold = serialGNSSRxFullThreshold; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 48)
        {
            systemPrint("Enter Core used for GNSS UART Interrupts (0 or 1): ");
            int gnssUartInterruptsCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((gnssUartInterruptsCore != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (gnssUartInterruptsCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (gnssUartInterruptsCore < 0 || gnssUartInterruptsCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.gnssUartInterruptsCore = gnssUartInterruptsCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 49)
        {
            systemPrint("Not yet implemented! - Enter Core used for Bluetooth Interrupts (0 or 1): ");
            int bluetoothInterruptsCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((bluetoothInterruptsCore != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (bluetoothInterruptsCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (bluetoothInterruptsCore < 0 || bluetoothInterruptsCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.bluetoothInterruptsCore = bluetoothInterruptsCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 50)
        {
            systemPrint("Enter Core used for I2C Interrupts (0 or 1): ");
            int i2cInterruptsCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((i2cInterruptsCore != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (i2cInterruptsCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (i2cInterruptsCore < 0 || i2cInterruptsCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.i2cInterruptsCore = i2cInterruptsCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 51)
        {
            int seconds = getNumber();
            if ((seconds != INPUT_RESPONSE_GETNUMBER_EXIT) && (seconds != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                settings.periodicDisplayInterval = seconds * 1000;
        }
        else if (incoming == 52)
        {
            int value = getNumber();
            if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                settings.periodicDisplay = value;
        }
        else if (incoming == 53)
        {
            PERIODIC_TOGGLE(PD_BLUETOOTH_DATA_RX);
        }
        else if (incoming == 54)
        {
            PERIODIC_TOGGLE(PD_BLUETOOTH_DATA_TX);
        }
        else if (incoming == 55)
        {
            PERIODIC_TOGGLE(PD_ETHERNET_IP_ADDRESS);
        }
        else if (incoming == 56)
        {
            PERIODIC_TOGGLE(PD_ETHERNET_STATE);
        }
        else if (incoming == 57)
        {
            PERIODIC_TOGGLE(PD_NETWORK_STATE);
        }
        else if (incoming == 58)
        {
            PERIODIC_TOGGLE(PD_NTRIP_CLIENT_DATA);
        }
        else if (incoming == 59)
        {
            PERIODIC_TOGGLE(PD_NTRIP_CLIENT_STATE);
        }
        else if (incoming == 60)
        {
            PERIODIC_TOGGLE(PD_NTRIP_SERVER_DATA);
        }
        else if (incoming == 61)
        {
            PERIODIC_TOGGLE(PD_NTRIP_SERVER_STATE);
        }
        else if (incoming == 62)
        {
            PERIODIC_TOGGLE(PD_PVT_CLIENT_DATA);
        }
        else if (incoming == 63)
        {
            PERIODIC_TOGGLE(PD_PVT_CLIENT_STATE);
        }
        else if (incoming == 64)
        {
            PERIODIC_TOGGLE(PD_PVT_SERVER_DATA);
        }
        else if (incoming == 65)
        {
            PERIODIC_TOGGLE(PD_PVT_SERVER_STATE);
        }
        else if (incoming == 66)
        {
            PERIODIC_TOGGLE(PD_PVT_SERVER_CLIENT_DATA);
        }
        else if (incoming == 67)
        {
            PERIODIC_TOGGLE(PD_SD_LOG_WRITE);
        }
        else if (incoming == 68)
        {
            PERIODIC_TOGGLE(PD_WIFI_STATE);
        }
        else if (incoming == 69)
        {
            PERIODIC_TOGGLE(PD_ZED_DATA_RX);
        }
        else if (incoming == 70)
        {
            PERIODIC_TOGGLE(PD_ZED_DATA_TX);
        }
        else if (incoming == 71)
        {
            PERIODIC_TOGGLE(PD_NTRIP_CLIENT_GGA);
        }
        else if (incoming == 72)
        {
            PERIODIC_TOGGLE(PD_RING_BUFFER_MILLIS);
        }
        else if (incoming == 73)
        {
            settings.debugPvtServer ^= 1;
        }
        else if (incoming == 74)
        {
            settings.debugNetworkLayer ^= 1;
        }
        else if (incoming == 75)
        {
            settings.printNetworkStatus ^= 1;
        }
        else if (incoming == 76)
        {
            PERIODIC_TOGGLE(PD_NTP_SERVER_DATA);
        }
        else if (incoming == 77)
        {
            PERIODIC_TOGGLE(PD_NTP_SERVER_STATE);
        }
        else if (incoming == 'e')
        {
            systemPrintln("Erasing LittleFS and resetting");
            LittleFS.format();
            ESP.restart();
        }
        else if (incoming == 'r')
        {
            recordSystemSettings();

            ESP.restart();
        }
        else if (incoming == 't')
        {
            requestChangeState(STATE_TEST); // We'll enter test mode once exiting all serial menus
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
}

// Print the current long/lat/alt/HPA/SIV
// From Example11_GetHighPrecisionPositionUsingDouble
void printCurrentConditions()
{
    if (online.gnss == true)
    {
        systemPrint("SIV: ");
        systemPrint(numSV);

        systemPrint(", HPA (m): ");
        systemPrint(horizontalAccuracy, 3);

        systemPrint(", Lat: ");
        systemPrint(latitude, haeNumberOfDecimals);
        systemPrint(", Lon: ");
        systemPrint(longitude, haeNumberOfDecimals);

        systemPrint(", Altitude (m): ");
        systemPrint(altitude, 1);

        systemPrintln();
    }
}

void printCurrentConditionsNMEA()
{
    if (online.gnss == true)
    {
        char systemStatus[100];
        snprintf(systemStatus, sizeof(systemStatus),
                 "%02d%02d%02d.%02d,%02d%02d%02d,%0.3f,%d,%0.9f,%0.9f,%0.2f,%d,%d,%d", gnssHour, gnssMinute, gnssSecond,
                 mseconds, gnssDay, gnssMonth, gnssYear % 2000, // Limit to 2 digits
                 horizontalAccuracy, numSV, latitude, longitude, altitude, fixType, carrSoln, battLevel);

        char nmeaMessage[100]; // Max NMEA sentence length is 82
        createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, sizeof(nmeaMessage),
                           systemStatus); // textID, buffer, sizeOfBuffer, text
        systemPrintln(nmeaMessage);
    }
    else
    {
        char nmeaMessage[100]; // Max NMEA sentence length is 82
        createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, sizeof(nmeaMessage),
                           (char *)"OFFLINE"); // textID, buffer, sizeOfBuffer, text
        systemPrintln(nmeaMessage);
    }
}

// When called, prints the contents of root folder list of files on SD card
// This allows us to replace the sd.ls() function to point at Serial and BT outputs
void printFileList()
{
    bool sdCardAlreadyMounted = online.microSD;
    if (!online.microSD)
        beginSD();

    // Notify the user if the microSD card is not available
    if (!online.microSD)
        systemPrintln("microSD card not online!");
    else
    {
        // Attempt to gain access to the SD card
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
        {
            markSemaphore(FUNCTION_PRINT_FILE_LIST);

            if (USE_SPI_MICROSD)
            {
                SdFile dir;
                dir.open("/"); // Open root
                uint16_t fileCount = 0;

                SdFile tempFile;

                systemPrintln("Files found:");

                while (tempFile.openNext(&dir, O_READ))
                {
                    if (tempFile.isFile())
                    {
                        fileCount++;

                        // 2017-05-19 187362648 800_0291.MOV

                        // Get File Date from sdFat
                        uint16_t fileDate;
                        uint16_t fileTime;
                        tempFile.getCreateDateTime(&fileDate, &fileTime);

                        // Convert sdFat file date fromat into YYYY-MM-DD
                        char fileDateChar[20];
                        snprintf(fileDateChar, sizeof(fileDateChar), "%d-%02d-%02d",
                                 ((fileDate >> 9) + 1980),   // Year
                                 ((fileDate >> 5) & 0b1111), // Month
                                 (fileDate & 0b11111)        // Day
                        );

                        char fileSizeChar[20];
                        String fileSize;
                        stringHumanReadableSize(fileSize, tempFile.fileSize());
                        fileSize.toCharArray(fileSizeChar, sizeof(fileSizeChar));

                        char fileName[50]; // Handle long file names
                        tempFile.getName(fileName, sizeof(fileName));

                        char fileRecord[100];
                        snprintf(fileRecord, sizeof(fileRecord), "%s\t%s\t%s", fileDateChar, fileSizeChar, fileName);

                        systemPrintln(fileRecord);
                    }
                }

                dir.close();
                tempFile.close();

                if (fileCount == 0)
                    systemPrintln("No files found");
            }
#ifdef COMPILE_SD_MMC
            else
            {
                File dir = SD_MMC.open("/"); // Open root
                uint16_t fileCount = 0;

                if (dir && dir.isDirectory())
                {
                    systemPrintln("Files found:");

                    File tempFile = dir.openNextFile();
                    while (tempFile)
                    {
                        if (!tempFile.isDirectory())
                        {
                            fileCount++;

                            // 2017-05-19 187362648 800_0291.MOV

                            // Get time of last write
                            time_t lastWrite = tempFile.getLastWrite();

                            struct tm *timeinfo = localtime(&lastWrite);

                            char fileDateChar[20];
                            snprintf(fileDateChar, sizeof(fileDateChar), "%.0f-%02.0f-%02.0f",
                                     (float)timeinfo->tm_year + 1900, // Year - ESP32 2.0.2 starts the year at 1900...
                                     (float)timeinfo->tm_mon + 1,     // Month
                                     (float)timeinfo->tm_mday         // Day
                            );

                            char fileSizeChar[20];
                            String fileSize;
                            stringHumanReadableSize(fileSize, tempFile.size());
                            fileSize.toCharArray(fileSizeChar, sizeof(fileSizeChar));

                            char fileName[50]; // Handle long file names
                            snprintf(fileName, sizeof(fileName), "%s", tempFile.name());

                            char fileRecord[100];
                            snprintf(fileRecord, sizeof(fileRecord), "%s\t%s\t%s", fileDateChar, fileSizeChar,
                                     fileName);

                            systemPrintln(fileRecord);
                        }

                        tempFile.close();
                        tempFile = dir.openNextFile();
                    }
                }

                dir.close();

                if (fileCount == 0)
                    systemPrintln("No files found");
            }
#endif // COMPILE_SD_MMC
        }
        else
        {
            char semaphoreHolder[50];
            getSemaphoreFunction(semaphoreHolder);

            // This is an error because the current settings no longer match the settings
            // on the microSD card, and will not be restored to the expected settings!
            systemPrintf("sdCardSemaphore failed to yield, held by %s, menuSystem.ino line %d\r\n", semaphoreHolder,
                         __LINE__);
        }

        // Release the SD card if not originally mounted
        if (sdCardAlreadyMounted)
            xSemaphoreGive(sdCardSemaphore);
        else
            endSD(true, true);
    }
}
