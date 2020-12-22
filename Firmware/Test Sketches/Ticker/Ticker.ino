/*
  Use Ticker library to periodically check:
  Blink BT LED
  Check battery
  Update display
  Check GPS

  Update GPS data - We are going to use autoPVT and setAutoHPPOSLLH with Explicit Update. We will checkUblox() every 100ms.
  This will cause all dataums (SIV, HPA, etc) to update regularly but when we call getSIV() we will not force wait for the most recent
  data. This will also have the added benefit of regularly feeding processRTCM. Every checkUblox() call will pass any waiting RTCM
  bytes to a future NTRIP server.

  Because tickers can interrupt other tickers, we use a mutex semphore for the I2C hardware.

  This implementation still has display errors for unknown reasons. Going to try xTasks next.
*/


const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 1;

#include "settings.h"

SemaphoreHandle_t xI2CSemaphore;
const int i2cSemaphore_maxWait = 5; //TickType_t

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

//Low frequency tasks
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <Ticker.h>

Ticker checkUbloxTask;
float checkUbloxTaskPace = 0.1; //Seconds

Ticker btLEDTask;
float btLEDTaskPace = 0.5; //Seconds

Ticker updateDisplayTask;
float updateDisplayTaskPace = 0.5; //Seconds

Ticker battCheckTask;
float battCheckTaskPace = 2.0; //Seconds
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

#include "SparkFun_Ublox_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Ublox_GPS
//SFE_UBLOX_GPS myGPS;

// Extend the class for getModuleInfo - See Example21_ModuleInfo
class SFE_UBLOX_GPS_ADD : public SFE_UBLOX_GPS
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

SFE_UBLOX_GPS_ADD myGPS;

//This string is used to verify the firmware on the ZED-F9P. This
//firmware relies on various features of the ZED and may require the latest
//u-blox firmware to work correctly. We check the module firmware at startup but
//don't prevent operation if firmware is mismatched.
char latestZEDFirmware[] = "FWVER=HPG 1.13";

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
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


void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Testing tasks");

  pinMode(bluetoothStatusLED, OUTPUT);

  Wire.begin();
  //Wire.setClock(100000);

  beginLEDs(); //LED and PWM setup

  beginDisplay(); //Check if an external Qwiic OLED is attached

  beginFuelGauge(); //Start I2C device

  beginGNSS(); //Connect and configure ZED-F9P

  bluetoothState = BT_ON_NOCONNECTION;

  //Wire.setClock(400000); //The battery and ublox modules seem to dislike 400kHz

  if (xI2CSemaphore == NULL)
  {
    xI2CSemaphore = xSemaphoreCreateMutex();
    if (xI2CSemaphore != NULL)
      xSemaphoreGive(xI2CSemaphore);  //Make the I2C hardware available for use
  }

  //Start tasks
  btLEDTask.attach(btLEDTaskPace, updateBTled); //Rate in seconds, callback
  battCheckTask.attach(battCheckTaskPace, checkBatteryLevels);
  checkUbloxTask.attach(checkUbloxTaskPace, checkUblox); //Call the checkUblox function

  if (online.display == true)
    updateDisplayTask.attach(updateDisplayTaskPace, updateDisplay);

}

void loop() {

}
