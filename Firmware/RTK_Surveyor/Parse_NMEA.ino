/*------------------------------------------------------------------------------
Parse_NMEA.ino

  NMEA message parsing support routines
------------------------------------------------------------------------------*/

//
//    NMEA Message
//
//    +----------+---------+--------+---------+----------+----------+
//    | Preamble |  Name   | Comma  |  Data   | Asterisk | Checksum |
//    |  8 bits  | n bytes | 8 bits | n bytes |  8 bits  | 2 bytes  |
//    |     $    |         |    ,   |         |          |          |
//    +----------+---------+--------+---------+----------+----------+
//               |                            |
//               |<-------- Checksum -------->|
//

// Check for the preamble
uint8_t nmeaPreamble(PARSE_STATE *parse, uint8_t data)
{
    if (data == '$')
    {
        parse->crc = 0;
        parse->computeCrc = false;
        parse->nmeaMessageNameLength = 0;
        parse->state = nmeaFindFirstComma;
        return SENTENCE_TYPE_NMEA;
    }
    return SENTENCE_TYPE_NONE;
}

// Read the message name
uint8_t nmeaFindFirstComma(PARSE_STATE *parse, uint8_t data)
{
    parse->crc ^= data;
    if ((data != ',') || (parse->nmeaMessageNameLength == 0))
    {
        if ((data < 'A') || (data > 'Z'))
        {
            parse->length = 0;
            parse->buffer[parse->length++] = data;
            return gpsMessageParserFirstByte(parse, data);
        }

        // Save the message name
        parse->nmeaMessageName[parse->nmeaMessageNameLength++] = data;
    }
    else
    {
        // Zero terminate the message name
        parse->nmeaMessageName[parse->nmeaMessageNameLength++] = 0;
        parse->state = nmeaFindAsterisk;
    }
    return SENTENCE_TYPE_NMEA;
}

// Read the message data
uint8_t nmeaFindAsterisk(PARSE_STATE *parse, uint8_t data)
{
    if (data != '*')
        parse->crc ^= data;
    else
        parse->state = nmeaChecksumByte1;
    return SENTENCE_TYPE_NMEA;
}

// Read the first checksum byte
uint8_t nmeaChecksumByte1(PARSE_STATE *parse, uint8_t data)
{
    parse->state = nmeaChecksumByte2;
    return SENTENCE_TYPE_NMEA;
}

// Read the second checksum byte
uint8_t nmeaChecksumByte2(PARSE_STATE *parse, uint8_t data)
{
    parse->nmeaLength = parse->length;
    parse->state = nmeaLineTermination;
    return SENTENCE_TYPE_NMEA;
}

// Read the line termination
uint8_t nmeaLineTermination(PARSE_STATE *parse, uint8_t data)
{
    int checksum;

    // Process the line termination
    if ((data != '\r') && (data != '\n'))
    {
        // Don't include this character in the buffer
        parse->length--;

        // Convert the checksum characters into binary
        checksum = AsciiToNibble(parse->buffer[parse->nmeaLength - 2]) << 4;
        checksum |= AsciiToNibble(parse->buffer[parse->nmeaLength - 1]);

        // Validate the checksum
        if (checksum == parse->crc)
            parse->crc = 0;
        if (settings.enablePrintBadMessages && parse->crc && (!inMainMenu))
            printNmeaChecksumError(parse);

        // Process this message if CRC is valid
        if (parse->crc == 0)
            parse->eomCallback(parse, SENTENCE_TYPE_NMEA);
        else
            failedParserMessages_NMEA++;

        // Add this character to the beginning of the buffer
        parse->length = 0;
        parse->buffer[parse->length++] = data;
        return gpsMessageParserFirstByte(parse, data);
    }
    return SENTENCE_TYPE_NMEA;
}
