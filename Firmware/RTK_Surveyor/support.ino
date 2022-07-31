void printDebug(String thingToPrint)
{
  if (settings.printDebugMessages == true)
  {
    Serial.print(thingToPrint);
  }
}

//Option not known
void printUnknown(uint8_t unknownChoice)
{
  Serial.print("Unknown choice: ");
  Serial.write(unknownChoice);
  Serial.println();
}
void printUnknown(int unknownValue)
{
  Serial.print("Unknown value: ");
  Serial.printf("%d\r\n", unknownValue);
}

//Clear the Serial RX buffer before we begin scanning for characters
void clearBuffer()
{
  Serial.flush();
  delay(20);//Wait for any incoming chars to hit buffer
  while (Serial.available() > 0) Serial.read(); //Clear buffer
}

//Waits for and returns the menu choice that the user provides
//Returns GMCS_TIMEOUT, GMCS_OVERFLOW, GMCS_CHARACTER or number of digits
int getMenuChoice(int * value, int numberOfSeconds)
{
  int digits;
  byte incoming;
  bool makingChoice;
  int previous_value;

  clearBuffer();

  long startTime = millis();

  //Assume character value
  *value = 0;
  previous_value = 0;
  digits = 0;
  makingChoice = true;
  while (makingChoice)
  {
    delay(10); //Yield to processor

    //Regularly poll to get latest data
    if (online.gnss == true)
      i2cGNSS.checkUblox();

    //Get the next input character
    while (Serial.available() > 0)
    {
      incoming = Serial.read();

      //Handle single character input
      if ((!digits)
        && (((incoming >= 'a') && (incoming <= 'z'))
        || ((incoming >= 'A') && (incoming <= 'Z'))))
      {
        //Echo the incoming character
        Serial.printf("%c\r\n", incoming);

        //Return the character value
        *value = incoming;
        makingChoice = false;
        break;
      }

      //Handle numeric input
      else if ((incoming >= '0') && (incoming <= '9'))
      {
        //Echo the incoming character
        Serial.write(incoming);

        //Switch to numeric mode
        *value = (*value * 10) + incoming - '0';
        digits += 1;

        //Check for overflow
        if (*value < previous_value)
        {
          Serial.println("Error - number overflow!");
          return GMCS_OVERFLOW;
        }
        previous_value = *value;

        //Handle backspace
        if (digits && (incoming == 8))
        {
          Serial.print((char)0x08); //Move back one space
          Serial.print(" "); //Put a blank there to erase the letter from the terminal
          Serial.print((char)0x08); //Move back again

          //Switch to numeric mode
          *value /= 10;
          previous_value = *value;
          digits -= 1;
        }
      }

      //Handle end of number
      else if (digits && ((incoming == '\r') || (incoming == '\n')))
      {
        //Echo the incoming character
        Serial.print("\r\n");
        makingChoice = false;
        break;
      }
    }

    //Exit the routine when a timeout occurs
    if (makingChoice && (millis() - startTime) / 1000 >= numberOfSeconds)
    {
      Serial.println("No user input received.");
      return GMCS_TIMEOUT;
    }
  }
  return (digits);
}

//Get single byte from user
//Waits for and returns the character that the user provides
//Returns STATUS_GETNUMBER_TIMEOUT if input times out
//Returns 'x' if user presses 'x'
uint8_t getByteChoice(int numberOfSeconds)
{
  clearBuffer();

  long startTime = millis();
  byte incoming;
  while (1)
  {
    delay(10); //Yield to processor
    if (online.gnss == true)
      i2cGNSS.checkUblox(); //Regularly poll to get latest data

    if (Serial.available() > 0)
    {
      incoming = Serial.read();
      if (incoming >= 'a' && incoming <= 'z') break;
      if (incoming >= 'A' && incoming <= 'Z') break;
      if (incoming >= '0' && incoming <= '9') break;
    }

    if ( (millis() - startTime) / 1000 >= numberOfSeconds)
    {
      Serial.println("No user input received.");
      return (STATUS_GETBYTE_TIMEOUT); //Timeout. No user input.
    }
  }

  return (incoming);
}

