// High frequency tasks made by xTaskCreate()
// And any low frequency tasks that are called by Ticker

volatile static uint16_t dataHead = 0;  // Head advances as data comes in from GNSS's UART
volatile int availableHandlerSpace = 0; // settings.gnssHandlerBufferSize - usedSpace

// Buffer the incoming Bluetooth stream so that it can be passed in bulk over I2C
uint8_t bluetoothOutgoingToZed[100];
uint16_t bluetoothOutgoingToZedHead = 0;
unsigned long lastZedI2CSend = 0; // Timestamp of the last time we sent RTCM ZED over I2C

// If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
// Scan for escape characters to enter config menu
void btReadTask(void *e)
{
    while (true)
    {
        // Receive RTCM corrections or UBX config messages over bluetooth and pass along to ZED
        if (bluetoothGetState() == BT_CONNECTED)
        {
            while (btPrintEcho == false && bluetoothRxDataAvailable())
            {
                // Check stream for command characters
                byte incoming = bluetoothRead();

                if (incoming == btEscapeCharacter)
                {
                    // Ignore escape characters received within 2 seconds of serial traffic
                    // Allow escape characters received within first 2 seconds of power on
                    if (millis() - btLastByteReceived > btMinEscapeTime || millis() < btMinEscapeTime)
                    {
                        btEscapeCharsReceived++;
                        if (btEscapeCharsReceived == btMaxEscapeCharacters)
                        {
                            printEndpoint = PRINT_ENDPOINT_ALL;
                            systemPrintln("Echoing all serial to BT device");
                            btPrintEcho = true;

                            btEscapeCharsReceived = 0;
                            btLastByteReceived = millis();
                        }
                    }
                    else
                    {
                        // Ignore this escape character, passing along to output
                        if (USE_I2C_GNSS)
                        {
                            // serialGNSS.write(incoming);
                            addToZedI2CBuffer(btEscapeCharacter);
                        }
                        else
                            theGNSS.pushRawData(&incoming, 1);
                    }
                }
                else // This is just a character in the stream, ignore
                {
                    // Pass any escape characters that turned out to not be a complete escape sequence
                    while (btEscapeCharsReceived-- > 0)
                    {
                        if (USE_I2C_GNSS)
                        {
                            // serialGNSS.write(btEscapeCharacter);
                            addToZedI2CBuffer(btEscapeCharacter);
                        }
                        else
                        {
                            uint8_t escChar = btEscapeCharacter;
                            theGNSS.pushRawData(&escChar, 1);
                        }
                    }

                    // Pass byte to GNSS receiver or to system
                    // TODO - control if this RTCM source should be listened to or not
                    if (USE_I2C_GNSS)
                    {
                        // UART RX can be corrupted by UART TX
                        // See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/469
                        // serialGNSS.write(incoming);
                        addToZedI2CBuffer(incoming);
                    }
                    else
                        theGNSS.pushRawData(&incoming, 1);

                    btLastByteReceived = millis();
                    btEscapeCharsReceived = 0; // Update timeout check for escape char and partial frame

                    bluetoothIncomingRTCM = true;
                } // End just a character in the stream

            } // End btPrintEcho == false && bluetoothRxDataAvailable()

        } // End bluetoothGetState() == BT_CONNECTED

        if (bluetoothOutgoingToZedHead > 0 && ((millis() - lastZedI2CSend) > 100))
        {
            sendZedI2CBuffer(); // Send any outstanding RTCM
        }

        if (settings.enableTaskReports == true)
            systemPrintf("SerialWriteTask High watermark: %d\r\n", uxTaskGetStackHighWaterMark(nullptr));

        feedWdt();
        taskYIELD();
    } // End while(true)
}

// Add byte to buffer that will be sent to ZED
// We cannot write single characters to the ZED over I2C (as this will change the address pointer)
void addToZedI2CBuffer(uint8_t incoming)
{
    bluetoothOutgoingToZed[bluetoothOutgoingToZedHead] = incoming;

    bluetoothOutgoingToZedHead++;
    if (bluetoothOutgoingToZedHead == sizeof(bluetoothOutgoingToZed))
    {
        sendZedI2CBuffer();
    }
}

// Push the buffered data in bulk to the GNSS over I2C
bool sendZedI2CBuffer()
{
    bool response = theGNSS.pushRawData(bluetoothOutgoingToZed, bluetoothOutgoingToZedHead);

    if (response == true)
    {
        // log_d("Pushed %d bytes RTCM to ZED", bluetoothOutgoingToZedHead);
    }

    // No matter the response, wrap the head and reset the timer
    bluetoothOutgoingToZedHead = 0;
    lastZedI2CSend = millis();
    return (response);
}

// Normally a delay(1) will feed the WDT but if we don't want to wait that long, this feeds the WDT without delay
void feedWdt()
{
    vTaskDelay(1);
}

//----------------------------------------------------------------------
// The ESP32<->ZED-F9P serial connection is default 230,400bps to facilitate
// 10Hz fix rate with PPP Logging Defaults (NMEAx5 + RXMx2) messages enabled.
// ESP32 UART2 is begun with settings.uartReceiveBufferSize size buffer. The circular buffer
// is 1024*6. At approximately 46.1K characters/second, a 6144 * 2
// byte buffer should hold 267ms worth of serial data. Assuming SD writes are
// 250ms worst case, we should record incoming all data. Bluetooth congestion
// or conflicts with the SD card semaphore should clear within this time.
//
// Ring buffer empty when (dataHead == btTail) and (dataHead == sdTail)
//
//        +---------+
//        |         |
//        |         |
//        |         |
//        |         |
//        +---------+ <-- dataHead, btTail, sdTail
//
// Ring buffer contains data when (dataHead != btTail) or (dataHead != sdTail)
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
// Maximum ring buffer fill is settings.gnssHandlerBufferSize - 1
//----------------------------------------------------------------------

