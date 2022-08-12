void printElapsedTime(const char* title)
{
  Serial.printf("%s: %ld\n\r", title, millis() - startTime);
}

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
