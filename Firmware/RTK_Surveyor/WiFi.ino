//----------------------------------------
// Constants
//----------------------------------------

//If we cannot connect to local wifi for NTRIP client, give up/go to Rover after 8 seconds
static const int WIFI_CONNECTION_TIMEOUT = 8 * 1000;  //Milliseconds

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#ifdef  COMPILE_WIFI

//WiFi Timer usage:
//  * Measure connection time to access point
static unsigned long wifiTimer = 0;

//----------------------------------------
// WiFi Routines - compiled out
//----------------------------------------

IPAddress wifiGetIpAddress()
{
  return WiFi.localIP();
}

byte wifiGetStatus()
{
  return WiFi.status();
}

//----------------------------------------
// WiFi Config Support Routines - compiled out
//----------------------------------------

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

//Determine if the WiFi connection has timed out
bool wifiConnectionTimeout()
{
#ifdef  COMPILE_WIFI
  if ((millis() - wifiTimer) <= WIFI_CONNECTION_TIMEOUT)
    return false;
#endif  //COMPILE_WIFI
  return true;
}

void wifiStart(char* ssid, char* pw)
{
#ifdef  COMPILE_WIFI
  if (wifiState == WIFI_OFF)
    //Turn off Bluetooth
    stopBluetooth();

  if (wifiState == WIFI_OFF)
  {
    Serial.printf("WiFi connecting to %s\r\n", ssid);
    WiFi.begin(ssid, pw);
    wifiTimer = millis();

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
  stopWebServer();
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
