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
        serialGNSS.write(wBuffer, s);
        online.rxRtcmCorrectionData = true;

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

static uint8_t ringBuffer[SERIAL_SIZE_RX]; //Buffer for reading from F9P to SPP
static int availableBufferSpace = sizeof(ringBuffer) - 1; //Distance between head and furthest away tail
static uint16_t dataHead = 0; //Head advances as data comes in from GNSS's UART
static uint16_t btTail = 0; //BT Tail advances as it is sent over BT
static uint16_t sdTail = 0; //SD Tail advances as it is recorded to SD

void emptyRingBuffer(bool writeLogFile)
{
  bool btConnected;
  uint16_t btBytesToSend;
  uint16_t bytesSent;
  uint16_t sdBytesToRecord;

  //Determine BT connection state
  btConnected = (bluetoothGetState() == BT_CONNECTED)
                && (systemState != STATE_BASE_TEMP_SETTLE)
                && (systemState != STATE_BASE_TEMP_SURVEY_STARTED);

  //If we are actively survey-in then do not pass NMEA data from ZED to phone
  if (!btConnected)
    //Discard the data
    btTail = dataHead;
  else
  {
    //Display the Bluetooth tail offset
    if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
      Serial.printf("BT: %4d --> ", btTail);

    //Reduce bytes to send if we have more to send then the end of the buffer
    //We'll wrap next loop
    btBytesToSend = (btTail > dataHead) ? dataHead + sizeof(ringBuffer) - btTail
                                        : dataHead - btTail;
    if ((btTail + btBytesToSend) > sizeof(ringBuffer))
      btBytesToSend = sizeof(ringBuffer) - btTail;

    //Reduce bytes to send to match BT buffer size
    if (btBytesToSend > settings.sppTxQueueSize)
      btBytesToSend = settings.sppTxQueueSize;

    //Send the data if Bluetooth is not congested
    if ((bluetoothIsCongested() == false) || (settings.throttleDuringSPPCongestion == false))
    {
      //Push new data to BT SPP if not congested or not throttling
      bytesSent = bluetoothWriteBytes(&ringBuffer[btTail], btBytesToSend);
      online.txNtripDataCasting = true;
    }
    else
    {
      //Don't push data to BT SPP if there is congestion to prevent heap hits.
      bytesSent = btBytesToSend;
      if (btBytesToSend < (sizeof(ringBuffer) - 1))
        bytesSent = 0;
      else
        Serial.printf("ERROR - Congestion, dropped %d bytes: GNSS --> Bluetooth\r\n", btBytesToSend);
    }

    //Account for the sent data or dropped
    btTail += bytesSent;
    if (btTail >= sizeof(ringBuffer))
      btTail -= sizeof(ringBuffer);

    //Display the Bluetooth tail offset
    if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
      Serial.printf("%4d\r\n", btTail);
  }

  //Determine how much Bluetooth data is in the ring buffer
  btBytesToSend = (btTail > dataHead) ? dataHead + sizeof(ringBuffer) - btTail
                                      : dataHead - btTail;

  //If user wants to log, record to SD
  if (writeLogFile)
  {
    if (!online.logging)
      //Discard the data
      sdTail = dataHead;
    else
    {
      //Check if we are inside the max time window for logging
      if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
      {
        //Display the SD tail offset
        if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
          Serial.printf("SD: %4d --> ", sdTail);

        //Determine how much SD card data is in the ring buffer
        sdBytesToRecord = (sdTail > dataHead) ? dataHead + sizeof(ringBuffer) - sdTail
                                            : dataHead - sdTail;

        //Attempt to gain access to the SD card, avoids collisions with file
        //writing from other functions like recordSystemSettingsToFile()
        if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
        {
          //Reduce bytes to send if we have more to send then the end of the buffer
          //We'll wrap next loop
          if ((sdTail + sdBytesToRecord) > sizeof(ringBuffer))
            sdBytesToRecord = sizeof(ringBuffer) - sdTail;

          //Write the data to the file
          bytesSent = ubxFile->write(&ringBuffer[sdTail], sdBytesToRecord);
          xSemaphoreGive(sdCardSemaphore);

          //Account for the sent data or dropped
          sdTail += bytesSent;
          if (sdTail >= sizeof(ringBuffer))
            sdTail -= sizeof(ringBuffer);
        } //End sdCardSemaphore
        else
          log_w("WARNING - sdCardSemaphore failed to yield, Tasks.ino line %d\r\n", __LINE__);

        //Display the SD tail offset
        if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
          Serial.printf("%4d\r\n", sdTail);
      } //End maxLogTime
    } //End logging
  }

  //Determine how much SD card data is in the ring buffer
  sdBytesToRecord = (sdTail > dataHead) ? dataHead + sizeof(ringBuffer) - sdTail
                                      : dataHead - sdTail;

  //Update the number of bytes available in the ring buffer
  availableBufferSpace = sizeof(ringBuffer) - 1
                       - ((btBytesToSend > sdBytesToRecord) ? btBytesToSend
                                                            : sdBytesToRecord);
}

