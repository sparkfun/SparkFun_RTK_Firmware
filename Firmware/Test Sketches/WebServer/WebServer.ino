/*
  Web server test program
*/

//#define NO_LIBRARY              1
//#define INDEX_PAGE              1
//#define NOT_FOUND_PAGE          1
//#define MARK_BOUNDARY           1

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#ifndef NO_LIBRARY
#include <SdCardServer.h>
#endif  // NO_LIBRARY
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
char htmlBuffer[256];
Online online;
SdFat sd;
int sdCardMounted;
float sdCardSizeMB;
SdFile sdFile;
SdFile sdRootDir;
AsyncWebServer server(80);
Settings settings;
int status = WL_IDLE_STATUS;  // the Wifi radio's status
int wifiBeginCalled;
int wifiConnected;
SemaphoreHandle_t xFATSemaphore;

#ifdef NO_LIBRARY
#define MAX_FILE_NAME_SIZE        (256 * 3)

int sdCardEmpty;
int sdListingState;

enum {
  LS_HEADER = 0,
  LS_DISPLAY_FILES,
  LS_TRAILER,
  LS_DONE
} LISTING_STATE;
#endif  // NO_LIBRARY

// html document
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

prog_char htmlHeaderStart[] PROGMEM = "<!DOCTYPE HTML>\n<html lang=\"en\">\n<head>\n  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n  <title>";

prog_char htmlHeaderEndBodyStart[] PROGMEM = R"rawliteral(</title>
</head>
<body>)rawliteral";

prog_char htmlBodyEnd[] PROGMEM = R"rawliteral(</body>
</html>
)rawliteral";

// html anchor
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

prog_char htmlAnchorStart[] PROGMEM = R"rawliteral(<a target=)rawliteral";

prog_char htmlAnchorBlank[] PROGMEM = R"rawliteral(_blank)rawliteral";

prog_char htmlAnchorHref[] PROGMEM = R"rawliteral( href=)rawliteral";

prog_char htmlAnchorCenter[] PROGMEM = R"rawliteral(>)rawliteral";

prog_char htmlAnchorEnd[] PROGMEM = R"rawliteral(</a>)rawliteral";

// html lists
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

prog_char htmlListItemStart[] PROGMEM = R"rawliteral(    <li>)rawliteral";

prog_char htmlListItemEnd[] PROGMEM = R"rawliteral(</li>
)rawliteral";

prog_char htmlUlListStart[] PROGMEM = R"rawliteral(  <ol>
)rawliteral";

prog_char htmlUlListEnd[] PROGMEM = R"rawliteral(  </ol>
)rawliteral";

// sd/
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
const char sdFilesH1[] PROGMEM = "%SZ% SD Card";

prog_char sdHeader[] PROGMEM = R"rawliteral(%H%%H1%%/HB%
  <h1>%H1%</h1>
)rawliteral";

prog_char sdNoCard[] PROGMEM = R"rawliteral(
  <p>MicroSD card socket is empty!</p>
)rawliteral";

prog_char sdNoFiles[] PROGMEM = R"rawliteral(
  <p>No files found!</p>
)rawliteral";

prog_char sdDirectory[] PROGMEM = "/sd/";

// index.html
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
const char htmlTitle[] PROGMEM = "SD Card Server";

#ifdef NO_LIBRARY
const char index_html[] PROGMEM = R"rawliteral(%H%%T%%/HB%
  <h1>%T%</h1>
  <p>%A%%SD%%Q%>%H1%</a></p>
%/B%
)rawliteral";
#else   // NO_LIBRARY
const char index_html[] PROGMEM = R"rawliteral(%H%%T%%/HB%
  <h1>%T%</h1>
  <p>%SD%</p>
%/B%
)rawliteral";
#endif  // NO_LIBRARY

const char no_sd_card_html[] PROGMEM = R"rawliteral(%H%%T%%/HB%
  <h1>%T%</h1>
  <p>SD card not present!</a></p>
%/B%
)rawliteral";

const char not_formatted_html[] PROGMEM = R"rawliteral(%H%%T%%/HB%
  <h1>%T%</h1>
  <p>SD card not properly formatted!</a></p>
%/B%
)rawliteral";

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef NO_LIBRARY
class HtmlPrint : public Print
{
    uint8_t * buf;

public:
    HtmlPrint(void) {
        buf = NULL;
    }

    void setBufferAddress(uint8_t * buffer) {
        buf = buffer;
    }

    size_t write(uint8_t data) {
        *buf++ = data;
        *buf = 0;
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) {
        memcpy (buf, buffer, size);
        buf[size] = 0;
        buf += size;
        return size;
    }
};
#endif  // NO_LIBRARY

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
            csd_t csd;
            sd.card()->readCSD(&csd); //Card Specific Data
            sdCardSizeMB = 0.000512 * sdCardCapacity(&csd);
            Serial.printf ("SD Card Size: %3.0f %s\n\n",
                           sdCardSizeMB < 1000 ? sdCardSizeMB : sdCardSizeMB / 1000,
                           sdCardSizeMB < 1000 ? "MB" : "GB");
            sd.volumeBegin();
            sd.ls(LS_R | LS_DATE | LS_SIZE);
        }
    }
    return sdCardMounted;
}

