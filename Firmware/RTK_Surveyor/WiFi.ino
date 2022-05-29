//----------------------------------------
// Locals
//----------------------------------------

#ifdef  COMPILE_WIFI

static bool ntripClientAttempted = false; //Goes true once we attempt WiFi. Allows graceful failure.

#endif  //COMPILE_WIFI

//----------------------------------------
// NTRIP Client Routines - compiled out
//----------------------------------------

#ifdef  COMPILE_WIFI

//Used during Rover+WiFi NTRIP Client mode to provide caster with GGA sentence every 10 seconds
void ntripClientPushGPGGA(NMEA_GGA_data_t *nmeaData)
{
  //Provide the caster with our current position as needed
  if ((ntripClient.connected() == true) && (settings.ntripClient_TransmitGGA == true))
  {
    log_d("Pushing GGA to server: %s", nmeaData->nmea); //nmea is printable (NULL-terminated) and already has \r\n on the end

    ntripClient.print((const char *)nmeaData->nmea); //Push our current GGA sentence to caster
  }
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

//Check for the arrival of any correction data. Push it to the GNSS.
//Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
#ifdef  COMPILE_WIFI
  if (online.ntripClient == true)
  {
    if (ntripClient.connected() == true) // Check that the connection is still open
    {
      uint8_t rtcmData[512 * 4]; //Most incoming data is around 500 bytes but may be larger
      size_t rtcmCount = 0;

      //Collect any available RTCM data
      while (ntripClient.available())
      {
        rtcmData[rtcmCount++] = ntripClient.read();
        if (rtcmCount == sizeof(rtcmData))
          break;
      }

      if (rtcmCount > 0)
      {
        lastReceivedRTCM_ms = millis();

        //Push RTCM to GNSS module over I2C
        i2cGNSS.pushRawData(rtcmData, rtcmCount);

        //log_d("Pushed %d RTCM bytes to ZED", rtcmCount);
      }
    }
    else
    {
      Serial.println(F("NTRIP Client connection dropped"));
      online.ntripClient = false;
    }

    //Timeout if we don't have new data for maxTimeBeforeHangup_ms
    if ((millis() - lastReceivedRTCM_ms) > maxTimeBeforeHangup_ms)
    {
      Serial.println(F("NTRIP Client timeout"));
      ntripClient.stop();
      online.ntripClient = false;
    }
  }
#endif  //COMPILE_WIFI
}

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

bool ntripClientStart()
{
#ifdef  COMPILE_WIFI
  if (settings.enableNtripClient == true && ntripClientAttempted == false)
  {
    //Turn off Bluetooth and turn on WiFi
    stopBluetooth();
    wifiStart(settings.ntripClient_wifiSSID, settings.ntripClient_wifiPW);
    wifiStartTime = millis();

    ntripClientAttempted = true; //Do not allow re-entry into STATE_ROVER_CLIENT_WIFI_STARTED
    return true;
  }
#endif  //COMPILE_WIFI
  return false;
}

//----------------------------------------
// WiFi Routines - compile out
//----------------------------------------

#ifdef  COMPILE_WIFI

byte wifiGetStatus()
{
  return WiFi.status();
}

IPAddress wifiGetIpAddress()
{
  return WiFi.localIP();
}

void wifiStartAP()
{
  //When testing, operate on local WiFi instead of AP
  //#define LOCAL_WIFI_TESTING 1
#ifdef  LOCAL_WIFI_TESTING
  //Connect to local router
#define WIFI_SSID "TRex"
#define WIFI_PASSWORD "parachutes"
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (wifiGetStatus() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
#else   //LOCAL_WIFI_TESTING
  //Start in AP mode
  WiFi.mode(WIFI_AP);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  if (WiFi.softAP("RTK Config") == false) //Must be short enough to fit OLED Width
  {
    Serial.println(F("AP failed to start"));
    return;
  }
  Serial.print(F("AP Started with IP: "));
  Serial.println(WiFi.softAPIP());
#endif  //LOCAL_WIFI_TESTING
}

#endif  //COMPILE_WIFI

//----------------------------------------
// Global WiFi Routines
//----------------------------------------

void wifiStart(char* ssid, char* pw)
{
#ifdef  COMPILE_WIFI
  if (wifiState == WIFI_OFF)
  {
    Serial.printf("Connecting to WiFi: %s", ssid);
    WiFi.begin(ssid, pw);

    wifiState = WIFI_NOTCONNECTED;
    reportHeapNow();
  }
#endif  //COMPILE_WIFI
}

//Stop WiFi and release all resources
//See WiFiBluetoothSwitch sketch for more info
void wifiStop()
{
#ifdef  COMPILE_WIFI
  if (wifiState == WIFI_NOTCONNECTED || wifiState == WIFI_CONNECTED)
  {
    ntripServer.stop();
    WiFi.mode(WIFI_OFF);

    log_d("WiFi Stopped");
    wifiState = WIFI_OFF;
    reportHeapNow();
  }
#endif  //COMPILE_WIFI
}