//Process the RTCM message
void processUart1Message(PARSE_STATE * parse, uint8_t type)
{
  uint16_t bytesToCopy;
  uint16_t remainingBytes;

  //----------------------------------------------------------------------
  //At approximately 3.3K characters/second, a 6K byte buffer should hold
  //approximately 2 seconds worth of data.  Bluetooth congestion or conflicts
  //with the SD card semaphore should clear within this time.  At 57600 baud
  //the Bluetooth UART is able to send 7200 characters a second.  With a 10
  //mSec delay this routine runs approximately 100 times per second providing
  //multiple chances to empty the buffer.
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

  //Display the message
  if (settings.enablePrintLogFileMessages && (!parse->crc) && (!inMainMenu))
  {
    printTimeStamp();
    switch(type)
    {
    case SENTENCE_TYPE_NMEA:
      Serial.printf ("    %s NMEA %s, %2d bytes\r\n", parse->parserName,
                     parse->nmeaMessageName, parse->length);
      break;

    case SENTENCE_TYPE_RTCM:
      Serial.printf ("    %s RTCM %d, %2d bytes\r\n", parse->parserName,
                     parse->message, parse->length);
      break;

    case SENTENCE_TYPE_UBX:
      Serial.printf ("    %s UBX %d.%d, %2d bytes\r\n", parse->parserName,
                     parse->message >> 8, parse->message & 0xff, parse->length);
      break;
    }
  }

  //Determine if this message will fit into the ring buffer
  bytesToCopy = parse->length;
  if ((bytesToCopy > availableBufferSpace) && (!inMainMenu))
  {
    Serial.printf("Ring buffer full, discarding %d bytes\r\n", bytesToCopy);
    return;
  }

  //Account for this message
  availableBufferSpace -= bytesToCopy;

  //Fill the buffer to the end and then start at the beginning
  if ((dataHead + bytesToCopy) > sizeof(ringBuffer))
    bytesToCopy = sizeof(ringBuffer) - dataHead;

  //Display the dataHead offset
  if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
    Serial.printf("DH: %4d --> ", dataHead);

  //Copy the data into the ring buffer
  memcpy(&ringBuffer[dataHead], parse->buffer, bytesToCopy);
  dataHead += bytesToCopy;
  if (dataHead >= sizeof(ringBuffer))
    dataHead -= sizeof(ringBuffer);

  //Determine the remaining bytes
  remainingBytes = parse->length - bytesToCopy;
  if (remainingBytes)
  {
    //Copy the remaining bytes into the beginning of the ring buffer
    memcpy(ringBuffer, &parse->buffer[bytesToCopy], remainingBytes);
    dataHead = remainingBytes;
  }

  //Display the dataHead offset
  if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
    Serial.printf("%4d\r\n", dataHead);

  //Start emptying the ring buffer
  emptyRingBuffer(false);
}

//Read data from the ZED (GNSS receiver) into the parse buffer
void F9PSerialReadTask(void *e)
{
  static PARSE_STATE parse = {waitForPreamble, processUart1Message, "Log"};
  uint8_t data;

  while (true)
  {
    if (settings.enableTaskReports == true)
      Serial.printf("SerialReadTask High watermark: %d\n\r",  uxTaskGetStackHighWaterMark(NULL));

    //Determine if serial data is available
    while (serialGNSS.available())
    {
      //Read the data from UART1
      data = serialGNSS.read();

      //Save the data byte
      parse.buffer[parse.length++] = data;

      //Compute the CRC value for the message
      if (parse.computeCrc)
        parse.crc = COMPUTE_CRC24Q(&parse, data);

      //Parse the RTCM message
      parse.state(&parse, data);
    }

    //Finish emptying the ring buffer
    emptyRingBuffer(true);

    //Let other tasks run, prevent watch dog timer (WDT) resets
    delay(10);
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
            lastSystemState = systemState; //Remember this state to return after we mark an event
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
            lastSystemState = systemState; //Remember this state to return after we mark an event
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