#ifndef NO_LIBRARY
SdCardServer sdCardServer(&sd, mountSdCard, "/sd/", "SD Card Files");
#endif  // NO_LIBRARY

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifdef  INDEX_PAGE

String processor(const String& var) {
    if (var == "A") {
        strcpy_P(htmlBuffer, htmlAnchorStart);
        strcat(htmlBuffer, "\"");
        strcat_P(htmlBuffer, htmlAnchorBlank);
        strcat(htmlBuffer, "\"");
        strcat_P(htmlBuffer, htmlAnchorHref);
        strcat(htmlBuffer, "\"");
        return String(htmlBuffer);
    }
    if (var == "AC")
        return String(htmlAnchorCenter);
    if (var == "/A")
        return String(htmlAnchorEnd);
    if (var == "/B")
        return String(htmlBodyEnd);
    if (var == "H")
        return String(htmlHeaderStart);
    if (var == "H1")
        return String(sdFilesH1);
    if (var == "/HB")
        return String(htmlHeaderEndBodyStart);
    if (var == "LI")
        return String(htmlListItemStart);
    if (var == "/LI")
        return String(htmlListItemEnd);
#ifdef NO_LIBRARY
    if (var == "SD")
        return String(sdDirectory);
#else   // NO_LIBRARY
    if (var == "SD") {
        sdCardServer.sdCardListingWebPageLink((char *)&htmlBuffer[0], sizeof(htmlBuffer), "SD Card Files", "target=\"_blank\"");
        return String(htmlBuffer);
    }
#endif  // NO_LIBRARY
    if (var == "SZ") {
        sprintf (htmlBuffer, "%3.0f %s",
                 sdCardSizeMB < 1000. ? sdCardSizeMB : sdCardSizeMB / 1000.,
                 sdCardSizeMB < 1000. ? "MB" : "GB");
        return String(htmlBuffer);
    }
    if (var == "Q")
        return String("\"");
    if (var == "T")
        return String(htmlTitle);
    if (var == "UL")
        return String(htmlUlListStart);
    if (var == "/UL")
        return String(htmlUlListEnd);
    return String();
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void
indexHtml (AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", mountSdCard() ? index_html : no_sd_card_html, processor);
}

#endif  // INDEX_PAGE

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef NO_LIBRARY
void
buildHtmlAnchor(HtmlPrint * htmlPrint, uint8_t *buffer) {
    uint64_t u64;
    uint32_t u32;

    // Start the list if necessary
    if (sdCardEmpty) {
        strcpy_P((char *)buffer, htmlUlListStart);
        buffer += strlen((char *)buffer);
    }
    sdCardEmpty = 0;

    // Build the HTML anchor
    strcpy((char *)buffer, "%LI%%A%%SD%");
    htmlPrint->setBufferAddress(buffer +strlen((char *)buffer));
    sdFile.printName(htmlPrint);
    strcat((char *)buffer, "\">");
    htmlPrint->setBufferAddress(buffer +strlen((char *)buffer));
    sdFile.printName(htmlPrint);
    strcat((char *)buffer, "%/A% (");

    //  Display the file size
    u64 = sdFile.fileSize();
    u32 = u64 / (1ull * 1000 * 1000 * 1000);
    if (u32)
        strcat((char *)buffer, String(u32).c_str());
    u32 = u64 % (1ull * 1000 * 1000 * 1000);
    strcat((char *)buffer, String(u32).c_str());
    strcat((char *)buffer, ")%/LI%");
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int addFileName(HtmlPrint * htmlPrint, uint8_t * buffer) {
    uint8_t * bufferStart;

    bufferStart = buffer;
    switch (sdListingState) {
    case LS_HEADER:
        strcpy_P((char *)buffer, sdHeader);
        sdListingState = LS_DISPLAY_FILES;
        return strlen((char *)buffer);

    case LS_DISPLAY_FILES:
        sdFile = SdFile();
        if (!sdFile.openNext(&sdRootDir, O_RDONLY)) {
            sdListingState = LS_TRAILER;
            if (!sdCardEmpty)
                // No more files, at least one file displayed
                return addFileName(htmlPrint, buffer);

            // No files on the SD card
            strcpy_P ((char *)buffer, sdNoFiles);
            return strlen((char *)buffer);
        }

        // Add the anchor if another file exists
        buildHtmlAnchor (htmlPrint, buffer);

        // Close the file
        sdFile.close();
        return strlen((char *)buffer);

    case LS_TRAILER:
        if (!sdCardEmpty)
            strcat_P((char*)buffer, htmlUlListEnd);
        strcat_P((char *)buffer, htmlBodyEnd);
        sdListingState = LS_DONE;
        return strlen((char *)buffer);
    }

    // The listing is complete
    return 0;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int sdCardListing(uint8_t * buffer, size_t maxLen) {
    uint8_t * bufferStart;
    int bytesWritten;
    int dataBytes;
    HtmlPrint * htmlPrint;

    htmlPrint = new HtmlPrint();
    bufferStart = buffer;
    do {
        // Determine if the buffer is full enough
        bytesWritten = buffer - bufferStart;
        if (maxLen < MAX_FILE_NAME_SIZE) {
#ifdef  MARK_BOUNDARY
            if (bytesWritten) {
                Serial.println("Next buffer");
                strcat ((char *)buffer, "--------------------<br>\n");
                buffer += strlen((char *)buffer);
                bytesWritten = buffer - bufferStart;
            }
#endif  // MARK_BOUNDARY
            break;
        }

        // Add the next file name
        *buffer = 0;
        dataBytes = addFileName (htmlPrint, buffer);
        if (!dataBytes)
            break;
        buffer += dataBytes;
        maxLen -= dataBytes;
    } while (1);

    // Close the root directory when done
    if (!bytesWritten)
        sdRootDir.close();

    delete htmlPrint;
    return bytesWritten;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void sdDirectoryListing (AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response;

    mountSdCard();
    if (!online.microSD)
        // SD card not present
        request->send(404);
    else {
        // Open the root directory
        sdCardEmpty = 1;
        sdRootDir = SdFile();
        if (!sdRootDir.openRoot(sd.vol())) {
            // Invalid SD card format
            request->send_P(200, "text/html", not_formatted_html, processor);
        } else {
            sdListingState = LS_HEADER;
            response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                return sdCardListing(buffer, maxLen);
            }, processor);

            // Send the response
            response->addHeader("Server","SD Card Files");
            request->send(response);
        }
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int returnFile(uint8_t * buffer, size_t maxLen) {
    int bytesToRead;
    int bytesRead;

    // Read data from the file
    bytesToRead = maxLen;
    bytesRead = sdFile.read(buffer, bytesToRead);

    // Don't return any more bytes on error
    if (bytesRead < 0)
        bytesRead = 0;

    // Close the file when done
    if (!bytesRead)
        sdFile.close();

    // Return the number of bytes read
    return bytesRead;
}
#endif  // NO_LIBRARY

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifdef  NOT_FOUND_PAGE

void httpRequestNotFound(AsyncWebServerRequest *request) {
#ifndef NO_LIBRARY
        // Display the SD card page if necessary
        if (sdCardServer.isSdCardWebPage(request))
            return;
#else   // NO_LIBRARY

    int dirLength;
    const char * filename;
    const char * url;

    do {
        // Get the URL
        url = (const char*)(request->url().c_str());

        // Does the URL start with the SD device
        strcpy_P(htmlBuffer, sdDirectory);
        dirLength = strlen(htmlBuffer);
        if (strncmp(htmlBuffer, url, dirLength))
            // Unknown URL, return error
            break;

        // Locate the file name in the URL
        filename = &url[dirLength];

        // Attempt to open the root directory
        sdRootDir = SdFile();
        if (!sdRootDir.openRoot(sd.vol())) {
            Serial.println("ERROR - Failed to open root directory!");
            break;
        }

        // Attempt to open the file
        if (!sdFile.open(&sdRootDir, filename, O_RDONLY)) {
            // File not found
            sdRootDir.close();
            break;
        }

        // Close the root directory
        sdRootDir.close();

        // Return the file
        AsyncWebServerResponse *response = request->beginChunkedResponse("application/octet-stream", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            return returnFile(buffer, maxLen);
        });
        response->addHeader("Content-Length", String(4112));
        request->send(response);
        return;
    } while (0);
#endif  // NO_LIBRARY

    // URL not found
    request->send(404);
}
#endif  // NOT_FOUND_PAGE

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
#ifdef  INDEX_PAGE
            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
                indexHtml (request);
            });
#else   // INDEX_PAGE
            sdCardServer.sdCardWebSite(&server);
#endif  // INDEX_PAGE

#ifdef  NO_LIBRARY
            //  /sd/
            server.on("/sd/", HTTP_GET, [](AsyncWebServerRequest *request) {
                sdDirectoryListing (request);
            });
#endif  // NO_LIBRARY

            //  All other pages
#ifdef  NOT_FOUND_PAGE
            server.onNotFound([](AsyncWebServerRequest *request) {
                httpRequestNotFound(request);
            });
#else   // NOT_FOUND_PAGE
            sdCardServer.onNotFound(&server);
#endif  // NOT_FOUND_PAGE

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