//Get a string/value from user, remove all non-numeric values
//Returns STATUS_GETNUMBER_TIMEOUT if input times out
//Returns STATUS_PRESSED_X if user presses 'x'
int64_t getNumber(int numberOfSeconds)
{
  clearBuffer();

  //Get input from user
  char cleansed[20]; //Good for very large numbers: 123,456,789,012,345,678\0

  long startTime = millis();
  int spot = 0;
  while (spot < 20 - 1) //Leave room for terminating \0
  {
    while (Serial.available() == 0) //Wait for user input
    {
      delay(10); //Yield to processor
      if (online.gnss == true)
        i2cGNSS.checkUblox(); //Regularly poll to get latest data

      if ( (millis() - startTime) / 1000 >= numberOfSeconds)
      {
        if (spot == 0)
        {
          Serial.println("No user input received. Do you have line endings turned on?");
          return (STATUS_GETNUMBER_TIMEOUT); //Timeout. No user input.
        }
        else if (spot > 0)
        {
          break; //Timeout, but we have data
        }
      }
    }

    //See if we timed out waiting for a line ending
    if (spot > 0 && (millis() - startTime) / 1000 >= numberOfSeconds)
    {
      Serial.println("Do you have line endings turned on?");
      break; //Timeout, but we have data
    }

    byte incoming = Serial.read();
    if (incoming == '\n' || incoming == '\r')
    {
      Serial.println();
      break;
    }

    if ((isDigit(incoming) == true) || ((incoming == '-') && (spot == 0))) // Check for digits and a minus sign
    {
      Serial.write(incoming); //Echo user's typing
      cleansed[spot++] = (char)incoming;
    }

    if (incoming == 'x')
    {
      return (STATUS_PRESSED_X);
    }
  }

  cleansed[spot] = '\0';

  int64_t largeNumber = 0;
  int x = 0;
  if (cleansed[0] == '-') // If our number is negative
  {
    x = 1; // Skip the minus
  }
  for ( ; x < spot ; x++)
  {
    largeNumber *= 10;
    largeNumber += (cleansed[x] - '0');
  }
  if (cleansed[0] == '-') // If our number is negative
  {
    largeNumber = 0 - largeNumber; // Make it negative
  }
  return (largeNumber);
}

//Get a string/value from user, remove all non-numeric values
//Returns STATUS_GETNUMBER_TIMEOUT if input times out
//Returns STATUS_PRESSED_X if user presses 'x'
double getDouble(int numberOfSeconds)
{
  clearBuffer();

  //Get input from user
  char cleansed[20]; //Good for very large numbers: 123,456,789,012,345,678\0

  long startTime = millis();
  int spot = 0;
  bool dpSeen = false;
  while (spot < 20 - 1) //Leave room for terminating \0
  {
    while (Serial.available() == 0) //Wait for user input
    {
      delay(10); //Yield to processor
      if (online.gnss == true)
        i2cGNSS.checkUblox(); //Regularly poll to get latest data

      if ( (millis() - startTime) / 1000 >= numberOfSeconds)
      {
        if (spot == 0)
        {
          Serial.println("No user input received. Do you have line endings turned on?");
          return (STATUS_GETNUMBER_TIMEOUT); //Timeout. No user input.
        }
        else if (spot > 0)
        {
          break; //Timeout, but we have data
        }
      }
    }

    //See if we timed out waiting for a line ending
    if (spot > 0 && (millis() - startTime) / 1000 >= numberOfSeconds)
    {
      Serial.println("Do you have line endings turned on?");
      break; //Timeout, but we have data
    }

    byte incoming = Serial.read();
    if (incoming == '\n' || incoming == '\r')
    {
      Serial.println();
      break;
    }

    if ((isDigit(incoming) == true) || ((incoming == '-') && (spot == 0)) || ((incoming == '.') && (dpSeen == false))) // Check for digits/minus/dp
    {
      Serial.write(incoming); //Echo user's typing
      cleansed[spot++] = (char)incoming;
    }

    if (incoming == '.')
      dpSeen = true;

    if (incoming == 'x')
    {
      return (STATUS_PRESSED_X);
    }
  }

  cleansed[spot] = '\0';

  double largeNumber = 0;
  int x = 0;
  if (cleansed[0] == '-') // If our number is negative
  {
    x = 1; // Skip the minus
  }
  for ( ; x < spot ; x++)
  {
    if (cleansed[x] == '.')
      break;
    largeNumber *= 10;
    largeNumber += (cleansed[x] - '0');
  }
  if (x < spot) // Check if we found a '.'
  {
    x++;
    double divider = 0.1;
    for ( ; x < spot ; x++)
    {
      largeNumber += (cleansed[x] - '0') * divider;
      divider /= 10;
    }
  }
  if (cleansed[0] == '-') // If our number is negative
  {
    largeNumber = 0 - largeNumber; // Make it negative
  }
  return (largeNumber);
}

