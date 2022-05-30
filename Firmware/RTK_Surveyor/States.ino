/*
  This is the main state machine for the device. It's big but controls each step of the system.
  See system state diagram for a visual representation of how states can change to/from.
  Statemachine diagram: https://lucid.app/lucidchart/53519501-9fa5-4352-aa40-673f88ca0c9b/edit?invitationId=inv_ebd4b988-513d-4169-93fd-c291851108f8
*/

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
        changeState(requestedSystemState);
    }

    //Move between states as needed
    switch (systemState)
    {
      /*
                        .-----------------------------------.
           NTRIP Client |      STATE_ROVER_NOT_STARTED      |
           .------------| Text: 'Rover' and 'Rover Started' |
           |    Enabled '-----------------------------------'
           |    = False                   |
           |  Stop WiFi,                  | NTRIP Client Enabled = True
           |      Start                   | Stop Bluetooth
           |  Bluetooth                   | Start WiFi
           |                              V
           |            .-----------------------------------. 8 Sec
           |            |  STATE_ROVER_CLIENT_WIFI_STARTED  | Connection
           |            |         Blinking WiFi Icon        | Timeout
           |            |           "HPA: >30m"             |--------------.
           |            |             "SIV: 0"              |              |
           |            '-----------------------------------'              |
           |                              |                                |
           |                              | radioState = WIFI_CONNECTED    |
           |                              | WiFi connected = True          |
           |                              V                                |
           |            .-----------------------------------.              |
           |            | STATE_ROVER_CLIENT_WIFI_CONNECTED | Connection   |
           |            |         Solid WiFi Icon           |  failed      V
           |            |           "HPA: >30m"             |------------->+
           |            |             "SIV: 0"              | Stop WiFi,   |
           |            '-----------------------------------' Start        |
           |                              |                   Bluetooth    |
           |                              |                                |
           |                              | Client Started                 |
           |                              V                                |
           |            .-----------------------------------.              |
           |            |     STATE_ROVER_CLIENT_STARTED    | No response, |
           |            |         Blinking WiFi Icon        | unauthorized V
           |            |           "HPA: >30m"             |------------->+
           |            |             "SIV: 0"              | Stop WiFi,   |
           |            '-----------------------------------' Start        |
           |                              |                   Bluetooth    |
           |                              |                                |
           |                              | Client Connected               |
           |                              V                                |
           '----------------------------->+<-------------------------------'
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

          wifiStop(); //Turn off WiFi and release all resources
          startBluetooth(); //Turn on Bluetooth with 'Rover' name
          startUART2Tasks(); //Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)

          settings.updateZEDSettings = false; //On the next boot, no need to update the ZED on this profile
          settings.lastState = STATE_ROVER_NOT_STARTED;
          recordSystemSettings(); //Record this state for next POR

          displayRoverSuccess(500);

          if (ntripClientStart())
            changeState(STATE_ROVER_CLIENT_WIFI_STARTED);
          else
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

      case (STATE_ROVER_CLIENT_WIFI_STARTED):
        {
          if (wifiConnectionTimeout())
          {
            paintNClientWiFiFail(4000);
            changeState(STATE_ROVER_NOT_STARTED); //Give up and move to normal Rover mode
          }

#ifdef COMPILE_WIFI
          byte wifiStatus = wifiGetStatus();
          if (wifiStatus == WL_CONNECTED)
          {
            wifiState = WIFI_CONNECTED;

            // Set the Main Talker ID to "GP". The NMEA GGA messages will be GPGGA instead of GNGGA
            i2cGNSS.setMainTalkerID(SFE_UBLOX_MAIN_TALKER_ID_GP);

            i2cGNSS.setNMEAGPGGAcallbackPtr(&ntripClientPushGPGGA); // Set up the callback for GPGGA

            float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;

            i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_I2C, measurementFrequency * 10); // Tell the module to output GGA every 10 seconds

            changeState(STATE_ROVER_CLIENT_WIFI_CONNECTED);
          }
          else if (wifiStatus == WL_NO_SSID_AVAIL)
          {
            paintNClientWiFiFail(4000);
            changeState(STATE_ROVER_NOT_STARTED); //Give up and return to Bluetooth
          }
          else
          {
            Serial.print(".");
          }
#endif
        }
        break;

      case (STATE_ROVER_CLIENT_WIFI_CONNECTED):
        {
          //Open connection to caster service
#ifdef COMPILE_WIFI
          if (ntripClientConnect())
          {
            casterResponseWaitStartTime = millis();
            changeState(STATE_ROVER_CLIENT_STARTED);
          }
          else
          {
            log_d("Caster failed to connect. Trying again.");

            if (ntripClientConnectLimitReached())
            {
              Serial.println(F("Caster failed to connect. Do you have your caster address and port correct?"));
              ntripClientStop();

              wifiStop(); //Turn off WiFi and release all resources
              startBluetooth(); //Turn on Bluetooth with 'Rover' name

              changeState(STATE_ROVER_NO_FIX); //Start rover without WiFi
            }
          }
#endif
        }
        break;

      case (STATE_ROVER_CLIENT_STARTED):
        {
#ifdef COMPILE_WIFI
          //Check for no response from the caster service
          if (ntripClientReceiveDataAvailable() == 0)
          {
            //Check for response timeout
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              ntripClientStop();

              wifiStop(); //Turn off WiFi and release all resources
              startBluetooth(); //Turn on Bluetooth with 'Rover' name

              changeState(STATE_ROVER_NO_FIX); //Start rover without WiFi
            }
          }
          // Caster service has provided a response
          else
          {
            //Check reply
            char response[512];
            ntripClientResponse(&response[0], sizeof(response));

            //Look for '200 OK'
            if (strstr(response, "200") != NULL)
            {
              log_d("Connected to caster");

              lastReceivedRTCM_ms = millis(); //Reset timeout

              //We don't use a task because we use I2C hardware (and don't have a semphore).
              online.ntripClient = true;

              changeState(STATE_ROVER_NO_FIX); //Start rover *with* WiFi
            }
            else
            {
              //Look for '401 Unauthorized'
              Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?\n\r", response);
              ntripClientStop();

              wifiStop(); //Turn off WiFi and release all resources
              startBluetooth(); //Turn on Bluetooth with 'Rover' name

              changeState(STATE_ROVER_NO_FIX); //Start rover without WiFi
            }
          }
#endif
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
                                          |
                                          | NTRIP enabled = true
                                          V
                        .-----------------------------------.
                        |    STATE_BASE_TEMP_WIFI_STARTED   |
                        |         Blinking WiFi Icon        |
                        |             "Xmitting"            |
                        |              "RTCM: 0"            |
                        '-----------------------------------'
                                          |
                                          | WiFi connected = true
                                          | radioState = WIFI_CONNECTED
                                          V
                        .-----------------------------------.
                        |   STATE_BASE_TEMP_WIFI_CONNECTED  |
        .--------------->|          Solid WiFi Icon          |
        |                |             "Xmitting"            |
        |                |            "RTCM: 2145"           |
        |                '-----------------------------------'
        |                                  |
        |                                  | Caster enabled
        |                                  V
        |                .-----------------------------------.
        |                |   STATE_BASE_TEMP_CASTER_STARTED  |
        |  Caster failed |          Solid WiFi Icon          |
        +<---------------|            "Connecting"           |
        ^  Authorization |            "RTCM: 2145"           |
        |         failed '-----------------------------------'
        |                                  |
        |                                  | Caster connected
        |                                  V
        |                .-----------------------------------.
        |  Caster failed |  STATE_BASE_TEMP_CASTER_CONNECTED |
        '----------------|          Solid WiFi Icon          |
                        |             "Casting"             |
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
          stopBluetooth();
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
              startBluetooth();
              changeState(STATE_BASE_TEMP_SETTLE);
            }
            else if (settings.fixedBase == true)
            {
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
          if (settings.enableNtripServer == true)
          {
            //Turn off Bluetooth and turn on WiFi
            stopBluetooth();
            wifiStart(settings.ntripServer_wifiSSID, settings.ntripServer_wifiPW);

            changeState(STATE_BASE_TEMP_WIFI_STARTED);
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_TEMP_WIFI_STARTED):
        {
#ifdef COMPILE_WIFI
          byte wifiStatus = wifiGetStatus();
          if (wifiStatus == WL_CONNECTED)
          {
            wifiState = WIFI_CONNECTED;

            changeState(STATE_BASE_TEMP_WIFI_CONNECTED);
          }
          else
          {
            Serial.print(F("WiFi Status: "));
            switch (wifiStatus) {
              case WL_NO_SSID_AVAIL:
                Serial.printf("SSID '%s' not detected\n\r", settings.ntripServer_wifiSSID);
                break;
              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
            }
          }
#endif
        }
        break;

      case (STATE_BASE_TEMP_WIFI_CONNECTED):
        {
          if (productVariant == RTK_SURVEYOR)
          {
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
          }

          if (settings.enableNtripServer == true)
          {
            //Open connection to caster service
#ifdef COMPILE_WIFI
            if (ntripServer.connect(settings.ntripServer_CasterHost, settings.ntripServer_CasterPort) == true) //Attempt connection
            {
              changeState(STATE_BASE_TEMP_CASTER_STARTED);

              Serial.printf("Connected to %s:%d\n\r", settings.ntripServer_CasterHost, settings.ntripServer_CasterPort);

              const int SERVER_BUFFER_SIZE = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/v%d.%d\r\n\r\n",
                       settings.ntripServer_MountPointPW, settings.ntripServer_MountPoint, platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

              //Serial.printf("Sending credentials:\n%s\n\r", serverBuffer);
              ntripServer.write(serverBuffer, strlen(serverBuffer));

              casterResponseWaitStartTime = millis();
            }
#endif
          }
        }
        break;

      //Wait for response for caster service and make sure it's valid
      case (STATE_BASE_TEMP_CASTER_STARTED):
        {
#ifdef COMPILE_WIFI
          //Check if caster service responded
          if (ntripServer.available() == 0)
          {
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              ntripServer.stop();

              changeState(STATE_BASE_TEMP_WIFI_CONNECTED); //Return to previous state
            }
          }
          else
          {
            //Check reply
            bool connectionSuccess = false;
            char response[512];
            int responseSpot = 0;
            while (ntripServer.available())
            {
              response[responseSpot++] = ntripServer.read();
              if (strstr(response, "200") != NULL) //Look for 'ICY 200 OK'
                connectionSuccess = true;
              if (responseSpot == 512 - 1) break;
            }
            response[responseSpot] = '\0';
            //Serial.printf("Caster responded with: %s\n\r", response);

            if (connectionSuccess == false)
            {
              Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?\n\r", response);
              changeState(STATE_BASE_TEMP_WIFI_CONNECTED); //Return to previous state
            }
            else
            {
              //We're connected!
              //Serial.println(F("Connected to caster"));

              //Reset flags
              lastServerReport_ms = millis();
              lastServerSent_ms = millis();
              casterBytesSent = 0;

              rtcmPacketsSent = 0; //Reset any previous number

              changeState(STATE_BASE_TEMP_CASTER_CONNECTED);
            }
          }
#endif
        }
        break;

      //Monitor connected state
      case (STATE_BASE_TEMP_CASTER_CONNECTED):
        {
          cyclePositionLEDs();

#ifdef COMPILE_WIFI
          if (ntripServer.connected() == false)
          {
            Serial.println(F("Caster no longer connected. Reconnecting..."));
            changeState(STATE_BASE_TEMP_WIFI_CONNECTED); //Return to 2 earlier states to try to reconnect
          }
#endif
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
                                          |
                                          | NTRIP enabled = true
                                          | Stop Bluetooth
                                          | Start WiFi
                                          V
                        .-----------------------------------.
                        |   STATE_BASE_FIXED_WIFI_STARTED   |
                        |         Blinking WiFi Icon        |
                        |             "Xmitting"            |
                        |             "RTCM: 0"             |
                        '-----------------------------------'
                                          |
                                          | WiFi connected
                                          | radioState = WIFI_CONNECTED
                                          V
                        .-----------------------------------.
                        |  STATE_BASE_FIXED_WIFI_CONNECTED  |
           .----------->|          Solid WiFi Icon          |
           |            |             "Xmitting"            |
           |            |            "RTCM: 2145"           |
           |            '-----------------------------------'
           |                              |
           |                              | Caster enabled
           |                              V
           |            .-----------------------------------.
           |     Caster |  STATE_BASE_FIXED_CASTER_STARTED  |
           | Connection |          Solid WiFi Icon          |
           |     Failed |             "Xmitting"            |
           +------------|            "RTCM: 2145"           |
           ^     Failed '-----------------------------------'
           |  Authroization               |
           |                              | Caster connected
           |                              V
           |            .-----------------------------------.
           |     Caster |  STATE_BASE_FIXED_WIFI_CONNECTED  |
           | Connection |          Solid WiFi Icon          |
           |     Failed |             "Casting"             |
           '------------|            "RTCM: 2145"           |
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
          if (settings.enableNtripServer == true)
          {
            //Turn off Bluetooth and turn on WiFi
            stopBluetooth();
            wifiStart(settings.ntripServer_wifiSSID, settings.ntripServer_wifiPW);

            rtcmPacketsSent = 0; //Reset any previous number

            changeState(STATE_BASE_FIXED_WIFI_STARTED);
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_FIXED_WIFI_STARTED):
        {
#ifdef COMPILE_WIFI
          byte wifiStatus = wifiGetStatus();
          if (wifiStatus == WL_CONNECTED)
          {
            wifiState = WIFI_CONNECTED;

            changeState(STATE_BASE_FIXED_WIFI_CONNECTED);
          }
          else
          {
            Serial.print(F("WiFi Status: "));
            switch (wifiStatus) {
              case WL_NO_SSID_AVAIL:
                Serial.printf("SSID '%s' not detected\n\r", settings.ntripServer_wifiSSID);
                break;
              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
            }
          }
#endif
        }
        break;

      case (STATE_BASE_FIXED_WIFI_CONNECTED):
        {
          if (productVariant == RTK_SURVEYOR)
          {
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
          }
          if (settings.enableNtripServer == true)
          {
#ifdef COMPILE_WIFI
            //Open connection to caster service
            if (ntripServer.connect(settings.ntripServer_CasterHost, settings.ntripServer_CasterPort) == true) //Attempt connection
            {
              changeState(STATE_BASE_FIXED_CASTER_STARTED);

              Serial.printf("Connected to %s:%d\n\r", settings.ntripServer_CasterHost, settings.ntripServer_CasterPort);

              const int SERVER_BUFFER_SIZE  = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/v%d.%d\r\n\r\n",
                       settings.ntripServer_MountPointPW, settings.ntripServer_MountPoint, platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

              //Serial.printf("Sending credentials:\n%s\n\r", serverBuffer);
              ntripServer.write(serverBuffer, strlen(serverBuffer));

              casterResponseWaitStartTime = millis();
            }
#endif
          }
        }
        break;

      //Wait for response for caster service and make sure it's valid
      case (STATE_BASE_FIXED_CASTER_STARTED):
        {
#ifdef COMPILE_WIFI
          //Check if caster service responded
          if (ntripServer.available() < 10)
          {
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              ntripServer.stop();
              delay(10); //Yield to RTOS

              changeState(STATE_BASE_FIXED_WIFI_CONNECTED); //Return to previous state
            }
          }
          else
          {
            //Check reply
            bool connectionSuccess = false;
            char response[512];
            int responseSpot = 0;
            while (ntripServer.available())
            {
              response[responseSpot++] = ntripServer.read();
              if (strstr(response, "200") != NULL) //Look for 'ICY 200 OK'
                connectionSuccess = true;
              if (responseSpot == 512 - 1) break;
            }
            response[responseSpot] = '\0';
            //Serial.printf("Caster responded with: %s\n\r", response);

            if (connectionSuccess == false)
            {
              Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?", response);
              changeState(STATE_BASE_FIXED_WIFI_CONNECTED); //Return to previous state
            }
            else
            {
              //We're connected!
              //Serial.println(F("Connected to caster"));

              //Reset flags
              lastServerReport_ms = millis();
              lastServerSent_ms = millis();
              casterBytesSent = 0;

              rtcmPacketsSent = 0; //Reset any previous number

              changeState(STATE_BASE_FIXED_CASTER_CONNECTED);
            }
          }
#endif
        }
        break;

      //Monitor connected state
      case (STATE_BASE_FIXED_CASTER_CONNECTED):
        {
          cyclePositionLEDs();

#ifdef COMPILE_WIFI
          if (ntripServer.connected() == false)
          {
            changeState(STATE_BASE_FIXED_WIFI_CONNECTED);
          }
#endif
        }
        break;

      case (STATE_BUBBLE_LEVEL):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_PROFILE_1):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_PROFILE_2):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_PROFILE_3):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_PROFILE_4):
        {
          //Do nothing - display only
        }
        break;

      case (STATE_MARK_EVENT):
        {
          //Record this event to the log
          if (online.logging == true)
          {
            char nmeaMessage[82]; //Max NMEA sentence length is 82
            createNMEASentence(CUSTOM_NMEA_TYPE_WAYPOINT, nmeaMessage, (char*)"CustomEvent"); //textID, buffer, text
            ubxFile.println(nmeaMessage);
            displayEventMarked(500); //Show 'Event Marked'
          }
          else
            displayNoLogging(500); //Show 'No Logging'

          changeState(lastSystemState);
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

          stopBluetooth();
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
  if (newState == systemState)
    Serial.print(F("*"));

  reportHeapNow();
  systemState = newState;

  //Debug print
  switch (systemState)
  {
    case (STATE_ROVER_NOT_STARTED):
      Serial.println(F("State: Rover - Not Started"));
      break;
    case (STATE_ROVER_NO_FIX):
      Serial.println(F("State: Rover - No Fix"));
      break;
    case (STATE_ROVER_FIX):
      Serial.println(F("State: Rover - Fix"));
      break;
    case (STATE_ROVER_RTK_FLOAT):
      Serial.println(F("State: Rover - RTK Float"));
      break;
    case (STATE_ROVER_RTK_FIX):
      Serial.println(F("State: Rover - RTK Fix"));
      break;
    case (STATE_ROVER_CLIENT_WIFI_STARTED):
      Serial.println(F("State: Rover - Client WiFi Started"));
      break;
    case (STATE_ROVER_CLIENT_WIFI_CONNECTED):
      Serial.println(F("State: Rover - Client WiFi Connected"));
      break;
    case (STATE_ROVER_CLIENT_STARTED):
      Serial.println(F("State: Rover - Client Started"));
      break;
    case (STATE_BASE_NOT_STARTED):
      Serial.println(F("State: Base - Not Started"));
      break;
    case (STATE_BASE_TEMP_SETTLE):
      Serial.println(F("State: Base-Temp - Settle"));
      break;
    case (STATE_BASE_TEMP_SURVEY_STARTED):
      Serial.println(F("State: Base-Temp - Survey Started"));
      break;
    case (STATE_BASE_TEMP_TRANSMITTING):
      Serial.println(F("State: Base-Temp - Transmitting"));
      break;
    case (STATE_BASE_TEMP_WIFI_STARTED):
      Serial.println(F("State: Base-Temp - WiFi Started"));
      break;
    case (STATE_BASE_TEMP_WIFI_CONNECTED):
      Serial.println(F("State: Base-Temp - WiFi Connected"));
      break;
    case (STATE_BASE_TEMP_CASTER_STARTED):
      Serial.println(F("State: Base-Temp - Caster Started"));
      break;
    case (STATE_BASE_TEMP_CASTER_CONNECTED):
      Serial.println(F("State: Base-Temp - Caster Connected"));
      break;
    case (STATE_BASE_FIXED_NOT_STARTED):
      Serial.println(F("State: Base-Fixed - Not Started"));
      break;
    case (STATE_BASE_FIXED_TRANSMITTING):
      Serial.println(F("State: Base-Fixed - Transmitting"));
      break;
    case (STATE_BASE_FIXED_WIFI_STARTED):
      Serial.println(F("State: Base-Fixed - WiFi Started"));
      break;
    case (STATE_BASE_FIXED_WIFI_CONNECTED):
      Serial.println(F("State: Base-Fixed - WiFi Connected"));
      break;
    case (STATE_BASE_FIXED_CASTER_STARTED):
      Serial.println(F("State: Base-Fixed - Caster Started"));
      break;
    case (STATE_BASE_FIXED_CASTER_CONNECTED):
      Serial.println(F("State: Base-Fixed - Caster Connected"));
      break;
    case (STATE_BUBBLE_LEVEL):
      Serial.println(F("State: Bubble level"));
      break;
    case (STATE_MARK_EVENT):
      Serial.println(F("State: Mark Event"));
      break;
    case (STATE_DISPLAY_SETUP):
      Serial.println(F("State: Display Setup"));
      break;
    case (STATE_WIFI_CONFIG_NOT_STARTED):
      Serial.println(F("State: WiFi Config Not Started"));
      break;
    case (STATE_WIFI_CONFIG):
      Serial.println(F("State: WiFi Config"));
      break;
    case (STATE_TEST):
      Serial.println(F("State: System Test Setup"));
      break;
    case (STATE_TESTING):
      Serial.println(F("State: System Testing"));
      break;
    case (STATE_PROFILE_1):
      Serial.println(F("State: Profile 1"));
      break;
    case (STATE_PROFILE_2):
      Serial.println(F("State: Profile 2"));
      break;
    case (STATE_PROFILE_3):
      Serial.println(F("State: Profile 3"));
      break;
    case (STATE_PROFILE_4):
      Serial.println(F("State: Profile 4"));
      break;
    case (STATE_KEYS_STARTED):
      Serial.println(F("State: Keys Started "));
      break;
    case (STATE_KEYS_NEEDED):
      Serial.println(F("State: Keys Needed"));
      break;
    case (STATE_KEYS_WIFI_STARTED):
      Serial.println(F("State: Keys WiFi Started"));
      break;
    case (STATE_KEYS_WIFI_CONNECTED):
      Serial.println(F("State: Keys WiFi Connected"));
      break;
    case (STATE_KEYS_WIFI_TIMEOUT):
      Serial.println(F("State: Keys WiFi Timeout"));
      break;
    case (STATE_KEYS_EXPIRED):
      Serial.println(F("State: Keys Expired"));
      break;
    case (STATE_KEYS_DAYS_REMAINING):
      Serial.println(F("State: Keys Days Remaining"));
      break;
    case (STATE_KEYS_LBAND_CONFIGURE):
      Serial.println(F("State: Keys L-Band Configure"));
      break;
    case (STATE_KEYS_LBAND_ENCRYPTED):
      Serial.println(F("State: Keys L-Band Encrypted"));
      break;
    case (STATE_KEYS_PROVISION_WIFI_STARTED):
      Serial.println(F("State: Keys Provision - WiFi Started"));
      break;
    case (STATE_KEYS_PROVISION_WIFI_CONNECTED):
      Serial.println(F("State: Keys Provision - WiFi Connected"));
      break;
    case (STATE_KEYS_PROVISION_WIFI_TIMEOUT):
      Serial.println(F("State: Keys Provision - WiFi Timeout"));
      break;

    case (STATE_SHUTDOWN):
      Serial.println(F("State: Shut Down"));
      break;
    default:
      Serial.printf("Change State Unknown: %d\n\r", systemState);
      break;
  }
}
