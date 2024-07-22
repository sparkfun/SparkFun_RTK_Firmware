/*------------------------------------------------------------------------------
Tasks.ino

  This module implements the high frequency tasks made by xTaskCreate() and any
  low frequency tasks that are called by Ticker.

                                   GNSS
                                    |
                                    v
                           .--------+--------.
                           |                 |
                           v                 v
                          SPI       or      I2C
                           |                 |
                           |                 |
                           '------->+<-------'
                                    |
                                    | gnssReadTask
                                    |    gpsMessageParserFirstByte
                                    |        ...
                                    |    processUart1Message
                                    |
                                    v
                               Ring Buffer
                                    |
                                    | handleGnssDataTask
                                    |
                                    v
            .---------------+-------+-------+---------------+
            |               |               |               |
            |               |               |               |
            v               v               v               v
        Bluetooth      PVT Client      PVT Server        SD Card

------------------------------------------------------------------------------*/

//----------------------------------------
// Macros
//----------------------------------------

#define WRAP_OFFSET(offset, increment, arraySize)                                                                      \
    {                                                                                                                  \
        offset += increment;                                                                                           \
        if (offset >= arraySize)                                                                                       \
            offset -= arraySize;                                                                                       \
    }

//----------------------------------------
// Constants
//----------------------------------------

enum RingBufferConsumers
{
    RBC_BLUETOOTH = 0,
    RBC_PVT_CLIENT,
    RBC_PVT_SERVER,
    RBC_SD_CARD,
    RBC_PVT_UDP_SERVER,
    // Insert new consumers here
    RBC_MAX
};

const char *const ringBufferConsumer[] = {
    "Bluetooth", "PVT Client", "PVT Server", "SD Card", "PVT UDP Server",
};

const int ringBufferConsumerEntries = sizeof(ringBufferConsumer) / sizeof(ringBufferConsumer[0]);

//----------------------------------------
// Locals
//----------------------------------------

volatile static RING_BUFFER_OFFSET dataHead; // Head advances as data comes in from GNSS's UART
volatile int32_t availableHandlerSpace;      // settings.gnssHandlerBufferSize - usedSpace
volatile const char *slowConsumer;

// Buffer the incoming Bluetooth stream so that it can be passed in bulk over I2C
uint8_t bluetoothOutgoingToZed[100];
uint16_t bluetoothOutgoingToZedHead;
unsigned long lastZedI2CSend; // Timestamp of the last time we sent RTCM ZED over I2C

// Ring buffer tails
static RING_BUFFER_OFFSET btRingBufferTail; // BT Tail advances as it is sent over BT
static RING_BUFFER_OFFSET sdRingBufferTail; // SD Tail advances as it is recorded to SD

// Ring buffer offsets
static uint16_t rbOffsetHead;

//----------------------------------------
// Task routines
//----------------------------------------

