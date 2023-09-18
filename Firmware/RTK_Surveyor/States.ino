/*
  This is the main state machine for the device. It's big but controls each step of the system.
  See system state diagram for a visual representation of how states can change to/from.
  Statemachine diagram:
  https://lucid.app/lucidchart/53519501-9fa5-4352-aa40-673f88ca0c9b/edit?invitationId=inv_ebd4b988-513d-4169-93fd-c291851108f8
*/

static uint32_t lastStateTime = 0;

// Given the current state, see if conditions have moved us to a new state
// A user pressing the setup button (change between rover/base) is handled by checkpin_setupButton()
void updateSystemState()
{
    if (millis() - lastSystemStateUpdate > 500 || forceSystemStateUpdate == true)
    {
        lastSystemStateUpdate = millis();
        forceSystemStateUpdate = false;

        // Check to see if any external sources need to change state
        if (newSystemStateRequested == true)
        {
            newSystemStateRequested = false;
            if (systemState != requestedSystemState)
            {
                changeState(requestedSystemState);
                lastStateTime = millis();
            }
        }

        if (settings.enablePrintState && ((millis() - lastStateTime) > 15000))
        {
            changeState(systemState);
            lastStateTime = millis();
        }

        // Move between states as needed
        DMW_st(changeState, systemState);
        switch (systemState)
        {
        /*
                          .-----------------------------------.
                          |      STATE_ROVER_NOT_STARTED      |
                          | Text: 'Rover' and 'Rover Started' |
                          '-----------------------------------'
                                            |
                                            |
                                            |
                                            V
                          .-----------------------------------.
                          |         STATE_ROVER_NO_FIX        |
                          |           SIV Icon Blink          |
                          |            "HPA: >30m"            |
                          |             "SIV: 0"              |
                          '-----------------------------------'
                                            |
                                            | GPS Lock
                                            | 3D, 3D+DR
                                            V
                          .-----------------------------------.
                          |          STATE_ROVER_FIX          | Carrier
                          |           SIV Icon Solid          | Solution = 2
                .-------->|            "HPA: .513"            |---------.
                |         |             "SIV: 30"             |         |
                |         '-----------------------------------'         |
                |                           |                           |
                |                           | Carrier Solution = 1      |
                |                           V                           |
                |         .-----------------------------------.         |
                |         |       STATE_ROVER_RTK_FLOAT       |         |
                |  No RTK |     Double Crosshair Blinking     |         |
                +<--------|           "*HPA: .080"            |         |
                ^         |             "SIV: 30"             |         |
                |         '-----------------------------------'         |
                |                        ^         |                    |
                |                        |         | Carrier            |
                |                        |         | Solution = 2       |
                |                        |         V                    |
                |                Carrier |         +<-------------------'
                |           Solution = 1 |         |
                |                        |         V
                |         .-----------------------------------.
                |         |        STATE_ROVER_RTK_FIX        |
                |  No RTK |       Double Crosshair Solid      |
                '---------|           "*HPA: .014"            |
                          |             "SIV: 30"             |
                          '-----------------------------------'

        */
        case (STATE_ROVER_NOT_STARTED): {
            if (online.gnss == false)
            {
                firstRoverStart = false; // If GNSS is offline, we still need to allow button use
                return;
            }

            if (productVariant == RTK_SURVEYOR)
            {
                digitalWrite(pin_baseStatusLED, LOW);
                digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                digitalWrite(pin_positionAccuracyLED_100cm, LOW);
                ledcWrite(ledBTChannel, 0); // Turn off BT LED
            }

            if (productVariant == REFERENCE_STATION)
            {
                digitalWrite(pin_baseStatusLED, LOW);
            }

            // Configure for rover mode
            displayRoverStart(0);

            // If we are survey'd in, but switch is rover then disable survey
            if (configureUbloxModuleRover() == false)
            {
                systemPrintln("Rover config failed");
                displayRoverFail(1000);
                return;
            }

            setMuxport(settings.dataPortChannel); // Return mux to original channel

            wifiStop();       // Stop WiFi, ntripClient will start as needed.
            bluetoothStart(); // Turn on Bluetooth with 'Rover' name
            radioStart();     // Start internal radio if enabled, otherwise disable

            if (!tasksStartUART2()) // Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)
                displayRoverFail(1000);
            else
            {
                settings.updateZEDSettings = false; // On the next boot, no need to update the ZED on this profile
                settings.lastState = STATE_ROVER_NOT_STARTED;
                recordSystemSettings(); // Record this state for next POR

                displayRoverSuccess(500);

                ntripClientStart();
                changeState(STATE_ROVER_NO_FIX);

                firstRoverStart = false; // Do not allow entry into test menu again
            }
        }
        break;

        case (STATE_ROVER_NO_FIX): {
            if (fixType == 3 || fixType == 4) // 3D, 3D+DR
                changeState(STATE_ROVER_FIX);
        }
        break;

        case (STATE_ROVER_FIX): {
            updateAccuracyLEDs();

            if (carrSoln == 1) // RTK Float
                changeState(STATE_ROVER_RTK_FLOAT);
            else if (carrSoln == 2) // RTK Fix
                changeState(STATE_ROVER_RTK_FIX);
        }
        break;

        case (STATE_ROVER_RTK_FLOAT): {
            updateAccuracyLEDs();

            if (carrSoln == 0) // No RTK
                changeState(STATE_ROVER_FIX);
            if (carrSoln == 2) // RTK Fix
                changeState(STATE_ROVER_RTK_FIX);
        }
        break;

        case (STATE_ROVER_RTK_FIX): {
            updateAccuracyLEDs();

            if (carrSoln == 0) // No RTK
                changeState(STATE_ROVER_FIX);
            if (carrSoln == 1) // RTK Float
                changeState(STATE_ROVER_RTK_FLOAT);
        }
        break;

            /*
                              .-----------------------------------.
                  startBase() |      STATE_BASE_NOT_STARTED       |
                 .------------|            Text: 'Base'           |
                 |    = false '-----------------------------------'
                 |                              |
                 |  Stop WiFi,                  | startBase() = true
                 |      Stop                    | Stop WiFi
                 |  Bluetooth                   | Start Bluetooth
                 |                              V
                 |            .-----------------------------------.
                 |            |      STATE_BASE_TEMP_SETTLE       |
                 |            |   Temp Base Icon. Blinking HPA.   |
                 |            |           "HPA: 7.15"             |
                 |            |             "SIV: 5"              |
                 |            '-----------------------------------'
                 V                              |
              STATE_BASE_FIXED_NOT_STARTED      | horizontalAccuracy > 0.0
              (next diagram)                    | && horizontalAccuracy
                                                |  < settings.surveyInStartingAccuracy
                                                | && surveyInStart() == true
                                                V
                              .-----------------------------------.
                              |   STATE_BASE_TEMP_SURVEY_STARTED  | svinObservationTime >
                              |       Temp Base Icon blinking     | maxSurveyInWait_s
                              |           "Mean: 0.089"           |--------------.
                              |            "Time: 36"             |              |
                              '-----------------------------------'              |
                                                |                                |
                                                | getSurveyInValid()             |
                                                | = true                         V
                                                |              STATE_ROVER_NOT_STARTED
                                                V                   (Previous diagram)
                              .-----------------------------------.
                              |    STATE_BASE_TEMP_TRANSMITTING   |
                              |        Temp Base Icon solid       |
                              |             "Xmitting"            |
                              |            "RTCM: 2145"           |
                              '-----------------------------------'

            */

        case (STATE_BASE_NOT_STARTED): {
            firstRoverStart = false; // If base is starting, no test menu, normal button use.

            if (online.gnss == false)
                return;

            // Turn off base LED until we successfully enter temp/fix state
            if (productVariant == RTK_SURVEYOR)
            {
                digitalWrite(pin_baseStatusLED, LOW);
                digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                digitalWrite(pin_positionAccuracyLED_100cm, LOW);
                ledcWrite(ledBTChannel, 0); // Turn off BT LED
            }

            if (productVariant == REFERENCE_STATION)
                digitalWrite(pin_baseStatusLED, LOW);

            displayBaseStart(0); // Show 'Base'

            // Allow WiFi to continue running if NTRIP Client is needed for assisted survey in
            if (wifiIsNeeded() == false)
                wifiStop();

            bluetoothStop();
            bluetoothStart(); // Restart Bluetooth with 'Base' identifier

            // Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)
            if (tasksStartUART2() && configureUbloxModuleBase())
            {
                settings.updateZEDSettings = false; // On the next boot, no need to update the ZED on this profile
                settings.lastState = STATE_BASE_NOT_STARTED; // Record this state for next POR
                recordSystemSettings();                      // Record this state for next POR

                displayBaseSuccess(500); // Show 'Base Started'

                if (settings.fixedBase == false)
                    changeState(STATE_BASE_TEMP_SETTLE);
                else if (settings.fixedBase == true)
                    changeState(STATE_BASE_FIXED_NOT_STARTED);
            }
            else
            {
                displayBaseFail(1000);
            }
        }
        break;

        // Wait for horz acc of 5m or less before starting survey in
        case (STATE_BASE_TEMP_SETTLE): {
            // Blink base LED slowly while we wait for first fix
            if (millis() - lastBaseLEDupdate > 1000)
            {
                lastBaseLEDupdate = millis();

                if ((productVariant == RTK_SURVEYOR) || (productVariant == REFERENCE_STATION))
                    digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED));
            }

            // Check for <1m horz accuracy before starting surveyIn
            systemPrintf("Waiting for Horz Accuracy < %0.2f meters: %0.2f, SIV: %d\r\n",
                         settings.surveyInStartingAccuracy, horizontalAccuracy, numSV);

            if (horizontalAccuracy > 0.0 && horizontalAccuracy < settings.surveyInStartingAccuracy)
            {
                displaySurveyStart(0); // Show 'Survey'

                if (surveyInStart() == true) // Begin survey
                {
                    displaySurveyStarted(500); // Show 'Survey Started'

                    changeState(STATE_BASE_TEMP_SURVEY_STARTED);
                }
            }
        }
        break;

        // Check survey status until it completes or 15 minutes elapses and we go back to rover
        case (STATE_BASE_TEMP_SURVEY_STARTED): {
            // Blink base LED quickly during survey in
            if (millis() - lastBaseLEDupdate > 500)
            {
                lastBaseLEDupdate = millis();

                if ((productVariant == RTK_SURVEYOR) || (productVariant == REFERENCE_STATION))
                    digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED));
            }

            // Get the data once to avoid duplicate slow responses
            svinObservationTime = theGNSS.getSurveyInObservationTime(50);
            svinMeanAccuracy = theGNSS.getSurveyInMeanAccuracy(50);

            if (theGNSS.getSurveyInValid(50) == true) // Survey in complete
            {
                systemPrintf("Observation Time: %d\r\n", svinObservationTime);
                systemPrintln("Base survey complete! RTCM now broadcasting.");

                if ((productVariant == RTK_SURVEYOR) || (productVariant == REFERENCE_STATION))
                    digitalWrite(pin_baseStatusLED, HIGH); // Indicate survey complete

                // Start the NTRIP server if requested
                if (settings.enableNtripServer == true)
                    ntripServerStart();

                radioStart(); // Start internal radio if enabled, otherwise disable

                rtcmPacketsSent = 0; // Reset any previous number
                changeState(STATE_BASE_TEMP_TRANSMITTING);
            }
            else
            {
                systemPrint("Time elapsed: ");
                systemPrint(svinObservationTime);
                systemPrint(" Accuracy: ");
                systemPrint(svinMeanAccuracy, 3);
                systemPrint(" SIV: ");
                systemPrint(numSV);
                systemPrintln();

                if (svinObservationTime > maxSurveyInWait_s)
                {
                    systemPrintf("Survey-In took more than %d minutes. Returning to rover mode.\r\n",
                                 maxSurveyInWait_s / 60);

                    if (surveyInReset() == false)
                    {
                        systemPrintln("Survey reset failed - attempt 1/3");
                        if (surveyInReset() == false)
                        {
                            systemPrintln("Survey reset failed - attempt 2/3");
                            if (surveyInReset() == false)
                            {
                                systemPrintln("Survey reset failed - attempt 3/3");
                            }
                        }
                    }

                    changeState(STATE_ROVER_NOT_STARTED);
                }
            }
        }
        break;

        // Leave base temp transmitting over external radio, or WiFi/NTRIP, or ESP NOW
        case (STATE_BASE_TEMP_TRANSMITTING): {
        }
        break;

        /*
                          .-----------------------------------.
              startBase() |   STATE_BASE_FIXED_NOT_STARTED    |
                  = false |        Text: "Base Started"       |
            .-------------|                                   |
            |             '-----------------------------------'
            V                               |
          STATE_ROVER_NOT_STARTED             | startBase() = true
          (Rover diagram)                     V
                          .-----------------------------------.
                          |   STATE_BASE_FIXED_TRANSMITTING   |
                          |       Castle Base Icon solid      |
                          |            "Xmitting"             |
                          |            "RTCM: 0"              |
                          '-----------------------------------'

        */

        // User has set switch to base with fixed option enabled. Let's configure and try to get there.
        // If fixed base fails, we'll handle it here
        case (STATE_BASE_FIXED_NOT_STARTED): {
            bool response = startFixedBase();
            if (response == true)
            {
                if ((productVariant == RTK_SURVEYOR) || (productVariant == REFERENCE_STATION))
                    digitalWrite(pin_baseStatusLED, HIGH); // Turn on base LED

                // Start the NTRIP server if requested
                if (settings.enableNtripServer)
                    ntripServerStart();

                radioStart(); // Start internal radio if enabled, otherwise disable

                changeState(STATE_BASE_FIXED_TRANSMITTING);
            }
            else
            {
                systemPrintln("Fixed base start failed");
                displayBaseFail(1000);

                changeState(STATE_ROVER_NOT_STARTED); // Return to rover mode to avoid being in fixed base mode
            }
        }
        break;

        // Leave base fixed transmitting if user has enabled WiFi/NTRIP
        case (STATE_BASE_FIXED_TRANSMITTING): {
        }
        break;

        case (STATE_BUBBLE_LEVEL): {
            // Do nothing - display only
        }
        break;

        case (STATE_PROFILE): {
            // Do nothing - display only
        }
        break;

        case (STATE_MARK_EVENT): {
            bool logged = false;
            bool marked = false;

            // Gain access to the SPI controller for the microSD card
            if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
            {
                markSemaphore(FUNCTION_MARKEVENT);

                // Record this user event to the log
                if (online.logging == true)
                {
                    char nmeaMessage[82]; // Max NMEA sentence length is 82
                    createNMEASentence(CUSTOM_NMEA_TYPE_WAYPOINT, nmeaMessage, sizeof(nmeaMessage),
                                       (char *)"CustomEvent"); // textID, buffer, sizeOfBuffer, text
                    ubxFile->println(nmeaMessage);
                    logged = true;
                }

                // Record this point to the marks file
                if (settings.enableMarksFile)
                {
                    // Get the marks file name
                    char fileName[32];
                    bool fileOpen = false;
                    char markBuffer[100];
                    bool sdCardWasOnline;
                    int year;
                    int month;
                    int day;

                    // Get the date
                    year = rtc.getYear();
                    month = rtc.getMonth() + 1;
                    day = rtc.getDay();

                    // Build the file name
                    snprintf(fileName, sizeof(fileName), "/Marks_%04d_%02d_%02d.csv", year, month, day);

                    // Try to gain access the SD card
                    sdCardWasOnline = online.microSD;
                    if (online.microSD != true)
                        beginSD();

                    if (online.microSD == true)
                    {
                        // Check if the marks file already exists
                        bool marksFileExists = false;
                        if (USE_SPI_MICROSD)
                        {
                            marksFileExists = sd->exists(fileName);
                        }
#ifdef COMPILE_SD_MMC
                        else
                        {
                            marksFileExists = SD_MMC.exists(fileName);
                        }
#endif // COMPILE_SD_MMC

                        // Open the marks file
                        FileSdFatMMC marksFile;

                        if (marksFileExists)
                        {
                            if (marksFile && marksFile.open(fileName, O_APPEND | O_WRITE))
                            {
                                fileOpen = true;
                                marksFile.updateFileCreateTimestamp();
                            }
                        }
                        else
                        {
                            if (marksFile && marksFile.open(fileName, O_CREAT | O_WRITE))
                            {
                                fileOpen = true;
                                marksFile.updateFileAccessTimestamp();

                                // Add the column headers
                                // YYYYMMDDHHMMSS, Lat: xxxx, Long: xxxx, Alt: xxxx, SIV: xx, HPA: xxxx, Batt: xxx
                                //                            1         2         3         4         5         6 7 8 9
                                //                   1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901
                                strcpy(markBuffer, "Date, Time, Latitude, Longitude, Altitude Meters, SIV, HPA Meters, "
                                                   "Battery Level, Voltage\n");
                                marksFile.write((const uint8_t *)markBuffer, strlen(markBuffer));
                            }
                        }

                        if (fileOpen)
                        {
                            // Create the mark text
                            //          1         2         3         4         5         6         7         8
                            // 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
                            // YYYY-MM-DD, HH:MM:SS, ---Latitude---, --Longitude---, --Alt--,SIV, --HPA---,Level,Volts\n
                            if (horizontalAccuracy >= 100.)
                                snprintf(
                                    markBuffer, sizeof(markBuffer),
                                    "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.0f, %3d%%, %4.2f\n",
                                    year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(), latitude,
                                    longitude, altitude, numSV, horizontalAccuracy, battLevel, battVoltage);
                            else if (horizontalAccuracy >= 10.)
                                snprintf(
                                    markBuffer, sizeof(markBuffer),
                                    "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.1f, %3d%%, %4.2f\n",
                                    year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(), latitude,
                                    longitude, altitude, numSV, horizontalAccuracy, battLevel, battVoltage);
                            else if (horizontalAccuracy >= 1.)
                                snprintf(
                                    markBuffer, sizeof(markBuffer),
                                    "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.2f, %3d%%, %4.2f\n",
                                    year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(), latitude,
                                    longitude, altitude, numSV, horizontalAccuracy, battLevel, battVoltage);
                            else
                                snprintf(
                                    markBuffer, sizeof(markBuffer),
                                    "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.3f, %3d%%, %4.2f\n",
                                    year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(), latitude,
                                    longitude, altitude, numSV, horizontalAccuracy, battLevel, battVoltage);

                            // Write the mark to the file
                            marksFile.write((const uint8_t *)markBuffer, strlen(markBuffer));

                            // Update the file to create time & date
                            marksFile.updateFileCreateTimestamp();

                            // Close the mark file
                            marksFile.close();

                            marked = true;
                        }

                        // Dismount the SD card
                        if (!sdCardWasOnline)
                            endSD(true, false);
                    }
                }

                // Done with the SPI controller
                xSemaphoreGive(sdCardSemaphore);

                // Record this event to the log
                if ((online.logging == true) && (settings.enableMarksFile))
                {
                    if (logged && marked)
                        displayEventMarked(500); // Show 'Event Marked'
                    else if (marked)
                        displayNoLogging(500); // Show 'No Logging'
                    else if (logged)
                        displayNotMarked(500); // Show 'Not Marked'
                    else
                        displayMarkFailure(500); // Show 'Mark Failure'
                }
                else if (settings.enableMarksFile)
                {
                    if (marked)
                        displayMarked(500); // Show 'Marked'
                    else
                        displayNotMarked(500); // Show 'Not Marked'
                }
                else if (logged)
                    displayEventMarked(500); // Show 'Event Marked'
                else
                    displayNoLogging(500); // Show 'No Logging'

                // Return to the previous state
                changeState(lastSystemState);
            } // End sdCardSemaphore
            else
            {
                // Enable retry by not changing states
                log_d("sdCardSemaphore failed to yield in STATE_MARK_EVENT");
            }
        }
        break;

        case (STATE_DISPLAY_SETUP): {
            if (millis() - lastSetupMenuChange > 1500)
            {
                forceSystemStateUpdate = true; // Imediately go to this new state
                changeState(setupState);       // Change to last setup state
            }
        }
        break;

        case (STATE_WIFI_CONFIG_NOT_STARTED): {
            if (productVariant == RTK_SURVEYOR)
            {
                // Start BT LED Fade to indicate start of WiFi
                btLEDTask.detach();                               // Increase BT LED blinker task rate
                btLEDTask.attach(btLEDTaskPace33Hz, updateBTled); // Rate in seconds, callback

                digitalWrite(pin_baseStatusLED, LOW);
                digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                digitalWrite(pin_positionAccuracyLED_100cm, LOW);
            }

            if (productVariant == REFERENCE_STATION)
                digitalWrite(pin_baseStatusLED, LOW);

            displayWiFiConfigNotStarted(); // Display immediately during SD cluster pause

            bluetoothStop();
            espnowStop();

            tasksStopUART2(); // Delete F9 serial tasks if running
            if (!startWebServer()) // Start in AP mode and show config html page
                changeState(STATE_ROVER_NOT_STARTED);
            else
                changeState(STATE_WIFI_CONFIG);
        }
        break;

        case (STATE_WIFI_CONFIG): {
            if (incomingSettingsSpot > 0)
            {
                // Allow for 750ms before we parse buffer for all data to arrive
                if (millis() - timeSinceLastIncomingSetting > 750)
                {
                    currentlyParsingData =
                        true; // Disallow new data to flow from websocket while we are parsing the current data

                    systemPrint("Parsing: ");
                    for (int x = 0; x < incomingSettingsSpot; x++)
                        systemWrite(incomingSettings[x]);
                    systemPrintln();

                    parseIncomingSettings();
                    settings.updateZEDSettings =
                        true;               // When this profile is loaded next, force system to update ZED settings.
                    recordSystemSettings(); // Record these settings to unit

                    // Clear buffer
                    incomingSettingsSpot = 0;
                    memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

                    currentlyParsingData = false; // Allow new data from websocket
                }
            }

#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
            // Dynamically update the coordinates on the AP page
            if (websocketConnected == true)
            {
                if (millis() - lastDynamicDataUpdate > 1000)
                {
                    lastDynamicDataUpdate = millis();
                    createDynamicDataString(settingsCSV);

                    // log_d("Sending coordinates: %s", settingsCSV);
                    websocket->textAll(settingsCSV);
                }
            }
#endif // COMPILE_AP
#endif // COMPILE_WIFI
        }
        break;

        // Setup device for testing
        case (STATE_TEST): {
            // Debounce entry into test menu
            if (millis() - lastTestMenuChange > 500)
            {
                tasksStopUART2(); // Stop absoring ZED serial via task
                zedUartPassed = false;

                // Enable RTCM 1230. This is the GLONASS bias sentence and is transmitted
                // even if there is no GPS fix. We use it to test serial output.
                theGNSS.newCfgValset(); // Create a new Configuration Item VALSET message
                theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART2, 1); // Enable message 1230 every second
                theGNSS.sendCfgValset();                                          // Send the VALSET

                changeState(STATE_TESTING);
            }
        }
        break;

        // Display testing screen - do nothing
        case (STATE_TESTING): {
            // Exit via button press task
        }
        break;

