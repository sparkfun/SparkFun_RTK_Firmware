/*
  Web server test program
*/

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SdCardServer.h>
#include <SPI.h>
#include <pgmspace.h>
#include <WiFi.h>

#include "SdFat.h" //http://librarymanager/All#sdfat_exfat by Bill Greiman. Currently uses v2.1.1

#define ASCII_LF        0x0a
#define ASCII_CR        0x0d

int keyIndex = 0;
char password[1024];          // WiFi network password
char ssid[1024];              // WiFi network name

const int pin_microSD_CS = 25;

typedef struct struct_settings {
  bool enableSD = true;
  uint16_t spiFrequency = 16; //By default, use 16MHz SPI
} Settings;

typedef struct struct_online {
  bool microSD = false;
} Online;

const TickType_t fatSemaphore_shortWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_longWait_ms = 200 / portTICK_PERIOD_MS;
Online online;
SdFat sd;
int sdCardMounted;
AsyncWebServer server(80);
Settings settings;
int status = WL_IDLE_STATUS;  // the Wifi radio's status
int wifiBeginCalled;
int wifiConnected;
SemaphoreHandle_t xFATSemaphore;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void beginSD()
{
  pinMode(pin_microSD_CS, OUTPUT);
  digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Do a quick test to see if a card is present
    int tries = 0;
    int maxTries = 5;
    while (tries < maxTries)
    {
      if (sdPresent() == true) break;
      log_d("SD present failed. Trying again %d out of %d", tries + 1, maxTries);

      //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
      //Max current is 200mA average across 1s, peak 300mA
      delay(10);
      tries++;
    }
    if (tries == maxTries) return;

    //If an SD card is present, allow SdFat to take over
    log_d("SD card detected");

    if (settings.spiFrequency > 16)
    {
      Serial.println("Error: SPI Frequency out of range. Default to 16MHz");
      settings.spiFrequency = 16;
    }

    if (sd.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == false)
    {
      tries = 0;
      maxTries = 1;
      for ( ; tries < maxTries ; tries++)
      {
        log_d("SD init failed. Trying again %d out of %d", tries + 1, maxTries);

        delay(250); //Give SD more time to power up, then try again
        if (sd.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == true) break;
      }

      if (tries == maxTries)
      {
        Serial.println(F("SD init failed. Is card present? Formatted?"));
        digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      Serial.println(F("SD change directory failed"));
      online.microSD = false;
      return;
    }

    //Setup FAT file access semaphore
    if (xFATSemaphore == NULL)
    {
      xFATSemaphore = xSemaphoreCreateMutex();
      if (xFATSemaphore != NULL)
        xSemaphoreGive(xFATSemaphore);  //Make the file system available for use
    }

    if (createTestFile() == false)
    {
      Serial.println(F("Failed to create test file. Format SD card with 'SD Card Formatter'."));
      displaySDFail(5000);
      online.microSD = false;
      return;
    }

    online.microSD = true;

    Serial.println(F("microSD online"));
  }
  else
  {
    online.microSD = false;
  }
}

//Create a test file in file structure to make sure we can
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool createTestFile()
{
  SdFile testFile;
  char testFileName[40] = "testfile.txt";

  if (xFATSemaphore == NULL)
  {
    log_d("xFATSemaphore is Null");
    return (false);
  }

  //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
  if (xSemaphoreTake(xFATSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
  {
    if (testFile.open(testFileName, O_CREAT | O_APPEND | O_WRITE) == true)
    {
      testFile.close();

      if (sd.exists(testFileName))
        sd.remove(testFileName);
      xSemaphoreGive(xFATSemaphore);
      return (true);
    }
    xSemaphoreGive(xFATSemaphore);
  }

  return (false);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displayIpAddress(IPAddress ip) {

    // Display the IP address
    Serial.println(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displaySDFail(uint16_t displayTime)
{
  Serial.println ("SD card failed to initialize!");
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
int mountSdCard(void) {
    // Mount the SD card
    if (!sdCardMounted) {
        beginSD();
        if (online.microSD) {
            sdCardMounted = true;
            sd.volumeBegin();
        }
    }
    return sdCardMounted;
}

SdCardServer sdCardServer(&sd, mountSdCard, "/sd/", "SD Card Files");

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void setup() {
    char data;
    int length;

    // Initialize serial and wait for port to open:
    Serial.begin(115200);
    while (!Serial);  // Wait for native USB serial port to connect
    Serial.println("\n");

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

    // The SD card needs to be mounted
    sdCardMounted = 0;
    online.microSD = 0;
    if (mountSdCard())
        Serial.println();

    // Wait for WiFi connection
    wifiBeginCalled = false;
    wifiConnected = false;
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

            // index.html
            sdCardServer.sdCardWebSite(&server);

            //  All other pages
            sdCardServer.onNotFound(&server);

            // Start server
            server.begin();
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
