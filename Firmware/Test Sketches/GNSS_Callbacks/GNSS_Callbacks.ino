/*
  Logs NMEA incoming from ESP32 UART / ZED UART1 to SD, via UART task
  Logs RAWX and SFXBR incoming from ESP32 I2C / ZED I2C to SD, separate file, using callbacks

  We will pass all UART data from ZED-F9P to SPP. This should only be NMEA data, needed for
  SW Maps. Logging to SD happens in the UART task.

  We will pass all RAWX and SFXBR data through I2C and then log it to SD using callbacks.

  TODO
    Implement better valid date/time checks
    Turn off UBX on UART1
    How to handle the max log time? Is it max for both logs? I think so.
    (Fixed) During file date/time updates, we are only updating the NMEA file, not the UBX file
    (Fixed) Should we have a logUBX option instead? Ya. Log all UBX.

  This is working well at 4Hz with BT SPP connected, NMEA logging, and RXM logging. It begins to
  break around the 13Hz mark. Disabling NMEA logging helps.
  Working well at 400kHz I2C.
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
const byte PIN_MICROSD_CHIP_SELECT = 25;
const int zed_tx_ready = 26;
const int zed_reset = 27;
const int batteryLevelLED_Red = 32;
const int batteryLevelLED_Green = 33;
const int batteryLevel_alert = 36;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <Wire.h> //Needed for I2C to GPS

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS i2cGNSS; //I2C for config and UBX messages
//SFE_UBLOX_GPS myGPS;

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

SFE_UBLOX_GNSS_ADD i2cGPS;

//This string is used to verify the firmware on the ZED-F9P. This
//firmware relies on various features of the ZED and may require the latest
//u-blox firmware to work correctly. We check the module firmware at startup but
//don't prevent operation if firmware is mismatched.
char latestZEDFirmware[] = "FWVER=HPG 1.13";

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
uint8_t settingPayload[MAX_PAYLOAD_SIZE];
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


uint8_t gnss_SIV = 0;

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#include "esp_bt.h" //Core access is needed for BT stop. See customBTstop() for more info.
#include "esp_gap_bt_api.h" //Needed for setting of pin. See issue: https://github.com/sparkfun/SparkFun_RTK_Surveyor/issues/5

HardwareSerial serialGNSS(2); //Serial connection between ESP32 to ZED UART1
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 4096 //Reduced from 16384 to make room for WiFi/NTRIP server capabilities
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
TaskHandle_t F9PSerialWriteTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode

//Reduced stack size from 10,000 to 1,000 to make room for WiFi/NTRIP server capabilities
//Increase stacks to 2k to allow for dual file write of NMEA/UBX using built in SD library
const int readTaskStackSize = 2000;
const int writeTaskStackSize = 2000;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
//#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
//SdFat sd;
//SdFile nmeaFile; //File that all gnss nmea setences are written to
//SdFile ubxFile; //File that all gnss nmea setences are written to

#include <SD.h>
File nmeaFile; //File that all gnss nmea setences are written to
File ubxFile; //File that all gnss nmea setences are written to

char settingsFileName[40] = "SFE_Surveyor_Settings.txt"; //File to read/write system settings to

unsigned long lastNMEALogSyncTime = 0; //Used to record to SD every half second
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second

long startLogTime_minutes = 0; //Mark when we start logging so we can stop logging after maxLogTime_minutes
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


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

uint32_t lastTime = 0;

uint32_t lastUbxCountUpdate = 0;
uint32_t ubxCount = 0;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


//void printPVTdata(UBX_NAV_PVT_data_t ubxDataStruct)
//{
//  gnss_SIV = ubxDataStruct.numSV;
//
//  long latitude = ubxDataStruct.lat; // Print the latitude
//  Serial.print(F(" Lat: "));
//  Serial.print(latitude);
//
//  long longitude = ubxDataStruct.lon; // Print the longitude
//  Serial.print(F(" Long: "));
//  Serial.print(longitude);
//  Serial.print(F(" (degrees * 10^-7)"));
//
//  Serial.println();
//}

#define sdWriteSize 512 // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage
//#define fileBufferSize 2048 // Allocate 16KBytes of RAM for UBX message storage

void newSFRBX(UBX_RXM_SFRBX_data_t ubxDataStruct)
{
  //Do nothing, but we have to have callbacks in place to obtain RXM messages
}

void newRAWX(UBX_RXM_RAWX_data_t ubxDataStruct)
{
}


void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("SparkFun u-blox Example");

  serialGNSS.begin(115200);
  serialGNSS.setRxBufferSize(SERIAL_SIZE_RX);
  serialGNSS.setTimeout(1);

  Wire.begin();
  Wire.setClock(400000);

  //Setup for this test
  //Push NMEA to one file, RXM to other file
  settings.logNMEA = true;
  settings.logUBX = true;
  settings.logRAWX = true;
  settings.logSFRBX = true;

  beginSD(); //Test if SD is present

  beginBluetooth(); //Get MAC, start radio

  beginGNSS(); //Connect and configure ZED-F9P

  i2cGNSS.setAutoRXMSFRBXcallback(&newSFRBX); // Enable automatic RXM SFRBX messages with callback to newSFRBX
  i2cGNSS.logRXMSFRBX(); // Enable RXM SFRBX data logging
  i2cGNSS.setAutoRXMRAWXcallback(&newRAWX); // Enable automatic RXM RAWX messages with callback to newRAWX
  i2cGNSS.logRXMRAWX(); // Enable RXM RAWX data logging

  Serial.flush(); //Complete any previous prints
  serialGNSS.flush();

  if (online.microSD == true && settings.logNMEA == true) startLogTime_minutes = 0; //Mark now as start of logging
}

void loop()
{
  i2cGNSS.checkUblox(); // Check for the arrival of new data and process it.
  i2cGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  updateLogs(); //Record any new UBX data. Create files or close files as needed.

  delay(1);
}

void updateLogs()
{
  if (online.ubxLogging == false && settings.logUBX == true)
  {
    beginLoggingUBX();
  }
  else if (online.ubxLogging == true && settings.logUBX == false)
  {
    //Close down file
    //ubxFile.sync();
    ubxFile.flush();
    ubxFile.close();
    online.ubxLogging = false;
  }
  
  if (online.nmeaLogging == false && settings.logNMEA == true)
  {
    beginLoggingNMEA();
  }
  else if (online.nmeaLogging == true && settings.logNMEA == false)
  {
    //Close down file
    //nmeaFile.sync();
    nmeaFile.flush();
    nmeaFile.close();
    online.nmeaLogging = false;
  }



  if (online.ubxLogging == true)
  {
    if (millis() - lastUbxCountUpdate > 1000)
    {
      lastUbxCountUpdate = millis();
      Serial.printf("UBX byte total / kB/s: %d / %0.02f\n", ubxCount, (float)ubxCount / millis());
    }

    while (i2cGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
    {
      Serial.println("1");
      uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

      i2cGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

      Serial.println("2");
      ubxFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

      // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
      Serial.println("3");
      i2cGNSS.checkUblox(); // Check for the arrival of new data and process it.
      i2cGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

      ubxCount += sdWriteSize;
    }

    //Force sync every 500ms
    if (millis() - lastUBXLogSyncTime > 500)
    {
      Serial.println("Sync");
      lastUBXLogSyncTime = millis();

      ubxFile.flush();
//      ubxFile.sync();

      if (settings.frequentFileAccessTimestamps == true)
        updateDataFileAccess(&ubxFile); // Update the file access time & date
    }
  }
}
