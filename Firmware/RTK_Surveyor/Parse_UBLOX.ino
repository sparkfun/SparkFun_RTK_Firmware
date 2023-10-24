/*------------------------------------------------------------------------------
Parse_UBLOX.ino

  u-blox message parsing support routines
------------------------------------------------------------------------------*/

//
//    U-BLOX Message
//
//    |<-- Preamble --->|
//    |                 |
//    +--------+--------+---------+--------+---------+---------+--------+--------+
//    |  SYNC  |  SYNC  |  Class  |   ID   | Length  | Payload |  CK_A  |  CK_B  |
//    | 8 bits | 8 bits |  8 bits | 8 bits | 2 bytes | n bytes | 8 bits | 8 bits |
//    |  0xb5  |  0x62  |         |        |         |         |        |        |
//    +--------+--------+---------+--------+---------+---------+--------+--------+
//                      |                                      |
//                      |<------------- Checksum ------------->|
//
//  8-Bit Fletcher Algorithm, which is used in the TCP standard (RFC 1145)
//  http://www.ietf.org/rfc/rfc1145.txt
//  Checksum calculation
//      Initialization: CK_A = CK_B = 0
//      CK_A += data
//      CK_B += CK_A
//

// Check for the preamble
uint8_t ubloxPreamble(PARSE_STATE *parse, uint8_t data)
{
    if (data == 0xb5)
    {
        parse->state = ubloxSync2;
        return SENTENCE_TYPE_UBX;
    }
    return SENTENCE_TYPE_NONE;
}

// Read the second sync byte
uint8_t ubloxSync2(PARSE_STATE *parse, uint8_t data)
{
    // Verify the sync 2 byte
    if (data != 0x62)
    {
        // Display the invalid data
        if (settings.enablePrintBadMessages && (!inMainMenu))
            printUbloxInvalidData(parse);

        // Invalid sync 2 byte, place this byte at the beginning of the buffer
        parse->length = 0;
        parse->buffer[parse->length++] = data;

        // Start searching for a preamble byte
        return gpsMessageParserFirstByte(parse, data);
    }

    parse->state = ubloxClass;
    return SENTENCE_TYPE_UBX;
}

// Read the class byte
uint8_t ubloxClass(PARSE_STATE *parse, uint8_t data)
{
    // Start the checksum calculation
    parse->ck_a = data;
    parse->ck_b = data;

    // Save the class as the upper 8-bits of the message
    parse->message = ((uint16_t)data) << 8;
    parse->state = ubloxId;
    return SENTENCE_TYPE_UBX;
}

// Read the ID byte
uint8_t ubloxId(PARSE_STATE *parse, uint8_t data)
{
    // Calculate the checksum
    parse->ck_a += data;
    parse->ck_b += parse->ck_a;

    // Save the ID as the lower 8-bits of the message
    parse->message |= data;
    parse->state = ubloxLength1;
    return SENTENCE_TYPE_UBX;
}

// Read the first length byte
uint8_t ubloxLength1(PARSE_STATE *parse, uint8_t data)
{
    // Calculate the checksum
    parse->ck_a += data;
    parse->ck_b += parse->ck_a;

    // Save the first length byte
    parse->bytesRemaining = data;
    parse->state = ubloxLength2;
    return SENTENCE_TYPE_UBX;
}

// Read the second length byte
uint8_t ubloxLength2(PARSE_STATE *parse, uint8_t data)
{
    // Calculate the checksum
    parse->ck_a += data;
    parse->ck_b += parse->ck_a;

    // Save the second length byte
    parse->bytesRemaining |= ((uint16_t)data) << 8;
    parse->state = ubloxPayload;
    return SENTENCE_TYPE_UBX;
}

// Read the payload
uint8_t ubloxPayload(PARSE_STATE *parse, uint8_t data)
{
    // Compute the checksum over the payload
    if (parse->bytesRemaining--)
    {
        // Calculate the checksum
        parse->ck_a += data;
        parse->ck_b += parse->ck_a;
        return SENTENCE_TYPE_UBX;
    }
    return ubloxCkA(parse, data);
}

// Read the CK_A byte
uint8_t ubloxCkA(PARSE_STATE *parse, uint8_t data)
{
    parse->state = ubloxCkB;
    return SENTENCE_TYPE_UBX;
}

// Read the CK_B byte
uint8_t ubloxCkB(PARSE_STATE *parse, uint8_t data)
{
    bool badChecksum;

    // Validate the checksum
    badChecksum =
        ((parse->buffer[parse->length - 2] != parse->ck_a) || (parse->buffer[parse->length - 1] != parse->ck_b));
    if (settings.enablePrintBadMessages && badChecksum && (!inMainMenu))
        printUbloxChecksumError(parse);

    // Process this message if checksum is valid
    if (badChecksum == false)
        parse->eomCallback(parse, SENTENCE_TYPE_UBX);
    else
        failedParserMessages_UBX++;

    // Search for the next preamble byte
    parse->length = 0;
    parse->state = gpsMessageParserFirstByte;
    return SENTENCE_TYPE_NONE;
}