//Reads a line until the \n enter character is found
//Returns STATUS_GETBYTE_TIMEOUT if input times out
byte readLine(char* buffer, byte bufferLength, int numberOfSeconds)
{
  clearBuffer();

  byte readLength = 0;
  long startTime = millis();
  while (readLength < bufferLength - 1)
  {
    //See if we timed out waiting for a line ending
    if (readLength > 0 && (millis() - startTime) / 1000 >= numberOfSeconds)
    {
      Serial.println("Do you have line endings turned on?");
      break; //Timeout, but we have data
    }

    while (Serial.available() == 0) //Wait for user input
    {
      delay(10); //Yield to processor
      if (online.gnss == true)
        i2cGNSS.checkUblox(); //Regularly poll to get latest data

      if ( (millis() - startTime) / 1000 >= numberOfSeconds)
      {
        if (readLength == 0)
        {
          Serial.println("No user input received. Do you have line endings turned on?");
          return (STATUS_GETBYTE_TIMEOUT); //Timeout. No user input.
        }
        else if (readLength > 0)
        {
          break; //Timeout, but we have data
        }
      }
    }

    byte incoming = Serial.read();

    if (incoming == 0x08 || incoming == 0x7F) //Support backspace characters
    {
      if (readLength < 1)
        continue;

      readLength--;
      buffer[readLength] = '\0'; //Put a terminator on the string in case we are finished

      Serial.print((char)0x08); //Move back one space
      Serial.print(" "); //Put a blank there to erase the letter from the terminal
      Serial.print((char)0x08); //Move back again

      continue;
    }

    Serial.write(incoming); //Echo

    if (incoming == '\r' || incoming == '\n')
    {
      Serial.println();
      buffer[readLength] = '\0';
      break;
    }
    else
    {
      buffer[readLength] = incoming;
      readLength++;
    }
  }

  return readLength;
}

#define TIMESTAMP_INTERVAL              1000    //Milliseconds

//Print the timestamp
void printTimeStamp()
{
  uint32_t currentMilliseconds;
  static uint32_t previousMilliseconds;

  //Timestamp the messages
  currentMilliseconds = millis();
  if ((currentMilliseconds - previousMilliseconds) >= TIMESTAMP_INTERVAL)
  {
    //         1         2         3
    //123456789012345678901234567890
    //YYYY-mm-dd HH:MM:SS.xxxrn0
    struct tm timeinfo = rtc.getTimeStruct();
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("%s.%03ld\r\n", timestamp, rtc.getMillis());

    //Select the next time to display the timestamp
    previousMilliseconds = currentMilliseconds;
  }
}

//Convert nibble to ASCII
uint8_t nibbleToAscii(int nibble)
{
  nibble &= 0xf;
  return (nibble > 9) ? nibble + 'a' - 10 : nibble + '0';
}

//Convert nibble to ASCII
int AsciiToNibble(int data)
{
  //Convert the value to lower case
  data |= 0x20;
  if ((data >= 'a') && (data <= 'f'))
    return data - 'a' + 10;
  if ((data >= '0') && (data <= '9'))
    return data - '0';
  return -1;
}

