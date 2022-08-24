//High frequency tasks made by xTaskCreate()
//And any low frequency tasks that are called by Ticker

//If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
//Task for writing to the GNSS receiver
void F9PSerialWriteTask(void *e)
{
  while (true)
  {
    //Receive RTCM corrections or UBX config messages over bluetooth and pass along to ZED
    if (bluetoothGetState() == BT_CONNECTED)
    {
      while (bluetoothRxDataAvailable())
      {
        //Pass bytes to GNSS receiver
        int s = bluetoothReadBytes(wBuffer, sizeof(wBuffer));

        //TODO - control if this RTCM source should be listened to or not
        serialGNSS.write(wBuffer, s);
        bluetoothIncomingRTCM = true;
        if (!inMainMenu) log_d("Bluetooth received %d RTCM bytes, sent to ZED", s);

        if (settings.enableTaskReports == true)
          Serial.printf("SerialWriteTask High watermark: %d\n\r",  uxTaskGetStackHighWaterMark(NULL));
      }
      delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
      taskYIELD();
    }

    delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
    taskYIELD();
  }
}

//If the ZED has any new NMEA data, pass it out over Bluetooth
//Task for reading data from the GNSS receiver.
void F9PSerialReadTask(void *e)
{
  static uint8_t rBuffer[1024 * 6]; //Buffer for reading from F9P to SPP
  static uint16_t dataHead = 0; //Head advances as data comes in from GNSS's UART
  static uint16_t btTail = 0; //BT Tail advances as it is sent over BT
  static uint16_t sdTail = 0; //SD Tail advances as it is recorded to SD

  int btBytesToSend; //Amount of buffered Bluetooth data
  int sdBytesToRecord; //Amount of buffered microSD card logging data
  int availableBufferSpace; //Distance between head and furthest away tail

  int btConnected; //Is the RTK in a state to send Bluetooth data?
  int newBytesToRecord; //Size of data from GNSS

  while (true)
  {
    while (serialGNSS.available())
    {
      if (settings.enableTaskReports == true)
        Serial.printf("SerialReadTask High watermark: %d\n\r",  uxTaskGetStackHighWaterMark(NULL));

      //----------------------------------------------------------------------
      //The ESP32<->ZED-F9P serial connection is default 460,800bps to facilitate
      //10Hz fix rate with PPP Logging Defaults (NMEAx5 + RXMx2) messages enabled.
      //ESP32 UART2 is begun with SERIAL_SIZE_RX size buffer. The circular buffer
      //is 1024*6. At approximately 46.1K characters/second, a 6144 * 2
      //byte buffer should hold 267ms worth of serial data. Assuming SD writes are
      //250ms worst case, we should record incoming all data. Bluetooth congestion
      //or conflicts with the SD card semaphore should clear within this time.
      //
      //Ring buffer empty when (dataHead == btTail) and (dataHead == sdTail)
      //
      //        +---------+
      //        |         |
      //        |         |
      //        |         |
      //        |         |
      //        +---------+ <-- dataHead, btTail, sdTail
      //
      //Ring buffer contains data when (dataHead != btTail) or (dataHead != sdTail)
      //
      //        +---------+
      //        |         |
      //        |         |
      //        | yyyyyyy | <-- dataHead
      //        | xxxxxxx | <-- btTail (1 byte in buffer)
      //        +---------+ <-- sdTail (2 bytes in buffer)
      //
      //        +---------+
      //        | yyyyyyy | <-- btTail (1 byte in buffer)
      //        | xxxxxxx | <-- sdTail (2 bytes in buffer)
      //        |         |
      //        |         |
      //        +---------+ <-- dataHead
      //
      //Maximum ring buffer fill is sizeof(rBuffer) - 1
      //----------------------------------------------------------------------

      availableBufferSpace = sizeof(rBuffer);

      //Determine BT connection state
      btConnected = (bluetoothGetState() == BT_CONNECTED)
                    && (systemState != STATE_BASE_TEMP_SETTLE)
                    && (systemState != STATE_BASE_TEMP_SURVEY_STARTED);

      //Determine the amount of Bluetooth data in the buffer
      btBytesToSend = 0;
      if (btConnected)
      {
        btBytesToSend = dataHead - btTail;
        if (btBytesToSend < 0)
          btBytesToSend += sizeof(rBuffer);
      }

      //Determine the amount of microSD card logging data in the buffer
      sdBytesToRecord = 0;
      if (online.logging)
      {
        sdBytesToRecord = dataHead - sdTail;
        if (sdBytesToRecord < 0)
          sdBytesToRecord += sizeof(rBuffer);
      }

      //Determine the free bytes in the buffer
      if (btBytesToSend >= sdBytesToRecord)
        availableBufferSpace = sizeof(rBuffer) - btBytesToSend;
      else
        availableBufferSpace = sizeof(rBuffer) - sdBytesToRecord;

      //Don't fill the last byte to prevent buffer overflow
      if (availableBufferSpace)
        availableBufferSpace -= 1;

      //Fill the buffer to the end and then start at the beginning
      if ((dataHead + availableBufferSpace) >= sizeof(rBuffer))
        availableBufferSpace = sizeof(rBuffer) - dataHead;

      //If we have buffer space, read data from the GNSS into the buffer
      newBytesToRecord = 0;
      if (availableBufferSpace)
      {
        //Add new data into circular buffer in front of the head
        //availableBufferSpace is already reduced to avoid buffer overflow
        newBytesToRecord = serialGNSS.readBytes(&rBuffer[dataHead], availableBufferSpace);
      }

      //Account for the byte read
      if (newBytesToRecord > 0)
      {
        //Set the next fill offset
        dataHead += newBytesToRecord;
        if (dataHead >= sizeof(rBuffer))
          dataHead -= sizeof(rBuffer);

        //Account for the new data
        if (btConnected)
          btBytesToSend += newBytesToRecord;
        if (online.logging)
          sdBytesToRecord += newBytesToRecord;
      }

      //----------------------------------------------------------------------
      //Send data over Bluetooth
      //----------------------------------------------------------------------

      //If we are actively survey-in then do not pass NMEA data from ZED to phone
      if (!btConnected)
        //Discard the data
        btTail = dataHead;
      else
      {
        //Reduce bytes to send if we have more to send then the end of the buffer
        //We'll wrap next loop
        if ((btTail + btBytesToSend) >= sizeof(rBuffer))
          btBytesToSend = sizeof(rBuffer) - btTail;

        //Push new data to BT SPP if not congested or not throttling
        btBytesToSend = bluetoothWriteBytes(&rBuffer[btTail], btBytesToSend);
        if (btBytesToSend > 0)
        {
          //If we are in base mode, assume part of the outgoing data is RTCM
          if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
            bluetoothOutgoingRTCM = true;
        }
        else
          log_w("BT failed to send");


        //Account for the sent or dropped data
        btTail += btBytesToSend;
        if (btTail >= sizeof(rBuffer))
          btTail -= sizeof(rBuffer);
      }

      //----------------------------------------------------------------------
      //Log data to the SD card
      //----------------------------------------------------------------------

      //If user wants to log, record to SD
      if (!online.logging)
        //Discard the data
        sdTail = dataHead;
      else
      {
        //Check if we are inside the max time window for logging
        if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
        {
          //Attempt to gain access to the SD card, avoids collisions with file
          //writing from other functions like recordSystemSettingsToFile()
          if (xSemaphoreTake(sdCardSemaphore, loggingSemaphore_shortWait_ms) == pdPASS)
          {
            //Reduce bytes to send if we have more to send then the end of the buffer
            //We'll wrap next loop
            if ((sdTail + sdBytesToRecord) >= sizeof(rBuffer))
              sdBytesToRecord = sizeof(rBuffer) - sdTail;

            //Write the data to the file
            sdBytesToRecord = ubxFile->write(&rBuffer[sdTail], sdBytesToRecord);
            xSemaphoreGive(sdCardSemaphore);

            //Account for the sent data or dropped
            sdTail += sdBytesToRecord;
            if (sdTail >= sizeof(rBuffer))
              sdTail -= sizeof(rBuffer);
          } //End sdCardSemaphore
          else
          {
            //Retry the semaphore a little later if possible
            if (sdBytesToRecord >= (sizeof(rBuffer) - 1))
            {
              //Error - no more room in the buffer, drop a buffer's worth of data
              sdTail = dataHead;
              log_e("ERROR - sdCardSemaphore failed to yield, Tasks.ino line %d", __LINE__);
              Serial.printf("ERROR - Dropped %d bytes: GNSS --> log file\r\n", sdBytesToRecord);
            }
            else
              log_w("sdCardSemaphore failed to yield, Tasks.ino line %d", __LINE__);
          }
        } //End maxLogTime
      } //End logging
    } //End Serial.available()

    //----------------------------------------------------------------------
    //Let other tasks run, prevent watch dog timer (WDT) resets
    //----------------------------------------------------------------------

    delay(1);
    taskYIELD();
  }
}

