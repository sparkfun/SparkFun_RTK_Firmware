/*
  This is the main state machine for the device. It's big but controls each step of the system.
  See system state chart document for a visual representation of how states can change to/from.
*/

//Given the current state, see if conditions have moved us to a new state
//A user pressing the setup button (change between rover/base) is handled by checkSetupButton()
void updateSystemState()
{
  if (millis() - lastSystemStateUpdate > 500)
  {
    lastSystemStateUpdate = millis();

    //Move between states as needed
    switch (systemState)
    {
      case (STATE_ROVER_NOT_STARTED):
        {
          //Configure for rover mode
          displayRoverStart(0);

          //If we are survey'd in, but switch is rover then disable survey
          if (configureUbloxModuleRover() == false)
          {
            Serial.println(F("Rover config failed"));
            displayRoverFail(1000);
            return;
          }

          beginBluetooth(); //Restart Bluetooth with 'Rover' name

          digitalWrite(baseStatusLED, LOW);
          displayRoverSuccess(500);

          changeState(STATE_ROVER_NO_FIX);
        }
        break;

      case (STATE_ROVER_NO_FIX):
        {
          if (i2cGNSS.getFixType() == 3) //3D
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
          digitalWrite(baseStatusLED, LOW);
          digitalWrite(positionAccuracyLED_1cm, LOW);
          digitalWrite(positionAccuracyLED_10cm, LOW);
          digitalWrite(positionAccuracyLED_100cm, LOW);

          displayBaseStart(0); //Show 'Base'

          //Restart Bluetooth with 'Base' name
          //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
          beginBluetooth();

          if (configureUbloxModuleBase() == true)
          {
            displayBaseSuccess(500); //Show 'Base Started'

            if (settings.fixedBase == false)
            {
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
            digitalWrite(baseStatusLED, !digitalRead(baseStatusLED));
          }

          //Check for <1m horz accuracy before starting surveyIn
          uint32_t accuracy = i2cGNSS.getHorizontalAccuracy();

          float f_accuracy = accuracy;
          f_accuracy = f_accuracy / 10000.0; // Convert the horizontal accuracy (mm * 10^-1) to a float

          Serial.printf("Waiting for Horz Accuracy < %0.2f meters: %0.2f\n", settings.surveyInStartingAccuracy, f_accuracy);

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
            digitalWrite(baseStatusLED, !digitalRead(baseStatusLED));
          }

          if (i2cGNSS.getSurveyInValid() == true) //Survey in complete
          {
            Serial.println(F("Base survey complete! RTCM now broadcasting."));
            digitalWrite(baseStatusLED, HIGH); //Indicate survey complete

            rtcmPacketsSent = 0; //Reset any previous number
            changeState(STATE_BASE_TEMP_TRANSMITTING);
          }
          else
          {
            Serial.print(F("Time elapsed: "));
            Serial.print((String)i2cGNSS.getSurveyInObservationTime());
            Serial.print(F(" Accuracy: "));
            Serial.print((String)i2cGNSS.getSurveyInMeanAccuracy());
            Serial.print(F(" SIV: "));
            Serial.print(i2cGNSS.getSIV());
            Serial.println();

            if (i2cGNSS.getSurveyInObservationTime() > maxSurveyInWait_s)
            {
              Serial.printf("Survey-In took more than %d minutes. Returning to rover mode.\n", maxSurveyInWait_s / 60);

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
            endBluetooth();

            Serial.printf("Connecting to local WiFi: %s\n", settings.wifiSSID);
            WiFi.begin(settings.wifiSSID, settings.wifiPW);

            radioState = WIFI_ON_NOCONNECTION;
            changeState(STATE_BASE_TEMP_WIFI_STARTED);
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_TEMP_WIFI_STARTED):
        {
          byte wifiStatus = WiFi.status();
          if (wifiStatus == WL_CONNECTED)
          {
            radioState = WIFI_CONNECTED;

            changeState(STATE_BASE_TEMP_WIFI_CONNECTED);
          }
          //          else
          //          {
          //            Serial.print(F("WiFi Status: "));
          //            switch (wifiStatus) {
          //              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
          //              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
          //              case WL_NO_SSID_AVAIL: Serial.println(F("WL_NO_SSID_AVAIL")); break;
          //              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
          //              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
          //              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
          //              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
          //              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
          //            }
          //          }
        }
        break;

      case (STATE_BASE_TEMP_WIFI_CONNECTED):
        {
          if (settings.enableNtripServer == true)
          {
            //Open connection to caster service
            if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
            {
              changeState(STATE_BASE_TEMP_CASTER_STARTED);

              Serial.printf("Connected to %s:%d\n", settings.casterHost, settings.casterPort);

              const int SERVER_BUFFER_SIZE  = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/%s\r\n\r\n",
                       settings.mountPointPW, settings.mountPoint, ntrip_server_name, "App Version 1.0");

              //Serial.printf("Sending credentials:\n%s\n", serverBuffer);
              caster.write(serverBuffer, strlen(serverBuffer));

              casterResponseWaitStartTime = millis();
            }
          }
        }
        break;

      //Wait for response for caster service and make sure it's valid
      case (STATE_BASE_TEMP_CASTER_STARTED):
        {
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
            //Serial.printf("Caster responded with: %s\n", response);

            if (connectionSuccess == false)
            {
              Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?\n", response);
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
        }
        break;

      //Monitor connected state
      case (STATE_BASE_TEMP_CASTER_CONNECTED):
        {
          if (caster.connected() == false)
          {
            Serial.println(F("Caster no longer connected. Reconnecting..."));
            changeState(STATE_BASE_TEMP_WIFI_CONNECTED); //Return to 2 earlier states to try to reconnect
          }
        }
        break;

      //User has set switch to base with fixed option enabled. Let's configure and try to get there.
      //If fixed base fails, we'll handle it here
      case (STATE_BASE_FIXED_NOT_STARTED):
        {
          bool response = startFixedBase();
          if (response == true)
          {
            digitalWrite(baseStatusLED, HIGH); //Turn on base LED

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
            endBluetooth();

            Serial.printf("Connecting to local WiFi: %s", settings.wifiSSID);
            WiFi.begin(settings.wifiSSID, settings.wifiPW);

            radioState = WIFI_ON_NOCONNECTION;

            rtcmPacketsSent = 0; //Reset any previous number

            changeState(STATE_BASE_FIXED_WIFI_STARTED);
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_FIXED_WIFI_STARTED):
        {
          byte wifiStatus = WiFi.status();
          if (wifiStatus == WL_CONNECTED)
          {
            radioState = WIFI_CONNECTED;

            changeState(STATE_BASE_FIXED_WIFI_CONNECTED);
          }
          //          else
          //          {
          //            Serial.print(F("WiFi Status: "));
          //            switch (wifiStatus) {
          //              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
          //              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
          //              case WL_NO_SSID_AVAIL: Serial.println(F("WL_NO_SSID_AVAIL")); break;
          //              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
          //              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
          //              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
          //              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
          //              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
          //            }
          //          }
        }
        break;

      case (STATE_BASE_FIXED_WIFI_CONNECTED):
        {
          if (settings.enableNtripServer == true)
          {
            //Open connection to caster service
            if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
            {
              changeState(STATE_BASE_FIXED_CASTER_STARTED);

              Serial.printf("Connected to %s:%d\n", settings.casterHost, settings.casterPort);

              const int SERVER_BUFFER_SIZE  = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/%s\r\n\r\n",
                       settings.mountPointPW, settings.mountPoint, ntrip_server_name, "App Version 1.0");

              //Serial.printf("Sending credentials:\n%s\n", serverBuffer);
              caster.write(serverBuffer, strlen(serverBuffer));

              casterResponseWaitStartTime = millis();
            }
          }
        }
        break;

      //Wait for response for caster service and make sure it's valid
      case (STATE_BASE_FIXED_CASTER_STARTED):
        {
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
            //Serial.printf("Caster responded with: %s\n", response);

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
        }
        break;

      //Monitor connected state
      case (STATE_BASE_FIXED_CASTER_CONNECTED):
        {
          if (caster.connected() == false)
          {
            changeState(STATE_BASE_FIXED_WIFI_CONNECTED);
          }
        }
        break;

    }
  }
}

//Change states and print the new state
void changeState(SystemState newState)
{
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
    default:
      Serial.printf("State Unknown: %d\n", systemState);
      break;
  }
}