#ifdef COMPILE_L_BAND
        case (STATE_KEYS_STARTED): {
            if (rtcWaitTime == 0)
                rtcWaitTime = millis();

            // We want an immediate change from this state
            forceSystemStateUpdate = true; // Imediately go to this new state

            // If user has turned off PointPerfect, skip everything
            if (settings.enablePointPerfectCorrections == false)
            {
                changeState(settings.lastState); // Go to either rover or base
            }

            // If there is no WiFi setup, and no keys, skip everything
            else if (wifiNetworkCount() == 0 && strlen(settings.pointPerfectCurrentKey) == 0)
            {
                changeState(settings.lastState); // Go to either rover or base
            }

            // If we don't have keys, begin zero touch provisioning
            else if (strlen(settings.pointPerfectCurrentKey) == 0 || strlen(settings.pointPerfectNextKey) == 0)
            {
                log_d("L_Band Keys starting WiFi");

                // Temporarily limit WiFi connection attempts
                wifiOriginalMaxConnectionAttempts = wifiMaxConnectionAttempts;
                wifiMaxConnectionAttempts = 0; // Override setting during key retrieval. Give up after single failure.

                wifiStart();
                changeState(STATE_KEYS_PROVISION_WIFI_STARTED);
            }

            // Determine if we have valid date/time RTC from last boot
            else if (online.rtc == false)
            {
                if (millis() - rtcWaitTime > 2000)
                {
                    // If RTC is not available, we will assume we need keys
                    changeState(STATE_KEYS_NEEDED);
                }
            }

            else
            {
                // Determine days until next key expires
                int daysRemaining =
                    daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
                log_d("Days until keys expire: %d", daysRemaining);

                if (daysRemaining >= 28 && daysRemaining <= 56)
                    changeState(STATE_KEYS_DAYS_REMAINING);
                else
                    changeState(STATE_KEYS_NEEDED);
            }
        }
        break;

        case (STATE_KEYS_NEEDED): {
            forceSystemStateUpdate = true; // Imediately go to this new state

            if (online.rtc == false)
            {
                log_d("Keys Needed RTC off starting WiFi");

                // Temporarily limit WiFi connection attempts
                wifiOriginalMaxConnectionAttempts = wifiMaxConnectionAttempts;
                wifiMaxConnectionAttempts = 0; // Override setting during key retrieval. Give up after single failure.

                wifiStart();
                changeState(STATE_KEYS_WIFI_STARTED); // If we can't check the RTC, continue
            }

            // When did we last try to get keys? Attempt every 24 hours
            else if (rtc.getEpoch() - settings.lastKeyAttempt > (60 * 60 * 24))
            {
                settings.lastKeyAttempt = rtc.getEpoch(); // Mark it
                recordSystemSettings();                   // Record these settings to unit

                log_d("Keys Needed. Starting WiFi");

                // Temporarily limit WiFi connection attempts
                wifiOriginalMaxConnectionAttempts = wifiMaxConnectionAttempts;
                wifiMaxConnectionAttempts = 0; // Override setting during key retrieval. Give up after single failure.

                wifiStart(); // Starts WiFi state machine
                changeState(STATE_KEYS_WIFI_STARTED);
            }
            else
            {
                log_d("Already tried to obtain keys for today");
                changeState(
                    STATE_KEYS_DAYS_REMAINING); // We have valid keys, we've already tried today. No need to try again.
            }
        }
        break;

        case (STATE_KEYS_WIFI_STARTED): {
            if (wifiIsConnected())
                changeState(STATE_KEYS_WIFI_CONNECTED);
            else if (wifiState == WIFI_OFF)
            {
                wifiMaxConnectionAttempts =
                    wifiOriginalMaxConnectionAttempts; // Override setting to 2 attemps during keys
                changeState(STATE_KEYS_WIFI_TIMEOUT);
            }
        }
        break;

        case (STATE_KEYS_WIFI_CONNECTED): {

            // Check that the certs are valid
            if (checkCertificates() == true)
            {
                // Update the keys
                if (pointperfectUpdateKeys() == true) // Connect to ThingStream MQTT and get PointPerfect key UBX packet
                    displayKeysUpdated();
            }
            else
            {
                // Erase keys
                erasePointperfectCredentials();

                // Provision device
                if(pointperfectProvisionDevice() == true) // Connect to ThingStream API and get keys
                    displayKeysUpdated();
            }

            // WiFi will be turned off once we exit this state, if no other service needs it

            forceSystemStateUpdate = true; // Imediately go to this new state
            changeState(STATE_KEYS_DAYS_REMAINING);
        }
        break;

        case (STATE_KEYS_DAYS_REMAINING): {
            if (online.rtc == true)
            {
                if (settings.pointPerfectNextKeyStart > 0)
                {
                    int daysRemaining =
                        daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
                    systemPrintf("Days until PointPerfect keys expire: %d\r\n", daysRemaining);
                    if (daysRemaining >= 0)
                    {
                        paintKeyDaysRemaining(daysRemaining, 2000);
                    }
                    else
                    {
                        paintKeysExpired();
                    }
                }
            }
            paintLBandConfigure();

            forceSystemStateUpdate = true; // Imediately go to this new state
            changeState(STATE_KEYS_LBAND_CONFIGURE);
        }
        break;

        case (STATE_KEYS_LBAND_CONFIGURE): {
            // Be sure we ignore any external RTCM sources
            theGNSS.setUART2Input(COM_TYPE_UBX); // Set the UART2 to input UBX (no RTCM)

            pointperfectApplyKeys(); // Send current keys, if available, to ZED-F9P

            forceSystemStateUpdate = true;   // Imediately go to this new state
            changeState(settings.lastState); // Go to either rover or base
        }
        break;

        case (STATE_KEYS_WIFI_TIMEOUT): {
            paintKeyWiFiFail(2000);

            forceSystemStateUpdate = true; // Imediately go to this new state

            if (online.rtc == true)
            {
                int daysRemaining =
                    daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);

                if (daysRemaining >= 0)
                {
                    changeState(STATE_KEYS_DAYS_REMAINING);
                }
                else
                {
                    paintKeysExpired();
                    changeState(STATE_KEYS_LBAND_ENCRYPTED);
                }
            }
            else
            {
                // No WiFi. No RTC. We don't know if the keys we have are expired. Attempt to use them.
                changeState(STATE_KEYS_LBAND_CONFIGURE);
            }
        }
        break;

        case (STATE_KEYS_LBAND_ENCRYPTED): {
            // Since L-Band is not available, be sure RTCM can be provided over UART2
            theGNSS.setUART2Input(COM_TYPE_RTCM3); // Set the UART2 to input RTCM

            forceSystemStateUpdate = true;   // Imediately go to this new state
            changeState(settings.lastState); // Go to either rover or base
        }
        break;

        case (STATE_KEYS_PROVISION_WIFI_STARTED): {
            if (wifiIsConnected())
                changeState(STATE_KEYS_PROVISION_WIFI_CONNECTED);
            else
                changeState(STATE_KEYS_WIFI_TIMEOUT);
        }
        break;

        case (STATE_KEYS_PROVISION_WIFI_CONNECTED): {
            forceSystemStateUpdate = true; // Imediately go to this new state

            if (pointperfectProvisionDevice() == true)
            {
                displayKeysUpdated();
                changeState(STATE_KEYS_DAYS_REMAINING);
            }
            else
            {
                paintKeyProvisionFail(10000); // Device not whitelisted. Show device ID.
                changeState(STATE_KEYS_LBAND_ENCRYPTED);
            }
        }
        break;

        case (STATE_KEYS_PROVISION_WIFI_TIMEOUT): {
            paintKeyWiFiFail(2000);

            changeState(settings.lastState); // Go to either rover or base
        }
        break;
