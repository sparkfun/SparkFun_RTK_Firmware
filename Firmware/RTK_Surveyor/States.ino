/*
  This is the main state machine for the device. It's big but controls each step of the system.
  See system state diagram for a visual representation of how states can change to/from.
  Statemachine diagram: https://lucid.app/lucidchart/53519501-9fa5-4352-aa40-673f88ca0c9b/edit?invitationId=inv_ebd4b988-513d-4169-93fd-c291851108f8
*/

static uint32_t lastStateTime = 0;

//Given the current state, see if conditions have moved us to a new state
//A user pressing the setup button (change between rover/base) is handled by checkpin_setupButton()
void updateSystemState()
{
  if (millis() - lastSystemStateUpdate > 500 || forceSystemStateUpdate == true)
  {
    lastSystemStateUpdate = millis();
    forceSystemStateUpdate = false;

    //Check to see if any external sources need to change state
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
      changeState (systemState);
      lastStateTime = millis();
    }

    //Move between states as needed
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
      case (STATE_ROVER_NOT_STARTED):
        {
          if (online.gnss == false)
          {
            firstRoverStart = false; //If GNSS is offline, we still need to allow button use
            return;
          }

          if (productVariant == RTK_SURVEYOR)
          {
            digitalWrite(pin_baseStatusLED, LOW);
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
            ledcWrite(ledBTChannel, 0); //Turn off BT LED
          }

          //Configure for rover mode
          displayRoverStart(0);

          //If we are survey'd in, but switch is rover then disable survey
          if (configureUbloxModuleRover() == false)
          {
            Serial.println(F("Rover config failed"));
            displayRoverFail(1000);
            return;
          }

          setMuxport(settings.dataPortChannel); //Return mux to original channel

          i2cGNSS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 0); //Disable RTCM sentences

          bluetoothStart(); //Turn on Bluetooth with 'Rover' name
          startUART2Tasks(); //Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)

          settings.updateZEDSettings = false; //On the next boot, no need to update the ZED on this profile
          settings.lastState = STATE_ROVER_NOT_STARTED;
          recordSystemSettings(); //Record this state for next POR

          displayRoverSuccess(500);

          ntripClientStart();
          changeState(STATE_ROVER_NO_FIX);

          firstRoverStart = false; //Do not allow entry into test menu again
        }
        break;

      case (STATE_ROVER_NO_FIX):
        {
          if (fixType == 3 || fixType == 4) //3D, 3D+DR
            changeState(STATE_ROVER_FIX);
        }
        break;

      case (STATE_ROVER_FIX):
        {
          updateAccuracyLEDs();

          if (carrSoln == 1) //RTK Float
            changeState(STATE_ROVER_RTK_FLOAT);
          else if (carrSoln == 2) //RTK Fix
            changeState(STATE_ROVER_RTK_FIX);
        }
        break;

      case (STATE_ROVER_RTK_FLOAT):
        {
          updateAccuracyLEDs();

          if (carrSoln == 0) //No RTK
            changeState(STATE_ROVER_FIX);
          if (carrSoln == 2) //RTK Fix
            changeState(STATE_ROVER_RTK_FIX);
        }
        break;

      case (STATE_ROVER_RTK_FIX):
        {
          updateAccuracyLEDs();

          if (carrSoln == 0) //No RTK
            changeState(STATE_ROVER_FIX);
          if (carrSoln == 1) //RTK Float
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
                                          | && beginSurveyIn() == true
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

      case (STATE_BASE_NOT_STARTED):
        {
          firstRoverStart = false; //If base is starting, no test menu, normal button use.

          if (online.gnss == false)
            return;

          //Turn off base LED until we successfully enter temp/fix state
          if (productVariant == RTK_SURVEYOR)
          {
            digitalWrite(pin_baseStatusLED, LOW);
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
            ledcWrite(ledBTChannel, 0); //Turn off BT LED
          }

          displayBaseStart(0); //Show 'Base'

          //Stop all WiFi and BT. Re-enable in each specific base start state.
          wifiStop();
          bluetoothStop();
          startUART2Tasks(); //Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)

          if (configureUbloxModuleBase() == true)
          {
            settings.updateZEDSettings = false; //On the next boot, no need to update the ZED on this profile
            settings.lastState = STATE_BASE_NOT_STARTED; //Record this state for next POR
            recordSystemSettings(); //Record this state for next POR

            displayBaseSuccess(500); //Show 'Base Started'

            if (settings.fixedBase == false)
            {
              //Restart Bluetooth with 'Base' name
              //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
              if (settings.ntripServer_StartAtSurveyIn)
                ntripServerStart();
              else
                bluetoothStart();
              changeState(STATE_BASE_TEMP_SETTLE);
            }
            else if (settings.fixedBase == true)
            {
              if (settings.enableNtripServer)
                ntripServerStart();
              else
                bluetoothStart();
              changeState(STATE_BASE_FIXED_NOT_STARTED);
            }
          }
          else
          {
            displayBaseFail(1000);
          }
        }
        break;

      //Wait for horz acc of 5m or less before starting survey in
      case (STATE_BASE_TEMP_SETTLE):
        {
          //Blink base LED slowly while we wait for first fix
          if (millis() - lastBaseLEDupdate > 1000)
          {
            lastBaseLEDupdate = millis();

            if (productVariant == RTK_SURVEYOR)
              digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED));
          }

          //Check for <1m horz accuracy before starting surveyIn
          Serial.printf("Waiting for Horz Accuracy < %0.2f meters: %0.2f\n\r", settings.surveyInStartingAccuracy, horizontalAccuracy);

          if (horizontalAccuracy > 0.0 && horizontalAccuracy < settings.surveyInStartingAccuracy)
          {
            displaySurveyStart(0); //Show 'Survey'

            if (beginSurveyIn() == true) //Begin survey
            {
              displaySurveyStarted(500); //Show 'Survey Started'

              changeState(STATE_BASE_TEMP_SURVEY_STARTED);
            }
          }
        }
        break;

      //Check survey status until it completes or 15 minutes elapses and we go back to rover
      case (STATE_BASE_TEMP_SURVEY_STARTED):
        {
          //Blink base LED quickly during survey in
          if (millis() - lastBaseLEDupdate > 500)
          {
            lastBaseLEDupdate = millis();

            if (productVariant == RTK_SURVEYOR)
              digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED));
          }

          //Get the data once to avoid duplicate slow responses
          svinObservationTime = i2cGNSS.getSurveyInObservationTime(50);
          svinMeanAccuracy = i2cGNSS.getSurveyInMeanAccuracy(50);

          if (i2cGNSS.getSurveyInValid(50) == true) //Survey in complete
          {
            Serial.printf("Observation Time: %d\n\r", svinObservationTime);
            Serial.println(F("Base survey complete! RTCM now broadcasting."));

            if (productVariant == RTK_SURVEYOR)
              digitalWrite(pin_baseStatusLED, HIGH); //Indicate survey complete

            //Start the NTRIP server if requested
            if ((settings.ntripServer_StartAtSurveyIn == false)
                && (settings.enableNtripServer == true))
            {
              ntripServerStart();
            }

            rtcmPacketsSent = 0; //Reset any previous number
            changeState(STATE_BASE_TEMP_TRANSMITTING);
          }
          else
          {
            Serial.print(F("Time elapsed: "));
            Serial.print(svinObservationTime);
            Serial.print(F(" Accuracy: "));
            Serial.print(svinMeanAccuracy);
            Serial.print(F(" SIV: "));
            Serial.print(numSV);
            Serial.println();

            if (svinObservationTime > maxSurveyInWait_s)
            {
              Serial.printf("Survey-In took more than %d minutes. Returning to rover mode.\n\r", maxSurveyInWait_s / 60);

              resetSurvey();

              changeState(STATE_ROVER_NOT_STARTED);
            }
          }
        }
        break;

      //Leave base temp transmitting if user has enabled WiFi/NTRIP
      case (STATE_BASE_TEMP_TRANSMITTING):
        {
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

      //User has set switch to base with fixed option enabled. Let's configure and try to get there.
      //If fixed base fails, we'll handle it here
      case (STATE_BASE_FIXED_NOT_STARTED):
        {
          bool response = startFixedBase();
          if (response == true)
          {
            if (productVariant == RTK_SURVEYOR)
              digitalWrite(pin_baseStatusLED, HIGH); //Turn on base LED

            changeState(STATE_BASE_FIXED_TRANSMITTING);
          }
          else
          {
            Serial.println(F("Fixed base start failed"));
            displayBaseFail(1000);

            changeState(STATE_ROVER_NOT_STARTED); //Return to rover mode to avoid being in fixed base mode
          }
        }
        break;

      //Leave base fixed transmitting if user has enabled WiFi/NTRIP
      case (STATE_BASE_FIXED_TRANSMITTING):
        {
        }
        break;

      case (STATE_BUBBLE_LEVEL):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_PROFILE):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_MARK_EVENT):
        {
          bool logged = false;
          bool marked = false;

          //Gain access to the SPI controller for the microSD card
          if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
          {
            //Record this event to the log
            if (online.logging == true)
            {
              char nmeaMessage[82]; //Max NMEA sentence length is 82
              createNMEASentence(CUSTOM_NMEA_TYPE_WAYPOINT, nmeaMessage, (char*)"CustomEvent"); //textID, buffer, text
              ubxFile->println(nmeaMessage);
              logged = true;
            }

            //Record this point to the marks file
            if (settings.enableMarksFile) {
              //Get the marks file name
              char fileName[32];
              bool fileOpen = false;
              char markBuffer[100];
              bool sdCardWasOnline;
              int year;
              int month;
              int day;

              //Get the date
              year = rtc.getYear();
              month = rtc.getMonth() + 1;
              day = rtc.getDay();

              //Build the file name
              sprintf (fileName, "Marks_%04d_%02d_%02d.csv", year, month, day);

              //Try to gain access the SD card
              sdCardWasOnline = online.microSD;
              if (online.microSD != true)
                beginSD();

              if (online.microSD == true)
              {
                //Open the marks file
                SdFile * marksFile = new SdFile();
                if (marksFile && marksFile->open(fileName, O_APPEND | O_WRITE))
                {
                  fileOpen = true;
                  marksFile->timestamp(T_CREATE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(),
                                       rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
                }
                else if (marksFile && marksFile->open(fileName, O_CREAT | O_WRITE))
                {
                  fileOpen = true;
                  marksFile->timestamp(T_ACCESS, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(),
                                       rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
                  marksFile->timestamp(T_WRITE, rtc.getYear(), rtc.getMonth() + 1, rtc.getDay(),
                                       rtc.getHour(true), rtc.getMinute(), rtc.getSecond());

                  //Add the column headers
                  //YYYYMMDDHHMMSS, Lat: xxxx, Long: xxxx, Alt: xxxx, SIV: xx, HPA: xxxx, Batt: xxx
                  //                           1         2         3         4         5         6         7         8         9
                  //                  1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901
                  strcpy(markBuffer, "Date, Time, Latitude, Longitude, Altitude Meters, SIV, HPA Meters, Battery Level, Voltage\n");
                  marksFile->write(markBuffer, strlen(markBuffer));
                }
                if (fileOpen)
                {
                  //Create the mark text
                  //         1         2         3         4         5         6         7         8
                  //12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
                  //YYYY-MM-DD, HH:MM:SS, ---Latitude---, --Longitude---, --Alt--,SIV, --HPA---,Level,Volts\n
                  if (horizontalAccuracy >= 100.)
                    sprintf (markBuffer, "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.0f, %3d%%, %4.2f\n",
                             year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(),
                             latitude, longitude, altitude, numSV, horizontalAccuracy,
                             battLevel, battVoltage);
                  else if (horizontalAccuracy >= 10.)
                    sprintf (markBuffer, "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.1f, %3d%%, %4.2f\n",
                             year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(),
                             latitude, longitude, altitude, numSV, horizontalAccuracy,
                             battLevel, battVoltage);
                  else if (horizontalAccuracy >= 1.)
                    sprintf (markBuffer, "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.2f, %3d%%, %4.2f\n",
                             year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(),
                             latitude, longitude, altitude, numSV, horizontalAccuracy,
                             battLevel, battVoltage);
                  else
                    sprintf (markBuffer, "%04d-%02d-%02d, %02d:%02d:%02d, %14.9f, %14.9f, %7.1f, %2d, %8.3f, %3d%%, %4.2f\n",
                             year, month, day, rtc.getHour(true), rtc.getMinute(), rtc.getSecond(),
                             latitude, longitude, altitude, numSV, horizontalAccuracy,
                             battLevel, battVoltage);

                  //Write the mark to the file
                  marksFile->write(markBuffer, strlen(markBuffer));

                  // Update the file to create time & date
                  updateDataFileCreate(marksFile);

                  //Close the mark file
                  marksFile->close();
                  marked = true;
                }

                //Done with the file
                if (marksFile)
                  delete (marksFile);

                //Dismount the SD card
                if (!sdCardWasOnline)
                  endSD(true, false);
              }
            }

            //Done with the SPI controller
            xSemaphoreGive(sdCardSemaphore);

            //Record this event to the log
            if ((online.logging == true) && (settings.enableMarksFile))
            {
              if (logged && marked)
                displayEventMarked(500); //Show 'Event Marked'
              else if (marked)
                displayNoLogging(500); //Show 'No Logging'
              else if (logged)
                displayNotMarked(500); //Show 'Not Marked'
              else
                displayMarkFailure(500); //Show 'Mark Failure'
            }
            else if (settings.enableMarksFile)
            {
              if (marked)
                displayMarked(500);  //Show 'Marked'
              else
                displayNotMarked(500); //Show 'Not Marked'
            }
            else if (logged)
              displayEventMarked(500); //Show 'Event Marked'
            else
              displayNoLogging(500); //Show 'No Logging'

            // Return to the previous state
            changeState(lastSystemState);
          } //End sdCardSemaphore
          else
          {
            //Enable retry by not changing states
            log_d("sdCardSemaphore failed to yield in STATE_MARK_EVENT\r\n");
          }
        }
        break;

      case (STATE_DISPLAY_SETUP):
        {
          if (millis() - lastSetupMenuChange > 1500)
          {
            forceSystemStateUpdate = true; //Imediately go to this new state
            changeState(setupState); //Change to last setup state
          }
        }
        break;

      case (STATE_WIFI_CONFIG_NOT_STARTED):
        {
          if (productVariant == RTK_SURVEYOR)
          {
            //Start BT LED Fade to indicate start of WiFi
            btLEDTask.detach(); //Increase BT LED blinker task rate
            btLEDTask.attach(btLEDTaskPace33Hz, updateBTled); //Rate in seconds, callback

            digitalWrite(pin_baseStatusLED, LOW);
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
          }

          displayWiFiConfigNotStarted(); //Display immediately during SD cluster pause

          bluetoothStop();
          stopUART2Tasks(); //Delete F9 serial tasks if running
          startWebServer(); //Start in AP mode and show config html page

          changeState(STATE_WIFI_CONFIG);
        }
        break;
      case (STATE_WIFI_CONFIG):
        {
          if (incomingSettingsSpot > 0)
          {
            //Allow for 750ms before we parse buffer for all data to arrive
            if (millis() - timeSinceLastIncomingSetting > 750)
            {
              Serial.print("Parsing: ");
              for (int x = 0 ; x < incomingSettingsSpot ; x++)
                Serial.write(incomingSettings[x]);
              Serial.println();

              parseIncomingSettings();
              settings.updateZEDSettings = true; //When this profile is loaded next, force system to update ZED settings.
              recordSystemSettings(); //Record these settings to unit

              //Clear buffer
              incomingSettingsSpot = 0;
              memset(incomingSettings, 0, sizeof(incomingSettings));
            }
          }
        }
        break;

      //Setup device for testing
      case (STATE_TEST):
        {
          //Debounce entry into test menu
          if (millis() - lastTestMenuChange > 500)
          {
            stopUART2Tasks(); //Stop absoring ZED serial via task
            zedUartPassed = false;

            //Enable RTCM 1230. This is the GLONASS bias sentence and is transmitted
            //even if there is no GPS fix. We use it to test serial output.
            i2cGNSS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 1); //Enable message every second

            changeState(STATE_TESTING);
          }
        }
        break;

      //Display testing screen - do nothing
      case (STATE_TESTING):
        {
          //Exit via button press task
        }
        break;

      case (STATE_KEYS_STARTED):
        {
          if (rtcWaitTime == 0) rtcWaitTime = millis();

          //We want an immediate change from this state
          forceSystemStateUpdate = true; //Imediately go to this new state

          //If user has turned off PointPerfect, skip everything
          if (settings.enablePointPerfectCorrections == false)
          {
            changeState(settings.lastState); //Go to either rover or base
          }

          //If there is no WiFi setup, and no keys, skip everything
          else if (strlen(settings.home_wifiSSID) == 0 && strlen(settings.pointPerfectCurrentKey) == 0)
          {
            changeState(settings.lastState); //Go to either rover or base
          }

          //If we don't have keys, begin zero touch provisioning
          else if (strlen(settings.pointPerfectCurrentKey) == 0 || strlen(settings.pointPerfectNextKey) == 0)
          {
            wifiStart(settings.home_wifiSSID, settings.home_wifiPW);
            changeState(STATE_KEYS_PROVISION_WIFI_STARTED);
          }

          //Determine if we have valid date/time RTC from last boot
          else if (online.rtc == false)
          {
            if (millis() - rtcWaitTime > 2000)
            {
              //If RTC is not available, we will assume we need keys
              changeState(STATE_KEYS_NEEDED);
            }
          }

          else
          {
            //Determine days until next key expires
            uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
            log_d("Days until keys expire: %d", daysRemaining);

            if (daysRemaining >= 28)
              changeState(STATE_KEYS_LBAND_CONFIGURE);
            else
              changeState(STATE_KEYS_NEEDED);
          }
        }
        break;

      case (STATE_KEYS_NEEDED):
        {
          forceSystemStateUpdate = true; //Imediately go to this new state

          if (online.rtc == false)
          {
            wifiStart(settings.home_wifiSSID, settings.home_wifiPW);
            changeState(STATE_KEYS_WIFI_STARTED); //If we can't check the RTC, continue
          }

          //When did we last try to get keys? Attempt every 24 hours
          else if (rtc.getEpoch() - settings.lastKeyAttempt > (60 * 60 * 24))
          {
            settings.lastKeyAttempt = rtc.getEpoch(); //Mark it
            recordSystemSettings(); //Record these settings to unit

            wifiStart(settings.home_wifiSSID, settings.home_wifiPW);
            changeState(STATE_KEYS_WIFI_STARTED);
          }
          else
          {
            log_d("Already tried to obtain keys for today");
            changeState(STATE_KEYS_LBAND_CONFIGURE); //We have valid keys, we've already tried today. No need to try again.
          }
        }
        break;

      case (STATE_KEYS_WIFI_STARTED):
        {
#ifdef COMPILE_WIFI
          byte wifiStatus = wifiGetStatus();
          if (wifiStatus == WL_CONNECTED)
          {
            wifiState = WIFI_CONNECTED;

            changeState(STATE_KEYS_WIFI_CONNECTED);
          }
          else if (wifiStatus == WL_NO_SSID_AVAIL)
          {
            changeState(STATE_KEYS_WIFI_TIMEOUT);
          }
          else
          {
            Serial.print(".");
            if (wifiConnectionTimeout())
            {
              //Give up after connection timeout
              changeState(STATE_KEYS_WIFI_TIMEOUT);
            }
          }
#endif
        }
        break;

      case (STATE_KEYS_WIFI_CONNECTED):
        {
          if (updatePointPerfectKeys() == true) //Connect to ThingStream MQTT and get PointPerfect key UBX packet
          {
            displayKeysUpdated();
          }

          wifiStop();

          forceSystemStateUpdate = true; //Imediately go to this new state
          changeState(STATE_KEYS_DAYS_REMAINING);
        }
        break;

      case (STATE_KEYS_DAYS_REMAINING):
        {
          if (online.rtc == true)
          {
            if (settings.pointPerfectNextKeyStart > 0)
            {
              uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
              Serial.printf("Days until PointPerfect keys expire: %d\n\r", daysRemaining);
              paintKeyDaysRemaining(daysRemaining, 2000);
            }
          }

          forceSystemStateUpdate = true; //Imediately go to this new state
          changeState(STATE_KEYS_LBAND_CONFIGURE);
        }
        break;

      case (STATE_KEYS_LBAND_CONFIGURE):
        {
          //Be sure we ignore any external RTCM sources
          i2cGNSS.setPortInput(COM_PORT_UART2, COM_TYPE_UBX); //Set the UART2 to input UBX (no RTCM)

          applyLBandKeys(); //Send current keys, if available, to ZED-F9P

          forceSystemStateUpdate = true; //Imediately go to this new state
          changeState(settings.lastState); //Go to either rover or base
        }
        break;

      case (STATE_KEYS_WIFI_TIMEOUT):
        {
          wifiStop();

          paintKeyWiFiFail(2000);

          forceSystemStateUpdate = true; //Imediately go to this new state

          if (online.rtc == true)
          {
            uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);

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
            //No WiFi. No RTC. We don't know if the keys we have are expired. Attempt to use them.
            changeState(STATE_KEYS_LBAND_CONFIGURE);
          }
        }
        break;

      case (STATE_KEYS_LBAND_ENCRYPTED):
        {
          //Since L-Band is not available, be sure RTCM can be provided over UART2
          i2cGNSS.setPortInput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to input RTCM

          forceSystemStateUpdate = true; //Imediately go to this new state
          changeState(settings.lastState); //Go to either rover or base
        }
        break;

      case (STATE_KEYS_PROVISION_WIFI_STARTED):
        {
#ifdef COMPILE_WIFI
          byte wifiStatus = wifiGetStatus();
          if (wifiStatus == WL_CONNECTED)
          {
            wifiState = WIFI_CONNECTED;

            changeState(STATE_KEYS_PROVISION_WIFI_CONNECTED);
          }
          else if (wifiStatus == WL_NO_SSID_AVAIL)
          {
            changeState(STATE_KEYS_WIFI_TIMEOUT);
          }
          else
          {
            Serial.print(".");
            if (wifiConnectionTimeout())
            {
              //Give up after connection timeout
              changeState(STATE_KEYS_WIFI_TIMEOUT);
            }
          }
#endif
        }
        break;

      case (STATE_KEYS_PROVISION_WIFI_CONNECTED):
        {
          forceSystemStateUpdate = true; //Imediately go to this new state

          if (provisionDevice() == true)
          {
            displayKeysUpdated();
            changeState(STATE_KEYS_DAYS_REMAINING);
          }
          else
          {
            paintKeyProvisionFail(10000); //Device not whitelisted. Show device ID.
            changeState(STATE_KEYS_LBAND_ENCRYPTED);
          }

          wifiStop();

        }
        break;

      case (STATE_KEYS_PROVISION_WIFI_TIMEOUT):
        {
          wifiStop();

          paintKeyWiFiFail(2000);

          changeState(settings.lastState); //Go to either rover or base
        }
        break;

      case (STATE_SHUTDOWN):
        {
          forceDisplayUpdate = true;
          powerDown(true);
        }
        break;

      default:
        {
          Serial.printf("Unknown state: %d\n\r", systemState);
        }
        break;
    }
  }
}

