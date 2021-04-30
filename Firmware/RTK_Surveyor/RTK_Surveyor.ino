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

    Fix file date/time updates
    Test firmware loading
    Test file listing from debug menu
    Test file creation from cold start (date/time invalid)

*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 2;

//Define the RTK Surveyor board identifier:
//  This is an int which is unique to this variant of the RTK Surveyor and which allows us
//  to make sure that the settings in EEPROM are correct for this version of the RTK Surveyor
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
//#include <SD.h> //Use built-in ESP32 SD library. Not as fast as SdFat but works better with dual file access.

SdFat sd;
SPIClass spi = SPIClass(VSPI); //We need to pass the class into SD.begin so we can set the SPI freq in beginSD()
#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, DEDICATED_SPI, SD_SCK_MHZ(36), &spi)

SdFile nmeaFile; //File that all gnss nmea setences are written to
SdFile ubxFile; //File that all gnss ubx messages setences are written to

char settingsFileName[40] = "SFE_Surveyor_Settings.txt"; //File to read/write system settings to

unsigned long lastNMEALogSyncTime = 0; //Used to record to SD every half second
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second

long startLogTime_minutes = 0; //Mark when we start logging so we can stop logging after maxLogTime_minutes

//SdFat crashes when F9PSerialReadTask() is called in the middle of a ubx file write within loop()
//So we use a semaphore to see if SPI is available before
SemaphoreHandle_t xFATSemaphore;
const int fatSemaphore_maxWait = 5; //TickType_t

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

#include <Wire.h> //Needed for I2C to GNSS

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
//SFE_UBLOX_GNSS i2cGNSS;

// Extend the class for getModuleInfo - See Example21_ModuleInfo
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

//This string is used to verify the firmware on the ZED-F9P. This
//firmware relies on various features of the ZED and may require the latest
//u-blox firmware to work correctly. We check the module firmware at startup but
//don't prevent operation if firmware is mismatched.
char latestZEDFirmware[] = "FWVER=HPG 1.13";

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
uint8_t settingPayload[MAX_PAYLOAD_SIZE];

#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define gnssFileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage
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

HardwareSerial GPS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 4096 //Reduced from 16384 to make room for WiFi/NTRIP server capabilities
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
TaskHandle_t F9PSerialWriteTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode

TaskHandle_t startUART2TaskHandle = NULL; //Dummy task to start UART2 on core 0.
bool uart2Started = false;

//Reduced stack size from 10,000 to 1,000 to make room for WiFi/NTRIP server capabilities
//Increase stacks to 2k to allow for dual file write of NMEA/UBX using built in SD library
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

uint32_t lastRoverUpdate = 0;
uint32_t lastBaseUpdate = 0;
uint32_t lastBattUpdate = 0;
uint32_t lastDisplayUpdate = 0;

uint32_t lastFileReport = 0; //When logging, print file record stats every few seconds

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

  if (online.microSD == true && settings.logNMEA == true) startLogTime_minutes = 0; //Mark now as start of logging
  if (online.microSD == true && settings.logUBX == true) startLogTime_minutes = 0; //Mark now as start of logging

  //Start tasks
  btLEDTask.attach(btLEDTaskPace, updateBTled); //Rate in seconds, callback
  //battCheckTask.attach(battCheckTaskPace, checkBatteryLevels);

  //i2cGNSS.enableDebugging(); //Enable debug messages over Serial (default)
}

void loop()
{
  i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
  i2cGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH && baseState != BASE_OFF)
  {
    //Configure for rover mode
    Serial.println(F("Rover Mode"));

    baseState = BASE_OFF;

    beginBluetooth(); //Restart Bluetooth with 'Rover' name

    //If we are survey'd in, but switch is rover then disable survey
    if (configureUbloxModuleRover() == false)
    {
      Serial.println(F("Rover config failed!"));
    }

    digitalWrite(baseStatusLED, LOW);
  }
  else if (digitalRead(baseSwitch) == LOW && baseState == BASE_OFF)
  {
    //Configure for base mode
    Serial.println(F("Base Mode"));

    if (configureUbloxModuleBase() == false)
    {
      Serial.println(F("Base config failed!"));
    }

    //Restart Bluetooth with 'Base' name
    //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
    beginBluetooth();

    baseState = BASE_SURVEYING_IN_NOTSTARTED; //Switch to new state
  }

  if (baseState == BASE_SURVEYING_IN_NOTSTARTED || baseState == BASE_SURVEYING_IN_SLOW || baseState == BASE_SURVEYING_IN_FAST)
  {
    updateSurveyInStatus();
  }
  else if (baseState == BASE_TRANSMITTING)
  {
    if (settings.enableNtripServer == true)
    {
      updateNtripServer();
    }
  }
  else if (baseState == BASE_OFF)
  {
    updateRoverStatus();
  }

  updateBattLEDs();

  updateDisplay();

  updateRTC(); //Set system time to GNSS once we have fix

  updateLogs(); //Record any new UBX data. Create files or close NMEA and UBX files as needed.

  //Menu system via ESP32 USB connection
  if (Serial.available()) menuMain(); //Present user menu

  //Convert current system time to minutes. This is used in F9PSerialReadTask()/updateLogs() to see if we are within max log window.
  systemTime_minutes = millis() / 1000L / 60;

  delay(10); //A small delay prevents panic if no other I2C or functions are called
}

