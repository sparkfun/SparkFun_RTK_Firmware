//High frequency tasks made by xTaskCreate()
//And any low frequency tasks that are called by Ticker

volatile static uint16_t dataHead = 0; //Head advances as data comes in from GNSS's UART
volatile int availableHandlerSpace = 0; //settings.gnssHandlerBufferSize - usedSpace

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
          Serial.printf("SerialWriteTask High watermark: %d\r\n",  uxTaskGetStackHighWaterMark(NULL));
      }
      delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
      taskYIELD();
    }

    delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
    taskYIELD();
  }
}

//----------------------------------------------------------------------
//The ESP32<->ZED-F9P serial connection is default 460,800bps to facilitate
//10Hz fix rate with PPP Logging Defaults (NMEAx5 + RXMx2) messages enabled.
//ESP32 UART2 is begun with settings.uartReceiveBufferSize size buffer. The circular buffer
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
//Maximum ring buffer fill is settings.gnssHandlerBufferSize - 1
//----------------------------------------------------------------------

//Read bytes from UART2 into circular buffer
//If data is coming in at 230,400bps = 23,040 bytes/s = one byte every 0.043ms
//If SD blocks for 150ms (not extraordinary) that is 3,488 bytes that must be buffered
//The ESP32 Arduino FIFO is ~120 bytes. We use this task to harvest from FIFO into circular buffer
//during SD write blocking time.
void F9PSerialReadTask(void *e)
{
  bool newDataLost = false; //Goes true at the first instance of 0 bytes available

  while (true)
  {
    if (settings.enableTaskReports == true)
      Serial.printf("SerialReadTask High watermark: %d\r\n",  uxTaskGetStackHighWaterMark(NULL));

    int serialBytesAvailable = serialGNSS.available();
    if (serialBytesAvailable)
    {
      //Check for buffer overruns
      int availableUARTSpace = settings.uartReceiveBufferSize - serialBytesAvailable;
      combinedSpaceRemaining = availableHandlerSpace + availableUARTSpace;
      if (combinedSpaceRemaining <= 0)
      {
        if (settings.enablePrintBufferOverrun)
          Serial.printf("Data loss likely! Incoming Serial: %d\tToRead: %d\tMovedToBuffer: %d\tavailableUARTSpace: %d\tavailableHandlerSpace: %d\tToRecord: %d\tRecorded: %d\tBO: %d\n\r", serialBytesAvailable, 0, 0, availableUARTSpace, availableHandlerSpace, 0, 0, bufferOverruns);

        if (newDataLost == false)
        {
          newDataLost = true;
          bufferOverruns++;
        }
      }
      else if (combinedSpaceRemaining < ( (settings.gnssHandlerBufferSize + settings.uartReceiveBufferSize) / 16))
      {
        if (settings.enablePrintBufferOverrun)
          Serial.printf("Low space: Incoming Serial: %d\tToRead: %d\tMovedToBuffer: %d\tavailableUARTSpace: %d\tavailableHandlerSpace: %d\tToRecord: %d\tRecorded: %d\tBO: %d\n\r", serialBytesAvailable, 0, 0, availableUARTSpace, availableHandlerSpace, 0, 0, bufferOverruns);
      }
      else
      {
        newDataLost = false; //Reset

        if (settings.enablePrintSDBuffers && !inMainMenu)
          Serial.printf("UT  Incoming Serial: %04d\tToRead: %04d\tMovedToBuffer: %04d\tavailableUARTSpace: %04d\tavailableHandlerSpace: %04d\tToRecord: %04d\tRecorded: %04d\tBO: %d\n\r", serialBytesAvailable, 0, 0, availableUARTSpace, availableHandlerSpace, 0, 0, bufferOverruns);
      }

      //While there is free buffer space and UART2 has at least one RX byte
      while (availableHandlerSpace && serialGNSS.available())
      {
        //Fill the buffer to the end
        int uartToRead = availableHandlerSpace;
        if ((dataHead + uartToRead) > settings.gnssHandlerBufferSize)
          uartToRead = settings.gnssHandlerBufferSize - dataHead;

        //Add new data into circular buffer in front of the head
        //uartToRead is already reduced to avoid buffer overflow
        int movedToBuffer = serialGNSS.read(&rBuffer[dataHead], uartToRead);

        //Check for negative (error or no bytes remaining)
        if (movedToBuffer <= 0)
          break;

        //Account for the bytes read
        availableHandlerSpace -= movedToBuffer;

        if (settings.enablePrintSDBuffers && !inMainMenu)
        {
          serialBytesAvailable = serialGNSS.available();
          int availableUARTSpace = settings.uartReceiveBufferSize - serialBytesAvailable;
          Serial.printf("UT2 Incoming Serial: %04d\tToRead: %04d\tMovedToBuffer: %04d\tavailableUARTSpace: %04d\tavailableHandlerSpace: %04d\tToRecord: %04d\tRecorded: %04d\tBO: %d\n\r", serialBytesAvailable, uartToRead, movedToBuffer, availableUARTSpace, availableHandlerSpace, 0, 0, bufferOverruns);
        }

        dataHead += movedToBuffer;
        if (dataHead >= settings.gnssHandlerBufferSize)
          dataHead -= settings.gnssHandlerBufferSize;

        delay(1);
        taskYIELD();
      }
    } //End Serial.available()
  }

  delay(1);
  taskYIELD();
}