void dumpBuffer(uint8_t * buffer, uint16_t length)
{
  int bytes;
  uint8_t * end;
  int index;
  uint16_t offset;

  end = &buffer[length];
  offset = 0;
  while (buffer < end)
  {
    //Determine the number of bytes to display on the line
    bytes = end - buffer;
    if (bytes > (16 - (offset & 0xf)))
      bytes = 16 - (offset & 0xf);

    //Display the offset
    Serial.printf("0x%08lx: ", offset);

    //Skip leading bytes
    for (index = 0; index < (offset & 0xf); index++)
      Serial.printf("   ");

    //Display the data bytes
    for (index = 0; index < bytes; index++)
      Serial.printf("%02x ", buffer[index]);

    //Separate the data bytes from the ASCII
    for (; index < (16 - (offset & 0xf)); index++)
      Serial.printf("   ");
    Serial.printf(" ");

    //Skip leading bytes
    for (index = 0; index < (offset & 0xf); index++)
      Serial.printf(" ");

    //Display the ASCII values
    for (index = 0; index < bytes; index++)
      Serial.printf("%c", ((buffer[index] < ' ') || (buffer[index] >= 0x7f))
                   ? '.' : buffer[index]);
    Serial.printf("\r\n");

    //Set the next line of data
    buffer += bytes;
    offset += bytes;
  }
}

//Read the line termination
uint8_t nmeaLineTermination(PARSE_STATE * parse, uint8_t data)
{
  int checksum;

  //Process the line termination
  if ((data != '\r') && (data != '\n'))
  {
    //Don't include this character in the buffer
    parse->length--;

    //Convert the checksum characters into binary
    checksum = AsciiToNibble(parse->buffer[parse->nmeaLength - 2]) << 4;
    checksum |= AsciiToNibble(parse->buffer[parse->nmeaLength - 1]);

    //Validate the checksum
    if (checksum == parse->crc)
      parse->crc = 0;
    if (settings.enablePrintBadMessages && parse->crc && (!inMainMenu))
    {
      printTimeStamp();
      Serial.printf ("    %s NMEA %s, %2d bytes, bad checksum, expecting 0x%c%c, computed: 0x%02x\r\n",
                     parse->parserName,
                     parse->nmeaMessageName,
                     parse->length,
                     parse->buffer[parse->nmeaLength-2],
                     parse->buffer[parse->nmeaLength-1],
                     parse->crc);
    }

    //Process this message
    parse->eomCallback(parse, SENTENCE_TYPE_NMEA);

    //Add this character to the beginning of the buffer
    parse->length = 0;
    parse->buffer[parse->length++] = data;
    return waitForPreamble(parse, data);
  }
  return SENTENCE_TYPE_NMEA;
}

//Read the second checksum byte
uint8_t nmeaChecksumByte2(PARSE_STATE * parse, uint8_t data)
{
  parse->nmeaLength = parse->length;
  parse->state = nmeaLineTermination;
  return SENTENCE_TYPE_NMEA;
}

//Read the first checksum byte
uint8_t nmeaChecksumByte1(PARSE_STATE * parse, uint8_t data)
{
  parse->state = nmeaChecksumByte2;
  return SENTENCE_TYPE_NMEA;
}

//Read the message data
uint8_t nmeaFindAsterisk(PARSE_STATE * parse, uint8_t data)
{
  if (data != '*')
    parse->crc ^= data;
  else
    parse->state = nmeaChecksumByte1;
  return SENTENCE_TYPE_NMEA;
}

//Read the message name
uint8_t nmeaFindFirstComma(PARSE_STATE * parse, uint8_t data)
{
  parse->crc ^= data;
  if ((data != ',') || (parse->nmeaMessageNameLength == 0))
  {
    if ((data < 'A') || (data > 'Z'))
    {
      parse->length = 0;
      parse->buffer[parse->length++] = data;
      return waitForPreamble (parse, data);
    }

    //Save the message name
    parse->nmeaMessageName[parse->nmeaMessageNameLength++] = data;
  }
  else
  {
    //Zero terminate the message name
    parse->nmeaMessageName[parse->nmeaMessageNameLength++] = 0;
    parse->state = nmeaFindAsterisk;
  }
  return SENTENCE_TYPE_NMEA;
}