void updateDisplay()
{

  //Update the display if connected
  if (online.display == true)
  {
    if (millis() - lastDisplayUpdate > 1000)
    {
      lastDisplayUpdate = millis();

      oled.clear(PAGE); // Clear the display's internal buffer

      //Current battery charge level
      if (battLevel < 25)
        oled.drawIcon(45, 0, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
      else if (battLevel < 50)
        oled.drawIcon(45, 0, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
      else if (battLevel < 75)
        oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);
      else //batt level > 75
        oled.drawIcon(45, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);

      //Bluetooth Address or RSSI
      if (radioState == BT_CONNECTED)
      {
        oled.drawIcon(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol, sizeof(BT_Symbol), true);
      }
      else
      {
        char macAddress[5];
        sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
        oled.setFontType(0); //Set font to smallest
        oled.setCursor(0, 4);
        oled.print(macAddress);
      }

      if (digitalRead(baseSwitch) == LOW)
        oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true); //true - blend with other pixels
      else
        oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);

      //Horz positional accuracy
      oled.setFontType(1); //Set font to type 1: 8x16
      oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
      oled.setCursor(16, 20); //x, y
      oled.print(":");
      float hpa = i2cGNSS.getHorizontalAccuracy() / 10000.0;
      if (hpa > 30.0)
      {
        oled.print(F(">30"));
      }
      else if (hpa > 9.9)
      {
        oled.print(hpa, 1); //Print down to decimeter
      }
      else if (hpa > 1.0)
      {
        oled.print(hpa, 2); //Print down to centimeter
      }
      else
      {
        oled.print("."); //Remove leading zero
        oled.printf("%03d", (int)(hpa * 1000)); //Print down to millimeter
      }

      //SIV
      oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
      oled.setCursor(16, 36); //x, y
      oled.print(":");

      if (i2cGNSS.getFixType() == 0) //0 = No Fix
      {
        oled.print("0");
      }
      else
      {
        oled.print(i2cGNSS.getSIV());
      }

      oled.display();
    }
  }
}

//Create or close UBX/NMEA files as needed (startup or as user changes settings)
//Push new UBX packets to log as needed
void updateLogs()
{
  if (online.nmeaLogging == false && settings.logNMEA == true)
  {
    beginLoggingNMEA();
  }
  else if (online.nmeaLogging == true && settings.logNMEA == false)
  {
    //Close down file
    nmeaFile.sync();
    nmeaFile.close();
    online.nmeaLogging = false;
  }

  if (online.ubxLogging == false && settings.logUBX == true)
  {
    beginLoggingUBX();
  }
  else if (online.ubxLogging == true && settings.logUBX == false)
  {
    //Close down file
    ubxFile.sync();
    ubxFile.close();
    online.ubxLogging = false;
  }

  if (online.ubxLogging == true)
  {
    //Check if we are inside the max time window for logging
    if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
    {
      while (i2cGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
      {
        //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
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
        i2cGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
      }
    }
  }

  //Report file sizes to show recording is working
  if (online.nmeaLogging == true || online.ubxLogging == true)
  {
    if (millis() - lastFileReport > 5000)
    {
      lastFileReport = millis();
      if (online.nmeaLogging == true && online.ubxLogging == true)
        Serial.printf("UBX and NMEA file size: %d / %d", ubxFile.fileSize(), nmeaFile.fileSize());
      else if (online.nmeaLogging == true)
        Serial.printf("NMEA file size: %d", nmeaFile.fileSize());
      else if (online.ubxLogging == true)
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
