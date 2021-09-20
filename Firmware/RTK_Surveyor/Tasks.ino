//High frequency tasks made by xTaskCreate()
//And any low frequency tasks that are called by Ticker

//If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
//Task for writing to the GNSS receiver
void F9PSerialWriteTask(void *e)
{
  while (true)
  {
#ifdef COMPILE_BT
    //Receive RTCM corrections or UBX config messages over bluetooth and pass along to ZED
    if (radioState == BT_CONNECTED)
    {
      while (SerialBT.available())
      {
        if (inTestMode == false)
        {
          //Pass bytes to GNSS receiver
          auto s = SerialBT.readBytes(wBuffer, SERIAL_SIZE_RX);
          serialGNSS.write(wBuffer, s);

          if (settings.enableTaskReports == true)
            Serial.printf("SerialWriteTask High watermark: %d\n\r",  uxTaskGetStackHighWaterMark(NULL));
        }
        else
        {
          char incoming = SerialBT.read();
          Serial.printf("I heard: %c\n", incoming);
          incomingBTTest = incoming; //Displayed during system test
        }
        delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
        taskYIELD();
      }
    }
#endif

    delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
    taskYIELD();
  }
}

//If the ZED has any new NMEA data, pass it out over Bluetooth
//Task for reading data from the GNSS receiver.
void F9PSerialReadTask(void *e)
{
  while (true)
  {
    while (serialGNSS.available())
    {
      auto s = serialGNSS.readBytes(rBuffer, SERIAL_SIZE_RX);

      //If we are actively survey-in then do not pass NMEA data from ZED to phone
      if (systemState == STATE_BASE_TEMP_SETTLE || systemState == STATE_BASE_TEMP_SURVEY_STARTED)
      {
        //Do nothing
        taskYIELD();
      }
#ifdef COMPILE_BT
      else if (radioState == BT_CONNECTED)
      {
        if (SerialBT.isCongested() == false)
        {
          SerialBT.write(rBuffer, s); //Push new data to BT SPP
        }
        else if (settings.throttleDuringSPPCongestion == false)
        {
          SerialBT.write(rBuffer, s); //Push new data to SPP regardless of congestion
        }
        else
        {
          //Don't push data to BT SPP if there is congestion to prevent heap hits.
          log_d("Dropped SPP Bytes: %d", s);
        }
      }
#endif

      if (settings.enableTaskReports == true)
        Serial.printf("SerialReadTask High watermark: %d\n\r",  uxTaskGetStackHighWaterMark(NULL));

      //If user wants to log, record to SD
      if (online.logging == true)
      {
        //Check if we are inside the max time window for logging
        if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
        {
          //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile()
          if (xSemaphoreTake(xFATSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
          {
            ubxFile.write(rBuffer, s);

            //Force file sync every 5000ms
            if (millis() - lastUBXLogSyncTime > 5000)
            {
              if (productVariant == RTK_SURVEYOR)
                digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Blink LED to indicate logging activity

              long startWriteTime = micros();
              taskYIELD();
              ubxFile.sync();
              taskYIELD();
              long stopWriteTime = micros();
              totalWriteTime += stopWriteTime - startWriteTime; //Used to calculate overall write speed

              if (productVariant == RTK_SURVEYOR)
                digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Return LED to previous state

              updateDataFileAccess(&ubxFile); // Update the file access time & date

              lastUBXLogSyncTime = millis();
            }

            xSemaphoreGive(xFATSemaphore);
          } //End xFATSemaphore
          else
          {
            log_d("F9SerialRead: Semaphore failed to yield");
          }
        } //End maxLogTime
      } //End logging

      delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
      taskYIELD();

    } //End Serial.available()

    delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
    taskYIELD();
  }
}



//Control BT status LED according to radioState
void updateBTled()
{
  if (productVariant == RTK_SURVEYOR)
  {
    if (radioState == BT_ON_NOCONNECTION)
    {
      //Blink on/off while we wait for BT connection
      if (btFadeLevel == 0) btFadeLevel = 255;
      else btFadeLevel = 0;
      ledcWrite(ledBTChannel, btFadeLevel);
    }
    else if (radioState == BT_CONNECTED)
      ledcWrite(ledBTChannel, 255);
    else if (radioState == WIFI_ON_NOCONNECTION || radioState == WIFI_CONNECTED)
    {
      //Fade in/out the BT LED during WiFi AP mode
      btFadeLevel += pwmFadeAmount;
      if (btFadeLevel <= 0 || btFadeLevel >= 255) pwmFadeAmount *= -1;

      if (btFadeLevel > 255) btFadeLevel = 255;
      if (btFadeLevel < 0) btFadeLevel = 0;

      ledcWrite(ledBTChannel, btFadeLevel);
    }
    else
      ledcWrite(ledBTChannel, 0);
  }
}

//For RTK Express and RTK Facet, monitor momentary buttons
void ButtonCheckTask(void *e)
{
  if (setupBtn != NULL) setupBtn->begin();
  if (powerBtn != NULL) powerBtn->begin();

  while (true)
  {
    if (productVariant == RTK_SURVEYOR)
    {
      setupBtn->read();

      //When switch is set to '1' = BASE, pin will be shorted to ground
      if (setupBtn->isPressed()) //Switch is set to base mode
      {
        if (buttonPreviousState == BUTTON_ROVER)
        {
          lastRockerSwitchChange = millis(); //Record for WiFi AP access
          buttonPreviousState = BUTTON_BASE;
          requestChangeState(STATE_BASE_NOT_STARTED);
        }
      }
      else if (setupBtn->wasReleased()) //Switch is set to Rover
      {
        if (buttonPreviousState == BUTTON_BASE)
        {
          buttonPreviousState = BUTTON_ROVER;

          //If quick toggle is detected (less than 500ms), enter WiFi AP Config mode
          if (millis() - lastRockerSwitchChange < 500)
          {
            requestChangeState(STATE_WIFI_CONFIG_NOT_STARTED);
          }
          else
          {
            requestChangeState(STATE_ROVER_NOT_STARTED);
          }
        }
      }
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS) //Express: Check both of the momentary switches
    {
      if (setupBtn != NULL) setupBtn->read();
      if (powerBtn != NULL) powerBtn->read();

      if (systemState == STATE_SHUTDOWN)
      {
        //Ignore button presses while shutting down
      }
      else if (powerBtn != NULL && powerBtn->pressedFor(shutDownButtonTime))
      {
        forceSystemStateUpdate = true;
        requestChangeState(STATE_SHUTDOWN);
      }
      else if ((setupBtn != NULL && setupBtn->pressedFor(500)) &&
               (powerBtn != NULL && powerBtn->pressedFor(500)))
      {
        forceSystemStateUpdate = true;
        requestChangeState(STATE_TEST);
        lastTestMenuChange = millis(); //Avoid exiting test menu for 1s
      }
      else if (setupBtn != NULL && setupBtn->wasReleased())
      {
        switch (systemState)
        {
          //If we are in any running state, change to STATE_DISPLAY_SETUP
          case STATE_ROVER_NOT_STARTED:
          case STATE_ROVER_NO_FIX:
          case STATE_ROVER_FIX:
          case STATE_ROVER_RTK_FLOAT:
          case STATE_ROVER_RTK_FIX:
          case STATE_BASE_NOT_STARTED:
          case STATE_BASE_TEMP_SETTLE:
          case STATE_BASE_TEMP_SURVEY_STARTED:
          case STATE_BASE_TEMP_TRANSMITTING:
          case STATE_BASE_TEMP_WIFI_STARTED:
          case STATE_BASE_TEMP_WIFI_CONNECTED:
          case STATE_BASE_TEMP_CASTER_STARTED:
          case STATE_BASE_TEMP_CASTER_CONNECTED:
          case STATE_BASE_FIXED_NOT_STARTED:
          case STATE_BASE_FIXED_TRANSMITTING:
          case STATE_BASE_FIXED_WIFI_STARTED:
          case STATE_BASE_FIXED_WIFI_CONNECTED:
          case STATE_BASE_FIXED_CASTER_STARTED:
          case STATE_BASE_FIXED_CASTER_CONNECTED:
          case STATE_BUBBLE_LEVEL:
          case STATE_WIFI_CONFIG_NOT_STARTED:
          case STATE_WIFI_CONFIG:
            lastSystemState = systemState; //Remember this state to return after we mark an event
            requestChangeState(STATE_DISPLAY_SETUP);
            setupState = STATE_MARK_EVENT;
            lastSetupMenuChange = millis();
            break;

          case STATE_MARK_EVENT:
            //If the user presses the setup button during a mark event, do nothing
            //Allow system to return to lastSystemState
            break;

          case STATE_TEST:
            //Do nothing. User is releasing the setup button.
            break;

          case STATE_TESTING:
            //If we are in testing, return to Rover Not Started
            requestChangeState(STATE_ROVER_NOT_STARTED);
            break;

          case STATE_DISPLAY_SETUP:
            //If we are displaying the setup menu, cycle through possible system states
            //Exit display setup and enter new system state after ~1500ms in updateSystemState()
            lastSetupMenuChange = millis();

            forceDisplayUpdate = true; //User is interacting so repaint display quickly

            switch (setupState)
            {
              case STATE_MARK_EVENT:
                setupState = STATE_ROVER_NOT_STARTED;
                break;
              case STATE_ROVER_NOT_STARTED:
                //If F9R, skip base state
                if (zedModuleType == PLATFORM_F9R)
                  setupState = STATE_BUBBLE_LEVEL;
                else
                  setupState = STATE_BASE_NOT_STARTED;
                break;
              case STATE_BASE_NOT_STARTED:
                setupState = STATE_BUBBLE_LEVEL;
                break;
              case STATE_BUBBLE_LEVEL:
                setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                break;
              case STATE_WIFI_CONFIG_NOT_STARTED:
                setupState = STATE_MARK_EVENT;
                break;
              default:
                Serial.printf("ButtonCheckTask unknown setup state: %d\n\r", setupState);
                setupState = STATE_MARK_EVENT;
                break;
            }
            break;

          default:
            Serial.printf("ButtonCheckTask unknown system state: %d\n\r", systemState);
            requestChangeState(STATE_ROVER_NOT_STARTED);
            break;
        }
      }
    } //End Platform = RTK Express
    else if (productVariant == RTK_FACET) //Check one momentary button
    {
      if (powerBtn != NULL) powerBtn->read();

      if (systemState == STATE_SHUTDOWN)
      {
        //Ignore button presses while shutting down
      }
      else if (powerBtn != NULL && powerBtn->pressedFor(shutDownButtonTime))
      {
        forceSystemStateUpdate = true;
        requestChangeState(STATE_SHUTDOWN);
      }
      //      else if ((setupBtn != NULL && setupBtn->pressedFor(500)) &&
      //               (powerBtn != NULL && powerBtn->pressedFor(500)))
      //      {
      //        forceSystemStateUpdate = true;
      //        requestChangeState(STATE_TEST);
      //      }
      else if (powerBtn != NULL && powerBtn->wasReleased())
      {
        switch (systemState)
        {
          //If we are in any running state, change to STATE_DISPLAY_SETUP
          case STATE_ROVER_NOT_STARTED:
          case STATE_ROVER_NO_FIX:
          case STATE_ROVER_FIX:
          case STATE_ROVER_RTK_FLOAT:
          case STATE_ROVER_RTK_FIX:
          case STATE_BASE_NOT_STARTED:
          case STATE_BASE_TEMP_SETTLE:
          case STATE_BASE_TEMP_SURVEY_STARTED:
          case STATE_BASE_TEMP_TRANSMITTING:
          case STATE_BASE_TEMP_WIFI_STARTED:
          case STATE_BASE_TEMP_WIFI_CONNECTED:
          case STATE_BASE_TEMP_CASTER_STARTED:
          case STATE_BASE_TEMP_CASTER_CONNECTED:
          case STATE_BASE_FIXED_NOT_STARTED:
          case STATE_BASE_FIXED_TRANSMITTING:
          case STATE_BASE_FIXED_WIFI_STARTED:
          case STATE_BASE_FIXED_WIFI_CONNECTED:
          case STATE_BASE_FIXED_CASTER_STARTED:
          case STATE_BASE_FIXED_CASTER_CONNECTED:
          case STATE_BUBBLE_LEVEL:
          case STATE_WIFI_CONFIG_NOT_STARTED:
          case STATE_WIFI_CONFIG:
            lastSystemState = systemState; //Remember this state to return after we mark an event
            requestChangeState(STATE_DISPLAY_SETUP);
            setupState = STATE_MARK_EVENT;
            lastSetupMenuChange = millis();
            break;

          case STATE_MARK_EVENT:
            //If the user presses the setup button during a mark event, do nothing
            //Allow system to return to lastSystemState
            break;

          case STATE_TEST:
            //Do nothing. User is releasing the setup button.
            break;

          case STATE_TESTING:
            //If we are in testing, return to Rover Not Started
            requestChangeState(STATE_ROVER_NOT_STARTED);
            break;

          case STATE_DISPLAY_SETUP:
            //If we are displaying the setup menu, cycle through possible system states
            //Exit display setup and enter new system state after ~1500ms in updateSystemState()
            lastSetupMenuChange = millis();

            forceDisplayUpdate = true; //User is interacting so repaint display quickly

            switch (setupState)
            {
              case STATE_MARK_EVENT:
                setupState = STATE_ROVER_NOT_STARTED;
                break;
              case STATE_ROVER_NOT_STARTED:
                //If F9R, skip base state
                if (zedModuleType == PLATFORM_F9R)
                  setupState = STATE_BUBBLE_LEVEL;
                else
                  setupState = STATE_BASE_NOT_STARTED;
                break;
              case STATE_BASE_NOT_STARTED:
                setupState = STATE_BUBBLE_LEVEL;
                break;
              case STATE_BUBBLE_LEVEL:
                setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                break;
              case STATE_WIFI_CONFIG_NOT_STARTED:
                setupState = STATE_MARK_EVENT;
                break;
              default:
                Serial.printf("ButtonCheckTask unknown setup state: %d\n\r", setupState);
                setupState = STATE_MARK_EVENT;
                break;
            }
            break;

          default:
            Serial.printf("ButtonCheckTask unknown system state: %d\n\r", systemState);
            requestChangeState(STATE_ROVER_NOT_STARTED);
            break;
        }
      }
    } //End Platform = RTK Facet

    delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
    taskYIELD();
  }
}
