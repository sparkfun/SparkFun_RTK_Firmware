/*
  Verify connection to the specified WiFi network
*/

#include <SPI.h>
#include <WiFi.h>

#define ASCII_LF        0x0a
#define ASCII_CR        0x0d

int keyIndex = 0;
char password[1024];          // WiFi network password
char ssid[1024];              // WiFi network name
int status = WL_IDLE_STATUS;  // the WiFi radio's status
int wifiBeginCalled;
int wifiConnected;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayIpAddress(IPAddress ip) {

  // Display the IP address
  Serial.println(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayWiFiGatewayIp() {

  // Display the subnet mask
  Serial.print ("Gateway: ");
  IPAddress ip = WiFi.gatewayIP();
  displayIpAddress(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayWiFiIpAddress() {

  // Display the IP address
  Serial.print ("IP Address: ");
  IPAddress ip = WiFi.localIP();
  displayIpAddress(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayWiFiMacAddress() {
  // Display the MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayWiFiNetwork() {

  // Display the SSID
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Display the receive signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.println(rssi);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayWiFiSubnetMask() {

  // Display the subnet mask
  Serial.print ("Subnet Mask: ");
  IPAddress ip = WiFi.subnetMask();
  displayIpAddress(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void setup() {
  char data;
  int length;

  // Initialize serial and wait for port to open:
   Serial.begin(115200);
  while (!Serial);  // Wait for native USB serial port to connect

  // Wait for WiFi connection
  wifiBeginCalled = false;
  wifiConnected = false;
  Serial.println();

  // Read the WiFi network name (SSID)
  length = 0;
  do {
    Serial.println("Please enter the WiFi network name (SSID):");
    do {
      while (!Serial.available());
      data = Serial.read();
      if ((data == ASCII_LF) || (data == ASCII_CR))
        break;
      ssid[length++] = data;
    } while (1);
    ssid[length] = 0;
  } while (!length);
  Serial.printf("SSID: %s\n", ssid);
  Serial.println();

  // Read the WiFi network password
  length = 0;
  do {
    Serial.println("Please enter the WiFi network password:");
    do {
      while (!Serial.available());
      data = Serial.read();
      if ((data == ASCII_LF) || (data == ASCII_CR))
        break;
      password[length++] = data;
    } while (1);
    password[length] = 0;
  } while (!length);
  Serial.printf("Password: %s\n", password);
  Serial.println();
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void loop() {

  // Determine the WiFi status
  status = WiFi.status();
  switch (status) {
  default:
    Serial.printf("Unknown WiFi status: %d\n", status);
    delay (100);
    break;

  case WL_DISCONNECTED:
  case WL_IDLE_STATUS:
  case WL_NO_SHIELD:
  case WL_SCAN_COMPLETED:
    break;

  case WL_NO_SSID_AVAIL:
    Serial.println("Please set SSID and pass values!\n");
    while (1);

  case WL_CONNECTED:
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.println("WiFi Connected");

      // Display the WiFi info
      displayWiFiNetwork();
      displayWiFiIpAddress();
      displayWiFiSubnetMask();
      displayWiFiGatewayIp();
    }
    break;

  case WL_CONNECTION_LOST:
    Serial.println("WiFi connection lost");
    WiFi.disconnect();
    wifiBeginCalled = false;
    wifiConnected = false;
    break;

  case WL_CONNECT_FAILED:
    wifiBeginCalled = false;
    wifiConnected = false;
    break;;
  }

  // Attempt to connect to Wifi network
  if (!wifiBeginCalled) {
    wifiBeginCalled = true;
    WiFi.begin(ssid, password);
    Serial.println("Waiting for WiFi connection...");
  }
}
