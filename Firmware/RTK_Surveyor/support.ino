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

//Verify the NMEA checksum byte
bool invalidChecksumByte(uint8_t data, uint32_t checksum)
{
  //Convert the data into a binary value
  if ((data >= 'A') && (data <= 'F'))
    data -= 'A' - 10;
  else if ((data >= 'a') && (data <= 'f'))
    data -= 'a' - 10;
  else if ((data >= '0') && (data <= '9'))
    data -= '0';

  //Compare with the lower 4 bits of the checksum
  return ((checksum & 0xf) != data);
}

#define COMPUTE_CRC24Q(data)  ((parse->crc << 8) ^ crc24q[data ^ ((parse->crc >> 16) & 0xff)])

//Parse the NMEA and RTCM messages
bool parseNmeaAndRtcmMessages(PARSE_STATE * parse, uint8_t data, bool sendMessage)
{
  //    NMEA
  //
  //    |<--------------- Checksum --------------->|
  //    |                                          |
  //    +----------+----------+-- *** --+----------+----------+----------+----------+
  //    | Preamble | Message  |         | Asterisk | Checksum | Carriage | Linefeed |
  //    |  8 bits  |   Name   |         |          |  2 Hex   |          |          |
  //    |     $    |  G.....  |         |     *    |  Bytes   |   0x0d   |   0x0a   |
  //    +----------+----------+-- ***---+----------+----------+----------+----------+
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

  switch (parse->state)
  {
    //Wait for the preamble byte (0xd3)
    case PARSE_STATE_WAIT_FOR_PREAMBLE:
      parse->sendMessage = false;
      parse->crc = 0;
      if (data == 0xd3)
      {
        parse->computeCRC = true;
        parse->sendMessage = sendMessage;
        parse->state = PARSE_STATE_RTCM_READ_LENGTH_1;
      }
      else if (data == '$')
      {
        parse->length = 1;
        parse->nameLength = 0;
        parse->sendMessage = sendMessage;
        parse->state = PARSE_STATE_NMEA_FIND_COMMA;
      }
      else if ((data != '\r') && (data != '\n'))
      {
        parse->invalidByte = true;
        parse->invalidCharacters++;
      }
      break;

    //Read the upper two bits of the length
    case PARSE_STATE_RTCM_READ_LENGTH_1:
      parse->length = data << 8;
      parse->state = PARSE_STATE_RTCM_READ_LENGTH_2;
      break;

    //Read the lower 8 bits of the length
    case PARSE_STATE_RTCM_READ_LENGTH_2:
      parse->length |= data;
      parse->bytesRemaining = parse->length;
      parse->state = PARSE_STATE_RTCM_READ_MESSAGE_1;
      break;

    //Read the upper 8 bits of the message number
    case PARSE_STATE_RTCM_READ_MESSAGE_1:
      parse->message = data << 4;
      parse->bytesRemaining -= 1;
      parse->state = PARSE_STATE_RTCM_READ_MESSAGE_2;
      break;

    //Read the lower 4 bits of the message number
    case PARSE_STATE_RTCM_READ_MESSAGE_2:
      parse->message |= data >> 4;
      parse->bytesRemaining -= 1;
      parse->state = PARSE_STATE_RTCM_READ_DATA;
      break;

    //Read the rest of the message
    case PARSE_STATE_RTCM_READ_DATA:
      parse->bytesRemaining -= 1;
      if (parse->bytesRemaining <= 0)
        parse->state = PARSE_STATE_RTCM_READ_CRC_1;
      break;

    //Read the upper 8 bits of the CRC
    case PARSE_STATE_RTCM_READ_CRC_1:
      parse->rtcmCrc = parse->crc & 0x00ffffff;
      parse->crcByte[0] = data;
      parse->state = PARSE_STATE_RTCM_READ_CRC_2;
      break;

    //Read the middle 8 bits of the CRC
    case PARSE_STATE_RTCM_READ_CRC_2:
      parse->crcByte[1] = data;
      parse->state = PARSE_STATE_RTCM_READ_CRC_3;
      break;

    //Read the lower 8 bits of the CRC
    case PARSE_STATE_RTCM_READ_CRC_3:
      parse->crcByte[2] = data;
      parse->crc = COMPUTE_CRC24Q(data);
      parse->invalidRtcmCrc = ((parse->crc & 0x00ffffff) != 0);
      parse->computeCRC = false;
      parse->rtcmPackets++;
      parse->messageNumber = parse->message;
      parse->printMessageNumber = true;
      parse->state = PARSE_STATE_WAIT_FOR_PREAMBLE;
      break;

    //Check for the end of the message name
    case PARSE_STATE_NMEA_FIND_COMMA:
      parse->length++;
      parse->crc ^= data;
      if (data == ',')
      {
        parse->name[parse->nameLength++] = 0;
        parse->state = PARSE_STATE_NMEA_FIND_ASTERISK;
      }
      else
      {
        if (parse->nameLength < (sizeof(parse->name) - 1))
          parse->name[parse->nameLength++] = data;
      }
      break;

    //Check for the end of the NMEA message
    case PARSE_STATE_NMEA_FIND_ASTERISK:
      parse->length++;
      if (data == '*')
      {
        parse->nmeaChecksum = (byte)parse->crc;
        parse->state = PARSE_STATE_NMEA_VERIFY_CHECKSUM_1;
      }
      else
        parse->crc ^= data;
      break;

    //Read the ASCII hex character representing the upper 4-bits of the checksum
    case PARSE_STATE_NMEA_VERIFY_CHECKSUM_1:
      parse->length++;
      parse->checksumByte1 = data;
      parse->state = PARSE_STATE_NMEA_VERIFY_CHECKSUM_2;
      break;

    //Read the ASCII hex character representing the lower 4-bits of the checksum
    case PARSE_STATE_NMEA_VERIFY_CHECKSUM_2:
      parse->length++;
      parse->checksumByte2 = data;
      if (invalidChecksumByte(parse->checksumByte1, parse->crc >> 4)
        || invalidChecksumByte(data, parse->crc))
      {
        parse->invalidNmeaChecksum = true;
        parse->invalidNmeaChecksums++;
      }
      parse->state = PARSE_STATE_WAIT_FOR_PREAMBLE;
      parse->printMessageName = true;
      strcpy(parse->messageName, parse->name);
      break;
  }

  //Compute the CRC if necessary
  if (parse->computeCRC)
    parse->crc = COMPUTE_CRC24Q(data);

  //Let the upper layer know if this message should be sent
  return parse->sendMessage && sendMessage;
}