//Control BT status LED according to bluetoothGetState()
void updateBTled()
{
  if (productVariant == RTK_SURVEYOR)
  {
    //Blink on/off while we wait for BT connection
    if (bluetoothGetState() == BT_NOTCONNECTED)
    {
      if (btFadeLevel == 0) btFadeLevel = 255;
      else btFadeLevel = 0;
      ledcWrite(ledBTChannel, btFadeLevel);
    }

    //Solid LED if BT Connected
    else if (bluetoothGetState() == BT_CONNECTED)
      ledcWrite(ledBTChannel, 255);

    //Pulse LED while no BT and we wait for WiFi connection
    else if (wifiState == WIFI_NOTCONNECTED || wifiState == WIFI_CONNECTED)
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
  uint8_t index;

  if (setupBtn != NULL) setupBtn->begin();
  if (powerBtn != NULL) powerBtn->begin();

  while (true)
  {
    /* RTK Surveyor

                                  .----------------------------.
                                  |                            |
                                  V                            |
                        .------------------.                   |
                        |     Power On     |                   |
                        '------------------'                   |
                                  |                            |
                                  | Setup button = 0           |
                                  V                            |
                        .------------------.                   |
                .------>|    Rover Mode    |                   |
                |       '------------------'                   |
                |                 |                            |
                |                 | Setup button = 1           |
                |                 V                            |
                |       .------------------.                   |
                '-------|    Base Mode     |                   |
       Setup button = 0 '------------------'                   |
       after long time    |             |                      |
                          |             | Setup button = 0     |
         Setup button = 0 |             | after short time     |
         after short time |             | (< 500 mSec)         |
             (< 500 mSec) |             |                      |
      STATE_ROVER_NOT_STARTED |             |                      |
                          V             V                      |
          .------------------.   .------------------.          |
          |    Test Mode     |   | WiFi Config Mode |----------'
          '------------------'   '------------------'

    */

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
            if (systemState == STATE_ROVER_NOT_STARTED && online.display == true) //Catch during Power On
              requestChangeState(STATE_TEST); //If RTK Surveyor, with display attached, during Rover not started, then enter test mode
            else
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
          case STATE_BASE_FIXED_NOT_STARTED:
          case STATE_BASE_FIXED_TRANSMITTING:
          case STATE_BUBBLE_LEVEL:
          case STATE_WIFI_CONFIG_NOT_STARTED:
          case STATE_WIFI_CONFIG:
          case STATE_ESPNOW_PAIRING_NOT_STARTED:
          case STATE_ESPNOW_PAIRING:
            lastSystemState = systemState; //Remember this state to return after we mark an event or ESP-Now pair
            requestChangeState(STATE_DISPLAY_SETUP);
            setupState = STATE_MARK_EVENT;
            lastSetupMenuChange = millis();
            break;

          case STATE_MARK_EVENT:
            //If the user presses the setup button during a mark event, do nothing
            //Allow system to return to lastSystemState
            break;

          case STATE_PROFILE:
            //If the user presses the setup button during a profile change, do nothing
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
                setupState = STATE_ESPNOW_PAIRING_NOT_STARTED;
                break;
              case STATE_ESPNOW_PAIRING_NOT_STARTED:
                //If only one active profile do not show any profiles
                index = getProfileNumberFromUnit(0);
                displayProfile = getProfileNumberFromUnit(1);
                setupState = (index >= displayProfile) ? STATE_MARK_EVENT : STATE_PROFILE;
                displayProfile = 0;
                break;
              case STATE_PROFILE:
                //Done when no more active profiles
                displayProfile++;
                if (!getProfileNumberFromUnit(displayProfile))
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
    else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND) //Check one momentary button
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
      else if (powerBtn != NULL && systemState == STATE_ROVER_NOT_STARTED && firstRoverStart == true && powerBtn->pressedFor(500))
      {
        forceSystemStateUpdate = true;
        requestChangeState(STATE_TEST);
        lastTestMenuChange = millis(); //Avoid exiting test menu for 1s
      }
      else if (powerBtn != NULL && powerBtn->wasReleased() && firstRoverStart == false)
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
          case STATE_BASE_FIXED_NOT_STARTED:
          case STATE_BASE_FIXED_TRANSMITTING:
          case STATE_BUBBLE_LEVEL:
          case STATE_WIFI_CONFIG_NOT_STARTED:
          case STATE_WIFI_CONFIG:
          case STATE_ESPNOW_PAIRING_NOT_STARTED:
          case STATE_ESPNOW_PAIRING:
            lastSystemState = systemState; //Remember this state to return after we mark an event or ESP-Now pair
            requestChangeState(STATE_DISPLAY_SETUP);
            setupState = STATE_MARK_EVENT;
            lastSetupMenuChange = millis();
            break;

          case STATE_MARK_EVENT:
            //If the user presses the setup button during a mark event, do nothing
            //Allow system to return to lastSystemState
            break;

          case STATE_PROFILE:
            //If the user presses the setup button during a profile change, do nothing
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
                setupState = STATE_ESPNOW_PAIRING_NOT_STARTED;
                break;
              case STATE_ESPNOW_PAIRING_NOT_STARTED:
                //If only one active profile do not show any profiles
                index = getProfileNumberFromUnit(0);
                displayProfile = getProfileNumberFromUnit(1);
                setupState = (index >= displayProfile) ? STATE_MARK_EVENT : STATE_PROFILE;
                displayProfile = 0;
                break;
              case STATE_PROFILE:
                //Done when no more active profiles
                displayProfile++;
                if (!getProfileNumberFromUnit(displayProfile))
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

void idleTask(void *e)
{
  int cpu = xPortGetCoreID();
  uint32_t idleCount = 0;
  uint32_t lastDisplayIdleTime = 0;
  uint32_t lastStackPrintTime = 0;

  while (1)
  {
    //Increment a count during the idle time
    idleCount++;

    //Determine if it is time to print the CPU idle times
    if ((millis() - lastDisplayIdleTime) >= (IDLE_TIME_DISPLAY_SECONDS * 1000))
    {
      lastDisplayIdleTime = millis();

      //Get the idle time
      if (idleCount > max_idle_count)
        max_idle_count = idleCount;

      //Display the idle times
      if (settings.enablePrintIdleTime) {
        Serial.printf("CPU %d idle time: %d%% (%d/%d)\r\n", cpu,
                      idleCount * 100 / max_idle_count,
                      idleCount, max_idle_count);

        //Print the task count
        if (cpu)
          Serial.printf("%d Tasks\r\n", uxTaskGetNumberOfTasks());
      }

      //Restart the idle count for the next display time
      idleCount = 0;
    }

    //Display the high water mark if requested
    if ((settings.enableTaskReports == true)
        && ((millis() - lastStackPrintTime) >= (IDLE_TIME_DISPLAY_SECONDS * 1000)))
    {
      lastStackPrintTime = millis();
      Serial.printf("idleTask %d High watermark: %d\n\r",
                    xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    }

    //Let other same priority tasks run
    taskYIELD();
  }
}
