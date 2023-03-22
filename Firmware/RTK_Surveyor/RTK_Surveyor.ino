/*
  September 1st, 2020
  SparkFun Electronics
  Nathan Seidle

  This firmware runs the core of the SparkFun RTK products. It runs on an ESP32
  and communicates with the ZED-F9P.

  Compiled with Arduino v1.8.15 with ESP32 core v2.0.2.

  For compilation instructions see https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#compiling-source

  Special thanks to Avinab Malla for guidance on getting xTasks implemented.

  The RTK Surveyor implements classic Bluetooth SPP to transfer data from the
  ZED-F9P to the phone and receive any RTCM from the phone and feed it back
  to the ZED-F9P to achieve RTK: F9PSerialWriteTask(), F9PSerialReadTask().

  Settings are loaded from microSD if available otherwise settings are pulled from ESP32's file system LittleFS.
*/

#define COMPILE_WIFI //Comment out to remove WiFi functionality
#define COMPILE_AP //Requires WiFi. Comment out to remove Access Point functionality
#define COMPILE_ESPNOW //Requires WiFi. Comment out to remove ESP-Now functionality.
#define COMPILE_BT //Comment out to remove Bluetooth functionality
#define COMPILE_L_BAND //Comment out to remove L-Band functionality
#define COMPILE_SD_MMC //Comment out to remove REFERENCE_STATION microSD SD_MMC support
#define COMPILE_ETHERNET //Comment out to remove REFERENCE_STATION Ethernet (W5500) support
//#define REF_STN_GNSS_DEBUG //Uncomment this line to output GNSS library debug messages on serialGNSS. Ref Stn only. Needs ENABLE_DEVELOPER

//Always define ENABLE_DEVELOPER to enable its use in conditional statements
#ifndef ENABLE_DEVELOPER
#define ENABLE_DEVELOPER true //This enable specials developer modes (don't check power button at startup). Passed in from compiler flags.
#endif

//This is passed in from compiler extra flags
#ifndef POINTPERFECT_TOKEN
#define FIRMWARE_VERSION_MAJOR 99
#define FIRMWARE_VERSION_MINOR 99
#endif

//Define the RTK board identifier:
// This is an int which is unique to this variant of the RTK Surveyor hardware which allows us
// to make sure that the settings stored in flash (LittleFS) are correct for this version of the RTK
// (sizeOfSettings is not necessarily unique and we want to avoid problems when swapping from one variant to another)
// It is the sum of:
//   the major firmware version * 0x10
//   the minor firmware version
#define RTK_IDENTIFIER (FIRMWARE_VERSION_MAJOR * 0x10 + FIRMWARE_VERSION_MINOR)

#include "settings.h"
#include "crc24q.h" //24-bit CRC-24Q cyclic redundancy checksum for RTCM parsing

#define MAX_CPU_CORES               2
#define IDLE_COUNT_PER_SECOND       1000
#define IDLE_TIME_DISPLAY_SECONDS   5
#define MAX_IDLE_TIME_COUNT         (IDLE_TIME_DISPLAY_SECONDS * IDLE_COUNT_PER_SECOND)
#define MILLISECONDS_IN_A_SECOND    1000
#define MILLISECONDS_IN_A_MINUTE    (60 * MILLISECONDS_IN_A_SECOND)
#define MILLISECONDS_IN_AN_HOUR     (60 * MILLISECONDS_IN_A_MINUTE)
#define MILLISECONDS_IN_A_DAY       (24 * MILLISECONDS_IN_AN_HOUR)

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//These pins are set in beginBoard()
int pin_batteryLevelLED_Red = -1;
int pin_batteryLevelLED_Green = -1;
int pin_positionAccuracyLED_1cm = -1;
int pin_positionAccuracyLED_10cm = -1;
int pin_positionAccuracyLED_100cm = -1;
int pin_baseStatusLED = -1;
int pin_bluetoothStatusLED = -1;
int pin_microSD_CS = -1;
int pin_zed_tx_ready = -1;
int pin_zed_reset = -1;
int pin_batteryLevel_alert = -1;

int pin_muxA = -1;
int pin_muxB = -1;
int pin_powerSenseAndControl = -1;
int pin_setupButton = -1;
int pin_powerFastOff = -1;
int pin_dac26 = -1;
int pin_adc39 = -1;
int pin_peripheralPowerControl = -1;

int pin_radio_rx = -1;
int pin_radio_tx = -1;
int pin_radio_rst = -1;
int pin_radio_pwr = -1;
int pin_radio_cts = -1;
int pin_radio_rts = -1;

int pin_Ethernet_CS = -1;
int pin_Ethernet_Interrupt = -1;
int pin_GNSS_CS = -1;
int pin_GNSS_TimePulse = -1;
int pin_microSD_CardDetect = -1;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "esp_ota_ops.h" //Needed for partition counting and updateFromSD

//I2C for GNSS, battery gauge, display, accelerometer
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//LittleFS for storing settings for different user profiles
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <LittleFS.h>

#define MAX_PROFILE_COUNT 8
uint8_t activeProfiles = 0; //Bit vector indicating which profiles are active
uint8_t displayProfile; //Range: 0 - (MAX_PROFILE_COUNT - 1)
uint8_t profileNumber = MAX_PROFILE_COUNT; //profileNumber gets set once at boot to save loading time
char profileNames[MAX_PROFILE_COUNT][50]; //Populated based on names found in LittleFS and SD
char settingsFileName[60]; //Contains the %s_Settings_%d.txt with current profile number set

