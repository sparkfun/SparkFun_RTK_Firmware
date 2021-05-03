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

  The main loop handles lower priority updates such as:
    Fuel gauge checking and power LED color update
    Setup switch monitoring (module configure between Rover and Base)
    Text menu interactions

  Main Menu (Display MAC address / broadcast name):
    (Done) GNSS - Configure measurement rate, enable/disable common NMEA sentences, RAWX, SBAS
    (Done) Log - Log to SD
    (Done) Base - Enter fixed coordinates, survey-in settings, WiFi/Caster settings,
    (Done) Ports - Configure Radio and Data port baud rates
    (Done) Test menu
    (Done) Firmware upgrade menu
    Enable various debug outputs sent over BT

    Test file date/time updates
    Test firmware loading
    Add blinking log file icon
*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 2;

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
const int positionAccuracyLED_1cm = 2;
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 12;
const int positionAccuracyLED_100cm = 13;
const int positionAccuracyLED_10cm = 15;
const int PIN_MICROSD_CHIP_SELECT = 25;
const int zed_tx_ready = 26;
const int zed_reset = 27;
const int batteryLevelLED_Red = 32;
const int batteryLevelLED_Green = 33;
const int batteryLevel_alert = 36;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//I2C for GNSS, battery gauge, display, accelerometer
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//EEPROM for storing settings
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <EEPROM.h>
#define EEPROM_SIZE 2048 //ESP32 emulates EEPROM in non-volatile storage (external flash IC). Max is 508k.
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
#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, DEDICATED_SPI, SD_SCK_MHZ(36), &spi)

char settingsFileName[40] = "SFE_Surveyor_Settings.txt"; //File to read/write system settings to

SdFile ubxFile; //File that all gnss ubx messages setences are written to
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second
long startLogTime_minutes = 0; //Mark when we start logging so we can stop logging after maxLogTime_minutes

//SdFat crashes when F9PSerialReadTask() is called in the middle of a ubx file write within loop()
//So we use a semaphore to see if file system is available
SemaphoreHandle_t xFATSemaphore;
const int fatSemaphore_maxWait = 5; //TickType_t

#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Connection settings to NTRIP Caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <WiFi.h>
WiFiClient caster;
const char * ntrip_server_name = "SparkFun_RTK_Surveyor";

unsigned long lastServerSent_ms = 0; //Time of last data pushed to caster
unsigned long lastServerReport_ms = 0; //Time of last report of caster bytes sent
int maxTimeBeforeHangup_ms = 10000; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

uint32_t serverBytesSent = 0; //Just a running total
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS i2cGNSS;

//Note: There are two prevalent versions of the ZED-F9P: v1.12 (part# -01B) and v1.13 (-02B).
//v1.13 causes the RTK LED to not function if SBAS is enabled. To avoid this, we
//disable SBAS by default.

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes
uint8_t settingPayload[MAX_PAYLOAD_SIZE];

#define gnssFileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage

TaskHandle_t F9PI2CTaskHandle = NULL; //Task for regularly checking I2C
const int i2cTaskStackSize = 2000;
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
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#include "esp_bt.h" //Core access is needed for BT stop. See customBTstop() for more info.
#include "esp_gap_bt_api.h" //Needed for setting of pin. See issue: https://github.com/sparkfun/SparkFun_RTK_Surveyor/issues/5

HardwareSerial serialGNSS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 4096 //Reduced from 16384 to make room for WiFi/NTRIP server capabilities
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
TaskHandle_t F9PSerialWriteTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode

TaskHandle_t startUART2TaskHandle = NULL; //Dummy task to start UART2 on core 0.
bool uart2Started = false;

//Reduced stack size from 10,000 to 2,000 to make room for WiFi/NTRIP server capabilities
const int readTaskStackSize = 2000;
const int writeTaskStackSize = 2000;
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

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
char deviceName[20]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'
const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds
bool inTestMode = false; //Used to re-route BT traffic while in test sub menu
long systemTime_minutes = 0; //Used to test if logging is less than max minutes

uint32_t lastBattUpdate = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastSystemStateUpdate = 0;

uint32_t lastFileReport = 0; //When logging, print file record stats every few seconds
long lastStackReport = 0; //Controls the report rate of stack highwater mark within a task

