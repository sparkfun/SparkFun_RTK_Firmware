/*
  September 1st, 2020
  SparkFun Electronics
  Nathan Seidle

  This firmware runs the core of the SparkFun RTK Surveyor product. It runs on an ESP32
  and communicates with the ZED-F9P.

  Select the ESP32 Dev Module from the boards list. This maps the same pins to the ESP32-WROOM module.
  Select 'Minimal SPIFFS (1.9MB App)' from the partition list. This will enable SD firmware updates.

  Special thanks to Avinab Malla for guidance on getting xTasks implemented.

  The RTK Surveyor implements classic Bluetooth SPP to transfer data from the
  ZED-F9P to the phone and receive any RTCM from the phone and feed it back
  to the ZED-F9P to achieve RTK: F9PSerialWriteTask(), F9PSerialReadTask().

  A settings file is accessed on microSD if available otherwise settings are pulled from
  ESP32's emulated EEPROM.

  As of v1.2, the heap is approximately 94072 during Rover Fix, 142260 during WiFi Casting. This is
  important to maintain as unit will begin to have stability issues at ~30k.

  The main loop handles lower priority updates such as:
    Fuel gauge checking and power LED color update
    Setup switch monitoring (module configure between Rover and Base)
    Text menu interactions

  Main Menu (Display MAC address / broadcast name):
    (Done) GNSS - Configure measurement rate, SBAS
    (Done) Log - Control messages logged to SD
    (Done) Broadcast - Control messages sent over BT SPP
    (Done) Base - Enter fixed coordinates, survey-in settings, WiFi/Caster settings,
    (Done) Ports - Configure Radio and Data port baud rates
    (Done) Test menu
    (Done) Firmware upgrade menu
    Enable various debug outputs sent over BT

*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 4;

//Define the RTK board identifier:
//  This is an int which is unique to this variant of the RTK Surveyor hardware which allows us
//  to make sure that the settings in EEPROM are correct for this version of the RTK
//  (sizeOfSettings is not necessarily unique and we want to avoid problems when swapping from one variant to another)
//  It is the sum of:
//    the major firmware version * 0x10
//    the minor firmware version
#define RTK_IDENTIFIER (FIRMWARE_VERSION_MAJOR * 0x10 + FIRMWARE_VERSION_MINOR)

#include "settings.h"

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//These pins are set in beginBoard()
int pin_batteryLevelLED_Red;
int pin_batteryLevelLED_Green;
int pin_positionAccuracyLED_1cm;
int pin_positionAccuracyLED_10cm;
int pin_positionAccuracyLED_100cm;
int pin_baseStatusLED;
int pin_bluetoothStatusLED;
int pin_baseSwitch;
int pin_microSD_CS;
int pin_zed_tx_ready;
int pin_zed_reset;
int pin_batteryLevel_alert;

int pin_muxA;
int pin_muxB;
int pin_powerSenseAndControl;
int pin_setupButton;
int pin_powerFastOff;
int pin_dac26;
int pin_adc39;
int pin_peripheralPowerControl;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//I2C for GNSS, battery gauge, display, accelerometer
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//EEPROM for storing settings
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <EEPROM.h>
#define EEPROM_SIZE 4096 //ESP32 emulates EEPROM in non-volatile storage (external flash IC). Max is 508k.
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Handy library for setting ESP32 system time to GNSS time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <ESP32Time.h> //http://librarymanager/All#ESP32Time
ESP32Time rtc;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include "SdFat.h"

SdFat sd;
SPIClass spi = SPIClass(VSPI); //We need to pass the class into SD.begin so we can set the SPI freq in beginSD()

char platformFilePrefix[40] = "SFE_Surveyor"; //Sets the prefix for logs and settings files

SdFile ubxFile; //File that all gnss ubx messages setences are written to
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second
int startLogTime_minutes = 0; //Mark when we start logging so we can stop logging after maxLogTime_minutes

//System crashes if two tasks access a file at the same time
//So we use a semaphore to see if file system is available
SemaphoreHandle_t xFATSemaphore;
const TickType_t fatSemaphore_shortWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_longWait_ms = 200 / portTICK_PERIOD_MS;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Connection settings to NTRIP Caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <WiFi.h>
#include "esp_wifi.h" //Needed for init/deinit of resources to free up RAM

WiFiClient caster;
const char * ntrip_server_name = "SparkFun_RTK_Surveyor";

unsigned long lastServerSent_ms = 0; //Time of last data pushed to caster
unsigned long lastServerReport_ms = 0; //Time of last report of caster bytes sent
int maxTimeBeforeHangup_ms = 10000; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

uint32_t casterBytesSent = 0; //Just a running total
uint32_t casterResponseWaitStartTime = 0; //Used to detect if caster service times out
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

//Note: There are two prevalent versions of the ZED-F9P: v1.12 (part# -01B) and v1.13 (-02B).
//v1.13 causes the RTK LED to not function if SBAS is enabled. To avoid this, we
//disable SBAS by default.

char zedFirmwareVersion[20]; //The string looks like 'FWVER=HPG 1.12'. Output to debug menu and settings file.

// Extend the class for getModuleInfo. Used to diplay ZED-F9P firmware version in debug menu.
class SFE_UBLOX_GNSS_ADD : public SFE_UBLOX_GNSS
{
  public:
    boolean getModuleInfo(uint16_t maxWait = 1100); //Queries module, texts

    struct minfoStructure // Structure to hold the module info (uses 341 bytes of RAM)
    {
      char swVersion[30];
      char hwVersion[10];
      uint8_t extensionNo = 0;
      char extension[10][30];
    } minfo;
};

SFE_UBLOX_GNSS_ADD i2cGNSS;

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes
uint8_t settingPayload[MAX_PAYLOAD_SIZE];
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Battery fuel gauge and PWM LEDs
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048);

// setting PWM properties
const int freq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int resolution = 8;

int battLevel = 0; //SOC measured from fuel gauge, in %. Used in multiple places (display, serial debug, log)
float battVoltage = 0.0;
float battChangeRate = 0.0;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//We use a local copy of the BluetoothSerial library so that we can increase the RX buffer. See issue: https://github.com/sparkfun/SparkFun_RTK_Surveyor/issues/18
#include "src/BluetoothSerial/BluetoothSerial.h"
BluetoothSerial SerialBT;
#include "esp_bt.h" //Core access is needed for BT stop. See customBTstop() for more info.
#include "esp_gap_bt_api.h" //Needed for setting of pin. See issue: https://github.com/sparkfun/SparkFun_RTK_Surveyor/issues/5

char platformPrefix[40] = "Surveyor"; //Sets the prefix for broadcast names

HardwareSerial serialGNSS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 4096 //Reduced from 16384 to make room for WiFi/NTRIP server capabilities
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading from F9P to SPP
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing from incoming SPP to F9P
TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
TaskHandle_t F9PSerialWriteTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode

TaskHandle_t startUART2TaskHandle = NULL; //Dummy task to start UART2 on core 0.
bool uart2Started = false;

//Reduced stack size from 10,000 to 2,000 to make room for WiFi/NTRIP server capabilities
const int readTaskStackSize = 2000;
const int writeTaskStackSize = 2000;

char incomingBTTest = 0; //Stores incoming text over BT when in test mode
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Firmware binaries loaded from SD
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <Update.h>
int binCount = 0;
char binFileNames[10][50];
const char* forceFirmwareFileName = "RTK_Surveyor_Firmware_Force.bin"; //File that will be loaded at startup regardless of user input
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Low frequency tasks
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <Ticker.h>

Ticker btLEDTask;
float btLEDTaskPace = 0.5; //Seconds

//Ticker battCheckTask;
//float battCheckTaskPace = 2.0; //Seconds
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Accelerometer for bubble leveling
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "SparkFun_LIS2DH12.h" //Click here to get the library: http://librarymanager/All#SparkFun_LIS2DH12
SPARKFUN_LIS2DH12 accel;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
char deviceName[20]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'
const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds
bool inTestMode = false; //Used to re-route BT traffic while in test sub menu
int systemTime_minutes = 0; //Used to test if logging is less than max minutes
uint32_t powerPressedStartTime = 0; //Times how long user has been holding power button, used for power down
uint8_t debounceDelay = 20; //ms to delay between button reads

uint32_t lastBattUpdate = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastSystemStateUpdate = 0;
uint32_t lastAccuracyLEDUpdate = 0;
uint32_t lastBaseLEDupdate = 0; //Controls the blinking of the Base LED

uint32_t lastFileReport = 0; //When logging, print file record stats every few seconds
long lastStackReport = 0; //Controls the report rate of stack highwater mark within a task
uint32_t lastHeapReport = 0; //Report heap every 1s if option enabled
uint32_t lastTaskHeapReport = 0; //Report task heap every 1s if option enabled
uint32_t lastCasterLEDupdate = 0; //Controls the cycling of position LEDs during casting

uint32_t lastSatelliteDishIconUpdate = 0;
bool satelliteDishIconDisplayed = false; //Toggles as lastSatelliteDishIconUpdate goes above 1000ms
uint32_t lastCrosshairIconUpdate = 0;
bool crosshairIconDisplayed = false; //Toggles as lastCrosshairIconUpdate goes above 1000ms
uint32_t lastBaseIconUpdate = 0;
bool baseIconDisplayed = false; //Toggles as lastBaseIconUpdate goes above 1000ms
uint32_t lastWifiIconUpdate = 0;
bool wifiIconDisplayed = false; //Toggles as lastWifiIconUpdate goes above 1000ms
uint32_t lastLoggingIconUpdate = 0;
int loggingIconDisplayed = 0; //Increases every 500ms while logging

uint64_t lastLogSize = 0;
bool logIncreasing = false; //Goes true when log file is greater than lastLogSize
bool reuseLastLog = false; //Goes true if we have a reset due to software (rather than POR)

uint32_t lastRTCMPacketSent = 0; //Used to count RTCM packets sent during base mode
uint32_t rtcmPacketsSent = 0; //Used to count RTCM packets sent via processRTCM()

uint32_t maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds

uint32_t totalWriteTime = 0; //Used to calculate overall write speed using SdFat library

bool setupByPowerButton = false; //We can change setup via tapping power button

uint16_t svinObservationTime = 0; //Use globals so we don't have to request these values multiple times (slow response)
float svinMeanAccuracy = 0;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging

  Wire.begin(); //Start I2C on core 1
  Wire.setClock(400000);

  beginBoard(); //Determine what hardware platform we are running on

  beginDisplay(); //Check if an external Qwiic OLED is attached

  beginLEDs(); //LED and PWM setup

  //Start EEPROM and SD for settings, and display for output
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  beginEEPROM();

  //eepromErase();

  beginSD(); //Test if SD is present
  if (online.microSD == true)
  {
    Serial.println(F("microSD online"));
    scanForFirmware(); //See if SD card contains new firmware that should be loaded at startup
  }

  loadSettings(); //Attempt to load settings after SD is started so we can read the settings file if available
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  beginUART2(); //Start UART2 on core 0, used to receive serial from ZED and pass out over SPP

  beginFuelGauge(); //Configure battery fuel guage monitor
  checkBatteryLevels(); //Force display so you see battery level immediately at power on

  beginGNSS(); //Connect and configure ZED-F9P

  beginAccelerometer();

  beginSystemState(); //Determine initial system state
  
  Serial.flush(); //Complete any previous prints

  danceLEDs(); //Turn on LEDs like a car dashboard
}

void loop()
{
  i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  checkButtons(); //Change system state as needed

  updateSystemState();

  updateBattLEDs();

  updateDisplay();

  updateRTC(); //Set system time to GNSS once we have fix

  updateLogs(); //Record any new data. Create or close files as needed.

  reportHeap(); //If debug enabled, report free heap

  //Menu system via ESP32 USB connection
  if (Serial.available()) menuMain(); //Present user menu

  //Convert current system time to minutes. This is used in F9PSerialReadTask()/updateLogs() to see if we are within max log window.
  systemTime_minutes = millis() / 1000L / 60;

  delay(10); //A small delay prevents panic if no other I2C or functions are called
}

//Create or close files as needed (startup or as user changes settings)
//Push new data to log as needed
void updateLogs()
{
  if (online.logging == false && settings.enableLogging == true)
  {
    beginLogging();
  }
  else if (online.logging == true && settings.enableLogging == false)
  {
    //Close down file
    if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      ubxFile.sync();
      ubxFile.close();
      online.logging = false;
      //xSemaphoreGive(xFATSemaphore); //Do not release semaphore
    }
  }

  //Report file sizes to show recording is working
  if (online.logging == true)
  {
    if (millis() - lastFileReport > 5000)
    {
      long fileSize = 0;

      //Attempt to access file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
      if (xSemaphoreTake(xFATSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
      {
        fileSize = ubxFile.fileSize();

        xSemaphoreGive(xFATSemaphore);
      }

      if (fileSize > 0)
      {
        lastFileReport = millis();
        Serial.printf("UBX file size: %ld", fileSize);

        if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
        {
          //Calculate generation and write speeds every 5 seconds
          uint32_t delta = fileSize - lastLogSize;
          Serial.printf(" - Generation rate: %0.1fkB/s", delta / 5.0 / 1000.0);
          Serial.printf(" - Write speed: %0.1fkB/s", delta / (totalWriteTime / 1000000.0) / 1000.0);
        }
        else
        {
          Serial.printf(" reached max log time %d", settings.maxLogTime_minutes);
        }

        Serial.println();

        totalWriteTime = 0; //Reset write time every 5s

        if (fileSize > lastLogSize)
        {
          lastLogSize = fileSize;
          logIncreasing = true;
        }
        else
          logIncreasing = false;
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
      if (i2cGNSS.getConfirmedDate() == true && i2cGNSS.getConfirmedTime() == true)
      {
        //Set the internal system time
        //This is normally set with WiFi NTP but we will rarely have WiFi
        rtc.setTime(i2cGNSS.getSecond(), i2cGNSS.getMinute(), i2cGNSS.getHour(), i2cGNSS.getDay(), i2cGNSS.getMonth(), i2cGNSS.getYear());  // 17th Jan 2021 15:24:30

        online.rtc = true;

        Serial.print(F("System time set to: "));
        Serial.println(rtc.getTime("%B %d %Y %H:%M:%S")); //From ESP32Time library example

        recordSystemSettingsToFile(); //This will re-record the setting file with current date/time.
      }
    }
  }
}