char stationCoordinateECEFFileName[60]; //Contains the /StationCoordinates-ECEF_%d.csv with current profile number set
char stationCoordinateGeodeticFileName[60]; //Contains the /StationCoordinates-Geodetic_%d.csv with current profile number set
const int COMMON_COORDINATES_MAX_STATIONS = 50; //Record upto 50 ECEF and Geodetic commonly used stations
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Handy library for setting ESP32 system time to GNSS time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <ESP32Time.h> //http://librarymanager/All#ESP32Time by FBiego v2.0.0
ESP32Time rtc;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>

#include "SdFat.h" //http://librarymanager/All#sdfat_exfat by Bill Greiman. Currently uses v2.1.1
SdFat *sd;

#include "FileSdFatMMC.h" //Hybrid SdFat and SD_MMC file access

char platformFilePrefix[40] = "SFE_Surveyor"; //Sets the prefix for logs and settings files

FileSdFatMMC * ubxFile; //File that all GNSS ubx messages sentences are written to
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second
int startLogTime_minutes = 0; //Mark when we start any logging so we can stop logging after maxLogTime_minutes
int startCurrentLogTime_minutes = 0; //Mark when we start this specific log file so we can close it after x minutes and start a new one

//System crashes if two tasks access a file at the same time
//So we use a semaphore to see if file system is available
SemaphoreHandle_t sdCardSemaphore;
TickType_t loggingSemaphoreWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_shortWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_longWait_ms = 200 / portTICK_PERIOD_MS;

//Display used/free space in menu and config page
uint64_t sdCardSize = 0;
uint64_t sdFreeSpace = 0;
bool outOfSDSpace = false;
const uint32_t sdMinAvailableSpace = 10000000; //Minimum available bytes before SD is marked as out of space

//Controls Logging Icon type
typedef enum LoggingType {
  LOGGING_UNKNOWN = 0,
  LOGGING_STANDARD,
  LOGGING_PPP,
  LOGGING_CUSTOM
} LoggingType;
LoggingType loggingType = LOGGING_UNKNOWN;

FileSdFatMMC * managerTempFile; //File used for uploading or downloading in file manager section of AP config
bool managerFileOpen = false;

TaskHandle_t sdSizeCheckTaskHandle = nullptr; //Store handles so that we can kill the task once size is found
const uint8_t sdSizeCheckTaskPriority = 0; //3 being the highest, and 0 being the lowest
const int sdSizeCheckStackSize = 2000;
bool sdSizeCheckTaskComplete = false;

char logFileName[sizeof("SFE_Facet_L-Band_230101_120101.ubx_plusExtraSpace")] = "";
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Connection settings to NTRIP Caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#ifdef COMPILE_WIFI
#include <WiFi.h> //Built-in.
#include <WiFiMulti.h> //Built-in.
#include <HTTPClient.h> //Built-in. Needed for ThingStream API for ZTP
#include <ArduinoJson.h> //http://librarymanager/All#Arduino_JSON_messagepack v6.19.4
#include <WiFiClientSecure.h> //Built-in.
#include <PubSubClient.h> //http://librarymanager/All#PubSubClient_MQTT_Lightweight v2.8.0 Used for MQTT obtaining of keys
#include "ESP32OTAPull.h" //http://librarymanager/All#ESP-OTA-Pull Used for getting latest firmware OTA

#include "esp_wifi.h" //Needed for esp_wifi_set_protocol()

#include "base64.h" //Built-in. Needed for NTRIP Client credential encoding.

static int ntripClientConnectionAttempts = 0; //Count the number of connection attempts between restarts
static int ntripServerConnectionAttempts = 0; //Count the number of connection attempts between restarts

#endif

volatile uint8_t wifiTcpConnected;

//NTRIP client timer usage:
// * Measure the connection response time
// * Receive NTRIP data timeout
static uint32_t ntripClientTimer;
static uint32_t ntripClientStartTime; //For calculating uptime
static int ntripClientConnectionAttemptsTotal; //Count the number of connection attempts absolutely

//NTRIP server timer usage:
// * Measure the connection response time
// * Receive RTCM correction data timeout
// * Monitor last RTCM byte received for frame counting
static uint32_t ntripServerTimer;
static uint32_t ntripServerStartTime;
static int ntripServerConnectionAttemptsTotal; //Count the number of connection attempts absolutely

#define OTA_FIRMWARE_JSON_URL "https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware_Binaries/main/RTK-Firmware.json"
#define OTA_RC_FIRMWARE_JSON_URL "https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware_Binaries/main/RTK-RC-Firmware.json"
bool apConfigFirmwareUpdateInProcess = false; //Goes true once WiFi is connected and OTA pull begins
bool enableRCFirmware = false; //Goes true from AP config page
bool currentlyParsingData = false; //Goes true when we hit 750ms timeout with new data
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3 v3.0.5

#define SENTENCE_TYPE_NMEA              DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_NMEA
#define SENTENCE_TYPE_NONE              DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_NONE
#define SENTENCE_TYPE_RTCM              DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_RTCM
#define SENTENCE_TYPE_UBX               DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_UBX

