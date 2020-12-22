/*

*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 6;

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

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
SdFat sd;
SdFile sensorDataFile; //File that all sensor data is written to
SdFile serialDataFile; //File that all incoming serial data is written to
//#define PRINT_LAST_WRITE_TIME // Uncomment this line to enable the 'measure the time between writes' diagnostic

char sensorDataFileName[30] = ""; //We keep a record of this file name so that we can re-open it upon wakeup from sleep
char serialDataFileName[30] = ""; //We keep a record of this file name so that we can re-open it upon wakeup from sleep
const int sdPowerDownDelay = 100; //Delay for this many ms before turning off the SD card power
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include <Wire.h> //Needed for I2C to GPS

//GPS configuration
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

uint8_t gnssUpdateRate = 4; //Increasing beyond 1Hz with SV sentence on can drown the BT link
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Battery fuel gauge and PWM LEDs
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048); // Create a MAX17048

// setting PWM properties
const int freq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int resolution = 8;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

HardwareSerial GPS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 16384 //Using a large buffer. This might be much bigger than needed but the ESP32 has enough RAM
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//Freeze and blink LEDs if we hit a bad error
typedef enum
{
  ERROR_NO_I2C = 2, //Avoid 0 and 1 as these are bad blink codes
  ERROR_GPS_CONFIG_FAIL,
} t_errorNumber;

//Bluetooth status LED goes from off (LED off), no connection (blinking), to connected (solid)
enum BluetoothState
{
  BT_OFF = 0,
  BT_ON_NOCONNECTION,
  BT_CONNECTED,
};
volatile byte bluetoothState = BT_OFF;

//Base status goes from Rover-Mode (LED off), surveying in (blinking), to survey is complete/trasmitting RTCM (solid)
typedef enum
{
  BASE_OFF = 0,
  BASE_SURVEYING_IN_NOTSTARTED, //User has indicated base, but current pos accuracy is too low
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
} BaseState;
volatile BaseState baseState = BASE_OFF;
unsigned long baseStateBlinkTime = 0;
const unsigned long maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds

//Return values for getByteChoice()
enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X,
};

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
char deviceName[20]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'
const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds
bool inTestMode = false; //Used to re-route BT traffic while in test sub menu
int battLevel = 0; //SOC measured from fuel gauge, in %

uint32_t lastBluetoothLEDBlink = 0;
uint32_t lastRoverUpdate = 0;
uint32_t lastBaseUpdate = 0;
uint32_t lastBattUpdate = 0;
uint32_t lastDisplayUpdate = 0;

uint32_t lastTime = 0;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging
  Serial.setRxBufferSize(SERIAL_SIZE_RX);
  Serial.setTimeout(1);

  GPS.begin(115200); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at 115200bps.
  GPS.setRxBufferSize(SERIAL_SIZE_RX);
  GPS.setTimeout(1);

  Wire.begin();

  Serial.println("SparkFun RTK Surveyor v1.0");

  pinMode(positionAccuracyLED_1cm, OUTPUT);
  pinMode(positionAccuracyLED_10cm, OUTPUT);
  pinMode(positionAccuracyLED_100cm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  digitalWrite(positionAccuracyLED_1cm, LOW);
  digitalWrite(positionAccuracyLED_10cm, LOW);
  digitalWrite(positionAccuracyLED_100cm, LOW);
  digitalWrite(baseStatusLED, LOW);
  digitalWrite(bluetoothStatusLED, LOW);

//  ledcSetup(ledRedChannel, freq, resolution);
//  ledcSetup(ledGreenChannel, freq, resolution);
//
//  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
//  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);
//
//  ledcWrite(ledRedChannel, 0);
//  ledcWrite(ledGreenChannel, 0);
//
//  setupLiPo(); //Configure battery fuel guage monitor
//  checkBatteryLevels(); //Display initial battery level

  //Get unit MAC address
//  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
//  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  //Check if an external Qwiic OLED is attached
  beginDisplay();
  if (online.display == true)
  {
    Serial.println(F("Display online"));

    //Display splash of some sort
    //oled.drawIcon(1, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
    oled.display();
  }

  //danceLEDs(); //Turn on LEDs like a car dashboard

  //myGPS.enableDebugging(); //Enable debug messages over Serial (default)
}

void loop()
{
  updateDisplay();

  delay(250); //Required if no other I2C or functions are called
}

int counter = 1;

void updateDisplay()
{
    oled.setFontType(1); //Set font to type 1: 8x16

    //3D Mean Accuracy
    oled.setCursor(17, 19); //x, y: Squeeze against the colon
    oled.print(":");
    oled.print(counter++);
    oled.display();

  //Update the display if connected
//  if (online.display == true)
//  {
//    if (millis() - lastDisplayUpdate > 1000)
//    {
//      lastDisplayUpdate = millis();
//      Serial.println("Display update");

      //oled.setFontType(1);
//      oled.setFontType(0); //Set font to smallest
//      oled.setCursor(0, 4);
//      oled.print("O");
//      oled.display();

      //while(1);
    //}
  //}
}

void beginSD()
{
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if (sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24)) == false) //Standard SdFat
    {
      printDebug("SD init failed (first attempt). Trying again...\r\n");
      //Give SD more time to power up, then try again
      delay(250);
      if (sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24)) == false) //Standard SdFat
      {
        Serial.println(F("SD init failed (second attempt). Is card present? Formatted?"));
        digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
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

    online.microSD = true;
  }
  else
  {
    online.microSD = false;
  }
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    online.display = true;

    //Init display
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory
    oled.clear(ALL);  // Clear the library's display buffer
  }
}