#endif // COMPILE_L_BAND

        case (STATE_ESPNOW_PAIRING_NOT_STARTED): {
#ifdef COMPILE_ESPNOW
            paintEspNowPairing();

            // Start ESP-Now if needed, put ESP-Now into broadcast state
            espnowBeginPairing();

            changeState(STATE_ESPNOW_PAIRING);
#else  // COMPILE_ESPNOW
            changeState(STATE_ROVER_NOT_STARTED);
#endif // COMPILE_ESPNOW
        }
        break;

        case (STATE_ESPNOW_PAIRING): {
            if (espnowIsPaired() == true)
            {
                paintEspNowPaired();

                // Return to the previous state
                changeState(lastSystemState);
            }
            else
            {
                uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                espnowSendPairMessage(broadcastMac); // Send unit's MAC address over broadcast, no ack, no encryption
            }
        }
        break;

#ifdef COMPILE_ETHERNET
        case (STATE_NTPSERVER_NOT_STARTED): {
            firstRoverStart = false; // If NTP is starting, no test menu, normal button use.

            if (online.gnss == false)
                return;

            displayNtpStart(500); // Show 'NTP'

            // Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)
            if (tasksStartUART2() && configureUbloxModuleNTP())
            {
                settings.updateZEDSettings = false; // On the next boot, no need to update the ZED on this profile
                settings.lastState = STATE_NTPSERVER_NOT_STARTED; // Record this state for next POR
                recordSystemSettings();

                if (online.NTPServer)
                {
                    if (settings.debugNtp)
                        systemPrintln("NTP Server started");
                    displayNtpStarted(500); // Show 'NTP Started'
                    changeState(STATE_NTPSERVER_NO_SYNC);
                }
                else
                {
                    if (settings.debugNtp)
                        systemPrintln("NTP Server waiting for Ethernet");
                    displayNtpNotReady(1000); // Show 'Ethernet Not Ready'
                    changeState(STATE_NTPSERVER_NO_SYNC);
                }
            }
            else
            {
                if (settings.debugNtp)
                    systemPrintln("NTP Server ZED configuration failed");
                displayNTPFail(1000); // Show 'NTP Failed'
                // Do we stay in STATE_NTPSERVER_NOT_STARTED? Or should we reset?
            }
        }
        break;

        case (STATE_NTPSERVER_NO_SYNC): {
            if (rtcSyncd)
            {
                if (settings.debugNtp)
                    systemPrintln("NTP Server RTC synchronized");
                changeState(STATE_NTPSERVER_SYNC);
            }
        }
        break;

        case (STATE_NTPSERVER_SYNC): {
            // Do nothing - display only
        }
        break;

        case (STATE_CONFIG_VIA_ETH_NOT_STARTED): {
            displayConfigViaEthNotStarted(1500);

            settings.updateZEDSettings = false; // On the next boot, no need to update the ZED on this profile
            settings.lastState = STATE_CONFIG_VIA_ETH_STARTED; // Record the _next_ state for POR
            recordSystemSettings();

            forceConfigureViaEthernet(); // Create a file in LittleFS to force code into configure-via-ethernet mode

            ESP.restart(); // Restart to go into the dedicated configure-via-ethernet mode
        }
        break;

        case (STATE_CONFIG_VIA_ETH_STARTED): {
            // The code should only be able to enter this state if configureViaEthernet is true.
            // If configureViaEthernet is not true, we need to restart again.
            //(If we continue, startEthernerWebServerESP32W5500 will fail as it won't have exclusive access to SPI and
            // ints).
            if (!configureViaEthernet)
            {
                displayConfigViaEthNotStarted(1500);
                settings.lastState = STATE_CONFIG_VIA_ETH_STARTED; // Re-record this state for POR
                recordSystemSettings();

                forceConfigureViaEthernet(); // Create a file in LittleFS to force code into configure-via-ethernet mode

                ESP.restart(); // Restart to go into the dedicated configure-via-ethernet mode
            }

            displayConfigViaEthStarted(1500);

            bluetoothStop();  // Should be redundant - but just in case
            espnowStop();     // Should be redundant - but just in case
            tasksStopUART2(); // Delete F9 serial tasks if running

            ethernetWebServerStartESP32W5500(); // Start Ethernet in dedicated configure-via-ethernet mode

            if (!startWebServer(false, settings.httpPort)) // Start the async web server
                changeState(STATE_ROVER_NOT_STARTED);
            else
                changeState(STATE_CONFIG_VIA_ETH);
        }
        break;

        case (STATE_CONFIG_VIA_ETH): {
            // Display will show the IP address (displayConfigViaEthernet)

            if (incomingSettingsSpot > 0)
            {
                // Allow for 750ms before we parse buffer for all data to arrive
                if (millis() - timeSinceLastIncomingSetting > 750)
                {
                    currentlyParsingData =
                        true; // Disallow new data to flow from websocket while we are parsing the current data

                    systemPrint("Parsing: ");
                    for (int x = 0; x < incomingSettingsSpot; x++)
                        systemWrite(incomingSettings[x]);
                    systemPrintln();

                    parseIncomingSettings();
                    settings.updateZEDSettings =
                        true;               // When this profile is loaded next, force system to update ZED settings.
                    recordSystemSettings(); // Record these settings to unit

                    // Clear buffer
                    incomingSettingsSpot = 0;
                    memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

                    currentlyParsingData = false; // Allow new data from websocket
                }
            }

#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
            // Dynamically update the coordinates on the AP page
            if (websocketConnected == true)
            {
                if (millis() - lastDynamicDataUpdate > 1000)
                {
                    lastDynamicDataUpdate = millis();
                    createDynamicDataString(settingsCSV);

                    // log_d("Sending coordinates: %s", settingsCSV);
                    websocket->textAll(settingsCSV);
                }
            }
#endif // COMPILE_AP
#endif // COMPILE_WIFI
        }
        break;

        case (STATE_CONFIG_VIA_ETH_RESTART_BASE): {
            displayConfigViaEthNotStarted(1000);

            ethernetWebServerStopESP32W5500();

            settings.updateZEDSettings = false;          // On the next boot, no need to update the ZED on this profile
            settings.lastState = STATE_BASE_NOT_STARTED; // Record the _next_ state for POR
            recordSystemSettings();

            ESP.restart();
        }
        break;
