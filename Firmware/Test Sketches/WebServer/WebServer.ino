/*
  Web server test program
*/

#include <ESPAsyncWebServer.h>  //Get from: https://github.com/me-no-dev/ESPAsyncWebServer
#include <SdCardServer.h>       //Get v1.0 from: https://github.com/LeeLeahy2/SdCardServer
#include <SPI.h>
#include <pgmspace.h>
#include <WiFi.h>

#include "SdFat.h" //http://librarymanager/All#sdfat_exfat by Bill Greiman. Currently uses v2.1.1

#define ASCII_LF        0x0a
#define ASCII_CR        0x0d

int keyIndex = 0;
char password[1024];          // WiFi network password
char ssid[1024];              // WiFi network name

typedef struct struct_online {
  bool microSD = false;
} Online;

Online online;
SdFat sd;
int sdCardMounted;
AsyncWebServer server(80);
int status = WL_IDLE_STATUS;  // the Wifi radio's status
int wifiBeginCalled;
int wifiConnected;

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
            printWiFiNetwork();
            printWiFiIpAddress();
            printWiFiSubnetMask();
            printWiFiGatewayIp();

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
