#ifdef COMPILE_L_BAND

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#define CONTENT_SIZE 2000

static SFE_UBLOX_GNSS_ADD i2cLBand; // NEO-D9S
static const char* pointPerfectKeyTopic = "/pp/ubx/0236/Lb";

//The PointPerfect token is provided at compile time via build flags
#ifndef POINTPERFECT_TOKEN
#define POINTPERFECT_TOKEN 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x11, 0x22, 0x33, 0x0A, 0x0B, 0x0C, 0x0D, 0x00, 0x01, 0x02, 0x03
#endif

static uint8_t pointPerfectTokenArray[16] = {POINTPERFECT_TOKEN}; //Token in HEX form

static const char* pointPerfectAPI = "https://api.thingstream.io/ztp/pointperfect/credentials";

//----------------------------------------
// Forward declarations - compiled out
//----------------------------------------

void checkRXMCOR(UBX_RXM_COR_data_t *ubxDataStruct);

//----------------------------------------
// L-Band Routines - compiled out
//----------------------------------------

void menuPointPerfectKeys()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: PointPerfect Keys");

    Serial.print("1) Set ThingStream Device Profile Token: ");
    if (strlen(settings.pointPerfectDeviceProfileToken) > 0)
      Serial.println(settings.pointPerfectDeviceProfileToken);
    else
      Serial.println("Use SparkFun Token");

    Serial.print("2) Set Current Key: ");
    if (strlen(settings.pointPerfectCurrentKey) > 0)
      Serial.println(settings.pointPerfectCurrentKey);
    else
      Serial.println("N/A");

    Serial.print("3) Set Current Key Expiration Date (DD/MM/YYYY): ");
    if (strlen(settings.pointPerfectCurrentKey) > 0 && settings.pointPerfectCurrentKeyStart > 0 && settings.pointPerfectCurrentKeyDuration > 0)
    {
      long long unixEpoch = thingstreamEpochToGPSEpoch(settings.pointPerfectCurrentKeyStart, settings.pointPerfectCurrentKeyDuration);

      uint16_t keyGPSWeek;
      uint32_t keyGPSToW;
      unixEpochToWeekToW(unixEpoch, &keyGPSWeek, &keyGPSToW);

      long expDay;
      long expMonth;
      long expYear;
      gpsWeekToWToDate(keyGPSWeek, keyGPSToW, &expDay, &expMonth, &expYear);

      Serial.printf("%02ld/%02ld/%ld\r\n", expDay, expMonth, expYear);
    }
    else
      Serial.println("N/A");

    Serial.print("4) Set Next Key: ");
    if (strlen(settings.pointPerfectNextKey) > 0)
      Serial.println(settings.pointPerfectNextKey);
    else
      Serial.println("N/A");

    Serial.print("5) Set Next Key Expiration Date (DD/MM/YYYY): ");
    if (strlen(settings.pointPerfectNextKey) > 0 && settings.pointPerfectNextKeyStart > 0 && settings.pointPerfectNextKeyDuration > 0)
    {
      long long unixEpoch = thingstreamEpochToGPSEpoch(settings.pointPerfectNextKeyStart, settings.pointPerfectNextKeyDuration);

      uint16_t keyGPSWeek;
      uint32_t keyGPSToW;
      unixEpochToWeekToW(unixEpoch, &keyGPSWeek, &keyGPSToW);

      long expDay;
      long expMonth;
      long expYear;
      gpsWeekToWToDate(keyGPSWeek, keyGPSToW, &expDay, &expMonth, &expYear);

      Serial.printf("%02ld/%02ld/%ld\r\n", expDay, expMonth, expYear);
    }
    else
      Serial.println("N/A");

    Serial.println("x) Exit");

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
    {
      Serial.print("Enter Device Profile Token: ");
      readLine(settings.pointPerfectDeviceProfileToken, sizeof(settings.pointPerfectDeviceProfileToken), menuTimeout);
    }
    else if (incoming == 2)
    {
      Serial.print("Enter Current Key: ");
      readLine(settings.pointPerfectCurrentKey, sizeof(settings.pointPerfectCurrentKey), menuTimeout);
    }
    else if (incoming == 3)
    {
      while (Serial.available()) Serial.read();

      Serial.println("Enter Current Key Expiration Date: ");
      uint8_t expDay;
      uint8_t expMonth;
      uint16_t expYear;
      while (getDate(expDay, expMonth, expYear) == false)
      {
        Serial.println("Date invalid. Please try again.");
      }

      dateToKeyStartDuration(expDay, expMonth, expYear, &settings.pointPerfectCurrentKeyStart, &settings.pointPerfectCurrentKeyDuration);

      //Calculate the next key expiration date
      if (settings.pointPerfectNextKeyStart == 0)
      {
        settings.pointPerfectNextKeyStart = settings.pointPerfectCurrentKeyStart + settings.pointPerfectCurrentKeyDuration + 1; //Next key starts after current key
        settings.pointPerfectNextKeyDuration = settings.pointPerfectCurrentKeyDuration;

        Serial.printf("  settings.pointPerfectNextKeyStart: %lld\r\n", settings.pointPerfectNextKeyStart);
        Serial.printf("  settings.pointPerfectNextKeyDuration: %lld\r\n", settings.pointPerfectNextKeyDuration);
      }
    }
    else if (incoming == 4)
    {
      Serial.print("Enter Next Key: ");
      readLine(settings.pointPerfectNextKey, sizeof(settings.pointPerfectNextKey), menuTimeout);
    }
    else if (incoming == 5)
    {
      while (Serial.available()) Serial.read();

      Serial.println("Enter Next Key Expiration Date: ");
      uint8_t expDay;
      uint8_t expMonth;
      uint16_t expYear;
      while (getDate(expDay, expMonth, expYear) == false)
      {
        Serial.println("Date invalid. Please try again.");
      }

      dateToKeyStartDuration(expDay, expMonth, expYear, &settings.pointPerfectNextKeyStart, &settings.pointPerfectNextKeyDuration);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Connect to 'home' WiFi and then ThingStream API. This will attach this unique device to the ThingStream network.
bool pointperfectProvisionDevice()
{
#ifdef COMPILE_WIFI
  bluetoothStop(); //Free heap before starting secure client (requires ~70KB)

  DynamicJsonDocument * jsonZtp = NULL;
  char * tempHolder = NULL;
  bool retVal = false;

  do
  {
    WiFiClientSecure client;
    client.setCACert(AWS_PUBLIC_CERT);

    char hardwareID[13];
    sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", lbandMACAddress[0], lbandMACAddress[1], lbandMACAddress[2], lbandMACAddress[3], lbandMACAddress[4], lbandMACAddress[5]); //Get ready for JSON

#ifdef WHITELISTED_ID
    //Override ID with testing ID
    sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", whitelistID[0], whitelistID[1], whitelistID[2], whitelistID[3], whitelistID[4], whitelistID[5]);
#endif

    char givenName[100];
    sprintf(givenName, "SparkFun RTK %s v%d.%d - %s", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, hardwareID); //Get ready for JSON

    StaticJsonDocument<256> pointPerfectAPIPost;

    //Determine if we use the SparkFun token or custom token
    char tokenString[37] = "\0";
    if (strlen(settings.pointPerfectDeviceProfileToken) == 0)
    {
      //Convert uint8_t array into string with dashes in spots
      //We must assume u-blox will not change the position of their dashes or length of their token
      for (int x = 0 ; x < sizeof(pointPerfectTokenArray) ; x++)
      {
        char temp[3];
        sprintf(temp, "%02x", pointPerfectTokenArray[x]);
        strcat(tokenString, temp);
        if (x == 3 || x == 5 || x == 7 || x == 9) strcat(tokenString, "-");
      }
    }
    else
    {
      //Use the user's custom token
      strcpy(tokenString, settings.pointPerfectDeviceProfileToken);
      Serial.printf("Using custom token: %s\r\n", tokenString);
    }

    pointPerfectAPIPost["token"] = tokenString;
    pointPerfectAPIPost["givenName"] = givenName;
    pointPerfectAPIPost["hardwareId"] = hardwareID;
    //pointPerfectAPIPost["tags"] = "mac";

    String json;
    serializeJson(pointPerfectAPIPost, json);

    Serial.printf("Connecting to: %s\r\n", pointPerfectAPI);

    HTTPClient http;
    http.begin(client, pointPerfectAPI);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(json);

    String response = http.getString();

    http.end();

    if (httpResponseCode != 200)
    {
      Serial.printf("HTTP response error %d: ", httpResponseCode);
      Serial.println(response);
      break;
    }
    else
    {
      //Device is now active with ThingStream
      //Pull pertinent values from response
      jsonZtp = new DynamicJsonDocument(4096);
      if (!jsonZtp)
      {
        Serial.println("ERROR - Failed to allocate jsonZtp!\r\n");
        break;
      }
      DeserializationError error = deserializeJson(*jsonZtp, response);
      if (DeserializationError::Ok != error)
      {
        Serial.println("JSON error");
        break;
      }
      else
      {
        tempHolder = (char *)malloc(2000);
        if (!tempHolder)
        {
          Serial.println("ERROR - Failed to allocate tempHolder buffer!\r\n");
          break;
        }
        strcpy(tempHolder, (const char*)((*jsonZtp)["certificate"]));
        //      Serial.printf("len of PrivateCert: %d\r\n", strlen(tempHolder));
        //      Serial.printf("privateCert: %s\r\n", tempHolder);
        recordFile("certificate", tempHolder, strlen(tempHolder));

        strcpy(tempHolder, (const char*)((*jsonZtp)["privateKey"]));
        //      Serial.printf("len of privateKey: %d\r\n", strlen(tempHolder));
        //      Serial.printf("privateKey: %s\r\n", tempHolder);
        recordFile("privateKey", tempHolder, strlen(tempHolder));

        strcpy(settings.pointPerfectClientID, (const char*)((*jsonZtp)["clientId"]));
        strcpy(settings.pointPerfectBrokerHost, (const char*)((*jsonZtp)["brokerHost"]));
        strcpy(settings.pointPerfectLBandTopic, (const char*)((*jsonZtp)["subscriptions"][0]["path"]));

        strcpy(settings.pointPerfectNextKey, (const char*)((*jsonZtp)["dynamickeys"]["next"]["value"]));
        settings.pointPerfectNextKeyDuration = (*jsonZtp)["dynamickeys"]["next"]["duration"];
        settings.pointPerfectNextKeyStart = (*jsonZtp)["dynamickeys"]["next"]["start"];

        strcpy(settings.pointPerfectCurrentKey, (const char*)((*jsonZtp)["dynamickeys"]["current"]["value"]));
        settings.pointPerfectCurrentKeyDuration = (*jsonZtp)["dynamickeys"]["current"]["duration"];
        settings.pointPerfectCurrentKeyStart = (*jsonZtp)["dynamickeys"]["current"]["start"];
      }
    } //HTTP Response was 200

    Serial.println("Device successfully provisioned. Keys obtained.");

    recordSystemSettings();
    retVal = true;
  } while (0);

  //Free the allocated buffers
  if (tempHolder)
    free (tempHolder);
  if (jsonZtp)
    delete jsonZtp;

  bluetoothStart();
  
  return (retVal);
#else
  return (false);
#endif
}

//Subscribe to MQTT channel, grab keys, then stop
bool pointperfectUpdateKeys()
{
#ifdef COMPILE_WIFI
  bluetoothStop(); //Release available heap to allow room for TLS 

  char * certificateContents = NULL; //Holds the contents of the keys prior to MQTT connection
  char * keyContents = NULL;
  WiFiClientSecure secureClient;
  bool gotKeys = false;

  do
  {
    //Allocate the buffers
    certificateContents = (char*)malloc(CONTENT_SIZE);
    keyContents = (char*)malloc(CONTENT_SIZE);
    if ((!certificateContents) || (!keyContents))
    {
      Serial.println("Failed to allocate content buffers!");
      break;
    }

    //Get the certificate
    memset(certificateContents, 0, CONTENT_SIZE);
    loadFile("certificate", certificateContents);
    secureClient.setCertificate(certificateContents);

    //Get the private key
    memset(keyContents, 0, CONTENT_SIZE);
    loadFile("privateKey", keyContents);
    secureClient.setPrivateKey(keyContents);

    secureClient.setCACert(AWS_PUBLIC_CERT);

    PubSubClient mqttClient(secureClient);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setServer(settings.pointPerfectBrokerHost, 8883);

    log_d("Connecting to MQTT broker: %s", settings.pointPerfectBrokerHost);

    //Loop until we're connected or until the maximum retries are exceeded
    mqttMessageReceived = false;
    int maxTries = 3;
    do
    {
      Serial.print("MQTT connecting...");

      //Attempt to the key broker
      if (mqttClient.connect(settings.pointPerfectClientID))
      {
        //Successful connection
        Serial.println("connected");
        //mqttClient.subscribe(settings.pointPerfectLBandTopic); //The /pp/key/Lb channel fails to respond with keys
        mqttClient.subscribe("/pp/ubx/0236/Lb"); //Alternate channel for L-Band keys
        break;
      }

      //Retry the connection attempt
      if (--maxTries)
      {
        Serial.print(".");
        log_d("failed, status code: %d try again in 1 second", mqttClient.state());
        delay(1000);
      }
    } while (maxTries);

    //Check for connection failure
    if (mqttClient.connected() == false)
    {
      Serial.println("failed!");
      log_d("MQTT failed to connect");
      break;
    }

    Serial.print("Waiting for keys");

    //Wait for callback
    startTime = millis();
    while (1)
    {
      mqttClient.loop();
      if (mqttMessageReceived == true)
        break;
      if (mqttClient.connected() == false)
      {
        log_d("Client disconnected");
        break;
      }
      if (millis() - startTime > 8000)
      {
        log_d("Channel failed to respond");
        break;
      }

      //Continue waiting for the keys
      delay(100);
      Serial.print(".");
    }
    Serial.println();

    //Determine if the keys were updated
    if (mqttMessageReceived)
    {
      Serial.println("Keys successfully updated");
      gotKeys = true;
    }

    //Done with the MQTT client
    mqttClient.disconnect();
  } while (0);

  //Free the content buffers
  if (keyContents)
    free(keyContents);
  if (certificateContents)
    free(certificateContents);

  bluetoothStart();

  //Return the key status
  return (gotKeys);
#else
  return (false);
#endif
}

char *ltrim(char *s)
{
  while (isspace(*s)) s++;
  return s;
}

//Called when a subscribed to message arrives
void mqttCallback(char* topic, byte* message, unsigned int length)
{
  if (String(topic) == pointPerfectKeyTopic)
  {
    //Separate the UBX message into its constituent Key/ToW/Week parts
    //Obtained from SparkFun u-blox Arduino library - setDynamicSPARTNKeys()
    byte* payLoad = &message[6];
    uint8_t currentKeyLength = payLoad[5];
    uint16_t currentWeek = (payLoad[7] << 8) | payLoad[6];
    uint32_t currentToW = (payLoad[11] << 8 * 3) | (payLoad[10] << 8 * 2) | (payLoad[9] << 8 * 1) | (payLoad[8] << 8 * 0);

    char currentKey[currentKeyLength];
    memcpy(&currentKey, &payLoad[20], currentKeyLength);

    uint8_t nextKeyLength = payLoad[13];
    uint16_t nextWeek = (payLoad[15] << 8) | payLoad[14];
    uint32_t nextToW = (payLoad[19] << 8 * 3) | (payLoad[18] << 8 * 2) | (payLoad[17] << 8 * 1) | (payLoad[16] << 8 * 0);

    char nextKey[nextKeyLength];
    memcpy(&nextKey, &payLoad[20 + currentKeyLength], nextKeyLength);

    //Convert byte array to HEX character array
    strcpy(settings.pointPerfectCurrentKey, ""); //Clear contents
    strcpy(settings.pointPerfectNextKey, ""); //Clear contents
    for (int x = 0 ; x < 16 ; x++) //Force length to max of 32 bytes
    {
      char temp[3];
      sprintf(temp, "%02X", currentKey[x]);
      strcat(settings.pointPerfectCurrentKey, temp);

      sprintf(temp, "%02X", nextKey[x]);
      strcat(settings.pointPerfectNextKey, temp);
    }

    //    Serial.println();

    //    Serial.printf("pointPerfectCurrentKeyStart: %lld\r\n", settings.pointPerfectCurrentKeyStart);
    //    Serial.printf("pointPerfectCurrentKeyDuration: %lld\r\n", settings.pointPerfectCurrentKeyDuration);
    //    Serial.printf("pointPerfectNextKeyStart: %lld\r\n", settings.pointPerfectNextKeyStart);
    //    Serial.printf("pointPerfectNextKeyDuration: %lld\r\n", settings.pointPerfectNextKeyDuration);
    //
    //    Serial.printf("currentWeek: %d\r\n", currentWeek);
    //    Serial.printf("currentToW: %d\r\n", currentToW);
    //    Serial.print("Current key: ");
    //    Serial.println(settings.pointPerfectCurrentKey);
    //
    //    Serial.printf("nextWeek: %d\r\n", nextWeek);
    //    Serial.printf("nextToW: %d\r\n", nextToW);
    //    Serial.print("nextKey key: ");
    //    Serial.println(settings.pointPerfectNextKey);

    //Convert from ToW and Week to key duration and key start
    WeekToWToUnixEpoch(&settings.pointPerfectCurrentKeyStart, currentWeek, currentToW);
    WeekToWToUnixEpoch(&settings.pointPerfectNextKeyStart, nextWeek, nextToW);

    settings.pointPerfectCurrentKeyStart -= getLeapSeconds(); //Remove GPS leap seconds to align with u-blox
    settings.pointPerfectNextKeyStart -= getLeapSeconds();

    settings.pointPerfectCurrentKeyStart *= 1000; //Convert to ms
    settings.pointPerfectNextKeyStart *= 1000;

    settings.pointPerfectCurrentKeyDuration = settings.pointPerfectNextKeyStart - settings.pointPerfectCurrentKeyStart - 1;
    settings.pointPerfectNextKeyDuration = settings.pointPerfectCurrentKeyDuration; //We assume next key duration is the same as current key duration because we have to

    //    Serial.printf("pointPerfectCurrentKeyStart: %lld\r\n", settings.pointPerfectCurrentKeyStart);
    //    Serial.printf("pointPerfectCurrentKeyDuration: %lld\r\n", settings.pointPerfectCurrentKeyDuration);
    //    Serial.printf("pointPerfectNextKeyStart: %lld\r\n", settings.pointPerfectNextKeyStart);
    //    Serial.printf("pointPerfectNextKeyDuration: %lld\r\n", settings.pointPerfectNextKeyDuration);
  }

  mqttMessageReceived = true;
}

//Get a date from a user
//Return true if validated
//https://www.includehelp.com/c-programs/validate-date.aspx
bool getDate(uint8_t &dd, uint8_t &mm, uint16_t &yy)
{
  char temp[10];

  Serial.print("Enter Day: ");
  readLine(temp, sizeof(temp), menuTimeout);
  dd = atoi(temp);

  Serial.print("Enter Month: ");
  readLine(temp, sizeof(temp), menuTimeout);
  mm = atoi(temp);

  Serial.print("Enter Year (YYYY): ");
  readLine(temp, sizeof(temp), menuTimeout);
  yy = atoi(temp);

  //check year
  if (yy >= 2022 && yy <= 9999)
  {
    //check month
    if (mm >= 1 && mm <= 12)
    {
      //check days
      if ((dd >= 1 && dd <= 31) && (mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12))
        return (true);
      else if ((dd >= 1 && dd <= 30) && (mm == 4 || mm == 6 || mm == 9 || mm == 11))
        return (true);
      else if ((dd >= 1 && dd <= 28) && (mm == 2))
        return (true);
      else if (dd == 29 && mm == 2 && (yy % 400 == 0 || (yy % 4 == 0 && yy % 100 != 0)))
        return (true);
      else
      {
        printf("Day is invalid.\n");
        return (false);
      }
    }
    else
    {
      printf("Month is not valid.\n");
      return (false);
    }
  }

  printf("Year is not valid.\n");
  return (false);
}

//Given an epoch in ms, return the number of days from given and Epoch now
int daysFromEpoch(long long endEpoch)
{
  endEpoch /= 1000; //Convert PointPerfect ms Epoch to s

  long localEpoch = rtc.getEpoch();

  long delta = endEpoch - localEpoch; //number of s between dates
  delta /= (60 * 60); //hours
  delta /= 24; //days
  return ((int)delta);
}

//Given the key's starting epoch time, and the key's duration
//Convert from ms to s
//Add leap seconds (the API reports start times with GPS leap seconds removed)
//Convert from unix epoch (the API reports unix epoch time) to GPS epoch (the NED-D9S expects)
//Note: I believe the Thingstream API is reporting startEpoch 18 seconds less than actual
//Even though we are adding 18 leap seconds, the ToW is still coming out as 518400 instead of 518418 (midnight)
long long thingstreamEpochToGPSEpoch(long long startEpoch, long long duration)
{
  long long epoch = startEpoch + duration;
  epoch /= 1000; //Convert PointPerfect ms Epoch to s

  //Convert Unix Epoch time from PointPerfect to GPS Time Of Week needed for UBX message
  long long gpsEpoch = epoch - 315964800 + getLeapSeconds(); //Shift to GPS Epoch.
  return (gpsEpoch);
}

//Query GNSS for current leap seconds
uint8_t getLeapSeconds()
{
  if (online.gnss == true)
  {
    if (leapSeconds == 0) //Check to see if we've already set it
    {
      sfe_ublox_ls_src_e leapSecSource;
      leapSeconds = i2cGNSS.getCurrentLeapSeconds(leapSecSource);
      return (leapSeconds);
    }
  }
  return (18); //Default to 18 if GNSS if offline
}

//Covert a given the key's expiration date to a GPS Epoch, so that we can calculate GPS Week and ToW
//Add a millisecond to roll over from 11:59UTC to midnight of the following day
//Convert from unix epoch (time lib outputs unix) to GPS epoch (the NED-D9S expects)
long long dateToGPSEpoch(uint8_t day, uint8_t month, uint16_t year)
{
  long long unixEpoch = dateToUnixEpoch(day, month, year); //Returns Unix Epoch

  //Convert Unix Epoch time from PP to GPS Time Of Week needed for UBX message
  long long gpsEpoch = unixEpoch - 315964800; //Shift to GPS Epoch.

  return (gpsEpoch);
}

//Given an epoch, set the GPSWeek and GPSToW
void unixEpochToWeekToW(long long unixEpoch, uint16_t *GPSWeek, uint32_t *GPSToW)
{
  *GPSWeek = (uint16_t)(unixEpoch / (7 * 24 * 60 * 60));
  *GPSToW = (uint32_t)(unixEpoch % (7 * 24 * 60 * 60));
}

//Given an epoch, set the GPSWeek and GPSToW
void WeekToWToUnixEpoch(uint64_t *unixEpoch, uint16_t GPSWeek, uint32_t GPSToW)
{
  *unixEpoch = GPSWeek * (7 * 24 * 60 * 60); //2192
  *unixEpoch += GPSToW; //518400
  *unixEpoch += 315964800;
}

//Given a GPS Week and ToW, convert to an expiration date
void gpsWeekToWToDate(uint16_t keyGPSWeek, uint32_t keyGPSToW, long *expDay, long *expMonth, long *expYear)
{
  long gpsDays = gpsToMjd(0, (long)keyGPSWeek, (long)keyGPSToW); //Covert ToW and Week to # of days since Jan 6, 1980
  mjdToDate(gpsDays, expYear, expMonth, expDay);
}

//Given a date, convert into epoch
//https://www.epochconverter.com/programming/c
long dateToUnixEpoch(uint8_t day, uint8_t month, uint16_t year)
{
  struct tm t;
  time_t t_of_day;

  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;

  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;
  t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown

  t_of_day = mktime(&t);

  return (t_of_day);
}

//Given a date, calculate and store the key start and duration
void dateToKeyStartDuration(uint8_t expDay, uint8_t expMonth, uint16_t expYear, uint64_t *settingsKeyStart, uint64_t *settingsKeyDuration)
{
  long long expireUnixEpoch = dateToUnixEpoch(expDay, expMonth, expYear);

  //Thingstream lists the date that a key expires at midnight
  //So if a user types in March 7th, 2022 as exp date the key's Week and ToW need to be
  //calculated from (March 7th - 27 days).
  long long startUnixEpoch = expireUnixEpoch - (27 * 24 * 60 * 60); //Move back 27 days

  //Additionally, Thingstream seems to be reporting Epochs that do not have leap seconds
  startUnixEpoch -= getLeapSeconds(); //Modify our Epoch to match Point Perfect

  //PointPerfect uses/reports unix epochs in milliseconds
  *settingsKeyStart = startUnixEpoch * 1000L; //Convert to ms
  *settingsKeyDuration = (28 * 24 * 60 * 60 * 1000LL) - 1; //We assume keys last for 28 days (minus one ms to be before midnight)

  Serial.printf("  KeyStart: %lld\r\n", *settingsKeyStart);
  Serial.printf("  KeyDuration: %lld\r\n", *settingsKeyDuration);

  //Print ToW and Week for debugging
  uint16_t keyGPSWeek;
  uint32_t keyGPSToW;
  long long unixEpoch = thingstreamEpochToGPSEpoch(*settingsKeyStart, *settingsKeyDuration);
  unixEpochToWeekToW(unixEpoch, &keyGPSWeek, &keyGPSToW);
  Serial.printf("  keyGPSWeek: %d\r\n", keyGPSWeek);
  Serial.printf("  keyGPSToW: %d\r\n", keyGPSToW);
}

/*
   http://www.leapsecond.com/tools/gpsdate.c
   Return Modified Julian Day given calendar year,
   month (1-12), and day (1-31).
   - Valid for Gregorian dates from 17-Nov-1858.
   - Adapted from sci.astro FAQ.
*/

long dateToMjd(long Year, long Month, long Day)
{
  return
    367 * Year
    - 7 * (Year + (Month + 9) / 12) / 4
    - 3 * ((Year + (Month - 9) / 7) / 100 + 1) / 4
    + 275 * Month / 9
    + Day
    + 1721028
    - 2400000;
}

/*
   Convert Modified Julian Day to calendar date.
   - Assumes Gregorian calendar.
   - Adapted from Fliegel/van Flandern ACM 11/#10 p 657 Oct 1968.
*/

void mjdToDate(long Mjd, long *Year, long *Month, long *Day)
{
  long J, C, Y, M;

  J = Mjd + 2400001 + 68569;
  C = 4 * J / 146097;
  J = J - (146097 * C + 3) / 4;
  Y = 4000 * (J + 1) / 1461001;
  J = J - 1461 * Y / 4 + 31;
  M = 80 * J / 2447;
  *Day = J - 2447 * M / 80;
  J = M / 11;
  *Month = M + 2 - (12 * J);
  *Year = 100 * (C - 49) + Y + J;
}

/*
   Convert GPS Week and Seconds to Modified Julian Day.
   - Ignores UTC leap seconds.
*/

long gpsToMjd(long GpsCycle, long GpsWeek, long GpsSeconds)
{
  long GpsDays = ((GpsCycle * 1024) + GpsWeek) * 7 + (GpsSeconds / 86400);
  //GpsDays -= 1; //Correction
  return dateToMjd(1980, 1, 6) + GpsDays;
}

//When new PMP message arrives from NEO-D9S push it back to ZED-F9P
void pushRXMPMP(UBX_RXM_PMP_message_data_t *pmpData)
{
  uint16_t payloadLen = ((uint16_t)pmpData->lengthMSB << 8) | (uint16_t)pmpData->lengthLSB;
  log_d("Pushing %d bytes of RXM-PMP data to GNSS", payloadLen);

  i2cGNSS.pushRawData(&pmpData->sync1, (size_t)payloadLen + 6); // Push the sync chars, class, ID, length and payload
  i2cGNSS.pushRawData(&pmpData->checksumA, (size_t)2); // Push the checksum bytes
}

//If we have decryption keys, and L-Band is online, configure module
void pointperfectApplyKeys()
{
  if (online.lband == true)
  {
    if (online.gnss == false)
    {
      log_d("ZED-F9P not avaialable");
      return;
    }

    //NEO-D9S encrypted PMP messages are only supported on ZED-F9P firmware v1.30 and above
    if (zedModuleType != PLATFORM_F9P)
    {
      Serial.println("Error: PointPerfect corrections currently only supported on the ZED-F9P.");
      return;
    }
    if (zedFirmwareVersionInt < 130)
    {
      Serial.println("Error: PointPerfect corrections currently supported by ZED-F9P firmware v1.30 and above. Please upgrade your ZED firmware: https://learn.sparkfun.com/tutorials/how-to-upgrade-firmware-of-a-u-blox-gnss-receiver");
      return;
    }

    if (strlen(settings.pointPerfectNextKey) > 0)
    {
      const uint8_t currentKeyLengthBytes = 16;
      const uint8_t nextKeyLengthBytes = 16;

      uint16_t currentKeyGPSWeek;
      uint32_t currentKeyGPSToW;
      long long epoch = thingstreamEpochToGPSEpoch(settings.pointPerfectCurrentKeyStart, settings.pointPerfectCurrentKeyDuration);
      unixEpochToWeekToW(epoch, &currentKeyGPSWeek, &currentKeyGPSToW);

      uint16_t nextKeyGPSWeek;
      uint32_t nextKeyGPSToW;
      epoch = thingstreamEpochToGPSEpoch(settings.pointPerfectNextKeyStart, settings.pointPerfectNextKeyDuration);
      unixEpochToWeekToW(epoch, &nextKeyGPSWeek, &nextKeyGPSToW);

      i2cGNSS.setVal8(UBLOX_CFG_SPARTN_USE_SOURCE, 1); // use LBAND PMP message

      i2cGNSS.setVal8(UBLOX_CFG_MSGOUT_UBX_RXM_COR_I2C, 1); // Enable UBX-RXM-COR messages on I2C

      i2cGNSS.setDGNSSConfiguration(SFE_UBLOX_DGNSS_MODE_FIXED); // Set the differential mode - ambiguities are fixed whenever possible

      bool response = i2cGNSS.setDynamicSPARTNKeys(
                        currentKeyLengthBytes, currentKeyGPSWeek, currentKeyGPSToW, settings.pointPerfectCurrentKey,
                        nextKeyLengthBytes, nextKeyGPSWeek, nextKeyGPSToW, settings.pointPerfectNextKey);

      if (response == false)
        Serial.println("setDynamicSPARTNKeys failed");
      else
      {
        log_d("PointPerfect keys applied");
        online.lbandCorrections = true;
      }
    }
    else
    {
      log_d("No PointPerfect keys available");
    }
  }
}

// Check if the PMP data is being decrypted successfully
void checkRXMCOR(UBX_RXM_COR_data_t *ubxDataStruct)
{
  log_d("L-Band Eb/N0[dB] (>9 is good): %0.2f", ubxDataStruct->ebno * pow(2, -3));
  lBandEBNO = ubxDataStruct->ebno * pow(2, -3);

  if (ubxDataStruct->statusInfo.bits.msgDecrypted == 2) //Successfully decrypted
  {
    lbandCorrectionsReceived = true;
    lastLBandDecryption = millis();
  }
  else
  {
    log_d("PMP decryption failed");
  }
}

#endif  //COMPILE_L_BAND

//----------------------------------------
// Global L-Band Routines
//----------------------------------------

//Check if NEO-D9S is connected. Configure if available.
void beginLBand()
{
#ifdef COMPILE_L_BAND
  if (i2cLBand.begin(Wire, 0x43) == false) //Connect to the u-blox NEO-D9S using Wire port. The D9S default I2C address is 0x43 (not 0x42)
  {
    log_d("L-Band not detected");
    return;
  }

  //Check the firmware version of the NEO-D9S. Based on Example21_ModuleInfo.
  if (i2cLBand.getModuleInfo(1100) == true) // Try to get the module info
  {
    //i2cLBand.minfo.extension[1] looks like 'FWVER=HPG 1.12'
    strcpy(neoFirmwareVersion, i2cLBand.minfo.extension[1]);

    //Remove 'FWVER='. It's extraneous and = causes settings file parsing issues
    char *ptr = strstr(neoFirmwareVersion, "FWVER=");
    if (ptr != NULL)
      strcpy(neoFirmwareVersion, ptr + strlen("FWVER="));

    printNEOInfo(); //Print module firmware version
  }

  if (online.gnss == true)
  {
    i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
    i2cGNSS.checkCallbacks(); //Process any callbacks: ie, eventTriggerReceived
  }

  //If we have a fix, check which frequency to use
  if (fixType == 2 || fixType == 3 || fixType == 4 || fixType == 5) //2D, 3D, 3D+DR, or Time
  {
    if ( (longitude > -125 && longitude < -67) && (latitude > -90 && latitude < 90))
    {
      log_d("Setting L-Band to US");
      settings.LBandFreq = 1556290000; //We are in US band
    }
    else if ( (longitude > -25 && longitude < 70) && (latitude > -90 && latitude < 90))
    {
      log_d("Setting L-Band to EU");
      settings.LBandFreq = 1545260000; //We are in EU band
    }
    else
    {
      Serial.println("Error: Unknown band area. Defaulting to US band.");
      settings.LBandFreq = 1556290000; //Default to US
    }
    recordSystemSettings();
  }
  else
    log_d("No fix available for L-Band frequency determination");

  bool response = true;
  response &= i2cLBand.setVal32(UBLOX_CFG_PMP_CENTER_FREQUENCY,   settings.LBandFreq); // Default 1539812500 Hz
  response &= i2cLBand.setVal16(UBLOX_CFG_PMP_SEARCH_WINDOW,      2200);        // Default 2200 Hz
  response &= i2cLBand.setVal8(UBLOX_CFG_PMP_USE_SERVICE_ID,      0);           // Default 1
  response &= i2cLBand.setVal16(UBLOX_CFG_PMP_SERVICE_ID,         21845);       // Default 50821
  response &= i2cLBand.setVal16(UBLOX_CFG_PMP_DATA_RATE,          2400);        // Default 2400 bps
  response &= i2cLBand.setVal8(UBLOX_CFG_PMP_USE_DESCRAMBLER,     1);           // Default 1
  response &= i2cLBand.setVal16(UBLOX_CFG_PMP_DESCRAMBLER_INIT,   26969);       // Default 23560
  response &= i2cLBand.setVal8(UBLOX_CFG_PMP_USE_PRESCRAMBLING,   0);           // Default 0
  response &= i2cLBand.setVal64(UBLOX_CFG_PMP_UNIQUE_WORD,        16238547128276412563ull);
  response &= i2cLBand.setVal(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C,   1); // Ensure UBX-RXM-PMP is enabled on the I2C port
  response &= i2cLBand.setVal(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART1, 1); // Output UBX-RXM-PMP on UART1
  response &= i2cLBand.setVal(UBLOX_CFG_UART2OUTPROT_UBX, 1);         // Enable UBX output on UART2
  response &= i2cLBand.setVal(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2, 1); // Output UBX-RXM-PMP on UART2
  response &= i2cLBand.setVal32(UBLOX_CFG_UART1_BAUDRATE,         38400); // match baudrate with ZED default
  response &= i2cLBand.setVal32(UBLOX_CFG_UART2_BAUDRATE,         38400); // match baudrate with ZED default

  if (response == false)
    Serial.println("L-Band failed to configure");

  i2cLBand.softwareResetGNSSOnly(); // Do a restart

  i2cLBand.setRXMPMPmessageCallbackPtr(&pushRXMPMP); // Call pushRXMPMP when new PMP data arrives. Push it to the GNSS

  i2cGNSS.setRXMCORcallbackPtr(&checkRXMCOR); // Check if the PMP data is being decrypted successfully

  log_d("L-Band online");

  online.lband = true;
#endif //COMPILE_L_BAND
}

//Set 'home' WiFi credentials
//Provision device on ThingStream
//Download keys
void menuPointPerfect()
{
#ifdef COMPILE_L_BAND
  while (1)
  {
    Serial.println();
    Serial.println("Menu: PointPerfect Corrections");

    char hardwareID[13];
    sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", lbandMACAddress[0], lbandMACAddress[1], lbandMACAddress[2], lbandMACAddress[3], lbandMACAddress[4], lbandMACAddress[5]); //Get ready for JSON
    Serial.printf("Device ID: %s\r\n", hardwareID);

    Serial.print("Days until keys expire: ");
    if (strlen(settings.pointPerfectCurrentKey) > 0)
    {
      uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
      Serial.println(daysRemaining);
    }
    else
      Serial.println("No keys");

    Serial.print("1) Use PointPerfect Corrections: ");
    if (settings.enablePointPerfectCorrections == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("2) Set Home WiFi SSID: ");
    Serial.println(settings.home_wifiSSID);

    Serial.print("3) Set Home WiFi PW: ");
    Serial.println(settings.home_wifiPW);

    Serial.print("4) Toggle Auto Key Renewal: ");
    if (settings.autoKeyRenewal == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    if (strlen(settings.pointPerfectCurrentKey) == 0 || strlen(settings.pointPerfectLBandTopic) == 0)
      Serial.println("5) Provision Device");
    else
      Serial.println("5) Update Keys");

    Serial.println("k) Manual Key Entry");

    Serial.println("x) Exit");

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enablePointPerfectCorrections ^= 1;
    }
    else if (incoming == '2')
    {
      Serial.print("Enter Home WiFi SSID: ");
      readLine(settings.home_wifiSSID, sizeof(settings.home_wifiSSID), menuTimeout);
    }
    else if (incoming == '3')
    {
      Serial.printf("Enter password for Home WiFi network %s: ", settings.home_wifiSSID);
      readLine(settings.home_wifiPW, sizeof(settings.home_wifiPW), menuTimeout);
    }
    else if (incoming == '4')
    {
      settings.autoKeyRenewal ^= 1;
    }
    else if (incoming == '5')
    {
#ifdef COMPILE_WIFI
      if (strlen(settings.home_wifiSSID) == 0)
      {
        Serial.println("Error: Please enter SSID before getting keys");
      }
      else
      {
        wifiStart(settings.home_wifiSSID, settings.home_wifiPW);

        unsigned long startTime = millis();
        while (wifiGetStatus() != WL_CONNECTED)
        {
          delay(500);
          Serial.print(".");
          if (millis() - startTime > 15000)
          {
            Serial.println("Error: No WiFi available");
            break;
          }
        }

        if (wifiGetStatus() == WL_CONNECTED)
        {

          Serial.println();
          Serial.print("WiFi connected: ");
          Serial.println(wifiGetIpAddress());

          //Check if we have certificates
          char fileName[80];
          sprintf(fileName, "/%s_%s_%d.txt", platformFilePrefix, "certificate", profileNumber);
          if (LittleFS.exists(fileName) == false)
          {
            pointperfectProvisionDevice(); //Connect to ThingStream API and get keys
          }
          else if (strlen(settings.pointPerfectCurrentKey) == 0 || strlen(settings.pointPerfectLBandTopic) == 0)
          {
            pointperfectProvisionDevice(); //Connect to ThingStream API and get keys
          }
          else
            pointperfectUpdateKeys();

        }

        wifiStop();
      } //End strlen SSID check
#endif
    }
    else if (incoming == '6')
    {
      LittleFS.format();
      log_d("Formatted");
    }
    else if (incoming == 'k')
    {
      menuPointPerfectKeys();
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  if (strlen(settings.pointPerfectClientID) > 0)
  {
    pointperfectApplyKeys();
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
#endif  //COMPILE_L_BAND
}

//Process any new L-Band from I2C
//If a certain amount of time has elapsed between last decryption, turn off L-Band icon
void updateLBand()
{
#ifdef COMPILE_L_BAND
  if (online.lbandCorrections == true)
  {
    i2cLBand.checkUblox(); // Check for the arrival of new PMP data and process it.
    i2cLBand.checkCallbacks(); // Check if any L-Band callbacks are waiting to be processed.

    if (lbandCorrectionsReceived == true && millis() - lastLBandDecryption > 5000)
      lbandCorrectionsReceived = false;
  }
#endif  //COMPILE_L_BAND
}