// Read bytes from ZED-F9P UART1 into ESP32 circular buffer
// If data is coming in at 230,400bps = 23,040 bytes/s = one byte every 0.043ms
// If SD blocks for 150ms (not extraordinary) that is 3,488 bytes that must be buffered
// The ESP32 Arduino FIFO is ~120 bytes by default but overridden to 50 bytes (see pinUART2Task() and
// uart_set_rx_full_threshold()). We use this task to harvest from FIFO into circular buffer during SD write blocking
// time.
void gnssReadTask(void *e)
{
    static PARSE_STATE parse = {waitForPreamble, processUart1Message, "Log"};

    uint8_t incomingData = 0;

    availableHandlerSpace = settings.gnssHandlerBufferSize;

    while (true)
    {
        if (settings.enableTaskReports == true)
            systemPrintf("SerialReadTask High watermark: %d\r\n", uxTaskGetStackHighWaterMark(nullptr));

        // Determine if serial data is available
        if (USE_I2C_GNSS)
        {
            while (serialGNSS.available())
            {
                // Read the data from UART1
                uint8_t incomingData[500];
                int bytesIncoming = serialGNSS.read(incomingData, sizeof(incomingData));

                for (int x = 0; x < bytesIncoming; x++)
                {
                    // Save the data byte
                    parse.buffer[parse.length++] = incomingData[x];
                    parse.length %= PARSE_BUFFER_LENGTH;

                    // Compute the CRC value for the message
                    if (parse.computeCrc)
                        parse.crc = COMPUTE_CRC24Q(&parse, incomingData[x]);

                    // Update the parser state based on the incoming byte
                    parse.state(&parse, incomingData[x]);
                }
            }
        }
        else // SPI GNSS
        {
            theGNSS.checkUblox(); // Check for new data
            while (theGNSS.fileBufferAvailable() > 0)
            {
                // Read the data from the logging buffer
                theGNSS.extractFileBufferData(&incomingData,
                                              1); // TODO: make this more efficient by reading multiple bytes?

                // Save the data byte
                parse.buffer[parse.length++] = incomingData;
                parse.length %= PARSE_BUFFER_LENGTH;

                // Compute the CRC value for the message
                if (parse.computeCrc)
                    parse.crc = COMPUTE_CRC24Q(&parse, incomingData);

                // Update the parser state based on the incoming byte
                parse.state(&parse, incomingData);
            }
        }

        feedWdt();
        taskYIELD();
    }
}

// Process a complete message incoming from parser
// If we get a complete NMEA/UBX/RTCM sentence, pass on to SD/BT/TCP interfaces
void processUart1Message(PARSE_STATE *parse, uint8_t type)
{
    uint16_t bytesToCopy;
    uint16_t remainingBytes;

    // Display the message
    if (settings.enablePrintLogFileMessages && (!parse->crc) && (!inMainMenu))
    {
        printTimeStamp();
        switch (type)
        {
        case SENTENCE_TYPE_NMEA:
            systemPrintf("    %s NMEA %s, %2d bytes\r\n", parse->parserName, parse->nmeaMessageName, parse->length);
            break;

        case SENTENCE_TYPE_RTCM:
            systemPrintf("    %s RTCM %d, %2d bytes\r\n", parse->parserName, parse->message, parse->length);
            break;

        case SENTENCE_TYPE_UBX:
            systemPrintf("    %s UBX %d.%d, %2d bytes\r\n", parse->parserName, parse->message >> 8,
                         parse->message & 0xff, parse->length);
            break;
        }
    }

    // Determine if this message will fit into the ring buffer
    bytesToCopy = parse->length;
    if ((bytesToCopy > availableHandlerSpace) && (!inMainMenu))
    {
        systemPrintf("Ring buffer full: discarding %d bytes (availableHandlerSpace is %d / %d)\r\n", bytesToCopy,
                     availableHandlerSpace, settings.gnssHandlerBufferSize);
        return;
    }

    // Account for this message
    availableHandlerSpace -= bytesToCopy;

    // Fill the buffer to the end and then start at the beginning
    if ((dataHead + bytesToCopy) > settings.gnssHandlerBufferSize)
        bytesToCopy = settings.gnssHandlerBufferSize - dataHead;

    // Display the dataHead offset
    if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
        systemPrintf("DH: %4d --> ", dataHead);

    // Copy the data into the ring buffer
    memcpy(&ringBuffer[dataHead], parse->buffer, bytesToCopy);
    dataHead += bytesToCopy;
    if (dataHead >= settings.gnssHandlerBufferSize)
        dataHead -= settings.gnssHandlerBufferSize;

    // Determine the remaining bytes
    remainingBytes = parse->length - bytesToCopy;
    if (remainingBytes)
    {
        // Copy the remaining bytes into the beginning of the ring buffer
        memcpy(ringBuffer, &parse->buffer[bytesToCopy], remainingBytes);
        dataHead += remainingBytes;
        if (dataHead >= settings.gnssHandlerBufferSize)
            dataHead -= settings.gnssHandlerBufferSize;
    }

    // Display the dataHead offset
    if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
        systemPrintf("%4d\r\n", dataHead);
}