char zedFirmwareVersion[20]; //The string looks like 'HPG 1.12'. Output to system status menu and settings file.
char neoFirmwareVersion[20]; //Output to system status menu.
uint8_t zedFirmwareVersionInt = 0; //Controls which features (constellations) can be configured (v1.12 doesn't support SBAS). Note: will fail above 2.55!
uint8_t zedModuleType = PLATFORM_F9P; //Controls which messages are supported and configured

//Use Michael's lock/unlock methods to prevent the UART2 task from calling checkUblox during a sendCommand and waitForResponse.
//Also prevents pushRawData from being called too.
class SFE_UBLOX_GNSS_SUPER_DERIVED : public SFE_UBLOX_GNSS_SUPER
{
public:
  SemaphoreHandle_t gnssSemaphore = nullptr;
  bool createLock(void)
  {
    if (gnssSemaphore == nullptr)
      gnssSemaphore = xSemaphoreCreateMutex();
    return gnssSemaphore;
  }
  bool lock(void)
  {
    return (xSemaphoreTake(gnssSemaphore, 2100) == pdPASS);
  }
  void unlock(void)
  {
    xSemaphoreGive(gnssSemaphore);
  }
  void deleteLock(void)
  {
    vSemaphoreDelete(gnssSemaphore);
  }
};

SFE_UBLOX_GNSS_SUPER_DERIVED theGNSS;

//These globals are updated regularly via the storePVTdata callback
bool pvtUpdated = false;
double latitude;
double longitude;
float altitude;
float horizontalAccuracy;
bool validDate;
bool validTime;
bool confirmedDate;
bool confirmedTime;
uint8_t gnssDay;
uint8_t gnssMonth;
uint16_t gnssYear;
uint8_t gnssHour;
uint8_t gnssMinute;
uint8_t gnssSecond;
uint16_t mseconds;
uint8_t numSV;
uint8_t fixType;
uint8_t carrSoln;

const byte haeNumberOfDecimals = 8; //Used for printing and transmitting lat/lon
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Battery fuel gauge and PWM LEDs
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048);

//RTK Surveyor LED PWM properties
const int pwmFreq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int ledBTChannel = 2;
const int pwmResolution = 8;

int pwmFadeAmount = 10;
int btFadeLevel = 0;

int battLevel = 0; //SOC measured from fuel gauge, in %. Used in multiple places (display, serial debug, log)
float battVoltage = 0.0;
float battChangeRate = 0.0;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#ifdef COMPILE_BT
//See bluetoothSelect.h for implemenation
#include "bluetoothSelect.h"
#endif

char platformPrefix[55] = "Surveyor"; //Sets the prefix for broadcast names

#include <driver/uart.h> //Required for uart_set_rx_full_threshold() on cores <v2.0.5
HardwareSerial serialGNSS(2); //TX on 17, RX on 16

#define SERIAL_SIZE_TX 512
uint8_t wBuffer[SERIAL_SIZE_TX]; //Buffer for writing from incoming SPP to F9P
TaskHandle_t F9PSerialWriteTaskHandle = nullptr; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
const uint8_t F9PSerialWriteTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int writeTaskStackSize = 2000;

uint8_t * ringBuffer; //Buffer for reading from F9P. At 230400bps, 23040 bytes/s. If SD blocks for 250ms, we need 23040 * 0.25 = 5760 bytes worst case.
TaskHandle_t F9PSerialReadTaskHandle = nullptr; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
const uint8_t F9PSerialReadTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int readTaskStackSize = 2000;

TaskHandle_t handleGNSSDataTaskHandle = nullptr;
const uint8_t handleGNSSDataTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int handleGNSSDataTaskStackSize = 3000;

TaskHandle_t pinUART2TaskHandle = nullptr; //Dummy task to start UART2 on core 0.
volatile bool uart2pinned = false; //This variable is touched by core 0 but checked by core 1. Must be volatile.

volatile static int combinedSpaceRemaining = 0; //Overrun indicator
volatile static long fileSize = 0; //Updated with each write
int bufferOverruns = 0; //Running count of possible data losses since power-on

bool zedUartPassed = false; //Goes true during testing if ESP can communicate with ZED over UART
const uint8_t btEscapeCharacter = '+';
const uint8_t btMaxEscapeCharacters = 3; //Number of characters needed to enter command mode over B

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_Graphic_OLED
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Firmware binaries loaded from SD
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <Update.h>
int binCount = 0;
const int maxBinFiles = 10;
char binFileNames[maxBinFiles][50];
const char* forceFirmwareFileName = "RTK_Surveyor_Firmware_Force.bin"; //File that will be loaded at startup regardless of user input
unsigned int binBytesSent = 0; //Tracks firmware bytes sent over WiFi OTA update via AP config.
int binBytesLastUpdate = 0; //Allows websocket notification to be sent every 100k bytes
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Low frequency tasks
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <Ticker.h>

Ticker btLEDTask;
float btLEDTaskPace2Hz = 0.5;
float btLEDTaskPace33Hz = 0.03;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Accelerometer for bubble leveling
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "SparkFun_LIS2DH12.h" //Click here to get the library: http://librarymanager/All#SparkFun_LIS2DH12
SPARKFUN_LIS2DH12 accel;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Buttons - Interrupt driven and debounce
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <JC_Button.h> //http://librarymanager/All#JC_Button v2.1.2
Button *setupBtn = nullptr; //We can't instantiate the buttons here because we don't yet know what pin numbers to use
Button *powerBtn = nullptr;

