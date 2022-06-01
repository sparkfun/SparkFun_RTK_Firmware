/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
WiFi Status Values:
    WL_CONNECTED: assigned when connected to a WiFi network
    WL_CONNECTION_LOST: assigned when the connection is lost
    WL_CONNECT_FAILED: assigned when the connection fails for all the attempts
    WL_DISCONNECTED: assigned when disconnected from a network
    WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and
                    remains active until the number of attempts expires (resulting in
                    WL_CONNECT_FAILED) or a connection is established (resulting in
                    WL_CONNECTED)
    WL_NO_SHIELD: assigned when no WiFi shield is present
    WL_NO_SSID_AVAIL: assigned when no SSID are available
    WL_SCAN_COMPLETED: assigned when the scan networks is completed

WiFi Station States:

                                  WIFI_OFF (Using Bluetooth)
                                    |   ^
                           Use WiFi |   | Use Bluetooth
                                    |   | WL_CONNECT_FAILED (Bad password)
                                    |   | WL_NO_SSID_AVAIL (Out of range)
                                    v   |
                                   WIFI_ON
                                    |   ^
                         WiFi.begin |   | WiFi.stop
                                    |   | WL_CONNECTION_LOST
                                    |   | WL_CONNECT_FAILED (Bad password)
                                    |   | WL_DISCONNECTED
                                    |   | WL_NO_SSID_AVAIL (Out of range)
                                    v   |
                                  WIFI_NOT_CONNECTED
                                    |   ^
                       WL_CONNECTED |   | WiFi.disconnect
                                    |   | WL_CONNECTION_LOST
                                    |   | WL_CONNECT_FAILED (Bad password)
                                    |   | WL_DISCONNECTED
                                    |   | WL_NO_SSID_AVAIL (Out of range)
                                    v   |
                                  WIFI_CONNECTED

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Constants
//----------------------------------------

//If we cannot connect to local wifi, give up/go to Rover
static const int WIFI_CONNECTION_TIMEOUT = 8 * 1000;  //Milliseconds

//Interval to use when displaying the IP address
static const int WIFI_IP_ADDRESS_DISPLAY_INTERVAL = 12 * 1000;  //Milliseconds

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#ifdef  COMPILE_WIFI

//WiFi Timer usage:
//  * Measure connection time to access point
//  * Measure interval to display IP address
static unsigned long wifiTimer = 0;

//----------------------------------------
// WiFi Routines - compiled out
//----------------------------------------

void wifiDisplayIpAddress()
{
  Serial.print("Wi-Fi IP address: ");
  Serial.println(WiFi.localIP());
  wifiTimer = millis();
}

IPAddress wifiGetIpAddress()
{
  return WiFi.localIP();
}

byte wifiGetStatus()
{
  return WiFi.status();
}

bool wifiIsConnected()
{
  bool isConnected;

  isConnected = (wifiGetStatus() == WL_CONNECTED);
  if (isConnected)
  {
    wifiState = WIFI_CONNECTED;
    wifiDisplayIpAddress();
  }
  return isConnected;
}

void wifiPeriodicallyDisplayIpAddress()
{
  if (wifiGetStatus() == WL_CONNECTED)
    if ((millis() - wifiTimer) >= wifiTimer)
      wifiDisplayIpAddress();
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
  Serial.print("Wi-Fi connecting to");
  while (wifiGetStatus() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Wi-Fi connected with IP: ");
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
    Serial.println(F("Wi-Fi AP failed to start"));
    return;
  }
  Serial.print(F("Wi-Fi AP Started with IP: "));
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
  Serial.println("Wi-Fi connection timeout!");
#endif  //COMPILE_WIFI
  return true;
}

void wifiStart(char* ssid, char* pw)
{
#ifdef  COMPILE_WIFI
  if (wifiState == WIFI_OFF)
    //Turn off Bluetooth
    stopBluetooth();

  if ((wifiState == WIFI_OFF) || (wifiState == WIFI_ON))
  {
    Serial.printf("Wi-Fi connecting to %s\r\n", ssid);
    WiFi.begin(ssid, pw);
    wifiTimer = millis();
    wifiState = WIFI_NOTCONNECTED;

    //Display the heap state
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
    wifiState = WIFI_OFF;
    Serial.println(F("Wi-Fi Stopped"));

    //Display the heap state
    reportHeapNow();
  }
#endif  //COMPILE_WIFI
}