// If new data is in the ringBuffer, dole it out to appropriate interface
// Send data out Bluetooth, record to SD, or send over TCP
void handleGnssDataTask(void *e)
{
    volatile static uint16_t btTail = 0;          // BT Tail advances as it is sent over BT
    volatile static uint16_t tcpTailWiFi = 0;     // TCP client tail
    volatile static uint16_t tcpTailEthernet = 0; // TCP client tail
    volatile static uint16_t sdTail = 0;          // SD Tail advances as it is recorded to SD

    int btBytesToSend;          // Amount of buffered Bluetooth data
    int tcpBytesToSendWiFi;     // Amount of buffered TCP data
    int tcpBytesToSendEthernet; // Amount of buffered TCP data
    int sdBytesToRecord;        // Amount of buffered microSD card logging data

    int btConnected; // Is the device in a state to send Bluetooth data?

    while (true)
    {
        // Determine the amount of Bluetooth data in the buffer
        btBytesToSend = 0;

        // Determine BT connection state
        btConnected = (bluetoothGetState() == BT_CONNECTED) && (systemState != STATE_BASE_TEMP_SETTLE) &&
                      (systemState != STATE_BASE_TEMP_SURVEY_STARTED);

        if (btConnected)
        {
            btBytesToSend = dataHead - btTail;
            if (btBytesToSend < 0)
                btBytesToSend += settings.gnssHandlerBufferSize;
        }

        // Determine the amount of TCP data in the buffer
        tcpBytesToSendWiFi = 0;
        if (settings.enableTcpServer || settings.enableTcpClient)
        {
            tcpBytesToSendWiFi = dataHead - tcpTailWiFi;
            if (tcpBytesToSendWiFi < 0)
                tcpBytesToSendWiFi += settings.gnssHandlerBufferSize;
        }

        tcpBytesToSendEthernet = 0;
        if (settings.enableTcpClientEthernet)
        {
            tcpBytesToSendEthernet = dataHead - tcpTailEthernet;
            if (tcpBytesToSendEthernet < 0)
                tcpBytesToSendEthernet += settings.gnssHandlerBufferSize;
        }

        // Determine the amount of microSD card logging data in the buffer
        sdBytesToRecord = 0;
        if (online.logging)
        {
            sdBytesToRecord = dataHead - sdTail;
            if (sdBytesToRecord < 0)
                sdBytesToRecord += settings.gnssHandlerBufferSize;
        }

        //----------------------------------------------------------------------
        // Send data over Bluetooth
        //----------------------------------------------------------------------

        if (!btConnected)
            // Discard the data
            btTail = dataHead;
        else if (btBytesToSend > 0)
        {
            // Reduce bytes to send if we have more to send then the end of the buffer
            // We'll wrap next loop
            if ((btTail + btBytesToSend) > settings.gnssHandlerBufferSize)
                btBytesToSend = settings.gnssHandlerBufferSize - btTail;

            // If we are in the config menu, supress data flowing from ZED to cell phone
            if (btPrintEcho == false)
                // Push new data to BT SPP
                btBytesToSend = bluetoothWrite(&ringBuffer[btTail], btBytesToSend);

            if (btBytesToSend > 0)
            {
                // If we are in base mode, assume part of the outgoing data is RTCM
                if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
                    bluetoothOutgoingRTCM = true;

                // Account for the sent or dropped data
                btTail += btBytesToSend;
                if (btTail >= settings.gnssHandlerBufferSize)
                    btTail -= settings.gnssHandlerBufferSize;
            }
            else
                log_w("BT failed to send");
        }

        //----------------------------------------------------------------------
        // Send data to the TCP clients
        //----------------------------------------------------------------------

        if (((!online.tcpServer) && (!online.tcpClient)) || (!wifiTcpConnected))
            tcpTailWiFi = dataHead;
        else if (tcpBytesToSendWiFi > 0)
        {
            // Reduce bytes to send if we have more to send then the end of the buffer
            // We'll wrap next loop
            if ((tcpTailWiFi + tcpBytesToSendWiFi) > settings.gnssHandlerBufferSize)
                tcpBytesToSendWiFi = settings.gnssHandlerBufferSize - tcpTailWiFi;

            // Send the data to the TCP clients
            wifiSendTcpData(&ringBuffer[tcpTailWiFi], tcpBytesToSendWiFi);

            // Assume all data was sent, wrap the buffer pointer
            tcpTailWiFi += tcpBytesToSendWiFi;
            if (tcpTailWiFi >= settings.gnssHandlerBufferSize)
                tcpTailWiFi -= settings.gnssHandlerBufferSize;
        }

        if ((!online.tcpClientEthernet) || (!ethernetTcpConnected))
            tcpTailEthernet = dataHead;
        else if (tcpBytesToSendEthernet > 0)
        {
            // Reduce bytes to send if we have more to send then the end of the buffer
            // We'll wrap next loop
            if ((tcpTailEthernet + tcpBytesToSendEthernet) > settings.gnssHandlerBufferSize)
                tcpBytesToSendEthernet = settings.gnssHandlerBufferSize - tcpTailEthernet;

            // Send the data to the TCP clients
            ethernetSendTcpData(&ringBuffer[tcpTailEthernet], tcpBytesToSendEthernet);

            // Assume all data was sent, wrap the buffer pointer
            tcpTailEthernet += tcpBytesToSendEthernet;
            if (tcpTailEthernet >= settings.gnssHandlerBufferSize)
                tcpTailEthernet -= settings.gnssHandlerBufferSize;
        }

        //----------------------------------------------------------------------
        // Log data to the SD card
        //----------------------------------------------------------------------

        // If user wants to log, record to SD
        if (!online.logging)
            // Discard the data
            sdTail = dataHead;
        else if (sdBytesToRecord > 0)
        {
            // Check if we are inside the max time window for logging
            if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
            {
                // Attempt to gain access to the SD card, avoids collisions with file
                // writing from other functions like recordSystemSettingsToFile()
                if (xSemaphoreTake(sdCardSemaphore, loggingSemaphoreWait_ms) == pdPASS)
                {
                    markSemaphore(FUNCTION_WRITESD);

                    // Reduce bytes to record if we have more then the end of the buffer
                    int sliceToRecord = sdBytesToRecord;
                    if ((sdTail + sliceToRecord) > settings.gnssHandlerBufferSize)
                        sliceToRecord = settings.gnssHandlerBufferSize - sdTail;

                    if (settings.enablePrintSDBuffers && (!inMainMenu))
                    {
                        int bufferAvailable;
                        if (USE_I2C_GNSS)
                            bufferAvailable = serialGNSS.available();
                        else
                        {
                            theGNSS.checkUblox();
                            bufferAvailable = theGNSS.fileBufferAvailable();
                        }
                        int availableUARTSpace;
                        if (USE_I2C_GNSS)
                            availableUARTSpace = settings.uartReceiveBufferSize - bufferAvailable;
                        else
                            // Use gnssHandlerBufferSize for now. TODO: work out if the SPI GNSS needs its own buffer
                            // size setting
                            availableUARTSpace = settings.gnssHandlerBufferSize - bufferAvailable;
                        systemPrintf("SD Incoming Serial: %04d\tToRead: %04d\tMovedToBuffer: %04d\tavailableUARTSpace: "
                                     "%04d\tavailableHandlerSpace: %04d\tToRecord: %04d\tRecorded: %04d\tBO: %d\r\n",
                                     bufferAvailable, 0, 0, availableUARTSpace, availableHandlerSpace, sliceToRecord, 0,
                                     bufferOverruns);
                    }

                    // Write the data to the file
                    long startTime = millis();

                    int sdBytesRecorded = ubxFile->write(&ringBuffer[sdTail], sliceToRecord);

                    static unsigned long lastFlush = 0;
                    if (USE_MMC_MICROSD)
                    {
                        if (millis() > (lastFlush + 250)) // Flush every 250ms, not every write
                        {
                            ubxFile->flush();
                            lastFlush += 250;
                        }
                    }
                    fileSize = ubxFile->fileSize(); // Update file size

                    sdFreeSpace -= sdBytesRecorded; // Update remaining space on SD

                    // Force file sync every 60s
                    if (millis() - lastUBXLogSyncTime > 60000)
                    {
                        if (productVariant == RTK_SURVEYOR)
                            digitalWrite(pin_baseStatusLED,
                                         !digitalRead(pin_baseStatusLED)); // Blink LED to indicate logging activity

                        ubxFile->sync();
                        ubxFile->updateFileAccessTimestamp(); // Update the file access time & date

                        if (productVariant == RTK_SURVEYOR)
                            digitalWrite(pin_baseStatusLED,
                                         !digitalRead(pin_baseStatusLED)); // Return LED to previous state

                        lastUBXLogSyncTime = millis();
                    }

                    long endTime = millis();

                    if (settings.enablePrintBufferOverrun)
                    {
                        if (endTime - startTime > 150)
                            systemPrintf("Long Write! Time: %ld ms / Location: %ld / Recorded %d bytes / "
                                         "spaceRemaining %d bytes\r\n",
                                         endTime - startTime, fileSize, sdBytesRecorded, combinedSpaceRemaining);
                    }

                    xSemaphoreGive(sdCardSemaphore);

                    // Account for the sent data or dropped
                    if (sdBytesRecorded > 0)
                    {
                        sdTail += sdBytesRecorded;
                        if (sdTail >= settings.gnssHandlerBufferSize)
                            sdTail -= settings.gnssHandlerBufferSize;
                    }
                } // End sdCardSemaphore
                else
                {
                    char semaphoreHolder[50];
                    getSemaphoreFunction(semaphoreHolder);
                    log_w("sdCardSemaphore failed to yield for SD write, held by %s, Tasks.ino line %d",
                          semaphoreHolder, __LINE__);

                    delay(1); // Needed to prevent WDT resets during long Record Settings locks
                    taskYIELD();
                }
            } // End maxLogTime
        }     // End logging

        // Update space available for use in UART task
        btBytesToSend = dataHead - btTail;
        if (btBytesToSend < 0)
            btBytesToSend += settings.gnssHandlerBufferSize;

        tcpBytesToSendWiFi = dataHead - tcpTailWiFi;
        if (tcpBytesToSendWiFi < 0)
            tcpBytesToSendWiFi += settings.gnssHandlerBufferSize;

        tcpBytesToSendEthernet = dataHead - tcpTailEthernet;
        if (tcpBytesToSendEthernet < 0)
            tcpBytesToSendEthernet += settings.gnssHandlerBufferSize;

        sdBytesToRecord = dataHead - sdTail;
        if (sdBytesToRecord < 0)
            sdBytesToRecord += settings.gnssHandlerBufferSize;

        // Determine the inteface that is most behind: SD writing, SPP transmission, or TCP transmission
        int usedSpace = 0;
        if (tcpBytesToSendWiFi >= btBytesToSend && tcpBytesToSendWiFi >= sdBytesToRecord &&
            tcpBytesToSendWiFi >= tcpBytesToSendEthernet)
            usedSpace = tcpBytesToSendWiFi;
        else if (tcpBytesToSendEthernet >= btBytesToSend && tcpBytesToSendEthernet >= sdBytesToRecord &&
                 tcpBytesToSendEthernet >= tcpBytesToSendWiFi)
            usedSpace = tcpBytesToSendEthernet;
        else if (btBytesToSend >= sdBytesToRecord && btBytesToSend >= tcpBytesToSendWiFi &&
                 btBytesToSend >= tcpBytesToSendEthernet)
            usedSpace = btBytesToSend;
        else
            usedSpace = sdBytesToRecord;

        availableHandlerSpace = settings.gnssHandlerBufferSize - usedSpace;

        // Don't fill the last byte to prevent buffer overflow
        if (availableHandlerSpace)
            availableHandlerSpace -= 1;

        //----------------------------------------------------------------------
        // Let other tasks run, prevent watch dog timer (WDT) resets
        //----------------------------------------------------------------------

        delay(1);
        taskYIELD();
    }
}

