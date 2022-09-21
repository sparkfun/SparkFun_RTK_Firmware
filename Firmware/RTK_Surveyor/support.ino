void printElapsedTime(const char* title)
{
  Serial.printf("%s: %ld\r\n", title, millis() - startTime);
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

//Parse the RTCM transport data
bool checkRtcmMessage(uint8_t data)
{
  static uint16_t bytesRemaining;
  static uint16_t length;
  static uint16_t message;
  static bool sendMessage = false;

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

  switch (rtcmParsingState)
  {
    //Read the upper two bits of the length
    case RTCM_TRANSPORT_STATE_READ_LENGTH_1:
      if (!(data & 3))
      {
        length = data << 8;
        rtcmParsingState = RTCM_TRANSPORT_STATE_READ_LENGTH_2;
        break;
      }

      //Wait for the preamble byte
      rtcmParsingState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;

    //Fall through
    //     |
    //     |
    //     V

    //Wait for the preamble byte (0xd3)
    case RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3:
      sendMessage = false;
      if (data == 0xd3)
      {
        rtcmParsingState = RTCM_TRANSPORT_STATE_READ_LENGTH_1;
        sendMessage = true;
      }
      break;

    //Read the lower 8 bits of the length
    case RTCM_TRANSPORT_STATE_READ_LENGTH_2:
      length |= data;
      bytesRemaining = length;
      rtcmParsingState = RTCM_TRANSPORT_STATE_READ_MESSAGE_1;
      break;

    //Read the upper 8 bits of the message number
    case RTCM_TRANSPORT_STATE_READ_MESSAGE_1:
      message = data << 4;
      bytesRemaining -= 1;
      rtcmParsingState = RTCM_TRANSPORT_STATE_READ_MESSAGE_2;
      break;

    //Read the lower 4 bits of the message number
    case RTCM_TRANSPORT_STATE_READ_MESSAGE_2:
      message |= data >> 4;
      bytesRemaining -= 1;
      rtcmParsingState = RTCM_TRANSPORT_STATE_READ_DATA;
      break;

    //Read the rest of the message
    case RTCM_TRANSPORT_STATE_READ_DATA:
      bytesRemaining -= 1;
      if (bytesRemaining <= 0)
        rtcmParsingState = RTCM_TRANSPORT_STATE_READ_CRC_1;
      break;

    //Read the upper 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_1:
      rtcmParsingState = RTCM_TRANSPORT_STATE_READ_CRC_2;
      break;

    //Read the middle 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_2:
      rtcmParsingState = RTCM_TRANSPORT_STATE_READ_CRC_3;
      break;

    //Read the lower 8 bits of the CRC
    case RTCM_TRANSPORT_STATE_READ_CRC_3:
      rtcmParsingState = RTCM_TRANSPORT_STATE_CHECK_CRC;
      break;
  }

  //Check the CRC
  if (rtcmParsingState == RTCM_TRANSPORT_STATE_CHECK_CRC)
  {
    rtcmParsingState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;

    //Display the RTCM message header
    if (settings.enablePrintNtripServerRtcm && (!inMainMenu))
    {
      printTimeStamp();
      Serial.printf ("    Tx RTCM %d, %2d bytes\r\n", message, 3 + length + 3);
    }
  }

  //Let the upper layer know if this message should be sent
  return sendMessage;
}

const double WGS84_A = 6378137; //https://geographiclib.sourceforge.io/html/Constants_8hpp_source.html
const double WGS84_E = 0.081819190842622; //http://docs.ros.org/en/hydro/api/gps_common/html/namespacegps__common.html and https://gist.github.com/uhho/63750c4b54c7f90f37f958cc8af0c718

//From: https://stackoverflow.com/questions/19478200/convert-latitude-and-longitude-to-ecef-coordinates-system
void geodeticToEcef(double lat, double lon, double alt, double *x, double *y, double *z)
{
  double clat = cos(lat * DEG_TO_RAD);
  double slat = sin(lat * DEG_TO_RAD);
  double clon = cos(lon * DEG_TO_RAD);
  double slon = sin(lon * DEG_TO_RAD);

  double N = WGS84_A / sqrt(1.0 - WGS84_E * WGS84_E * slat * slat);

  *x = (N + alt) * clat * clon;
  *y = (N + alt) * clat * slon;
  *z = (N * (1.0 - WGS84_E * WGS84_E) + alt) * slat;
}

//From: https://danceswithcode.net/engineeringnotes/geodetic_to_ecef/geodetic_to_ecef.html
void ecefToGeodetic(double x, double y, double z, double *lat, double *lon, double *alt)
{
  double  a = 6378137.0;              //WGS-84 semi-major axis
  double e2 = 6.6943799901377997e-3;  //WGS-84 first eccentricity squared
  double a1 = 4.2697672707157535e+4;  //a1 = a*e2
  double a2 = 1.8230912546075455e+9;  //a2 = a1*a1
  double a3 = 1.4291722289812413e+2;  //a3 = a1*e2/2
  double a4 = 4.5577281365188637e+9;  //a4 = 2.5*a2
  double a5 = 4.2840589930055659e+4;  //a5 = a1+a3
  double a6 = 9.9330562000986220e-1;  //a6 = 1-e2

  double zp, w2, w, r2, r, s2, c2, s, c, ss;
  double g, rg, rf, u, v, m, f, p;

  zp = abs(z);
  w2 = x * x + y * y;
  w = sqrt(w2);
  r2 = w2 + z * z;
  r = sqrt(r2);
  *lon = atan2(y, x);       //Lon (final)

  s2 = z * z / r2;
  c2 = w2 / r2;
  u = a2 / r;
  v = a3 - a4 / r;
  if (c2 > 0.3)
  {
    s = (zp / r) * (1.0 + c2 * (a1 + u + s2 * v) / r);
    *lat = asin(s);      //Lat
    ss = s * s;
    c = sqrt(1.0 - ss);
  }
  else
  {
    c = ( w / r ) * ( 1.0 - s2 * ( a5 - u - c2 * v ) / r );
    *lat = acos( c );      //Lat
    ss = 1.0 - c * c;
    s = sqrt( ss );
  }

  g = 1.0 - e2 * ss;
  rg = a / sqrt( g );
  rf = a6 * rg;
  u = w - rg * c;
  v = zp - rf * s;
  f = c * u + s * v;
  m = c * v - s * u;
  p = m / ( rf / g + f );
  *lat = *lat + p;      //Lat
  *alt = f + m * p / 2.0; //Altitude
  if ( z < 0.0 ) {
    *lat *= -1.0;     //Lat
  }

  *lat *= RAD_TO_DEG; //Convert to degrees
  *lon *= RAD_TO_DEG;
}
