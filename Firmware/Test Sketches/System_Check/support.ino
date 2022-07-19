//Option not known
void printUnknown(uint8_t unknownChoice)
{
  Serial.print(F("Unknown choice: "));
  Serial.write(unknownChoice);
  Serial.println();
}
void printUnknown(int unknownValue)
{
  Serial.print(F("Unknown value: "));
  Serial.write(unknownValue);
  Serial.println();
}

//Clear the Serial RX buffer before we begin scanning for characters
void clearBuffer()
{
  Serial.flush();
  delay(20);//Wait for any incoming chars to hit buffer
  while (Serial.available() > 0) Serial.read(); //Clear buffer
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
      Serial.println(F("No user input received."));
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
          Serial.println(F("No user input received. Do you have line endings turned on?"));
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
      Serial.println(F("Do you have line endings turned on?"));
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