// If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
// Scan for escape characters to enter config menu
void btReadTask(void *e)
{
    int rxBytes;

    while (true)
    {
        // Display an alive message
        if (PERIODIC_DISPLAY(PD_TASK_BLUETOOTH_READ))
        {
            PERIODIC_CLEAR(PD_TASK_BLUETOOTH_READ);
            systemPrintln("btReadTask running");
        }

        // Receive RTCM corrections or UBX config messages over bluetooth and pass along to ZED
        rxBytes = 0;
        if (bluetoothGetState() == BT_CONNECTED)
        {
            while (btPrintEcho == false && bluetoothRxDataAvailable())
            {
                // Check stream for command characters
                byte incoming = bluetoothRead();
                rxBytes += 1;

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

                    // Record the arrival of RTCM from the Bluetooth connection (a phone or tablet is providing the RTCM
                    // via NTRIP). This resets the RTCM timeout used on the L-Band.
                    rtcmLastPacketReceived = millis();

                } // End just a character in the stream

            } // End btPrintEcho == false && bluetoothRxDataAvailable()

            if (PERIODIC_DISPLAY(PD_BLUETOOTH_DATA_RX))
            {
                PERIODIC_CLEAR(PD_BLUETOOTH_DATA_RX);
                systemPrintf("Bluetooth received %d bytes\r\n", rxBytes);
            }
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
        if (PERIODIC_DISPLAY(PD_ZED_DATA_TX))
        {
            PERIODIC_CLEAR(PD_ZED_DATA_TX);
            systemPrintf("ZED TX: Sending %d bytes from I2C\r\n", bluetoothOutgoingToZedHead);
        }
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
// Ring buffer empty when all the tails == dataHead
//
//        +---------+
//        |         |
//        |         |
//        |         |
//        |         |
//        +---------+ <-- dataHead, btRingBufferTail, sdRingBufferTail, etc.
//
// Ring buffer contains data when any tail != dataHead
//
//        +---------+
//        |         |
//        |         |
//        | yyyyyyy | <-- dataHead
//        | xxxxxxx | <-- btRingBufferTail (1 byte in buffer)
//        +---------+ <-- sdRingBufferTail (2 bytes in buffer)
//
//        +---------+
//        | yyyyyyy | <-- btRingBufferTail (1 byte in buffer)
//        | xxxxxxx | <-- sdRingBufferTail (2 bytes in buffer)
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
    static PARSE_STATE parse = {gpsMessageParserFirstByte, processUart1Message, "Log"};

    uint8_t incomingData = 0;

    while (true)
    {
        // Display an alive message
        if (PERIODIC_DISPLAY(PD_TASK_GNSS_READ))
        {
            PERIODIC_CLEAR(PD_TASK_GNSS_READ);
            systemPrintln("gnssReadTask running");
        }

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
// If we get a complete NMEA/UBX/RTCM message, pass on to SD/BT/PVT interfaces
void processUart1Message(PARSE_STATE *parse, uint8_t type)
{
    int32_t bytesToCopy;
    const char *consumer;
    RING_BUFFER_OFFSET remainingBytes;
    int32_t space;
    int32_t use;

    // Display the message
    if ((settings.enablePrintLogFileMessages || PERIODIC_DISPLAY(PD_ZED_DATA_RX)) && (!parse->crc) && (!inMainMenu))
    {
        PERIODIC_CLEAR(PD_ZED_DATA_RX);
        if (settings.enablePrintLogFileMessages)
        {
            printTimeStamp();
            systemPrint("    ");
        }
        else
            systemPrint("ZED RX: ");
        switch (type)
        {
        case SENTENCE_TYPE_NMEA:
            systemPrintf("%s NMEA %s, %2d bytes\r\n", parse->parserName, parse->nmeaMessageName, parse->length);
            break;

        case SENTENCE_TYPE_RTCM:
            systemPrintf("%s RTCM %d, %2d bytes\r\n", parse->parserName, parse->message, parse->length);
            break;

        case SENTENCE_TYPE_UBX:
            systemPrintf("%s UBX %d.%d, %2d bytes\r\n", parse->parserName, parse->message >> 8, parse->message & 0xff,
                         parse->length);
            break;
        }
    }

    // Determine if this message will fit into the ring buffer
    bytesToCopy = parse->length;
    space = availableHandlerSpace;
    use = settings.gnssHandlerBufferSize - space;
    consumer = (char *)slowConsumer;
    if ((bytesToCopy > space) && (!inMainMenu))
    {
        int32_t bufferedData;
        int32_t bytesToDiscard;
        int32_t discardedBytes;
        int32_t listEnd;
        int32_t messageLength;
        int32_t offsetBytes;
        int32_t previousTail;
        int32_t rbOffsetTail;

        // Determine the tail of the ring buffer
        previousTail = dataHead + space + 1;
        if (previousTail >= settings.gnssHandlerBufferSize)
            previousTail -= settings.gnssHandlerBufferSize;

        /*  The rbOffsetArray holds the offsets into the ring buffer of the
         *  start of each of the parsed messages.  A head (rbOffsetHead) and
         *  tail (rbOffsetTail) offsets are used for this array to insert and
         *  remove entries.  Typically this task only manipulates the head as
         *  new messages are placed into the ring buffer.  The handleGnssDataTask
         *  normally manipulates the tail as data is removed from the buffer.
         *  However this task will manipulate the tail under two conditions:
         *
         *  1.  The ring buffer gets full and data must be discarded
         *
         *  2.  The rbOffsetArray is too small to hold all of the message
         *      offsets for the data in the ring buffer.  The array is full
         *      when (Head + 1) == Tail
         *
         *  Notes:
         *      The rbOffsetArray is allocated along with the ring buffer in
         *      Begin.ino
         *
         *      The first entry rbOffsetArray[0] is initialized to zero (0)
         *      in Begin.ino
         *
         *      The array always has one entry in it containing the head offset
         *      which contains a valid offset into the ringBuffer, handled below
         *
         *      The empty condition is Tail == Head
         *
         *      The amount of data described by the rbOffsetArray is
         *      rbOffsetArray[Head] - rbOffsetArray[Tail]
         *
         *              rbOffsetArray                  ringBuffer
         *           .-----------------.           .-----------------.
         *           |                 |           |                 |
         *           +-----------------+           |                 |
         *  Tail --> |   Msg 1 Offset  |---------->+-----------------+ <-- Tail n
         *           +-----------------+           |      Msg 1      |
         *           |   Msg 2 Offset  |--------.  |                 |
         *           +-----------------+        |  |                 |
         *           |   Msg 3 Offset  |------. '->+-----------------+
         *           +-----------------+      |    |      Msg 2      |
         *  Head --> |   Head Offset   |--.   |    |                 |
         *           +-----------------+  |   |    |                 |
         *           |                 |  |   |    |                 |
         *           +-----------------+  |   |    |                 |
         *           |                 |  |   '--->+-----------------+
         *           +-----------------+  |        |      Msg 3      |
         *           |                 |  |        |                 |
         *           +-----------------+  '------->+-----------------+ <-- dataHead
         *           |                 |           |                 |
         */

        // Determine the index for the end of the circular ring buffer
        // offset list
        listEnd = rbOffsetHead;
        WRAP_OFFSET(listEnd, 1, rbOffsetEntries);

        // Update the tail, walk newest message to oldest message
        rbOffsetTail = rbOffsetHead;
        bufferedData = 0;
        messageLength = 0;
        while ((rbOffsetTail != listEnd) && (bufferedData < use))
        {
            // Determine the amount of data in the ring buffer up until
            // either the tail or the end of the rbOffsetArray
            //
            //                      |           |
            //                      |           | Valid, still in ring buffer
            //                      |  Newest   |
            //                      +-----------+ <-- rbOffsetHead
            //                      |           |
            //                      |           | free space
            //                      |           |
            //     rbOffsetTail --> +-----------+ <-- bufferedData
            //                      |   ring    |
            //                      |  buffer   | <-- used
            //                      |   data    |
            //                      +-----------+ Valid, still in ring buffer
            //                      |           |
            //
            messageLength = rbOffsetArray[rbOffsetTail];
            WRAP_OFFSET(rbOffsetTail, rbOffsetEntries - 1, rbOffsetEntries);
            messageLength -= rbOffsetArray[rbOffsetTail];
            if (messageLength < 0)
                messageLength += settings.gnssHandlerBufferSize;
            bufferedData += messageLength;
        }

        // Account for any data in the ring buffer not described by the array
        //
        //                      |           |
        //                      +-----------+
        //                      |  Oldest   |
        //                      |           |
        //                      |   ring    |
        //                      |  buffer   | <-- used
        //                      |   data    |
        //                      +-----------+ Valid, still in ring buffer
        //                      |           |
        //     rbOffsetTail --> +-----------+ <-- bufferedData
        //                      |           |
        //                      |  Newest   |
        //                      +-----------+ <-- rbOffsetHead
        //                      |           |
        //
        discardedBytes = 0;
        if (bufferedData < use)
            discardedBytes = use - bufferedData;

        // Writing to the SD card, the network or Bluetooth, a partial
        // message may be written leaving the tail pointer mid-message
        //
        //                      |           |
        //     rbOffsetTail --> +-----------+
        //                      |  Oldest   |
        //                      |           |
        //                      |   ring    |
        //                      |  buffer   | <-- used
        //                      |   data    | Valid, still in ring buffer
        //                      +-----------+ <--
        //                      |           |
        //                      +-----------+
        //                      |           |
        //                      |  Newest   |
        //                      +-----------+ <-- rbOffsetHead
        //                      |           |
        //
        else if (bufferedData > use)
        {
            // Remove the remaining portion of the oldest entry in the array
            discardedBytes = messageLength + use - bufferedData;
            WRAP_OFFSET(rbOffsetTail, 1, rbOffsetEntries);
        }

        // rbOffsetTail now points to the beginning of a message in the
        // ring buffer
        // Determine the amount of data to discard
        bytesToDiscard = discardedBytes;
        if (bytesToDiscard < bytesToCopy)
            bytesToDiscard = bytesToCopy;
        if (bytesToDiscard < AMOUNT_OF_RING_BUFFER_DATA_TO_DISCARD)
            bytesToDiscard = AMOUNT_OF_RING_BUFFER_DATA_TO_DISCARD;

        // Walk the ring buffer messages from oldest to newest
        while ((discardedBytes < bytesToDiscard) && (rbOffsetTail != rbOffsetHead))
        {
            // Determine the length of the oldest message
            WRAP_OFFSET(rbOffsetTail, 1, rbOffsetEntries);
            discardedBytes = rbOffsetArray[rbOffsetTail] - previousTail;
            if (discardedBytes < 0)
                discardedBytes += settings.gnssHandlerBufferSize;
        }

        // Discard the oldest data from the ring buffer
        if (consumer)
            systemPrintf("Ring buffer full: discarding %d bytes, %s is slow\r\n", discardedBytes, consumer);
        else
            systemPrintf("Ring buffer full: discarding %d bytes\r\n", discardedBytes);
        updateRingBufferTails(previousTail, rbOffsetArray[rbOffsetTail]);
        availableHandlerSpace += discardedBytes;
    }

    // Add another message to the ring buffer
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

    // Add the head offset to the offset array
    WRAP_OFFSET(rbOffsetHead, 1, rbOffsetEntries);
    rbOffsetArray[rbOffsetHead] = dataHead;

    // Display the dataHead offset
    if (settings.enablePrintRingBufferOffsets && (!inMainMenu))
        systemPrintf("%4d\r\n", dataHead);
}

// Remove previous messages from the ring buffer
void updateRingBufferTails(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail)
{
    // Trim any long or medium tails
    discardRingBufferBytes(&btRingBufferTail, previousTail, newTail);
    discardRingBufferBytes(&sdRingBufferTail, previousTail, newTail);
    discardPvtClientBytes(previousTail, newTail);
    discardPvtServerBytes(previousTail, newTail);
    discardPvtUdpServerBytes(previousTail, newTail);
}

// Remove previous messages from the ring buffer
void discardRingBufferBytes(RING_BUFFER_OFFSET *tail, RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail)
{
    // The longest tail is being trimmed.  Medium length tails may contain
    // some data within the region begin trimmed.  The shortest tails will
    // be trimmed.
    //
    // Devices that get their tails trimmed, may output a partial message
    // prior to the buffer trimming.  After the trimming, the tail of the
    // ring buffer points to the beginning of a new message.
    //
    //                 previousTail                newTail
    //                      |                         |
    //  Before trimming     v         Discarded       v   After trimming
    //  ----+-----------------  ...  -----+--  ..  ---+-----------+------
    //      | Partial message             |           |           |
    //  ----+-----------------  ...  -----+--  ..  ---+-----------+------
    //                      ^          ^                     ^
    //                      |          |                     |
    //        long tail ----'          '--- medium tail      '-- short tail
    //
    // Determine if the trimmed data wraps the end of the buffer
    if (previousTail < newTail)
    {
        // No buffer wrap occurred
        // Only discard the data from long and medium tails
        if ((*tail >= previousTail) && (*tail < newTail))
            *tail = newTail;
    }
    else
    {
        // Buffer wrap occurred
        if ((*tail >= previousTail) || (*tail < newTail))
            *tail = newTail;
    }
}

// If new data is in the ringBuffer, dole it out to appropriate interface
// Send data out Bluetooth, record to SD, or send to network clients
// Each device (Bluetooth, SD and network client) gets its own tail.  If the
// device is running too slowly then data for that device is dropped.
// The usedSpace variable tracks the total space in use in the buffer.
void handleGnssDataTask(void *e)
{
    int32_t bytesToSend;
    bool connected;
    uint32_t deltaMillis;
    int32_t freeSpace;
    uint16_t listEnd;
    static uint32_t maxMillis[RBC_MAX];
    uint32_t startMillis;
    int32_t usedSpace;

    // Initialize the tails
    btRingBufferTail = 0;
    pvtClientZeroTail();
    pvtServerZeroTail();
    pvtUdpServerZeroTail();
    sdRingBufferTail = 0;

    while (true)
    {
        // Display an alive message
        if (PERIODIC_DISPLAY(PD_TASK_HANDLE_GNSS_DATA))
        {
            PERIODIC_CLEAR(PD_TASK_HANDLE_GNSS_DATA);
            systemPrintln("handleGnssDataTask running");
        }

        usedSpace = 0;

        //----------------------------------------------------------------------
        // Send data over Bluetooth
        //----------------------------------------------------------------------

        startMillis = millis();

        // Determine BT connection state
        bool connected = (bluetoothGetState() == BT_CONNECTED) && (systemState != STATE_BASE_TEMP_SETTLE) &&
                         (systemState != STATE_BASE_TEMP_SURVEY_STARTED);
        if (!connected)
            // Discard the data
            btRingBufferTail = dataHead;
        else
        {
            // Determine the amount of Bluetooth data in the buffer
            bytesToSend = dataHead - btRingBufferTail;
            if (bytesToSend < 0)
                bytesToSend += settings.gnssHandlerBufferSize;
            if (bytesToSend > 0)
            {
                // Reduce bytes to send if we have more to send then the end of
                // the buffer, we'll wrap next loop
                if ((btRingBufferTail + bytesToSend) > settings.gnssHandlerBufferSize)
                    bytesToSend = settings.gnssHandlerBufferSize - btRingBufferTail;

                // If we are in the config menu, suppress data flowing from ZED to cell phone
                if (btPrintEcho == false)
                    // Push new data to BT SPP
                    bytesToSend = bluetoothWrite(&ringBuffer[btRingBufferTail], bytesToSend);

                // Account for the data that was sent
                if (bytesToSend > 0)
                {
                    // If we are in base mode, assume part of the outgoing data is RTCM
                    if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
                        bluetoothOutgoingRTCM = true;

                    // Account for the sent or dropped data
                    btRingBufferTail += bytesToSend;
                    if (btRingBufferTail >= settings.gnssHandlerBufferSize)
                        btRingBufferTail -= settings.gnssHandlerBufferSize;

                    // Remember the maximum transfer time
                    deltaMillis = millis() - startMillis;
                    if (maxMillis[RBC_BLUETOOTH] < deltaMillis)
                        maxMillis[RBC_BLUETOOTH] = deltaMillis;

                    // Display the data movement
                    if (PERIODIC_DISPLAY(PD_BLUETOOTH_DATA_TX))
                    {
                        PERIODIC_CLEAR(PD_BLUETOOTH_DATA_TX);
                        systemPrintf("Bluetooth: %d bytes written\r\n", bytesToSend);
                    }
                }
                else
                    log_w("BT failed to send");

                // Determine the amount of data that remains in the buffer
                bytesToSend = dataHead - btRingBufferTail;
                if (bytesToSend < 0)
                    bytesToSend += settings.gnssHandlerBufferSize;
                if (usedSpace < bytesToSend)
                {
                    usedSpace = bytesToSend;
                    slowConsumer = "Bluetooth";
                }
            }
        }

        //----------------------------------------------------------------------
        // Send data to the network clients
        //----------------------------------------------------------------------

        startMillis = millis();

        // Update space available for use in UART task
        bytesToSend = pvtClientSendData(dataHead);
        if (usedSpace < bytesToSend)
        {
            usedSpace = bytesToSend;
            slowConsumer = "PVT client";
        }

        // Remember the maximum transfer time
        deltaMillis = millis() - startMillis;
        if (maxMillis[RBC_PVT_CLIENT] < deltaMillis)
            maxMillis[RBC_PVT_CLIENT] = deltaMillis;

        startMillis = millis();

        // Update space available for use in UART task
        bytesToSend = pvtServerSendData(dataHead);
        if (usedSpace < bytesToSend)
        {
            usedSpace = bytesToSend;
            slowConsumer = "PVT server";
        }

        // Remember the maximum transfer time
        deltaMillis = millis() - startMillis;
        if (maxMillis[RBC_PVT_SERVER] < deltaMillis)
            maxMillis[RBC_PVT_SERVER] = deltaMillis;

        startMillis = millis();

        // Update space available for use in UART task
        bytesToSend = pvtUdpServerSendData(dataHead);
        if (usedSpace < bytesToSend)
        {
            usedSpace = bytesToSend;
            slowConsumer = "PVT UDP server";
        }

        // Remember the maximum transfer time
        deltaMillis = millis() - startMillis;
        if (maxMillis[RBC_PVT_UDP_SERVER] < deltaMillis)
            maxMillis[RBC_PVT_UDP_SERVER] = deltaMillis;

        //----------------------------------------------------------------------
        // Log data to the SD card
        //----------------------------------------------------------------------

        // Determine if the SD card is enabled for logging
        connected = online.logging && ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes);

        // If user wants to log, record to SD
        if (!connected)
            // Discard the data
            sdRingBufferTail = dataHead;
        else
        {
            // Determine the amount of microSD card logging data in the buffer
            bytesToSend = dataHead - sdRingBufferTail;
            if (bytesToSend < 0)
                bytesToSend += settings.gnssHandlerBufferSize;
            if (bytesToSend > 0)
            {
                // Attempt to gain access to the SD card, avoids collisions with file
                // writing from other functions like recordSystemSettingsToFile()
                if (xSemaphoreTake(sdCardSemaphore, loggingSemaphoreWait_ms) == pdPASS)
                {
                    markSemaphore(FUNCTION_WRITESD);

                    // Reduce bytes to record if we have more then the end of the buffer
                    if ((sdRingBufferTail + bytesToSend) > settings.gnssHandlerBufferSize)
                        bytesToSend = settings.gnssHandlerBufferSize - sdRingBufferTail;

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
                                     bufferAvailable, 0, 0, availableUARTSpace, availableHandlerSpace, bytesToSend, 0,
                                     bufferOverruns);
                    }

                    // Write the data to the file
                    long startTime = millis();
                    startMillis = millis();

                    bytesToSend = ubxFile->write(&ringBuffer[sdRingBufferTail], bytesToSend);
                    if (PERIODIC_DISPLAY(PD_SD_LOG_WRITE) && (bytesToSend > 0))
                    {
                        PERIODIC_CLEAR(PD_SD_LOG_WRITE);
                        systemPrintf("SD %d bytes written to log file\r\n", bytesToSend);
                    }

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

                    sdFreeSpace -= bytesToSend; // Update remaining space on SD

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

                    // Remember the maximum transfer time
                    deltaMillis = millis() - startMillis;
                    if (maxMillis[RBC_SD_CARD] < deltaMillis)
                        maxMillis[RBC_SD_CARD] = deltaMillis;
                    long endTime = millis();

                    if (settings.enablePrintBufferOverrun)
                    {
                        if (endTime - startTime > 150)
                            systemPrintf("Long Write! Time: %ld ms / Location: %ld / Recorded %d bytes / "
                                         "spaceRemaining %d bytes\r\n",
                                         endTime - startTime, fileSize, bytesToSend, combinedSpaceRemaining);
                    }

                    xSemaphoreGive(sdCardSemaphore);

                    // Account for the sent data or dropped
                    if (bytesToSend > 0)
                    {
                        sdRingBufferTail += bytesToSend;
                        if (sdRingBufferTail >= settings.gnssHandlerBufferSize)
                            sdRingBufferTail -= settings.gnssHandlerBufferSize;
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

                // Update space available for use in UART task
                bytesToSend = dataHead - sdRingBufferTail;
                if (bytesToSend < 0)
                    bytesToSend += settings.gnssHandlerBufferSize;
                if (usedSpace < bytesToSend)
                {
                    usedSpace = bytesToSend;
                    slowConsumer = "SD card";
                }
            } // bytesToSend
        } // End connected

        //----------------------------------------------------------------------
        // Update the available space in the ring buffer
        //----------------------------------------------------------------------

        freeSpace = settings.gnssHandlerBufferSize - usedSpace;

        // Don't fill the last byte to prevent buffer overflow
        if (freeSpace)
            freeSpace -= 1;
        availableHandlerSpace = freeSpace;

        //----------------------------------------------------------------------
        // Display the millisecond values for the different ring buffer consumers
        //----------------------------------------------------------------------

        if (PERIODIC_DISPLAY(PD_RING_BUFFER_MILLIS))
        {
            int milliseconds;
            int seconds;

            PERIODIC_CLEAR(PD_RING_BUFFER_MILLIS);
            for (int index = 0; index < RBC_MAX; index++)
            {
                milliseconds = maxMillis[index];
                if (milliseconds > 1)
                {
                    seconds = milliseconds / MILLISECONDS_IN_A_SECOND;
                    milliseconds %= MILLISECONDS_IN_A_SECOND;
                    systemPrintf("%s: %d:%03d Sec\r\n", ringBufferConsumer[index], seconds, milliseconds);
                }
            }
        }

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
        else if (wifiState == WIFI_STATE_CONNECTING || wifiState == WIFI_STATE_CONNECTED)
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
        // Display an alive message
        if (PERIODIC_DISPLAY(PD_TASK_BUTTON_CHECK))
        {
            PERIODIC_CLEAR(PD_TASK_BUTTON_CHECK);
            systemPrintln("ButtonCheckTask running");
        }

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
            if (setupBtn &&
                (settings.disableSetupButton == false)) // Allow check of the setup button if not overridden by settings
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
                            if (systemState == STATE_ROVER_NOT_STARTED &&
                                online.display == true)         // Catch during Power On
                                requestChangeState(STATE_TEST); // If RTK Surveyor, with display attached, during Rover
                                                                // not started, then enter test mode
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
                if (settings.disableSetupButton ==
                    false) // Allow check of the setup button if not overridden by settings
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
                } // End disabdisableSetupButton check
            }
        } // End Platform = RTK Express
        else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND ||
                 productVariant == RTK_FACET_LBAND_DIRECT) // Check one momentary button
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
            else if (powerBtn != nullptr &&
                     (systemState == STATE_ROVER_NOT_STARTED || systemState == STATE_KEYS_STARTED) &&
                     firstRoverStart == true && powerBtn->pressedFor(500))
            {
                forceSystemStateUpdate = true;
                requestChangeState(STATE_TEST);
                lastTestMenuChange = millis(); // Avoid exiting test menu for 1s
            }
            else if (powerBtn != nullptr && powerBtn->wasReleased() && firstRoverStart == false)
            {
                if (settings.disableSetupButton ==
                    false) // Allow check of the setup button if not overridden by settings
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
                            if (productVariant == RTK_FACET_LBAND || productVariant == RTK_FACET_LBAND_DIRECT)
                            {
                                lBandForceGetKeys = true;
                                setupState = STATE_KEYS_NEEDED;
                            }
                            else
                                setupState = STATE_ESPNOW_PAIRING_NOT_STARTED;
                            break;

                        case STATE_KEYS_NEEDED:
                            lBandForceGetKeys = false; // User has scrolled past the GetKeys option
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
                } // End disableSetupButton check
            }
        } // End Platform = RTK Facet
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
                if (settings.disableSetupButton ==
                    false) // Allow check of the setup button if not overridden by settings
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
                        case STATE_MARK_EVENT: // Skip the warning message if setupState is still in the default Mark
                                               // Event state
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
                } // End disableSetupButton check
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
        if ((millis() - lastDisplayIdleTime) >= (IDLE_TIME_DISPLAY_SECONDS * 1000) && !inMainMenu)
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

        // The idle task should NOT delay or yield
    }
}