TaskHandle_t ButtonCheckTaskHandle = nullptr;
const uint8_t ButtonCheckTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int buttonTaskStackSize = 2000;

const int shutDownButtonTime = 2000; //ms press and hold before shutdown
unsigned long lastRockerSwitchChange = 0; //If quick toggle is detected (less than 500ms), enter WiFi AP Config mode
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Webserver for serving config page from ESP32 as Acess Point
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

#include "ESPAsyncWebServer.h" //Get from: https://github.com/me-no-dev/ESPAsyncWebServer v1.2.3
#include "form.h"

AsyncWebServer *webserver = nullptr;
AsyncWebSocket *websocket = nullptr;

char *settingsCSV = nullptr; //Push large array onto heap

#endif
#endif

//Because the incoming string is longer than max len, there are multiple callbacks so we
//use a global to combine the incoming
#define AP_CONFIG_SETTING_SIZE 5000
char *incomingSettings = nullptr;
int incomingSettingsSpot = 0;
unsigned long timeSinceLastIncomingSetting = 0;
unsigned long lastDynamicDataUpdate = 0;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//PointPerfect Corrections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#if __has_include("tokens.h")
#include "tokens.h"
#endif

float lBandEBNO = 0.0; //Used on system status menu
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//ESP NOW for multipoint wireless broadcasting over 2.4GHz
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifdef COMPILE_ESPNOW

#include <esp_now.h>

uint8_t espnowOutgoing[250]; //ESP NOW has max of 250 characters
unsigned long espnowLastAdd; //Tracks how long since last byte was added to the outgoing buffer
uint8_t espnowOutgoingSpot = 0; //ESP Now has max of 250 characters
uint16_t espnowBytesSent = 0; //May be more than 255
uint8_t receivedMAC[6]; //Holds the broadcast MAC during pairing

int packetRSSI = 0;
unsigned long lastEspnowRssiUpdate = 0;

#endif

int espnowRSSI = 0;
const uint8_t ESPNOW_MAX_PEERS = 5; //Maximum of 5 rovers

//Ethernet
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#ifdef COMPILE_ETHERNET
#include <Ethernet.h> // http://librarymanager/All#Arduino_Ethernet
//#include <EthernetLarge.h> // https://github.com/OPEnSLab-OSU/EthernetLarge
IPAddress ethernetIPAddress;
IPAddress ethernetDNS;
IPAddress ethernetGateway;
IPAddress ethernetSubnetMask;
EthernetServer *ethernetHTTPServer = nullptr; //This will be instantiated when we know the HTTP port
class derivedEthernetUDP : public EthernetUDP
{
  public:
    uint8_t getSockIndex() { return sockindex; } // sockindex is protected in EthernetUDP. A derived class can access it.
};
derivedEthernetUDP *ethernetNTPServer = nullptr; //This will be instantiated when we know the NTP port
volatile uint8_t ntpSockIndex; //The W5500 socket index for NTP - so we can enable and read the correct interrupt
#endif
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#define lbandMACAddress         btMACAddress
uint8_t wifiMACAddress[6]; //Display this address in the system menu
uint8_t btMACAddress[6]; //Display this address when Bluetooth is enabled, otherwise display wifiMACAddress
uint8_t ethernetMACAddress[6]; //Display this address when Ethernet is enabled, otherwise display wifiMACAddress
char deviceName[70]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'
const uint16_t menuTimeout = 60 * 10; //Menus will exit/timeout after this number of seconds
int systemTime_minutes = 0; //Used to test if logging is less than max minutes
uint32_t powerPressedStartTime = 0; //Times how long user has been holding power button, used for power down
bool inMainMenu = false; //Set true when in the serial config menu system.
bool btPrintEcho = false; //Set true when in the serial config menu system via Bluetooth.
bool btPrintEchoExit = false; //When true, exit all config menus.

uint32_t lastBattUpdate = 0;
uint32_t lastDisplayUpdate = 0;
bool forceDisplayUpdate = false; //Goes true when setup is pressed, causes display to refresh real time
uint32_t lastSystemStateUpdate = 0;
bool forceSystemStateUpdate = false; //Set true to avoid update wait
uint32_t lastAccuracyLEDUpdate = 0;
uint32_t lastBaseLEDupdate = 0; //Controls the blinking of the Base LED

uint32_t lastFileReport = 0; //When logging, print file record stats every few seconds
long lastStackReport = 0; //Controls the report rate of stack highwater mark within a task
uint32_t lastHeapReport = 0; //Report heap every 1s if option enabled
uint32_t lastTaskHeapReport = 0; //Report task heap every 1s if option enabled
uint32_t lastCasterLEDupdate = 0; //Controls the cycling of position LEDs during casting
uint32_t lastRTCAttempt = 0; //Wait 1000ms between checking GNSS for current date/time
uint32_t lastPrintPosition = 0; //For periodic display of the position

uint32_t lastBaseIconUpdate = 0;
bool baseIconDisplayed = false; //Toggles as lastBaseIconUpdate goes above 1000ms
uint8_t loggingIconDisplayed = 0; //Increases every 500ms while logging
uint8_t espnowIconDisplayed = 0; //Increases every 500ms while transmitting