//If new data is in the buffer, dole it out to appropriate interface
//Send data out Bluetooth, record to SD, or send over TCP
void handleGNSSDataTask(void *e)
{
  volatile static uint16_t btTail = 0; //BT Tail advances as it is sent over BT
  volatile static uint16_t nmeaTail = 0; //NMEA TCP client tail
  volatile static uint16_t sdTail = 0; //SD Tail advances as it is recorded to SD

  int btBytesToSend; //Amount of buffered Bluetooth data
  int nmeaBytesToSend; //Amount of buffered NMEA TCP data
  int sdBytesToRecord; //Amount of buffered microSD card logging data

  int btConnected; //Is the device in a state to send Bluetooth data?

  while (true)
  {
    if (settings.enableTaskReports == true)
      Serial.printf("SerialReadTask High watermark: %d\r\n",  uxTaskGetStackHighWaterMark(NULL));

    volatile int lastDataHead = dataHead; //dataHead may change during this task by the harvesting task. Use a snapshot.

    //Determine the amount of Bluetooth data in the buffer
    btBytesToSend = 0;

    //Determine BT connection state
    btConnected = (bluetoothGetState() == BT_CONNECTED)
                  && (systemState != STATE_BASE_TEMP_SETTLE)
                  && (systemState != STATE_BASE_TEMP_SURVEY_STARTED);

    if (btConnected)
    {
      btBytesToSend = lastDataHead - btTail;
      if (btBytesToSend < 0)
        btBytesToSend += settings.gnssHandlerBufferSize;
    }

    //Determine the amount of NMEA TCP data in the buffer
    nmeaBytesToSend = 0;
    if (settings.enableNmeaServer || settings.enableNmeaClient)
    {
      nmeaBytesToSend = lastDataHead - nmeaTail;
      if (nmeaBytesToSend < 0)
        nmeaBytesToSend += settings.gnssHandlerBufferSize;
    }

    //Determine the amount of microSD card logging data in the buffer
    sdBytesToRecord = 0;
    if (online.logging)
    {
      sdBytesToRecord = lastDataHead - sdTail;
      if (sdBytesToRecord < 0)
        sdBytesToRecord += settings.gnssHandlerBufferSize;
    }

    //----------------------------------------------------------------------
    //Send data over Bluetooth
    //----------------------------------------------------------------------

    //If we are actively survey-in then do not pass NMEA data from ZED to phone
    if (!btConnected)
      //Discard the data
      btTail = lastDataHead;
    else if (btBytesToSend > 0)
    {
      //Reduce bytes to send if we have more to send then the end of the buffer
      //We'll wrap next loop
      if ((btTail + btBytesToSend) > settings.gnssHandlerBufferSize)
        btBytesToSend = settings.gnssHandlerBufferSize - btTail;

      //Push new data to BT SPP
      btBytesToSend = bluetoothWriteBytes(&rBuffer[btTail], btBytesToSend);

      if (btBytesToSend > 0)
      {
        //If we are in base mode, assume part of the outgoing data is RTCM
        if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
          bluetoothOutgoingRTCM = true;

        //Account for the sent or dropped data
        btTail += btBytesToSend;
        if (btTail >= settings.gnssHandlerBufferSize)
          btTail -= settings.gnssHandlerBufferSize;
      }
      else
        log_w("BT failed to send");
    }

    //----------------------------------------------------------------------
    //Send data to the NMEA clients
    //----------------------------------------------------------------------

    if ((!settings.enableNmeaServer) && (!settings.enableNmeaClient) && (!wifiNmeaConnected))
      nmeaTail = lastDataHead;
    else if (nmeaBytesToSend > 0)
    {
      //Reduce bytes to send if we have more to send then the end of the buffer
      //We'll wrap next loop
      if ((nmeaTail + nmeaBytesToSend) > settings.gnssHandlerBufferSize)
        nmeaBytesToSend = settings.gnssHandlerBufferSize - nmeaTail;

      //Send the data to the NMEA TCP clients
      wifiNmeaData (&rBuffer[nmeaTail], nmeaBytesToSend);

      //Assume all data was sent, wrap the buffer pointer
      nmeaTail += nmeaBytesToSend;
      if (nmeaTail >= settings.gnssHandlerBufferSize)
        nmeaTail -= settings.gnssHandlerBufferSize;
    }

    //----------------------------------------------------------------------
    //Log data to the SD card
    //----------------------------------------------------------------------

    //If user wants to log, record to SD
    if (!online.logging)
      //Discard the data
      sdTail = lastDataHead;
    else if (sdBytesToRecord > 0)
    {
      //Check if we are inside the max time window for logging
      if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
      {
        //Attempt to gain access to the SD card, avoids collisions with file
        //writing from other functions like recordSystemSettingsToFile()
        if (xSemaphoreTake(sdCardSemaphore, loggingSemaphoreWait_ms) == pdPASS)
        {
          markSemaphore(FUNCTION_WRITESD);

          //Reduce bytes to record if we have more then the end of the buffer
          int sliceToRecord = sdBytesToRecord;
          if ((sdTail + sliceToRecord) > settings.gnssHandlerBufferSize)
            sliceToRecord = settings.gnssHandlerBufferSize - sdTail;

          if (settings.enablePrintSDBuffers && !inMainMenu)
          {
            int availableUARTSpace = settings.uartReceiveBufferSize - serialGNSS.available();
            Serial.printf("SD  Incoming Serial: %04d\tToRead: %04d\tMovedToBuffer: %04d\tavailableUARTSpace: %04d\tavailableHandlerSpace: %04d\tToRecord: %04d\tRecorded: %04d\tBO: %d\n\r", serialGNSS.available(), 0, 0, availableUARTSpace, availableHandlerSpace, sliceToRecord, 0, bufferOverruns);
          }

          //Write the data to the file
          long startTime = millis();
          sdBytesToRecord = ubxFile->write(&rBuffer[sdTail], sliceToRecord);

          fileSize = ubxFile->fileSize(); //Get updated filed size

          //Force file sync every 60s
          if (millis() - lastUBXLogSyncTime > 60000)
          {
            if (productVariant == RTK_SURVEYOR)
              digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Blink LED to indicate logging activity

            ubxFile->sync();
            updateDataFileAccess(ubxFile); // Update the file access time & date
            if (productVariant == RTK_SURVEYOR)
              digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Return LED to previous state

            lastUBXLogSyncTime = millis();
          }

          long endTime = millis();

          if (settings.enablePrintBufferOverrun)
          {
            if (endTime - startTime > 150)
              Serial.printf("Long Write! Time: %ld ms / Location: %ld / Recorded %d bytes / spaceRemaining %d bytes\n\r", endTime - startTime, fileSize, sdBytesToRecord, combinedSpaceRemaining);
          }

          xSemaphoreGive(sdCardSemaphore);

          //Account for the sent data or dropped
          if (sdBytesToRecord > 0)
          {
            sdTail += sdBytesToRecord;
            if (sdTail >= settings.gnssHandlerBufferSize)
              sdTail -= settings.gnssHandlerBufferSize;
          }
        } //End sdCardSemaphore
        else
        {
          char semaphoreHolder[50];
          getSemaphoreFunction(semaphoreHolder);
          log_w("sdCardSemaphore failed to yield for SD write, held by %s, Tasks.ino line %d", semaphoreHolder, __LINE__);
        }
      } //End maxLogTime
    } //End logging

    //Update space available for use in UART task
    //    btBytesToSend = lastDataHead - btTail;
    //    if (btBytesToSend < 0)
    //      btBytesToSend += settings.gnssHandlerBufferSize;
    //
    //    nmeaBytesToSend = lastDataHead - nmeaTail;
    //    if (nmeaBytesToSend < 0)
    //      nmeaBytesToSend += settings.gnssHandlerBufferSize;

    sdBytesToRecord = lastDataHead - sdTail;
    if (sdBytesToRecord < 0)
      sdBytesToRecord += settings.gnssHandlerBufferSize;

    //Determine the inteface that is most behind: SD writing, SPP transmission, or TCP transmission
    //    int usedSpace = 0;
    //    if (sdBytesToRecord >= btBytesToSend && sdBytesToRecord >= nmeaBytesToSend)
    //      usedSpace = sdBytesToRecord;
    //    else if (btBytesToSend >= sdBytesToRecord && btBytesToSend >= nmeaBytesToSend)
    //      usedSpace = btBytesToSend;
    //    else
    //      usedSpace = nmeaBytesToSend;

    //    availableHandlerSpace = settings.gnssHandlerBufferSize - usedSpace;
    availableHandlerSpace = settings.gnssHandlerBufferSize - sdBytesToRecord;

    //Don't fill the last byte to prevent buffer overflow
    if (availableHandlerSpace)
      availableHandlerSpace -= 1;

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
                Serial.printf("ButtonCheckTask unknown setup state: %d\r\n", setupState);
                setupState = STATE_MARK_EVENT;
                break;
            }
            break;

          default:
            Serial.printf("ButtonCheckTask unknown system state: %d\r\n", systemState);
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
                Serial.printf("ButtonCheckTask unknown setup state: %d\r\n", setupState);
                setupState = STATE_MARK_EVENT;
                break;
            }
            break;

          default:
            Serial.printf("ButtonCheckTask unknown system state: %d\r\n", systemState);
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
      Serial.printf("idleTask %d High watermark: %d\r\n",
                    xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    }

    //Let other same priority tasks run
    taskYIELD();
  }
}