// Serial Read/Write tasks for the F9P must be started after BT is up and running otherwise SerialBT->available will
// cause reboot
bool tasksStartUART2()
{
    // Verify that the ring buffer was successfully allocated
    if (!ringBuffer)
    {
        systemPrintln("ERROR: Ring buffer allocation failure!");
        systemPrintln("Decrease GNSS handler (ring) buffer size");
        displayNoRingBuffer(5000);
        return false;
    }

    availableHandlerSpace = settings.gnssHandlerBufferSize;

    // Reads data from ZED and stores data into circular buffer
    if (gnssReadTaskHandle == nullptr)
        xTaskCreatePinnedToCore(gnssReadTask,                  // Function to call
                                "gnssRead",                    // Just for humans
                                gnssReadTaskStackSize,         // Stack Size
                                nullptr,                       // Task input parameter
                                settings.gnssReadTaskPriority, // Priority
                                &gnssReadTaskHandle,           // Task handle
                                settings.gnssReadTaskCore);    // Core where task should run, 0=core, 1=Arduino

    // Reads data from circular buffer and sends data to SD, SPP, or network clients
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
    return true;
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
        // Display an alive message
        if (PERIODIC_DISPLAY(PD_TASK_SD_SIZE_CHECK))
        {
            PERIODIC_CLEAR(PD_TASK_SD_SIZE_CHECK);
            systemPrintln("sdSizeCheckTask running");
        }

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
#endif // COMPILE_SD_MMC

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

// Validate the task table lengths
void tasksValidateTables()
{
    if (ringBufferConsumerEntries != RBC_MAX)
        reportFatalError("Fix ringBufferConsumer table to match RingBufferConsumers");
}