uint64_t lastLogSize = 0;
bool logIncreasing = false; //Goes true when log file is greater than lastLogSize or logPosition changes
bool reuseLastLog = false; //Goes true if we have a reset due to software (rather than POR)

uint16_t rtcmPacketsSent = 0; //Used to count RTCM packets sent via processRTCM()
uint32_t rtcmBytesSent = 0;
uint32_t rtcmLastReceived = 0;

uint32_t maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds

uint16_t svinObservationTime = 0; //Use globals so we don't have to request these values multiple times (slow response)
float svinMeanAccuracy = 0;

uint32_t lastSetupMenuChange = 0; //Auto-selects the setup menu option after 1500ms
uint32_t lastTestMenuChange = 0; //Avoids exiting the test menu for at least 1 second

bool firstRoverStart = false; //Used to detect if user is toggling power button at POR to enter test menu

bool newEventToRecord = false; //Goes true when INT pin goes high
uint32_t triggerCount = 0; //Global copy - TM2 event counter
uint32_t triggerTowMsR = 0; //Global copy - Time Of Week of rising edge (ms)
uint32_t triggerTowSubMsR = 0; //Global copy - Millisecond fraction of Time Of Week of rising edge in nanoseconds
uint32_t triggerAccEst = 0; //Global copy - Accuracy estimate in nanoseconds

bool firstPowerOn = true; //After boot, apply new settings to ZED if user switches between base or rover
unsigned long splashStart = 0; //Controls how long the splash is displayed for. Currently min of 2s.
bool restartBase = false; //If user modifies any NTRIP Server settings, we need to restart the base
bool restartRover = false; //If user modifies any NTRIP Client settings, we need to restart the rover

unsigned long startTime = 0; //Used for checking longest running functions
bool lbandCorrectionsReceived = false; //Used to display L-Band SIV icon when corrections are successfully decrypted
unsigned long lastLBandDecryption = 0; //Timestamp of last successfully decrypted PMP message
volatile bool mqttMessageReceived = false; //Goes true when the subscribed MQTT channel reports back
uint8_t leapSeconds = 0; //Gets set if GNSS is online
unsigned long systemTestDisplayTime = 0; //Timestamp for swapping the graphic during testing
uint8_t systemTestDisplayNumber = 0; //Tracks which test screen we're looking at
unsigned long rtcWaitTime = 0; //At poweron, we give the RTC a few seconds to update during PointPerfect Key checking

TaskHandle_t idleTaskHandle[MAX_CPU_CORES];
uint32_t max_idle_count = MAX_IDLE_TIME_COUNT;

bool firstRadioSpotBlink = false; //Controls when the shared icon space is toggled
unsigned long firstRadioSpotTimer = 0;
bool secondRadioSpotBlink = false; //Controls when the shared icon space is toggled
unsigned long secondRadioSpotTimer = 0;
bool thirdRadioSpotBlink = false; //Controls when the shared icon space is toggled
unsigned long thirdRadioSpotTimer = 0;

bool bluetoothIncomingRTCM = false;
bool bluetoothOutgoingRTCM = false;
bool wifiIncomingRTCM = false;
bool wifiOutgoingRTCM = false;
bool espnowIncomingRTCM = false;
bool espnowOutgoingRTCM = false;

static byte rtcmParsingState = RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3;
uint16_t failedParserMessages_UBX = 0;
uint16_t failedParserMessages_RTCM = 0;
uint16_t failedParserMessages_NMEA = 0;

unsigned long btLastByteReceived = 0; //Track when last BT transmission was received.
const long btMinEscapeTime = 2000; //Bluetooth serial traffic must stop this amount before an escape char is recognized
uint8_t btEscapeCharsReceived = 0; //Used to enter command mode

