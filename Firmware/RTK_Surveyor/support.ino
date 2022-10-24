//Gathers raw characters from user until \n or \r is received
//Handles backspace
//Used for raw mixed entry (SSID, pws, etc)
//Used by other menu input methods that use sscanf
//Returns INPUT_RESPONSE_TIMEOUT, INPUT_RESPONSE_OVERFLOW, INPUT_RESPONSE_EMPTY, or INPUT_RESPONSE_VALID
InputResponse getString(char *userString, uint8_t stringSize)
{
  clearBuffer();

  long startTime = millis();
  uint8_t spot = 0;

  while ((millis() - startTime) / 1000 <= menuTimeout)
  {
    delay(1); //Yield to processor

    //Regularly poll to get latest data
    if (online.gnss == true)
      i2cGNSS.checkUblox();

    //Get the next input character
    while (Serial.available() > 0)
    {
      byte incoming = Serial.read();

      if ((incoming == '\r') || (incoming == '\n'))
      {
        if (settings.echoUserInput) Serial.println(); //Echo if needed
        userString[spot] = '\0'; //Null terminate

        if (spot == 0) return INPUT_RESPONSE_EMPTY;

        return INPUT_RESPONSE_VALID;
      }
      //Handle backspace
      else if (incoming == '\b')
      {
        if (settings.echoUserInput == true && spot > 0)
        {
          Serial.write('\b'); //Move back one space
          Serial.print(" "); //Put a blank there to erase the letter from the terminal
          Serial.print('\b'); //Move back again
          spot--;
        }
      }
      else
      {
        if (settings.echoUserInput) Serial.write(incoming); //Echo if needed

        userString[spot++] = incoming;
        if (spot == (stringSize - 1)) //Leave room for termination
          return INPUT_RESPONSE_OVERFLOW;
      }
    }
  }

  return INPUT_RESPONSE_TIMEOUT;
}

//Gets a single character or number (0-32) from the user. Negative numbers become the positive equivalent.
//Numbers larger than 32 are allowed but will be confused with characters: ie, 74 = 'J'.
//Returns 255 if timeout
//Returns 0 if no data, only carriage return or newline
byte getCharacterNumber()
{
  char userEntry[50]; //Allow user to enter more than one char. sscanf will remove extra.
  int userByte = 0;

  InputResponse response = getString(userEntry, sizeof(userEntry));
  if (response == INPUT_RESPONSE_VALID)
  {
    int filled = sscanf(userEntry, "%d", &userByte);
    if (filled == 0) //Not a number
      sscanf(userEntry, "%c", (byte *)&userByte);
    else
    {
      if (userByte == 255)
        userByte = 0; //Not allowed
      else if (userByte > 128)
        userByte *= -1; //Drop negative sign
    }
  }
  else if (response == INPUT_RESPONSE_TIMEOUT)
  {
    Serial.println("\n\rNo user response - Do you have line endings turned on?");
    userByte = 255; //Timeout
  }
  else if (response == INPUT_RESPONSE_EMPTY)
  {
    userByte = 0; //Empty
  }

  return userByte;
}

//Get a long int from user, uses sscanf to obtain 64-bit int
//Returns INPUT_RESPONSE_GETNUMBER_EXIT if user presses 'x' or doesn't enter data
//Returns INPUT_RESPONSE_GETNUMBER_TIMEOUT if input times out
long getNumber()
{
  char userEntry[50]; //Allow user to enter more than one char. sscanf will remove extra.
  long userNumber = 0;

  InputResponse response = getString(userEntry, sizeof(userEntry));
  if (response == INPUT_RESPONSE_VALID)
  {
    if (strcmp(userEntry, "x") == 0 || strcmp(userEntry, "X") == 0)
      userNumber = INPUT_RESPONSE_GETNUMBER_EXIT;
    else
      sscanf(userEntry, "%ld", &userNumber);
  }
  else if (response == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
  {
    Serial.println("\n\rNo user response - Do you have line endings turned on?");
    userNumber = INPUT_RESPONSE_GETNUMBER_TIMEOUT; //Timeout
  }
  else if (response == INPUT_RESPONSE_EMPTY)
  {
    userNumber = INPUT_RESPONSE_GETNUMBER_EXIT; //Empty
  }

  return userNumber;
}

//Gets a double (float) from the user
//Returns 0 for timeout and empty response
double getDouble()
{
  char userEntry[50];
  double userFloat = 0.0;

  InputResponse response = getString(userEntry, sizeof(userEntry));
  if (response == INPUT_RESPONSE_VALID)
    sscanf(userEntry, "%lf", &userFloat);
  else if (response == INPUT_RESPONSE_TIMEOUT)
  {
    Serial.println("No user response - Do you have line endings turned on?");
    userFloat = 0.0;
  }
  else if (response == INPUT_RESPONSE_EMPTY)
  {
    userFloat = 0.0;
  }

  return userFloat;
}

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