uint32_t lastSatelliteDishIconUpdate = 0;
bool satelliteDishIconDisplayed = false; //Toggles as lastSatelliteDishIconUpdate goes above 1000ms
uint32_t lastCrosshairIconUpdate = 0;
bool crosshairIconDisplayed = false; //Toggles as lastCrosshairIconUpdate goes above 1000ms
uint32_t lastBaseIconUpdate = 0;
bool baseIconDisplayed = false; //Toggles as lastSatelliteDishIconUpdate goes above 1000ms
uint32_t lastWifiIconUpdate = 0;
bool wifiIconDisplayed = false; //Toggles as lastWifiIconUpdate goes above 1000ms

uint32_t lastRTCMPacketSent = 0; //Used to count RTCM packets sent during base mode
uint32_t rtcmPacketsSent = 0; //Used to count RTCM packets sent during base mode

uint32_t casterResponseWaitStartTime = 0; //Used to detect if caster service times out

uint32_t maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds

uint32_t lastBaseLEDupdate = 0; //Controls the blinking of the Base LED

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging
  Serial.setRxBufferSize(SERIAL_SIZE_RX);
  Serial.setTimeout(1);

  Wire.begin(); //Start I2C on core 1
  Wire.setClock(400000);

  beginUART2(); //Start UART2 on core 0

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

  beginDisplay(); //Check if an external Qwiic OLED is attached
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  beginFuelGauge(); //Configure battery fuel guage monitor
  checkBatteryLevels(); //Force display so you see battery level immediately at power on

  beginBluetooth(); //Get MAC, start radio

  beginGNSS(); //Connect and configure ZED-F9P

  Serial.flush(); //Complete any previous prints

  danceLEDs(); //Turn on LEDs like a car dashboard

  //i2cGNSS.enableDebugging(); //Enable debug messages over Serial (default)
}

void loop()
{
  i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  checkSetupButton(); //Change system state as needed

  updateSystemState();

  updateBattLEDs();

  updateDisplay();

  updateRTC(); //Set system time to GNSS once we have fix

  updateLogs(); //Record any new data. Create or close files as needed.

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
  if (online.logging == false && logMessages() == true)
  {
    beginLogging();
  }
  else if (online.logging == true && logMessages() == false)
  {
    //Close down file
    ubxFile.sync();
    ubxFile.close();
    online.logging = false;
  }

  if (online.logging == true)
  {
    //Check if we are inside the max time window for logging
    if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
    {
      while (i2cGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
      {
        //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile()
        if (xSemaphoreTake(xFATSemaphore, fatSemaphore_maxWait) == pdPASS)
        {
          uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

          i2cGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

          int bytesWritten = ubxFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

          //Force sync every 1000ms
          if (millis() - lastUBXLogSyncTime > 1000)
          {
            lastUBXLogSyncTime = millis();
            digitalWrite(baseStatusLED, !digitalRead(baseStatusLED)); //Blink LED to indicate logging activity
            ubxFile.sync();
            digitalWrite(baseStatusLED, !digitalRead(baseStatusLED)); //Return LED to previous state
          }

          xSemaphoreGive(xFATSemaphore);
        }

        // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
        i2cGNSS.checkUblox(); // Check for the arrival of new data and process it.
      }
    }
  }

  //Report file sizes to show recording is working
  if (online.logging == true)
  {
    if (millis() - lastFileReport > 5000)
    {
      lastFileReport = millis();
      Serial.printf("UBX file size: %d", ubxFile.fileSize());

      if ((systemTime_minutes - startLogTime_minutes) > settings.maxLogTime_minutes)
        Serial.printf(" reached max log time %d / System time %d",
                      settings.maxLogTime_minutes,
                      (systemTime_minutes - startLogTime_minutes));

      Serial.println();
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
        //For the ESP32 SD library, the date/time stamp of files is set using the internal system time
        //This is normally set with WiFi NTP but we will rarely have WiFi
        rtc.setTime(i2cGNSS.getSecond(), i2cGNSS.getMinute(), i2cGNSS.getHour(), i2cGNSS.getDay(), i2cGNSS.getMonth(), i2cGNSS.getYear());  // 17th Jan 2021 15:24:30

        online.rtc = true;

        Serial.print(F("System time set to: "));
        Serial.println(rtc.getTime("%B %d %Y %H:%M:%S")); //From ESP32Time library example
      }
    }
  }
}