bool externalPowerConnected = false; //Goes true when a high voltage is seen on power control pin

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/*
                     +---------------------------------------+      +----------+
                     |                 ESP32                 |      |   GNSS   |  Antenna
  +-------------+    |                                       |      |          |     |
  | Phone       |    |   .-----------.          .--------.   |27  42|          |     |
  |        RTCM |--->|-->|           |--------->|        |-->|----->|TXD, MISO |     |
  |             |    |   | Bluetooth |          | UART 2 |   |      | UART1    |     |
  | NMEA + RTCM |<---|<--|           |<-------+-|        |<--|<-----|RXD, MOSI |<----'
  +-------------+    |   '-----------'        | '--------'   |28  43|          |
                     |                        |              |      |          |
      .---------+    |                        |              |      |          |
     / uSD Card |    |                        |              |      |          |
    /           |    |   .----.               V              |      |          |
   |   Log File |<---|<--|    |<--------------+              |      |          |47
   |            |    |   |    |               |              |      |    D_SEL |<---- N/C (1)
   |  Profile # |<-->|<->| SD |<--> Profile   |              |      | 0 = SPI  |
   |            |    |   |    |               |              |      | 1 = I2C  |
   |  Settings  |<-->|<->|    |<--> Settings  |              |      |    UART1 |
   |            |    |   '----'               |              |      |          |
   +------------+    |                        |              |      |          |
                     |   .--------.           |              |      |          |
                     |   |        |<----------'              |      |          |
                     |   |  USB   |                          |      |          |
       USB UART <--->|<->| Serial |<-- Debug Output          |      |          |
    (Config ESP32)   |   |        |                          |      |          |
                     |   |        |<-- Serial Config         |      |   UART 2 |<--> Radio
                     |   '--------'                          |      |          |   Connector
                     |                                       |      |          |  (Correction
                     |   .------.                            |      |          |      Data)
        Browser <--->|<->|      |<---> WiFi Config           |      |          |
                     |   |      |                            |      |          |
  +--------------+   |   |      |                            |      |      USB |<--> USB UART
  |              |<--|<--| WiFi |<---- NMEA + RTCM <-.       |      |          |  (Config UBLOX)
  | NTRIP Caster |   |   |      |                    |       |      |          |
  |              |-->|-->|      |-----------.        |       |6   46|          |
  +--------------+   |   |      |           |        |  .----|<-----|TXREADY   |
                     |   '------'           |        |  v    |      |          |
                     |                      |      .-----.   |      |          |
                     |                      '----->|     |   |33  44|          |
                     |                             |     |<->|<---->|SDA, CS_N |
                     |           Commands -------->| I2C |   |      |    I2C   |
                     |                             |     |-->|----->|SCL, CLK  |
                     |             Status <--------|     |   |36  45|          |
                     |                             '-----'   |      +----------+
                     |                                       |
                     +---------------------------------------+
                                  26|   |24   A B
                                    |   |     0 0 = X0, Y0
                                    V   V     0 1 = X1, Y1
                                  +-------+   1 0 = X2, Y2
                                  | B   A |   1 1 = X3, Y3
                                  |       |
                                  |     X0|<--- GNSS UART1 TXD
                                  |       |
                                  |     X1|<--- GNSS PPS STAT
                            3 <---|X      |
                                  |     X2|<--- SCL
                                  |       |
                                  |     X3|<--- DAC2
                   Data Port      |       |
                                  |     Y0|----> ZED UART1 RXD
                                  |       |
                                  |     Y1|<--> ZED EXT INT
                            2 <-->|Y      |
                                  |     Y2|---> SDA
                                  |       |
                                  |     Y3|---> ADC39
                                  |       |
                                  |  MUX  |
                                  +-------+
*/
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging

  identifyBoard(); //Determine what hardware platform we are running on

  initializePowerPins(); //Initialize any essential power pins - e.g. enable power for the Display

  beginI2C();

  beginDisplay(); //Start display to be able to display any errors

  beginGNSS(); //Connect to GNSS to get module type

  beginFS(); //Start file system for settings

  beginBoard(); //Now finish settup up the board and check the on button

  displaySplash(); //Display the RTK product name and firmware version

  beginLEDs(); //LED and PWM setup

  beginSD(); //Test if SD is present

  loadSettings(); //Attempt to load settings after SD is started so we can read the settings file if available

  beginIdleTasks(); //Enable processor load calculations

  beginUART2(); //Start UART2 on core 0, used to receive serial from ZED and pass out over SPP

  beginFuelGauge(); //Configure battery fuel guage monitor

  configureGNSS(); //Configure ZED module

  beginEthernet(); //Start-up the Ethernet connection

  beginEthernetHTTPServer();

  beginEthernetNTPServer();

  beginAccelerometer();

  beginLBand();

  beginExternalTriggers(); //Configure the time pulse output and TM2 input

  beginSystemState(); //Determine initial system state. Start task for button monitoring.

  updateRTC(); //The GNSS likely has time/date. Update ESP32 RTC to match. Needed for PointPerfect key expiration.

  Serial.flush(); //Complete any previous prints

  log_d("Boot time: %d", millis());

  danceLEDs(); //Turn on LEDs like a car dashboard
}

void loop()
{
  if (online.gnss == true)
  {
    theGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
    theGNSS.checkCallbacks(); //Process any callbacks: ie, eventTriggerReceived
  }

  updateSystemState();

  updateBattery();

  updateDisplay();

  updateRTC(); //Set system time to GNSS once we have fix

  updateSD(); //Check if SD needs to be started or is at max capacity

  updateLogs(); //Record any new data. Create or close files as needed.

  reportHeap(); //If debug enabled, report free heap

  updateSerial(); //Menu system via ESP32 USB connection

  wifiUpdate(); //Bring up WiFi when services need it

  updateLBand(); //Check if we've recently received PointPerfect corrections or not

  updateRadio(); //Check if we need to finish sending any RTCM over link radio

  ntripClientUpdate(); //Check the NTRIP client connection and move data NTRIP --> ZED

  ntripServerUpdate(); //Check the NTRIP server connection and move data ZED --> NTRIP

  tcpUpdate(); //Turn on TCP Client or Server as needed

  printPosition(); //Periodically print GNSS coordinates if enabled

  //A small delay prevents panic if no other I2C or functions are called
  delay(10);
}

//Monitor if SD card is online or not
//Attempt to remount SD card if card is offline but present
//Capture card size when mounted
void updateSD()
{
  if (online.microSD == false)
  {
    //Are we offline because we are out of space?
    if (outOfSDSpace == true)
    {
      if (sdPresent() == false) //Poll card to see if user has removed card
        outOfSDSpace = false;
    }
    else if (sdPresent() == true) //Poll card to see if a card is inserted
    {
      systemPrintln("SD inserted");
      beginSD(); //Attempt to start SD
    }
  }

  if (online.logging == true && sdCardSize > 0 && sdFreeSpace < sdMinAvailableSpace) //Stop logging if we are below the min
  {
    log_d("Logging stopped. SD full.");
    outOfSDSpace = true;
    endSD(false, true); //(alreadyHaveSemaphore, releaseSemaphore) Close down file.
    return;
  }

  if (online.microSD && sdCardSize == 0)
    beginSDSizeCheckTask(); //Start task to determine SD card size

  if (sdSizeCheckTaskComplete == true)
    deleteSDSizeCheckTask();
}