// Control BT status LED according to bluetoothGetState()
void updateBTled()
{
    if (productVariant == RTK_SURVEYOR)
    {
        // Blink on/off while we wait for BT connection
        if (bluetoothGetState() == BT_NOTCONNECTED)
        {
            if (btFadeLevel == 0)
                btFadeLevel = 255;
            else
                btFadeLevel = 0;
            ledcWrite(ledBTChannel, btFadeLevel);
        }

        // Solid LED if BT Connected
        else if (bluetoothGetState() == BT_CONNECTED)
            ledcWrite(ledBTChannel, 255);

        // Pulse LED while no BT and we wait for WiFi connection
        else if (wifiState == WIFI_CONNECTING || wifiState == WIFI_CONNECTED)
        {
            // Fade in/out the BT LED during WiFi AP mode
            btFadeLevel += pwmFadeAmount;
            if (btFadeLevel <= 0 || btFadeLevel >= 255)
                pwmFadeAmount *= -1;

            if (btFadeLevel > 255)
                btFadeLevel = 255;
            if (btFadeLevel < 0)
                btFadeLevel = 0;

            ledcWrite(ledBTChannel, btFadeLevel);
        }
        else
            ledcWrite(ledBTChannel, 0);
    }
}

// For RTK Express and RTK Facet, monitor momentary buttons
void ButtonCheckTask(void *e)
{
    uint8_t index;

    if (setupBtn != nullptr)
        setupBtn->begin();
    if (powerBtn != nullptr)
        powerBtn->begin();

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

            // When switch is set to '1' = BASE, pin will be shorted to ground
            if (setupBtn->isPressed()) // Switch is set to base mode
            {
                if (buttonPreviousState == BUTTON_ROVER)
                {
                    lastRockerSwitchChange = millis(); // Record for WiFi AP access
                    buttonPreviousState = BUTTON_BASE;
                    requestChangeState(STATE_BASE_NOT_STARTED);
                }
            }
            else if (setupBtn->wasReleased()) // Switch is set to Rover
            {
                if (buttonPreviousState == BUTTON_BASE)
                {
                    buttonPreviousState = BUTTON_ROVER;

                    // If quick toggle is detected (less than 500ms), enter WiFi AP Config mode
                    if (millis() - lastRockerSwitchChange < 500)
                    {
                        if (systemState == STATE_ROVER_NOT_STARTED && online.display == true) // Catch during Power On
                            requestChangeState(STATE_TEST); // If RTK Surveyor, with display attached, during Rover not
                                                            // started, then enter test mode
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
        else if (productVariant == RTK_EXPRESS ||
                 productVariant == RTK_EXPRESS_PLUS) // Express: Check both of the momentary switches
        {
            if (setupBtn != nullptr)
                setupBtn->read();
            if (powerBtn != nullptr)
                powerBtn->read();

            if (systemState == STATE_SHUTDOWN)
            {
                // Ignore button presses while shutting down
            }
            else if (powerBtn != nullptr && powerBtn->pressedFor(shutDownButtonTime))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_SHUTDOWN);

                if (inMainMenu)
                    powerDown(true); // State machine is not updated while in menu system so go straight to power down
                                     // as needed
            }
            else if ((setupBtn != nullptr && setupBtn->pressedFor(500)) &&
                     (powerBtn != nullptr && powerBtn->pressedFor(500)))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_TEST);
                lastTestMenuChange = millis(); // Avoid exiting test menu for 1s
            }
            else if (setupBtn != nullptr && setupBtn->wasReleased())
            {
                switch (systemState)
                {
                // If we are in any running state, change to STATE_DISPLAY_SETUP
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
                    lastSystemState =
                        systemState; // Remember this state to return after we mark an event or ESP-Now pair
                    requestChangeState(STATE_DISPLAY_SETUP);
                    setupState = STATE_MARK_EVENT;
                    lastSetupMenuChange = millis();
                    break;

                case STATE_MARK_EVENT:
                    // If the user presses the setup button during a mark event, do nothing
                    // Allow system to return to lastSystemState
                    break;

                case STATE_PROFILE:
                    // If the user presses the setup button during a profile change, do nothing
                    // Allow system to return to lastSystemState
                    break;

                case STATE_TEST:
                    // Do nothing. User is releasing the setup button.
                    break;

                case STATE_TESTING:
                    // If we are in testing, return to Rover Not Started
                    requestChangeState(STATE_ROVER_NOT_STARTED);
                    break;

                case STATE_DISPLAY_SETUP:
                    // If we are displaying the setup menu, cycle through possible system states
                    // Exit display setup and enter new system state after ~1500ms in updateSystemState()
                    lastSetupMenuChange = millis();

                    forceDisplayUpdate = true; // User is interacting so repaint display quickly

                    switch (setupState)
                    {
                    case STATE_MARK_EVENT:
                        setupState = STATE_ROVER_NOT_STARTED;
                        break;
                    case STATE_ROVER_NOT_STARTED:
                        // If F9R, skip base state
                        if (zedModuleType == PLATFORM_F9R)
                        {
                            // If accel offline, skip bubble
                            if (online.accelerometer == true)
                                setupState = STATE_BUBBLE_LEVEL;
                            else
                                setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        }
                        else
                            setupState = STATE_BASE_NOT_STARTED;
                        break;
                    case STATE_BASE_NOT_STARTED:
                        // If accel offline, skip bubble
                        if (online.accelerometer == true)
                            setupState = STATE_BUBBLE_LEVEL;
                        else
                            setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        break;
                    case STATE_BUBBLE_LEVEL:
                        setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        break;
                    case STATE_WIFI_CONFIG_NOT_STARTED:
                        setupState = STATE_ESPNOW_PAIRING_NOT_STARTED;
                        break;
                    case STATE_ESPNOW_PAIRING_NOT_STARTED:
                        // If only one active profile do not show any profiles
                        index = getProfileNumberFromUnit(0);
                        displayProfile = getProfileNumberFromUnit(1);
                        setupState = (index >= displayProfile) ? STATE_MARK_EVENT : STATE_PROFILE;
                        displayProfile = 0;
                        break;
                    case STATE_PROFILE:
                        // Done when no more active profiles
                        displayProfile++;
                        if (!getProfileNumberFromUnit(displayProfile))
                            setupState = STATE_MARK_EVENT;
                        break;
                    default:
                        systemPrintf("ButtonCheckTask unknown setup state: %d\r\n", setupState);
                        setupState = STATE_MARK_EVENT;
                        break;
                    }
                    break;

                default:
                    systemPrintf("ButtonCheckTask unknown system state: %d\r\n", systemState);
                    requestChangeState(STATE_ROVER_NOT_STARTED);
                    break;
                }
            }
        }                                                                          // End Platform = RTK Express
        else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND) // Check one momentary button
        {
            if (powerBtn != nullptr)
                powerBtn->read();

            if (systemState == STATE_SHUTDOWN)
            {
                // Ignore button presses while shutting down
            }
            else if (powerBtn != nullptr && powerBtn->pressedFor(shutDownButtonTime))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_SHUTDOWN);

                if (inMainMenu)
                    powerDown(true); // State machine is not updated while in menu system so go straight to power down
                                     // as needed
            }
            else if (powerBtn != nullptr && systemState == STATE_ROVER_NOT_STARTED && firstRoverStart == true &&
                     powerBtn->pressedFor(500))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_TEST);
                lastTestMenuChange = millis(); // Avoid exiting test menu for 1s
            }
            else if (powerBtn != nullptr && powerBtn->wasReleased() && firstRoverStart == false)
            {
                switch (systemState)
                {
                // If we are in any running state, change to STATE_DISPLAY_SETUP
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
                    lastSystemState =
                        systemState; // Remember this state to return after we mark an event or ESP-Now pair
                    requestChangeState(STATE_DISPLAY_SETUP);
                    setupState = STATE_MARK_EVENT;
                    lastSetupMenuChange = millis();
                    break;

                case STATE_MARK_EVENT:
                    // If the user presses the setup button during a mark event, do nothing
                    // Allow system to return to lastSystemState
                    break;

                case STATE_PROFILE:
                    // If the user presses the setup button during a profile change, do nothing
                    // Allow system to return to lastSystemState
                    break;

                case STATE_TEST:
                    // Do nothing. User is releasing the setup button.
                    break;

                case STATE_TESTING:
                    // If we are in testing, return to Rover Not Started
                    requestChangeState(STATE_ROVER_NOT_STARTED);
                    break;

                case STATE_DISPLAY_SETUP:
                    // If we are displaying the setup menu, cycle through possible system states
                    // Exit display setup and enter new system state after ~1500ms in updateSystemState()
                    lastSetupMenuChange = millis();

                    forceDisplayUpdate = true; // User is interacting so repaint display quickly

                    switch (setupState)
                    {
                    case STATE_MARK_EVENT:
                        setupState = STATE_ROVER_NOT_STARTED;
                        break;
                    case STATE_ROVER_NOT_STARTED:
                        // If F9R, skip base state
                        if (zedModuleType == PLATFORM_F9R)
                        {
                            // If accel offline, skip bubble
                            if (online.accelerometer == true)
                                setupState = STATE_BUBBLE_LEVEL;
                            else
                                setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        }
                        else
                            setupState = STATE_BASE_NOT_STARTED;
                        break;
                    case STATE_BASE_NOT_STARTED:
                        // If accel offline, skip bubble
                        if (online.accelerometer == true)
                            setupState = STATE_BUBBLE_LEVEL;
                        else
                            setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        break;
                    case STATE_BUBBLE_LEVEL:
                        setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        break;
                    case STATE_WIFI_CONFIG_NOT_STARTED:
                        setupState = STATE_ESPNOW_PAIRING_NOT_STARTED;
                        break;
                    case STATE_ESPNOW_PAIRING_NOT_STARTED:
                        // If only one active profile do not show any profiles
                        index = getProfileNumberFromUnit(0);
                        displayProfile = getProfileNumberFromUnit(1);
                        setupState = (index >= displayProfile) ? STATE_MARK_EVENT : STATE_PROFILE;
                        displayProfile = 0;
                        break;
                    case STATE_PROFILE:
                        // Done when no more active profiles
                        displayProfile++;
                        if (!getProfileNumberFromUnit(displayProfile))
                            setupState = STATE_MARK_EVENT;
                        break;
                    default:
                        systemPrintf("ButtonCheckTask unknown setup state: %d\r\n", setupState);
                        setupState = STATE_MARK_EVENT;
                        break;
                    }
                    break;

                default:
                    systemPrintf("ButtonCheckTask unknown system state: %d\r\n", systemState);
                    requestChangeState(STATE_ROVER_NOT_STARTED);
                    break;
                }
            }
        }                                             // End Platform = RTK Facet
        else if (productVariant == REFERENCE_STATION) // Check one momentary button
        {
            if (setupBtn != nullptr)
                setupBtn->read();

            if (systemState == STATE_SHUTDOWN)
            {
                // Ignore button presses while shutting down
            }
            else if (setupBtn != nullptr && setupBtn->pressedFor(shutDownButtonTime))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_SHUTDOWN);

                if (inMainMenu)
                    powerDown(true); // State machine is not updated while in menu system so go straight to power down
                                     // as needed
            }
            else if (setupBtn != nullptr && systemState == STATE_BASE_NOT_STARTED && firstRoverStart == true &&
                     setupBtn->pressedFor(500))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_TEST);
                lastTestMenuChange = millis(); // Avoid exiting test menu for 1s
            }
            else if (setupBtn != nullptr && setupBtn->wasReleased() && firstRoverStart == false)
            {
                switch (systemState)
                {
                // If we are in any running state, change to STATE_DISPLAY_SETUP
                case STATE_BASE_NOT_STARTED:
                case STATE_BASE_TEMP_SETTLE:
                case STATE_BASE_TEMP_SURVEY_STARTED:
                case STATE_BASE_TEMP_TRANSMITTING:
                case STATE_BASE_FIXED_NOT_STARTED:
                case STATE_BASE_FIXED_TRANSMITTING:
                case STATE_ROVER_NOT_STARTED:
                case STATE_ROVER_NO_FIX:
                case STATE_ROVER_FIX:
                case STATE_ROVER_RTK_FLOAT:
                case STATE_ROVER_RTK_FIX:
                case STATE_NTPSERVER_NOT_STARTED:
                case STATE_NTPSERVER_NO_SYNC:
                case STATE_NTPSERVER_SYNC:
                case STATE_WIFI_CONFIG_NOT_STARTED:
                case STATE_WIFI_CONFIG:
                case STATE_CONFIG_VIA_ETH_NOT_STARTED:
                case STATE_ESPNOW_PAIRING_NOT_STARTED:
                case STATE_ESPNOW_PAIRING:
                    lastSystemState = systemState; // Remember this state to return after ESP-Now pair
                    requestChangeState(STATE_DISPLAY_SETUP);
                    setupState = STATE_BASE_NOT_STARTED;
                    lastSetupMenuChange = millis();
                    break;

                case STATE_CONFIG_VIA_ETH_STARTED:
                case STATE_CONFIG_VIA_ETH:
                    // If the user presses the button during configure-via-ethernet, then do a complete restart into
                    // Base mode
                    requestChangeState(STATE_CONFIG_VIA_ETH_RESTART_BASE);
                    break;

                case STATE_PROFILE:
                    // If the user presses the setup button during a profile change, do nothing
                    // Allow system to return to lastSystemState
                    break;

                case STATE_TEST:
                    // Do nothing. User is releasing the setup button.
                    break;

                case STATE_TESTING:
                    // If we are in testing, return to Base Not Started
                    requestChangeState(STATE_BASE_NOT_STARTED);
                    break;

                case STATE_DISPLAY_SETUP:
                    // If we are displaying the setup menu, cycle through possible system states
                    // Exit display setup and enter new system state after ~1500ms in updateSystemState()
                    lastSetupMenuChange = millis();

                    forceDisplayUpdate = true; // User is interacting so repaint display quickly

                    switch (setupState)
                    {
                    case STATE_BASE_NOT_STARTED:
                        setupState = STATE_ROVER_NOT_STARTED;
                        break;
                    case STATE_ROVER_NOT_STARTED:
                        setupState = STATE_NTPSERVER_NOT_STARTED;
                        break;
                    case STATE_NTPSERVER_NOT_STARTED:
                        setupState = STATE_CONFIG_VIA_ETH_NOT_STARTED;
                        break;
                    case STATE_CONFIG_VIA_ETH_NOT_STARTED:
                        setupState = STATE_WIFI_CONFIG_NOT_STARTED;
                        break;
                    case STATE_WIFI_CONFIG_NOT_STARTED:
                        setupState = STATE_ESPNOW_PAIRING_NOT_STARTED;
                        break;
                    case STATE_ESPNOW_PAIRING_NOT_STARTED:
                        // If only one active profile do not show any profiles
                        index = getProfileNumberFromUnit(0);
                        displayProfile = getProfileNumberFromUnit(1);
                        setupState = (index >= displayProfile) ? STATE_BASE_NOT_STARTED : STATE_PROFILE;
                        displayProfile = 0;
                        break;
                    case STATE_PROFILE:
                        // Done when no more active profiles
                        displayProfile++;
                        if (!getProfileNumberFromUnit(displayProfile))
                            setupState = STATE_BASE_NOT_STARTED;
                        break;
                    case STATE_MARK_EVENT: // Skip the warning message if setupState is still in the default Mark Event
                                           // state
                        setupState = STATE_BASE_NOT_STARTED;
                        break;
                    default:
                        systemPrintf("ButtonCheckTask unknown setup state: %d\r\n", setupState);
                        setupState = STATE_BASE_NOT_STARTED;
                        break;
                    }
                    break;

                default:
                    systemPrintf("ButtonCheckTask unknown system state: %d\r\n", systemState);
                    requestChangeState(STATE_BASE_NOT_STARTED);
                    break;
                }
            }
        } // End Platform = REFERENCE_STATION

        delay(1); // Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
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
        // Increment a count during the idle time
        idleCount++;

        // Determine if it is time to print the CPU idle times
        if ((millis() - lastDisplayIdleTime) >= (IDLE_TIME_DISPLAY_SECONDS * 1000))
        {
            lastDisplayIdleTime = millis();

            // Get the idle time
            if (idleCount > max_idle_count)
                max_idle_count = idleCount;

            // Display the idle times
            if (settings.enablePrintIdleTime)
            {
                systemPrintf("CPU %d idle time: %d%% (%d/%d)\r\n", cpu, idleCount * 100 / max_idle_count, idleCount,
                             max_idle_count);

                // Print the task count
                if (cpu)
                    systemPrintf("%d Tasks\r\n", uxTaskGetNumberOfTasks());
            }

            // Restart the idle count for the next display time
            idleCount = 0;
        }

        // Display the high water mark if requested
        if ((settings.enableTaskReports == true) &&
            ((millis() - lastStackPrintTime) >= (IDLE_TIME_DISPLAY_SECONDS * 1000)))
        {
            lastStackPrintTime = millis();
            systemPrintf("idleTask %d High watermark: %d\r\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(nullptr));
        }

        // Let other same priority tasks run
        taskYIELD();
    }
}