//System state changes may only occur within main state machine
//To allow state changes from external sources (ie, Button Tasks) requests can be made
//Requests are handled at the start of updateSystemState()
void requestChangeState(SystemState requestedState)
{
  newSystemStateRequested = true;
  requestedSystemState = requestedState;
  log_d("Requested System State: %d", requestedSystemState);
}

//Change states and print the new state
void changeState(SystemState newState)
{
  //Log the heap size at the state change
  reportHeapNow();

  //Set the new state
  systemState = newState;

  //Debug print of new state, add leading asterisk for repeated states
  if (newState == systemState)
    Serial.print(F("*"));
  switch (systemState)
  {
    case (STATE_ROVER_NOT_STARTED):
      Serial.print(F("State: Rover - Not Started"));
      break;
    case (STATE_ROVER_NO_FIX):
      Serial.print(F("State: Rover - No Fix"));
      break;
    case (STATE_ROVER_FIX):
      Serial.print(F("State: Rover - Fix"));
      break;
    case (STATE_ROVER_RTK_FLOAT):
      Serial.print(F("State: Rover - RTK Float"));
      break;
    case (STATE_ROVER_RTK_FIX):
      Serial.print(F("State: Rover - RTK Fix"));
      break;
    case (STATE_BASE_NOT_STARTED):
      Serial.print(F("State: Base - Not Started"));
      break;
    case (STATE_BASE_TEMP_SETTLE):
      Serial.print(F("State: Base-Temp - Settle"));
      break;
    case (STATE_BASE_TEMP_SURVEY_STARTED):
      Serial.print(F("State: Base-Temp - Survey Started"));
      break;
    case (STATE_BASE_TEMP_TRANSMITTING):
      Serial.print(F("State: Base-Temp - Transmitting"));
      break;
    case (STATE_BASE_FIXED_NOT_STARTED):
      Serial.print(F("State: Base-Fixed - Not Started"));
      break;
    case (STATE_BASE_FIXED_TRANSMITTING):
      Serial.print(F("State: Base-Fixed - Transmitting"));
      break;
    case (STATE_BUBBLE_LEVEL):
      Serial.print(F("State: Bubble level"));
      break;
    case (STATE_MARK_EVENT):
      Serial.print(F("State: Mark Event"));
      break;
    case (STATE_DISPLAY_SETUP):
      Serial.print(F("State: Display Setup"));
      break;
    case (STATE_WIFI_CONFIG_NOT_STARTED):
      Serial.print(F("State: WiFi Config Not Started"));
      break;
    case (STATE_WIFI_CONFIG):
      Serial.print(F("State: WiFi Config"));
      break;
    case (STATE_TEST):
      Serial.print(F("State: System Test Setup"));
      break;
    case (STATE_TESTING):
      Serial.print(F("State: System Testing"));
      break;
    case (STATE_PROFILE):
      Serial.print(F("State: Profile"));
      break;
    case (STATE_KEYS_STARTED):
      Serial.print(F("State: Keys Started "));
      break;
    case (STATE_KEYS_NEEDED):
      Serial.print(F("State: Keys Needed"));
      break;
    case (STATE_KEYS_WIFI_STARTED):
      Serial.print(F("State: Keys WiFi Started"));
      break;
    case (STATE_KEYS_WIFI_CONNECTED):
      Serial.print(F("State: Keys WiFi Connected"));
      break;
    case (STATE_KEYS_WIFI_TIMEOUT):
      Serial.print(F("State: Keys WiFi Timeout"));
      break;
    case (STATE_KEYS_EXPIRED):
      Serial.print(F("State: Keys Expired"));
      break;
    case (STATE_KEYS_DAYS_REMAINING):
      Serial.print(F("State: Keys Days Remaining"));
      break;
    case (STATE_KEYS_LBAND_CONFIGURE):
      Serial.print(F("State: Keys L-Band Configure"));
      break;
    case (STATE_KEYS_LBAND_ENCRYPTED):
      Serial.print(F("State: Keys L-Band Encrypted"));
      break;
    case (STATE_KEYS_PROVISION_WIFI_STARTED):
      Serial.print(F("State: Keys Provision - WiFi Started"));
      break;
    case (STATE_KEYS_PROVISION_WIFI_CONNECTED):
      Serial.print(F("State: Keys Provision - WiFi Connected"));
      break;
    case (STATE_KEYS_PROVISION_WIFI_TIMEOUT):
      Serial.print(F("State: Keys Provision - WiFi Timeout"));
      break;

    case (STATE_SHUTDOWN):
      Serial.print(F("State: Shut Down"));
      break;
    default:
      Serial.printf("Change State Unknown: %d", systemState);
      break;
  }

  if (online.rtc)
  {
    //Timestamp the state change
    //         1         2
    //12345678901234567890123456
    //YYYY-mm-dd HH:MM:SS.xxxrn0
    struct tm timeinfo = rtc.getTimeStruct();
    char s[30];
    strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf(", %s.%03ld\r\n", s, rtc.getMillis());
  }
}
