/*
  This is the main state machine for the device. It's big but controls each step of the system.
  See system state chart document for a visual representation of how states can change to/from.
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
      case (STATE_ROVER_NOT_STARTED):
        {
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

          stopWiFi(); //Turn off WiFi and release all resources
          startBluetooth(); //Turn on Bluetooth with 'Rover' name
          startUART2Tasks(); //Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)

          settings.lastState = STATE_ROVER_NOT_STARTED;
          recordSystemSettings();

          displayRoverSuccess(500);

          changeState(STATE_ROVER_NO_FIX);
          firstRoverStart = false; //Do not allow entry into test menu again
        }
        break;

      case (STATE_ROVER_NO_FIX):
        {
          if (i2cGNSS.getFixType() == 3 || i2cGNSS.getFixType() == 4) //3D, 3D+DR
            changeState(STATE_ROVER_FIX);
        }
        break;

      case (STATE_ROVER_FIX):
        {
          updateAccuracyLEDs();

          byte rtkType = i2cGNSS.getCarrierSolutionType();
          if (rtkType == 1) //RTK Float
            changeState(STATE_ROVER_RTK_FLOAT);
          else if (rtkType == 2) //RTK Fix
            changeState(STATE_ROVER_RTK_FIX);
        }
        break;

      case (STATE_ROVER_RTK_FLOAT):
        {
          updateAccuracyLEDs();

          byte rtkType = i2cGNSS.getCarrierSolutionType();
          if (rtkType == 0) //No RTK
            changeState(STATE_ROVER_FIX);
          if (rtkType == 2) //RTK Fix
            changeState(STATE_ROVER_RTK_FIX);
        }
        break;

      case (STATE_ROVER_RTK_FIX):
        {
          updateAccuracyLEDs();

          byte rtkType = i2cGNSS.getCarrierSolutionType();
          if (rtkType == 0) //No RTK
            changeState(STATE_ROVER_FIX);
          if (rtkType == 1) //RTK Float
            changeState(STATE_ROVER_RTK_FLOAT);
        }
        break;

      case (STATE_BASE_NOT_STARTED):
        {
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
          stopWiFi();
          stopBluetooth();
          startUART2Tasks(); //Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)

          if (configureUbloxModuleBase() == true)
          {
            settings.lastState = STATE_BASE_NOT_STARTED; //Record this state for next POR
            recordSystemSettings();

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
          uint32_t accuracy = i2cGNSS.getHorizontalAccuracy();

          float f_accuracy = accuracy;
          f_accuracy = f_accuracy / 10000.0; // Convert the horizontal accuracy (mm * 10^-1) to a float

          Serial.printf("Waiting for Horz Accuracy < %0.2f meters: %0.2f\n\r", settings.surveyInStartingAccuracy, f_accuracy);

          if (f_accuracy > 0.0 && f_accuracy < settings.surveyInStartingAccuracy)
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
          svinObservationTime = i2cGNSS.getSurveyInObservationTime(100);
          svinMeanAccuracy = i2cGNSS.getSurveyInMeanAccuracy(100);

          if (i2cGNSS.getSurveyInValid(100) == true) //Survey in complete
          {
            Serial.printf("obs time: %d\n\r", svinObservationTime);
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
            Serial.print(i2cGNSS.getSIV());
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
            startWiFi();

            changeState(STATE_BASE_TEMP_WIFI_STARTED);
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_TEMP_WIFI_STARTED):
        {
#ifdef COMPILE_WIFI
          byte wifiStatus = WiFi.status();
          if (wifiStatus == WL_CONNECTED)
          {
            radioState = WIFI_CONNECTED;

            changeState(STATE_BASE_TEMP_WIFI_CONNECTED);
          }
          else
          {
            Serial.print(F("WiFi Status: "));
            switch (wifiStatus) {
              case WL_NO_SSID_AVAIL:
                Serial.printf("SSID '%s' not detected\n\r", settings.wifiSSID);
                break;
              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
            }
            delay(500);
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
            if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
            {
              changeState(STATE_BASE_TEMP_CASTER_STARTED);

              Serial.printf("Connected to %s:%d\n\r", settings.casterHost, settings.casterPort);

              const int SERVER_BUFFER_SIZE = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/v%d.%d\r\n\r\n",
                       settings.mountPointPW, settings.mountPoint, platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

              //Serial.printf("Sending credentials:\n%s\n\r", serverBuffer);
              caster.write(serverBuffer, strlen(serverBuffer));

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
          if (caster.available() == 0)
          {
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              caster.stop();

              changeState(STATE_BASE_TEMP_WIFI_CONNECTED); //Return to previous state
            }
          }
          else
          {
            //Check reply
            bool connectionSuccess = false;
            char response[512];
            int responseSpot = 0;
            while (caster.available())
            {
              response[responseSpot++] = caster.read();
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
          if (caster.connected() == false)
          {
            Serial.println(F("Caster no longer connected. Reconnecting..."));
            changeState(STATE_BASE_TEMP_WIFI_CONNECTED); //Return to 2 earlier states to try to reconnect
          }
#endif
        }
        break;

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
            startWiFi();

            rtcmPacketsSent = 0; //Reset any previous number

            changeState(STATE_BASE_FIXED_WIFI_STARTED);
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_FIXED_WIFI_STARTED):
        {
#ifdef COMPILE_WIFI
          byte wifiStatus = WiFi.status();
          if (wifiStatus == WL_CONNECTED)
          {
            radioState = WIFI_CONNECTED;

            changeState(STATE_BASE_FIXED_WIFI_CONNECTED);
          }
          else
          {
            Serial.print(F("WiFi Status: "));
            switch (wifiStatus) {
              case WL_NO_SSID_AVAIL:
                Serial.printf("SSID '%s' not detected\n\r", settings.wifiSSID);
                break;
              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
            }
            delay(500);
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
            if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
            {
              changeState(STATE_BASE_FIXED_CASTER_STARTED);

              Serial.printf("Connected to %s:%d\n\r", settings.casterHost, settings.casterPort);

              const int SERVER_BUFFER_SIZE  = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP SparkFun_RTK_%s/v%d.%d\r\n\r\n",
                       settings.mountPointPW, settings.mountPoint, platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

              //Serial.printf("Sending credentials:\n%s\n\r", serverBuffer);
              caster.write(serverBuffer, strlen(serverBuffer));

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
          if (caster.available() < 10)
          {
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              caster.stop();
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
            while (caster.available())
            {
              response[responseSpot++] = caster.read();
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
          if (caster.connected() == false)
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

      case (STATE_MARK_EVENT):
        {
          //Record this event to the log
          if (online.logging == true)
          {
            char nmeaMessage[82]; //Max NMEA sentence length is 82
            createNMEASentence(1, 2, nmeaMessage, (char*)"CustomEvent"); //sentenceNumber, textID, buffer, text
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

          //Start in AP mode and show config html page
          startConfigAP();

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
  //If we are leaving WiFi config, record and implement settings
  if (systemState == STATE_WIFI_CONFIG)
    recordSystemSettings(); //Record the new settings to EEPROM and config file

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
    case (STATE_SHUTDOWN):
      Serial.println(F("State: Shut Down"));
      break;
    default:
      Serial.printf("Change State Unknown: %d\n\r", systemState);
      break;
  }
}