// Serial Read/Write tasks for the F9P must be started after BT is up and running otherwise SerialBT->available will
// cause reboot
void tasksStartUART2()
{
    // Reads data from ZED and stores data into circular buffer
    if (gnssReadTaskHandle == nullptr)
        xTaskCreatePinnedToCore(gnssReadTask,                  // Function to call
                                "gnssRead",                    // Just for humans
                                gnssReadTaskStackSize,         // Stack Size
                                nullptr,                       // Task input parameter
                                settings.gnssReadTaskPriority, // Priority
                                &gnssReadTaskHandle,           // Task handle
                                settings.gnssReadTaskCore);    // Core where task should run, 0=core, 1=Arduino

    // Reads data from circular buffer and sends data to SD, SPP, or TCP
    if (handleGnssDataTaskHandle == nullptr)
        xTaskCreatePinnedToCore(handleGnssDataTask,                  // Function to call
                                "handleGNSSData",                    // Just for humans
                                handleGnssDataTaskStackSize,         // Stack Size
                                nullptr,                             // Task input parameter
                                settings.handleGnssDataTaskPriority, // Priority
                                &handleGnssDataTaskHandle,           // Task handle
                                settings.handleGnssDataTaskCore);    // Core where task should run, 0=core, 1=Arduino

    // Reads data from BT and sends to ZED
    if (btReadTaskHandle == nullptr)
        xTaskCreatePinnedToCore(btReadTask,                  // Function to call
                                "btRead",                    // Just for humans
                                btReadTaskStackSize,         // Stack Size
                                nullptr,                     // Task input parameter
                                settings.btReadTaskPriority, // Priority
                                &btReadTaskHandle,           // Task handle
                                settings.btReadTaskCore);    // Core where task should run, 0=core, 1=Arduino
}

