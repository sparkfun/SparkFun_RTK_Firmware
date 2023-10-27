/*------------------------------------------------------------------------------
Parse_RTCM.ino

  RTCM message parsing support routines
------------------------------------------------------------------------------*/

//
//    RTCM Standard 10403.2 - Chapter 4, Transport Layer
//
//    |<------------- 3 bytes ------------>|<----- length ----->|<- 3 bytes ->|
//    |                                    |                    |             |
//    +----------+--------+----------------+---------+----------+-------------+
//    | Preamble |  Fill  | Message Length | Message |   Fill   |   CRC-24Q   |
//    |  8 bits  | 6 bits |    10 bits     |  n-bits | 0-7 bits |   24 bits   |
//    |   0xd3   | 000000 |   (in bytes)   |         |   zeros  |             |
//    +----------+--------+----------------+---------+----------+-------------+
//    |                                                         |
//    |<------------------------ CRC -------------------------->|
//

// Check for the preamble
uint8_t rtcmPreamble(PARSE_STATE *parse, uint8_t data)
{
    if (data == 0xd3)
    {
        // Start the CRC with this byte
        parse->crc = 0;
        parse->crc = COMPUTE_CRC24Q(parse, data);
        parse->computeCrc = true;

        // Get the message length
        parse->state = rtcmReadLength1;
        return SENTENCE_TYPE_RTCM;
    }
    return SENTENCE_TYPE_NONE;
}

// Read the upper two bits of the length
uint8_t rtcmReadLength1(PARSE_STATE *parse, uint8_t data)
{
    // Verify the length byte - check the 6 MS bits are all zero
    if (data & (~3))
    {
        // Invalid length, place this byte at the beginning of the buffer
        parse->length = 0;
        parse->buffer[parse->length++] = data;
        parse->computeCrc = false;

        // Start searching for a preamble byte
        return gpsMessageParserFirstByte(parse, data);
    }

    // Save the upper 2 bits of the length
    parse->bytesRemaining = data << 8;
    parse->state = rtcmReadLength2;
    return SENTENCE_TYPE_RTCM;
}

// Read the lower 8 bits of the length
uint8_t rtcmReadLength2(PARSE_STATE *parse, uint8_t data)
{
    parse->bytesRemaining |= data;
    parse->state = rtcmReadMessage1;
    return SENTENCE_TYPE_RTCM;
}

// Read the upper 8 bits of the message number
uint8_t rtcmReadMessage1(PARSE_STATE *parse, uint8_t data)
{
    parse->message = data << 4;
    parse->bytesRemaining -= 1;
    parse->state = rtcmReadMessage2;
    return SENTENCE_TYPE_RTCM;
}

// Read the lower 4 bits of the message number
uint8_t rtcmReadMessage2(PARSE_STATE *parse, uint8_t data)
{
    parse->message |= data >> 4;
    parse->bytesRemaining -= 1;
    parse->state = rtcmReadData;
    return SENTENCE_TYPE_RTCM;
}

// Read the rest of the message
uint8_t rtcmReadData(PARSE_STATE *parse, uint8_t data)
{
    // Account for this data byte
    parse->bytesRemaining -= 1;

    // Wait until all the data is received
    if (parse->bytesRemaining <= 0)
    {
        parse->rtcmCrc = parse->crc & 0x00ffffff;
        parse->bytesRemaining = 3;
        parse->state = rtcmReadCrc;
    }
    return SENTENCE_TYPE_RTCM;
}

// Read the CRC
uint8_t rtcmReadCrc(PARSE_STATE *parse, uint8_t data)
{
    // Account for this data byte
    parse->bytesRemaining -= 1;

    // Wait until all the data is received
    if (parse->bytesRemaining > 0)
        return SENTENCE_TYPE_RTCM;

    // Update the maximum message length
    if (parse->length > parse->maxLength)
    {
        parse->maxLength = parse->length;
        printRtcmMaxLength(parse);
    }

    // Display the RTCM messages with bad CRC
    parse->crc &= 0x00ffffff;
    if (settings.enablePrintBadMessages && parse->crc && (!inMainMenu))
        printRtcmChecksumError(parse);

    // Process the message if CRC is valid
    if (parse->crc == 0)
        parse->eomCallback(parse, SENTENCE_TYPE_RTCM);
    else
        failedParserMessages_RTCM++;

    // Search for another preamble byte
    parse->length = 0;
    parse->computeCrc = false;
    parse->state = gpsMessageParserFirstByte;
    return SENTENCE_TYPE_NONE;
}