//Read the CRC
uint8_t rtcmReadCrc(PARSE_STATE * parse, uint8_t data)
{
  uint16_t dataSent;

  //Account for this data byte
  parse->bytesRemaining -= 1;

  //Wait until all the data is received
  if (parse->bytesRemaining > 0)
    return SENTENCE_TYPE_RTCM;

  //Update the maximum message length
  if (parse->length > parse->maxLength)
  {
    parse->maxLength = parse->length;
    Serial.printf("maxLength: %d bytes\r\n", parse->maxLength);
  }

  //Display the RTCM messages with bad CRC
  parse->crc &= 0x00ffffff;
  if (settings.enablePrintBadMessages && parse->crc && (!inMainMenu))
  {
    printTimeStamp();
    Serial.printf ("    %s RTCM %d, %2d bytes, bad CRC, expecting 0x%02x%02x%02x, computed: 0x%06x\r\n",
                   parse->parserName,
                   parse->message,
                   parse->length,
                   parse->buffer[parse->length-3],
                   parse->buffer[parse->length-2],
                   parse->buffer[parse->length-1],
                   parse->rtcmCrc);
  }

  //Process the message
  parse->eomCallback(parse, SENTENCE_TYPE_RTCM);

  //Search for another preamble byte
  parse->length = 0;
  parse->computeCrc = false;
  parse->state = waitForPreamble;
  return SENTENCE_TYPE_NONE;
}

//Read the rest of the message
uint8_t rtcmReadData(PARSE_STATE * parse, uint8_t data)
{
  uint16_t dataSent;

  //Account for this data byte
  parse->bytesRemaining -= 1;

  //Wait until all the data is received
  if (parse->bytesRemaining <= 0)
  {
    parse->rtcmCrc = parse->crc & 0x00ffffff;
    parse->bytesRemaining = 3;
    parse->state = rtcmReadCrc;
  }
  return SENTENCE_TYPE_RTCM;
}

//Read the lower 4 bits of the message number
uint8_t rtcmReadMessage2(PARSE_STATE * parse, uint8_t data)
{
  parse->message |= data >> 4;
  parse->bytesRemaining -= 1;
  parse->state = rtcmReadData;
  return SENTENCE_TYPE_RTCM;
}

//Read the upper 8 bits of the message number
uint8_t rtcmReadMessage1(PARSE_STATE * parse, uint8_t data)
{
  parse->message = data << 4;
  parse->bytesRemaining -= 1;
  parse->state = rtcmReadMessage2;
  return SENTENCE_TYPE_RTCM;
}

//Read the lower 8 bits of the length
uint8_t rtcmReadLength2(PARSE_STATE * parse, uint8_t data)
{
  parse->bytesRemaining |= data;
  parse->state = rtcmReadMessage1;
  return SENTENCE_TYPE_RTCM;
}

//Read the upper two bits of the length
uint8_t rtcmReadLength1(PARSE_STATE * parse, uint8_t data)
{
  //Verify the length byte
  if (data & (~3))
  {
    //Invalid length, place this byte at the beginning of the buffer
    parse->length = 0;
    parse->buffer[parse->length++] = data;
    parse->computeCrc = false;

    //Start searching for a preamble byte
    return waitForPreamble(parse, data);
  }

  //Save the upper 2 bits of the length
  parse->bytesRemaining = data << 8;
  parse->state = rtcmReadLength2;
  return SENTENCE_TYPE_RTCM;
}

//Read the CK_B byte
uint8_t ubloxCkB(PARSE_STATE * parse, uint8_t data)
{
  bool badChecksum;

  //Validate the checksum
  badChecksum = ((parse->buffer[parse->length - 2] != parse->ck_a)
              || (parse->buffer[parse->length - 1] != parse->ck_b));
  if (settings.enablePrintBadMessages && badChecksum && (!inMainMenu))
  {
    printTimeStamp();
    Serial.printf ("    %s U-Blox %d.%d, %2d bytes, bad checksum, expecting 0x%02x%02x, computed: 0x%02x%02x\r\n",
                   parse->parserName,
                   parse->message >> 8,
                   parse->message & 0xff,
                   parse->length,
                   parse->buffer[parse->nmeaLength-2],
                   parse->buffer[parse->nmeaLength-1],
                   parse->ck_a,
                   parse->ck_b);
  }

  //Process this message
  parse->eomCallback(parse, SENTENCE_TYPE_UBX);

  //Search for the next preamble byte
  parse->length = 0;
  return SENTENCE_TYPE_NONE;
}

//Read the CK_A byte
uint8_t ubloxCkA(PARSE_STATE * parse, uint8_t data)
{
  parse->state = ubloxCkB;
  return SENTENCE_TYPE_UBX;
}