// Stop tasks - useful when running firmware update or WiFi AP is running
void tasksStopUART2()
{
    // Delete tasks if running
    if (gnssReadTaskHandle != nullptr)
    {
        vTaskDelete(gnssReadTaskHandle);
        gnssReadTaskHandle = nullptr;
    }
    if (handleGnssDataTaskHandle != nullptr)
    {
        vTaskDelete(handleGnssDataTaskHandle);
        handleGnssDataTaskHandle = nullptr;
    }
    if (btReadTaskHandle != nullptr)
    {
        vTaskDelete(btReadTaskHandle);
        btReadTaskHandle = nullptr;
    }

    // Give the other CPU time to finish
    // Eliminates CPU bus hang condition
    delay(100);
}

// Checking the number of available clusters on the SD card can take multiple seconds
// Rather than blocking the system, we run a background task
// Once the size check is complete, the task is removed
void sdSizeCheckTask(void *e)
{
    while (true)
    {
        if (online.microSD && sdCardSize == 0)
        {
            // Attempt to gain access to the SD card
            if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
            {
                markSemaphore(FUNCTION_SDSIZECHECK);

                if (USE_SPI_MICROSD)
                {
                    csd_t csd;
                    sd->card()->readCSD(&csd); // Card Specific Data
                    sdCardSize = (uint64_t)512 * sd->card()->sectorCount();

                    sd->volumeBegin();

                    // Find available cluster/space
                    sdFreeSpace = sd->vol()->freeClusterCount(); // This takes a few seconds to complete
                    sdFreeSpace *= sd->vol()->sectorsPerCluster();
                    sdFreeSpace *= 512L; // Bytes per sector
                }
#ifdef COMPILE_SD_MMC
                else
                {
                    sdCardSize = SD_MMC.cardSize();
                    sdFreeSpace = SD_MMC.totalBytes() - SD_MMC.usedBytes();
                }
#endif  // COMPILE_SD_MMC

                xSemaphoreGive(sdCardSemaphore);

                // uint64_t sdUsedSpace = sdCardSize - sdFreeSpace; //Don't think of it as used, think of it as unusable

                String cardSize;
                stringHumanReadableSize(cardSize, sdCardSize);
                String freeSpace;
                stringHumanReadableSize(freeSpace, sdFreeSpace);
                systemPrintf("SD card size: %s / Free space: %s\r\n", cardSize, freeSpace);

                outOfSDSpace = false;

                sdSizeCheckTaskComplete = true;
            }
            else
            {
                char semaphoreHolder[50];
                getSemaphoreFunction(semaphoreHolder);
                log_d("sdCardSemaphore failed to yield, held by %s, Tasks.ino line %d\r\n", semaphoreHolder, __LINE__);
            }
        }

        delay(1);
        taskYIELD(); // Let other tasks run
    }
}