//Create or close files as needed (startup or as user changes settings)
//Push new data to log as needed
void updateLogs()
{
  //Convert current system time to minutes. This is used in F9PSerialReadTask()/updateLogs() to see if we are within max log window.
  systemTime_minutes = millis() / 1000L / 60;

  //If we are in AP config, don't touch the SD card
  if (systemState == STATE_WIFI_CONFIG_NOT_STARTED || systemState == STATE_WIFI_CONFIG)
    return;

  if (online.microSD == false)
    return; //We can't log if there is no SD

  if (outOfSDSpace == true)
    return; //We can't log if we are out of SD space

  if (online.logging == false && settings.enableLogging == true)
  {
    beginLogging();

    setLoggingType(); //Determine if we are standard, PPP, or custom. Changes logging icon accordingly.
  }
  else if (online.logging == true && settings.enableLogging == false)
  {
    //Close down file
    endSD(false, true);
  }
  else if (online.logging == true && settings.enableLogging == true && (systemTime_minutes - startCurrentLogTime_minutes) >= settings.maxLogLength_minutes)
  {
    if (settings.runLogTest == false)
      endSD(false, true); //Close down file. A new one will be created at the next calling of updateLogs().
    else if (settings.runLogTest == true)
      updateLogTest();
  }

  if (online.logging == true)
  {
    //Record any pending trigger events
    if (newEventToRecord == true)
    {
      systemPrintln("Recording event");

      //Record trigger count with Time Of Week of rising edge (ms), Millisecond fraction of Time Of Week of rising edge (ns), and accuracy estimate (ns)
      char eventData[82]; //Max NMEA sentence length is 82
      snprintf(eventData, sizeof(eventData), "%d,%d,%d,%d", triggerCount, triggerTowMsR, triggerTowSubMsR, triggerAccEst);

      char nmeaMessage[82]; //Max NMEA sentence length is 82
      createNMEASentence(CUSTOM_NMEA_TYPE_EVENT, nmeaMessage, sizeof(nmeaMessage), eventData); //textID, buffer, sizeOfBuffer, text

      if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
      {
        markSemaphore(FUNCTION_EVENT);

        ubxFile->println(nmeaMessage);

        xSemaphoreGive(sdCardSemaphore);
        newEventToRecord = false;
      }
      else
      {
        char semaphoreHolder[50];
        getSemaphoreFunction(semaphoreHolder);

        //While a retry does occur during the next loop, it is possible to loose
        //trigger events if they occur too rapidly or if the log file is closed
        //before the trigger event is written!
        log_w("sdCardSemaphore failed to yield, held by %s, RTK_Surveyor.ino line %d", semaphoreHolder, __LINE__);
      }
    }

    //Report file sizes to show recording is working
    if ((millis() - lastFileReport) > 5000)
    {
      if (fileSize > 0)
      {
        lastFileReport = millis();
        if (settings.enablePrintLogFileStatus)
        {
          systemPrintf("Log file size: %ld", fileSize);

          if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
          {
            //Calculate generation and write speeds every 5 seconds
            uint32_t fileSizeDelta = fileSize - lastLogSize;
            systemPrintf(" - Generation rate: %0.1fkB/s", fileSizeDelta / 5.0 / 1000.0);
          }
          else
          {
            systemPrintf(" reached max log time %d", settings.maxLogTime_minutes);
          }

          systemPrintln();
        }

        if (fileSize > lastLogSize)
        {
          lastLogSize = fileSize;
          logIncreasing = true;
        }
        else
        {
          log_d("No increase in file size");
          logIncreasing = false;

          endSD(false, true); //alreadyHaveSemaphore, releaseSemaphore
        }
      }
    }
  }
}