#endif // COMPILE_ETHERNET

        case (STATE_SHUTDOWN): {
            forceDisplayUpdate = true;
            powerDown(true);
        }
        break;

        default: {
            systemPrintf("Unknown state: %d\r\n", systemState);
        }
        break;
        }
    }
}

// System state changes may only occur within main state machine
// To allow state changes from external sources (ie, Button Tasks) requests can be made
// Requests are handled at the start of updateSystemState()
void requestChangeState(SystemState requestedState)
{
    newSystemStateRequested = true;
    requestedSystemState = requestedState;
    log_d("Requested System State: %d", requestedSystemState);
}

// Change states and print the new state
void changeState(SystemState newState)
{
    // Log the heap size at the state change
    reportHeapNow(false);

    // Debug print of new state, add leading asterisk for repeated states
    if ((!settings.enablePrintDuplicateStates) && (newState == systemState))
        return;

    if (settings.enablePrintStates && (newState == systemState))
        systemPrint("*");

    // Set the new state
    systemState = newState;

    if (settings.enablePrintStates)
    {
        switch (systemState)
        {
        case (STATE_ROVER_NOT_STARTED):
            systemPrint("State: Rover - Not Started");
            break;
        case (STATE_ROVER_NO_FIX):
            systemPrint("State: Rover - No Fix");
            break;
        case (STATE_ROVER_FIX):
            systemPrint("State: Rover - Fix");
            break;
        case (STATE_ROVER_RTK_FLOAT):
            systemPrint("State: Rover - RTK Float");
            break;
        case (STATE_ROVER_RTK_FIX):
            systemPrint("State: Rover - RTK Fix");
            break;
        case (STATE_BASE_NOT_STARTED):
            systemPrint("State: Base - Not Started");
            break;
        case (STATE_BASE_TEMP_SETTLE):
            systemPrint("State: Base-Temp - Settle");
            break;
        case (STATE_BASE_TEMP_SURVEY_STARTED):
            systemPrint("State: Base-Temp - Survey Started");
            break;
        case (STATE_BASE_TEMP_TRANSMITTING):
            systemPrint("State: Base-Temp - Transmitting");
            break;
        case (STATE_BASE_FIXED_NOT_STARTED):
            systemPrint("State: Base-Fixed - Not Started");
            break;
        case (STATE_BASE_FIXED_TRANSMITTING):
            systemPrint("State: Base-Fixed - Transmitting");
            break;
        case (STATE_BUBBLE_LEVEL):
            systemPrint("State: Bubble level");
            break;
        case (STATE_MARK_EVENT):
            systemPrint("State: Mark Event");
            break;
        case (STATE_DISPLAY_SETUP):
            systemPrint("State: Display Setup");
            break;
        case (STATE_WIFI_CONFIG_NOT_STARTED):
            systemPrint("State: WiFi Config Not Started");
            break;
        case (STATE_WIFI_CONFIG):
            systemPrint("State: WiFi Config");
            break;
        case (STATE_TEST):
            systemPrint("State: System Test Setup");
            break;
        case (STATE_TESTING):
            systemPrint("State: System Testing");
            break;
        case (STATE_PROFILE):
            systemPrint("State: Profile");
            break;
#ifdef COMPILE_L_BAND
        case (STATE_KEYS_STARTED):
            systemPrint("State: Keys Started ");
            break;
        case (STATE_KEYS_NEEDED):
            systemPrint("State: Keys Needed");
            break;
        case (STATE_KEYS_WIFI_STARTED):
            systemPrint("State: Keys WiFi Started");
            break;
        case (STATE_KEYS_WIFI_CONNECTED):
            systemPrint("State: Keys WiFi Connected");
            break;
        case (STATE_KEYS_WIFI_TIMEOUT):
            systemPrint("State: Keys WiFi Timeout");
            break;
        case (STATE_KEYS_EXPIRED):
            systemPrint("State: Keys Expired");
            break;
        case (STATE_KEYS_DAYS_REMAINING):
            systemPrint("State: Keys Days Remaining");
            break;
        case (STATE_KEYS_LBAND_CONFIGURE):
            systemPrint("State: Keys L-Band Configure");
            break;
        case (STATE_KEYS_LBAND_ENCRYPTED):
            systemPrint("State: Keys L-Band Encrypted");
            break;
        case (STATE_KEYS_PROVISION_WIFI_STARTED):
            systemPrint("State: Keys Provision - WiFi Started");
            break;
        case (STATE_KEYS_PROVISION_WIFI_CONNECTED):
            systemPrint("State: Keys Provision - WiFi Connected");
            break;
        case (STATE_KEYS_PROVISION_WIFI_TIMEOUT):
            systemPrint("State: Keys Provision - WiFi Timeout");
            break;
#endif // COMPILE_L_BAND

        case (STATE_ESPNOW_PAIRING_NOT_STARTED):
            systemPrint("State: ESP-Now Pairing Not Started");
            break;
        case (STATE_ESPNOW_PAIRING):
            systemPrint("State: ESP-Now Pairing");
            break;

        case (STATE_NTPSERVER_NOT_STARTED):
            systemPrint("State: NTP Server - Not Started");
            break;
        case (STATE_NTPSERVER_NO_SYNC):
            systemPrint("State: NTP Server - No Sync");
            break;
        case (STATE_NTPSERVER_SYNC):
            systemPrint("State: NTP Server - Sync");
            break;

        case (STATE_CONFIG_VIA_ETH_NOT_STARTED):
            systemPrint("State: Configure Via Ethernet - Not Started");
            break;
        case (STATE_CONFIG_VIA_ETH_STARTED):
            systemPrint("State: Configure Via Ethernet - Started");
            break;
        case (STATE_CONFIG_VIA_ETH):
            systemPrint("State: Configure Via Ethernet");
            break;
        case (STATE_CONFIG_VIA_ETH_RESTART_BASE):
            systemPrint("State: Configure Via Ethernet - Restarting Base");
            break;

        case (STATE_SHUTDOWN):
            systemPrint("State: Shut Down");
            break;
        case (STATE_NOT_SET):
            systemPrint("State: Not Set");
            break;
        default:
            systemPrintf("Change State Unknown: %d", systemState);
            break;
        }

        if (online.rtc)
        {
            // Timestamp the state change
            //          1         2
            // 12345678901234567890123456
            // YYYY-mm-dd HH:MM:SS.xxxrn0
            struct tm timeinfo = rtc.getTimeStruct();
            char s[30];
            strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &timeinfo);
            systemPrintf(", %s.%03ld", s, rtc.getMillis());
        }

        systemPrintln();
    }
}