//Read the payload
uint8_t ubloxPayload(PARSE_STATE * parse, uint8_t data)
{
  //Compute the checksum over the payload
  if (parse->bytesRemaining--)
  {
    //Calculate the checksum
    parse->ck_a += data;
    parse->ck_b += parse->ck_a;
    return SENTENCE_TYPE_UBX;
  }
  return ubloxCkA(parse, data);
}

//Read the second length byte
uint8_t ubloxLength2(PARSE_STATE * parse, uint8_t data)
{
  //Calculate the checksum
  parse->ck_a += data;
  parse->ck_b += parse->ck_a;

  //Save the second length byte
  parse->bytesRemaining |= ((uint16_t)data) << 8;
  parse->state = ubloxPayload;
  return SENTENCE_TYPE_UBX;
}

//Read the first length byte
uint8_t ubloxLength1(PARSE_STATE * parse, uint8_t data)
{
  //Calculate the checksum
  parse->ck_a += data;
  parse->ck_b += parse->ck_a;

  //Save the first length byte
  parse->bytesRemaining = data;
  parse->state = ubloxLength2;
  return SENTENCE_TYPE_UBX;
}

//Read the ID byte
uint8_t ubloxId(PARSE_STATE * parse, uint8_t data)
{
  //Calculate the checksum
  parse->ck_a += data;
  parse->ck_b += parse->ck_a;

  //Save the ID as the lower 8-bits of the message
  parse->message |= data;
  parse->state = ubloxLength1;
  return SENTENCE_TYPE_UBX;
}

//Read the class byte
uint8_t ubloxClass(PARSE_STATE * parse, uint8_t data)
{
  //Start the checksum calculation
  parse->ck_a = data;
  parse->ck_b = data;

  //Save the class as the upper 8-bits of the message
  parse->message = ((uint16_t)data) << 8;
  parse->state = ubloxId;
  return SENTENCE_TYPE_UBX;
}

//Read the second sync byte
uint8_t ubloxSync2(PARSE_STATE * parse, uint8_t data)
{
  //Verify the sync 2 byte
  if (data != 0x62)
  {
    //Display the invalid data
    dumpBuffer(parse->buffer, parse->length - 1);
    printf ("    %s Invalid UBX data, %d bytes\r\n", parse->parserName, parse->length - 1);

    //Invalid sync 2 byte, place this byte at the beginning of the buffer
    parse->length = 0;
    parse->buffer[parse->length++] = data;

    //Start searching for a preamble byte
    return waitForPreamble(parse, data);
  }

  parse->state = ubloxClass;
  return SENTENCE_TYPE_UBX;
}

//Wait for the preamble byte (0xd3)
uint8_t waitForPreamble(PARSE_STATE * parse, uint8_t data)
{
  //Verify that this is the preamble byte
  switch(data)
  {
  case '$':

    //
    //    NMEA Message
    //
    //    +----------+---------+--------+---------+----------+----------+
    //    | Preamble |  Name   | Comma  |  Data   | Asterisk | Checksum |
    //    |  8 bits  | n bytes | 8 bits | n bytes |  8 bits  | 2 bytes  |
    //    |     $    |         |    ,   |         |          |          |
    //    +----------+---------+--------+---------+----------+----------+
    //    |                                                  |
    //    |<------------------- Checksum ------------------->|
    //

    parse->crc = 0;
    parse->computeCrc = false;
    parse->nmeaMessageNameLength = 0;
    parse->state = nmeaFindFirstComma;
    return SENTENCE_TYPE_NMEA;

  case 0xb5:

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

    parse->state = ubloxSync2;
    return SENTENCE_TYPE_UBX;

  case 0xd3:

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
    //    |                                                                       |
    //    |<-------------------------------- CRC -------------------------------->|
    //

    //Start the CRC with this byte
    parse->crc = 0;
    parse->crc = COMPUTE_CRC24Q(parse, data);
    parse->computeCrc = true;

    //Get the message length
    parse->state = rtcmReadLength1;
    return SENTENCE_TYPE_RTCM;
  }

  //preamble byte not found
  parse->length = 0;
  parse->state = waitForPreamble;
  return SENTENCE_TYPE_NONE;
}
