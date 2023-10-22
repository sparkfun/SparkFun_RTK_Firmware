/*------------------------------------------------------------------------------
GpsMessageParser.ino

  Parse messages from GPS radios
------------------------------------------------------------------------------*/

// Wait for the first byte in the GPS message
uint8_t gpsMessageParserFirstByte(PARSE_STATE *parse, uint8_t data)
{
    int index;
    PARSE_ROUTINE parseRoutine;
    uint8_t sentenceType;

    // Walk through the parse table
    for (index = 0; index < gpsParseTableEntries; index++)
    {
        parseRoutine = gpsParseTable[index];
        sentenceType = parseRoutine(parse, data);
        if (sentenceType)
            return sentenceType;
    }

    // preamble byte not found
    parse->length = 0;
    parse->state = gpsMessageParserFirstByte;
    return SENTENCE_TYPE_NONE;
}