//Serial Read/Write tasks for the F9P must be started after BT is up and running otherwise SerialBT->available will cause reboot
void tasksStartUART2()
{
  //Reads data from ZED and stores data into circular buffer
  if (F9PSerialReadTaskHandle == NULL)
    xTaskCreate(
      F9PSerialReadTask, //Function to call
      "F9Read", //Just for humans
      readTaskStackSize, //Stack Size
      NULL, //Task input parameter
      F9PSerialReadTaskPriority, //Priority
      &F9PSerialReadTaskHandle); //Task handle

  //Reads data from circular buffer and sends data to SD, SPP, or TCP
  if (handleGNSSDataTaskHandle == NULL)
    xTaskCreate(
      handleGNSSDataTask, //Function to call
      "handleGNSSData", //Just for humans
      handleGNSSDataTaskStackSize, //Stack Size
      NULL, //Task input parameter
      handleGNSSDataTaskPriority, //Priority
      &handleGNSSDataTaskHandle); //Task handle

  //Reads data from BT and sends to ZED
  if (F9PSerialWriteTaskHandle == NULL)
    xTaskCreate(
      F9PSerialWriteTask, //Function to call
      "F9Write", //Just for humans
      writeTaskStackSize, //Stack Size
      NULL, //Task input parameter
      F9PSerialWriteTaskPriority, //Priority
      &F9PSerialWriteTaskHandle); //Task handle
}

//Stop tasks - useful when running firmware update or WiFi AP is running
void tasksStopUART2()
{
  //Delete tasks if running
  if (F9PSerialReadTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialReadTaskHandle);
    F9PSerialReadTaskHandle = NULL;
  }
  if (handleGNSSDataTaskHandle != NULL)
  {
    vTaskDelete(handleGNSSDataTaskHandle);
    handleGNSSDataTaskHandle = NULL;
  }
  if (F9PSerialWriteTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialWriteTaskHandle);
    F9PSerialWriteTaskHandle = NULL;
  }

  //Give the other CPU time to finish
  //Eliminates CPU bus hang condition
  delay(100);
}
