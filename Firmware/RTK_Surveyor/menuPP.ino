//Set 'home' WiFi credentials
//Provision device on ThingStream
//Download keys
void menuPointPerfect()
{
  int menuTimeoutExtended = 30; //Increase time needed for complex data entry (mount point ID, caster credentials, etc).

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: PointPerfect Corrections"));

    char hardwareID[13];
    sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", unitMACAddress[0], unitMACAddress[1], unitMACAddress[2], unitMACAddress[3], unitMACAddress[4], unitMACAddress[5]); //Get ready for JSON
    Serial.printf("Device ID: %s\n\r", hardwareID);

    Serial.print("Days until keys expire: ");
    if (strlen(settings.pointPerfectCurrentKey) > 0)
    {
      uint8_t daysRemaining = daysFromEpoch(settings.pointPerfectNextKeyStart + settings.pointPerfectNextKeyDuration + 1);
      Serial.println(daysRemaining);
    }
    else
      Serial.println("No keys");

    Serial.print(F("1) Use PointPerfect Corrections: "));
    if (settings.enablePointPerfectCorrections == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Set Home WiFi SSID: "));
    Serial.println(settings.home_wifiSSID);

    Serial.print(F("3) Set Home WiFi PW: "));
    Serial.println(settings.home_wifiPW);

    Serial.print(F("4) Toggle Auto Key Renewal: "));
    if (settings.autoKeyRenewal == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (strlen(settings.pointPerfectCurrentKey) == 0 || strlen(settings.pointPerfectLBandTopic) == 0)
      Serial.println(F("5) Provision Device"));
    else
      Serial.println(F("5) Update Keys"));

    Serial.println(F("k) Manual Key Entry"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enablePointPerfectCorrections ^= 1;
    }
    else if (incoming == '2')
    {
      Serial.print(F("Enter Home WiFi SSID: "));
      readLine(settings.home_wifiSSID, sizeof(settings.home_wifiSSID), menuTimeoutExtended);
    }
    else if (incoming == '3')
    {
      Serial.printf("Enter password for Home WiFi network %s: ", settings.home_wifiSSID);
      readLine(settings.home_wifiPW, sizeof(settings.home_wifiPW), menuTimeoutExtended);
    }
    else if (incoming == '4')
    {
      settings.autoKeyRenewal ^= 1;
    }
    else if (incoming == '5')
    {
#ifdef COMPILE_WIFI
      wifiStart(settings.home_wifiSSID, settings.home_wifiPW);

      unsigned long startTime = millis();
      while (wifiGetStatus() != WL_CONNECTED)
      {
        delay(500);
        Serial.print(".");
        if (millis() - startTime > 8000) break; //Give up after 8 seconds
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
          provisionDevice(); //Connect to ThingStream API and get keys
        }
        else if (strlen(settings.pointPerfectCurrentKey) == 0 || strlen(settings.pointPerfectLBandTopic) == 0)
        {
          provisionDevice(); //Connect to ThingStream API and get keys
        }
        else
          updatePointPerfectKeys();
      }

      startBluetooth();
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
    applyLBandKeys();
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

void menuPointPerfectKeys()
{
  int menuTimeoutExtended = 30; //Increase time needed for complex data entry (mount point ID, caster credentials, etc).

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: PointPerfect Keys"));

    Serial.print("1) Set ThingStream Device Profile Token: ");
    if (strlen(settings.pointPerfectDeviceProfileToken) > 0)
      Serial.println(settings.pointPerfectDeviceProfileToken);
    else
      Serial.println(F("Use SparkFun Token"));

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

      Serial.printf("%02d/%02d/%d\n\r", expDay, expMonth, expYear);
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

      Serial.printf("%02d/%02d/%d\n\r", expDay, expMonth, expYear);
    }
    else
      Serial.println("N/A");

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
    {
      Serial.print(F("Enter Device Profile Token: "));
      readLine(settings.pointPerfectDeviceProfileToken, sizeof(settings.pointPerfectDeviceProfileToken), menuTimeoutExtended);
    }
    else if (incoming == 2)
    {
      Serial.print(F("Enter Current Key: "));
      readLine(settings.pointPerfectCurrentKey, sizeof(settings.pointPerfectCurrentKey), menuTimeoutExtended);
    }
    else if (incoming == 3)
    {
      while (Serial.available()) Serial.read();

      Serial.println(F("Enter Current Key Expiration Date: "));
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

        Serial.printf("  settings.pointPerfectNextKeyStart: %lld\n\r", settings.pointPerfectNextKeyStart);
        Serial.printf("  settings.pointPerfectNextKeyDuration: %lld\n\r", settings.pointPerfectNextKeyDuration);
      }
    }
    else if (incoming == 4)
    {
      Serial.print(F("Enter Next Key: "));
      readLine(settings.pointPerfectNextKey, sizeof(settings.pointPerfectNextKey), menuTimeoutExtended);
    }
    else if (incoming == 5)
    {
      while (Serial.available()) Serial.read();

      Serial.println(F("Enter Next Key Expiration Date: "));
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
bool provisionDevice()
{
#ifdef COMPILE_WIFI

  WiFiClientSecure client;
  client.setCACert(AWS_PUBLIC_CERT);

  char hardwareID[13];
  sprintf(hardwareID, "%02X%02X%02X%02X%02X%02X", unitMACAddress[0], unitMACAddress[1], unitMACAddress[2], unitMACAddress[3], unitMACAddress[4], unitMACAddress[5]); //Get ready for JSON

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
    Serial.printf("Using custom token: %s\n\r", tokenString);
  }

  pointPerfectAPIPost["token"] = tokenString;
  pointPerfectAPIPost["givenName"] = givenName;
  pointPerfectAPIPost["hardwareId"] = hardwareID;
  //pointPerfectAPIPost["tags"] = "mac";

  String json;
  serializeJson(pointPerfectAPIPost, json);

  Serial.printf("Connecting to: %s\n\r", pointPerfectAPI);

  HTTPClient http;
  http.begin(client, pointPerfectAPI);
  http.addHeader(F("Content-Type"), F("application/json"));

  int httpResponseCode = http.POST(json);

  String response = http.getString();

  http.end();

  if (httpResponseCode != 200)
  {
    Serial.printf("HTTP response error %d: ", httpResponseCode);
    Serial.println(response);
    return (false);
  }
  else
  {
    //Device is now active with ThingStream
    //Pull pertinent values from response
    DynamicJsonDocument jsonZtp(4096);
    DeserializationError error = deserializeJson(jsonZtp, response);
    if (DeserializationError::Ok != error)
    {
      Serial.println("JSON error");
      return (false);
    }
    else
    {
      char tempHolder[2000];
      strcpy(tempHolder, (const char*)jsonZtp["certificate"]);
      //      Serial.printf("len of PrivateCert: %d\n\r", strlen(tempHolder));
      //      Serial.printf("privateCert: %s\n\r", tempHolder);
      recordFile("certificate", tempHolder, strlen(tempHolder));

      strcpy(tempHolder, (const char*)jsonZtp["privateKey"]);
      //      Serial.printf("len of privateKey: %d\n\r", strlen(tempHolder));
      //      Serial.printf("privateKey: %s\n\r", tempHolder);
      recordFile("privateKey", tempHolder, strlen(tempHolder));

      strcpy(settings.pointPerfectClientID, (const char*)jsonZtp["clientId"]);
      strcpy(settings.pointPerfectBrokerHost, (const char*)jsonZtp["brokerHost"]);
      strcpy(settings.pointPerfectLBandTopic, (const char*)jsonZtp["subscriptions"][0]["path"]);

      strcpy(settings.pointPerfectNextKey, (const char*)jsonZtp["dynamickeys"]["next"]["value"]);
      settings.pointPerfectNextKeyDuration = jsonZtp["dynamickeys"]["next"]["duration"];
      settings.pointPerfectNextKeyStart = jsonZtp["dynamickeys"]["next"]["start"];

      strcpy(settings.pointPerfectCurrentKey, (const char*)jsonZtp["dynamickeys"]["current"]["value"]);
      settings.pointPerfectCurrentKeyDuration = jsonZtp["dynamickeys"]["current"]["duration"];
      settings.pointPerfectCurrentKeyStart = jsonZtp["dynamickeys"]["current"]["start"];
    }
  } //HTTP Response was 200

  Serial.println("Device successfully provisioned. Keys obtained.");

  recordSystemSettings();

  return (true);
#else
  return (false);
#endif
}

//Subscribe to MQTT channel, grab keys, then stop
bool updatePointPerfectKeys()
{
#ifdef COMPILE_WIFI

  WiFiClientSecure secureClient;

  loadFile("certificate", certificateContents);
  secureClient.setCertificate(certificateContents);

  loadFile("privateKey", keyContents);
  secureClient.setPrivateKey(keyContents);

  secureClient.setCACert(AWS_PUBLIC_CERT);

  PubSubClient mqttClient(secureClient);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setServer(settings.pointPerfectBrokerHost, 8883);

  log_d("Connecting to MQTT broker: %s", settings.pointPerfectBrokerHost);

  // Loop until we're reconnected
  int maxTries = 2;
  int tries = 0;
  while (mqttClient.connected() == false)
  {
    Serial.print("MQTT connecting...");

    if (mqttClient.connect(settings.pointPerfectClientID))
    {
      Serial.println("connected");
      //mqttClient.subscribe(settings.pointPerfectLBandTopic); //The /pp/key/Lb channel fails to respond with keys
      mqttClient.subscribe("/pp/ubx/0236/Lb"); //Alternate channel for L-Band keys
    }
    else
    {
      if (tries++ == maxTries)
      {
        log_d("MQTT failed to connect");
        return (false);
      }

      log_d("failed, status code: %d try again in 1 second", mqttClient.state());
      delay(1000);
    }
  }

  Serial.print("Waiting for keys");

  mqttMessageReceived = false;

  //Wait for callback
  startTime = millis();
  while (mqttMessageReceived == false)
  {
    mqttClient.loop();
    if (mqttClient.connected() == false)
    {
      log_d("Client disconnected");
      return (false);
    }

    delay(100);
    Serial.print(".");
    if (millis() - startTime > 8000)
    {
      Serial.println();
      log_d("Channel failed to respond");
      return (false);
    }
  }

  Serial.println();
  Serial.println("Keys successfully update");
  return (true);
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

    //    Serial.printf("pointPerfectCurrentKeyStart: %lld\n\r", settings.pointPerfectCurrentKeyStart);
    //    Serial.printf("pointPerfectCurrentKeyDuration: %lld\n\r", settings.pointPerfectCurrentKeyDuration);
    //    Serial.printf("pointPerfectNextKeyStart: %lld\n\r", settings.pointPerfectNextKeyStart);
    //    Serial.printf("pointPerfectNextKeyDuration: %lld\n\r", settings.pointPerfectNextKeyDuration);
    //
    //    Serial.printf("currentWeek: %d\n\r", currentWeek);
    //    Serial.printf("currentToW: %d\n\r", currentToW);
    //    Serial.print("Current key: ");
    //    Serial.println(settings.pointPerfectCurrentKey);
    //
    //    Serial.printf("nextWeek: %d\n\r", nextWeek);
    //    Serial.printf("nextToW: %d\n\r", nextToW);
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

    //    Serial.printf("pointPerfectCurrentKeyStart: %lld\n\r", settings.pointPerfectCurrentKeyStart);
    //    Serial.printf("pointPerfectCurrentKeyDuration: %lld\n\r", settings.pointPerfectCurrentKeyDuration);
    //    Serial.printf("pointPerfectNextKeyStart: %lld\n\r", settings.pointPerfectNextKeyStart);
    //    Serial.printf("pointPerfectNextKeyDuration: %lld\n\r", settings.pointPerfectNextKeyDuration);
  }

  mqttMessageReceived = true;
}

//Get a date from a user
//Return true if validated
//https://www.includehelp.com/c-programs/validate-date.aspx
bool getDate(uint8_t &dd, uint8_t &mm, uint16_t &yy)
{
  int menuTimeoutExtended = 30; //Increase time needed for complex data entry (mount point ID, caster credentials, etc).

  char temp[10];

  Serial.print(F("Enter Day: "));
  readLine(temp, sizeof(temp), menuTimeoutExtended);
  dd = atoi(temp);

  Serial.print(F("Enter Month: "));
  readLine(temp, sizeof(temp), menuTimeoutExtended);
  mm = atoi(temp);

  Serial.print(F("Enter Year (YYYY): "));
  readLine(temp, sizeof(temp), menuTimeoutExtended);
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
uint8_t daysFromEpoch(long long endEpoch)
{
  endEpoch /= 1000; //Convert PointPerfect ms Epoch to s

  long localEpoch = rtc.getEpoch();

  long delta = endEpoch - localEpoch; //number of s between dates
  delta /= (60 * 60); //hours
  delta /= 24; //days
  return ((uint8_t)delta);
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

  Serial.printf("  KeyStart: %lld\n\r", *settingsKeyStart);
  Serial.printf("  KeyDuration: %lld\n\r", *settingsKeyDuration);

  //Print ToW and Week for debugging
  uint16_t keyGPSWeek;
  uint32_t keyGPSToW;
  long long unixEpoch = thingstreamEpochToGPSEpoch(*settingsKeyStart, *settingsKeyDuration);
  unixEpochToWeekToW(unixEpoch, &keyGPSWeek, &keyGPSToW);
  Serial.printf("  keyGPSWeek: %d\n\r", keyGPSWeek);
  Serial.printf("  keyGPSToW: %d\n\r", keyGPSToW);
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

//Process any new L-Band from I2C
//If a certain amount of time has elapsed between last decryption, turn off L-Band icon
void updateLBand()
{
  if (online.lbandCorrections == true)
  {
    i2cLBand.checkUblox(); // Check for the arrival of new PMP data and process it.
    i2cLBand.checkCallbacks(); // Check if any L-Band callbacks are waiting to be processed.

    if (lbandCorrectionsReceived == true && millis() - lastLBandDecryption > 5000)
      lbandCorrectionsReceived = false;
  }
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
void applyLBandKeys()
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
        Serial.println(F("setDynamicSPARTNKeys failed"));
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