//Once we have a fix, sync system clock to GNSS
//All SD writes will use the system date/time
void updateRTC()
{
  if (online.rtc == false)
  {
    if (online.gnss == true)
    {
      if (millis() - lastRTCAttempt > 1000)
      {
        lastRTCAttempt = millis();

        theGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
        theGNSS.checkCallbacks(); //Process any callbacks: ie, eventTriggerReceived

        bool timeValid = false;
        if (validTime == true && validDate == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
          timeValid = true;
        if (confirmedTime == true && confirmedDate == true) //Requires GNSS fix
          timeValid = true;

        if (timeValid == true)
        {
          int hour;
          int minute;
          int second;

          //Get the latest time in the GNSS
          theGNSS.checkUblox();

          //Get the time values
          hour = theGNSS.getHour();     //Range: 0 - 23
          minute = theGNSS.getMinute(); //Range: 0 - 59
          second = theGNSS.getSecond(); //Range: 0 - 59

          //Perform time zone adjustment
          second += settings.timeZoneSeconds;
          minute += settings.timeZoneMinutes;
          hour += settings.timeZoneHours;

          //Set the internal system time
          //This is normally set with WiFi NTP but we will rarely have WiFi
          //rtc.setTime(gnssSecond, gnssMinute, gnssHour, gnssDay, gnssMonth, gnssYear);
          rtc.setTime(second, minute, hour, theGNSS.getDay(), theGNSS.getMonth(), theGNSS.getYear());

          online.rtc = true;

          systemPrint("System time set to: ");
          systemPrintln(rtc.getDateTime(true));

          recordSystemSettingsToFileSD(settingsFileName); //This will re-record the setting file with current date/time.
        }
        else
        {
          systemPrintln("No GNSS date/time available for system RTC.");
        } //End timeValid
      } //End lastRTCAttempt
    } //End online.gnss
  } //End online.rtc
}

//Called from main loop
//Control incoming/outgoing RTCM data from:
//External radio - this is normally a serial telemetry radio hung off the RADIO port
//Internal ESP NOW radio - Use the ESP32 to directly transmit/receive RTCM over 2.4GHz (no WiFi needed)
void updateRadio()
{
  //If we have not gotten new RTCM bytes for a period of time, assume end of frame
  if (millis() - rtcmLastReceived > 50 && rtcmBytesSent > 0)
  {
    rtcmBytesSent = 0;
    rtcmPacketsSent++; //If not checking RTCM CRC, count based on timeout
  }

#ifdef COMPILE_ESPNOW
  if (settings.radioType == RADIO_ESPNOW)
  {
    if (espnowState == ESPNOW_PAIRED)
    {
      //If it's been longer than a few ms since we last added a byte to the buffer
      //then we've reached the end of the RTCM stream. Send partial buffer.
      if (espnowOutgoingSpot > 0 && (millis() - espnowLastAdd) > 50)
      {
        if (settings.espnowBroadcast == false)
          esp_now_send(0, (uint8_t *) &espnowOutgoing, espnowOutgoingSpot); //Send partial packet to all peers
        else
        {
          uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
          esp_now_send(broadcastMac, (uint8_t *) &espnowOutgoing, espnowOutgoingSpot); //Send packet via broadcast
        }

        if (!inMainMenu) log_d("ESPNOW transmitted %d RTCM bytes", espnowBytesSent + espnowOutgoingSpot);
        espnowBytesSent = 0;
        espnowOutgoingSpot = 0; //Reset
      }

      //If we don't receive an ESP NOW packet after some time, set RSSI to very negative
      //This removes the ESPNOW icon from the display when the link goes down
      if (millis() - lastEspnowRssiUpdate > 5000 && espnowRSSI > -255)
        espnowRSSI = -255;
    }
  }
#endif
}


//Record who is holding the semaphore
volatile SemaphoreFunction semaphoreFunction = FUNCTION_NOT_SET;

void markSemaphore(SemaphoreFunction functionNumber)
{
  semaphoreFunction = functionNumber;
}

//Resolves the holder to a printable string
void getSemaphoreFunction(char* functionName)
{
  switch (semaphoreFunction)
  {
    default:
      strcpy(functionName, "Unknown");
      break;

    case FUNCTION_SYNC:
      strcpy(functionName, "Sync");
      break;
    case FUNCTION_WRITESD:
      strcpy(functionName, "Write");
      break;
    case FUNCTION_FILESIZE:
      strcpy(functionName, "FileSize");
      break;
    case FUNCTION_EVENT:
      strcpy(functionName, "Event");
      break;
    case FUNCTION_BEGINSD:
      strcpy(functionName, "BeginSD");
      break;
    case FUNCTION_RECORDSETTINGS:
      strcpy(functionName, "Record Settings");
      break;
    case FUNCTION_LOADSETTINGS:
      strcpy(functionName, "Load Settings");
      break;
    case FUNCTION_MARKEVENT:
      strcpy(functionName, "Mark Event");
      break;
    case FUNCTION_GETLINE:
      strcpy(functionName, "Get line");
      break;
    case FUNCTION_REMOVEFILE:
      strcpy(functionName, "Remove file");
      break;
    case FUNCTION_RECORDLINE:
      strcpy(functionName, "Record Line");
      break;
    case FUNCTION_CREATEFILE:
      strcpy(functionName, "Create File");
      break;
    case FUNCTION_ENDLOGGING:
      strcpy(functionName, "End Logging");
      break;
    case FUNCTION_FINDLOG:
      strcpy(functionName, "Find Log");
      break;
    case FUNCTION_LOGTEST:
      strcpy(functionName, "Log Test");
      break;
    case FUNCTION_FILELIST:
      strcpy(functionName, "File List");
      break;
    case FUNCTION_FILEMANAGER_OPEN1:
      strcpy(functionName, "FileManager Open1");
      break;
    case FUNCTION_FILEMANAGER_OPEN2:
      strcpy(functionName, "FileManager Open2");
      break;
    case FUNCTION_FILEMANAGER_OPEN3:
      strcpy(functionName, "FileManager Open3");
      break;
    case FUNCTION_FILEMANAGER_UPLOAD1:
      strcpy(functionName, "FileManager Upload1");
      break;
    case FUNCTION_FILEMANAGER_UPLOAD2:
      strcpy(functionName, "FileManager Upload2");
      break;
    case FUNCTION_FILEMANAGER_UPLOAD3:
      strcpy(functionName, "FileManager Upload3");
      break;
    case FUNCTION_SDSIZECHECK:
      strcpy(functionName, "SD Size Check");
      break;
    case FUNCTION_LOG_CLOSURE:
      strcpy(functionName, "Log Closure");
      break;
  }
}
