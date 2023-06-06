/*
  Stay on and check each subsystem via I2C, SPI, and serial

  Board ID
  GNSS
  Display
  Battery Gauge
  Accelerometer
  SD

  No:
  Bluetooth
  WiFi

  TODO:
  L-Band
  LoRaSerial
*/

const int FIRMWARE_VERSION_MAJOR = 2;
const int FIRMWARE_VERSION_MINOR = 3;

#define COMPILE_WIFI //Comment out to remove WiFi functionality
#define COMPILE_BT //Comment out to remove Bluetooth functionality
#define COMPILE_AP //Comment out to remove Access Point functionality
#define ENABLE_DEVELOPER //Uncomment this line to enable special developer modes (don't check power button at startup)

//Define the RTK board identifier:
//  This is an int which is unique to this variant of the RTK Surveyor hardware which allows us
//  to make sure that the settings stored in flash (LittleFS) are correct for this version of the RTK
//  (sizeOfSettings is not necessarily unique and we want to avoid problems when swapping from one variant to another)
//  It is the sum of:
//    the major firmware version * 0x10
//    the minor firmware version
#define RTK_IDENTIFIER (FIRMWARE_VERSION_MAJOR * 0x10 + FIRMWARE_VERSION_MINOR)

#include "settings.h"

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
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3 v3.0.2

#define SENTENCE_TYPE_NMEA              DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_NMEA
#define SENTENCE_TYPE_NONE              DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_NONE
#define SENTENCE_TYPE_RTCM              DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_RTCM
#define SENTENCE_TYPE_UBX               DevUBLOXGNSS::SFE_UBLOX_SENTENCE_TYPE_UBX

char zedFirmwareVersion[20]; //The string looks like 'HPG 1.12'. Output to system status menu and settings file.
char neoFirmwareVersion[20]; //Output to system status menu.
uint8_t zedFirmwareVersionInt = 0; //Controls which features (constellations) can be configured (v1.12 doesn't support SBAS)
uint8_t zedModuleType = PLATFORM_F9P; //Controls which messages are supported and configured

//// Extend the class for getModuleInfo. Used to diplay ZED-F9P firmware version in debug menu.
//class SFE_UBLOX_GNSS_ADD : public SFE_UBLOX_GNSS
//{
//  public:
//    boolean getModuleInfo(uint16_t maxWait = 1100); //Queries module, texts
//
//    struct minfoStructure // Structure to hold the module info (uses 341 bytes of RAM)
//    {
//      char swVersion[30];
//      char hwVersion[10];
//      uint8_t extensionNo = 0;
//      char extension[10][30];
//    } minfo;
//};

//SFE_UBLOX_GNSS_ADD theGNSS;

class SFE_UBLOX_GNSS_SUPER_DERIVED : public SFE_UBLOX_GNSS_SUPER
{
public:
  volatile bool _iAmLocked = false;
  bool lock(void)
  {
    if (_iAmLocked)
    {
      unsigned long startTime = millis();
      while (_iAmLocked && (millis() < (startTime + 2100)))
        delay(1); //YIELD
      if (_iAmLocked)
        return false;
    }
    _iAmLocked = true;
    return true;
  }
  void unlock(void)
  {
    _iAmLocked = false;  
  }
};
SFE_UBLOX_GNSS_SUPER_DERIVED theGNSS;

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
#ifdef MAX_PAYLOAD_SIZE
#undef MAX_PAYLOAD_SIZE
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes
#endif
uint8_t settingPayload[MAX_PAYLOAD_SIZE];

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
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Accelerometer for bubble leveling
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "SparkFun_LIS2DH12.h" //Click here to get the library: http://librarymanager/All#SparkFun_LIS2DH12
SPARKFUN_LIS2DH12 accel;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Battery fuel gauge and PWM LEDs
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048);

// setting PWM properties
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

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_Graphic_OLED
QwiicMicroOLED oled;

#include <res/qw_fnt_5x7.h>
#include <res/qw_fnt_8x16.h>
#include <res/qw_fnt_largenum.h>
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include "SdFat.h" //http://librarymanager/All#sdfat_exfat by Bill Greiman. Currently uses v2.1.1

SdFat * sd;

char platformFilePrefix[40] = "SFE_Surveyor"; //Sets the prefix for logs and settings files

SdFile * ubxFile; //File that all GNSS ubx messages sentences are written to
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second
int startLogTime_minutes = 0; //Mark when we start any logging so we can stop logging after maxLogTime_minutes
int startCurrentLogTime_minutes = 0; //Mark when we start this specific log file so we can close it after x minutes and start a new one

//System crashes if two tasks access a file at the same time
//So we use a semaphore to see if file system is available
SemaphoreHandle_t sdCardSemaphore;
const TickType_t fatSemaphore_shortWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_longWait_ms = 200 / portTICK_PERIOD_MS;

//Display used/free space in menu and config page
uint32_t sdCardSizeMB = 0;
uint32_t sdFreeSpaceMB = 0;
uint32_t sdUsedSpaceMB = 0;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HardwareSerial serialGNSS(2); //TX on 17, RX on 16
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
char deviceName[30]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'
const byte menuTimeout = 250; //Menus will exit/timeout after this number of seconds
unsigned long splashStart = 0; //Controls how long the splash is displayed for. Currently min of 2s.

char platformPrefix[40] = "Surveyor"; //Sets the prefix for broadcast names
bool zedUartPassed = false; //Goes true during testing if ESP can communicate with ZED over UART
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200);
  delay(250);

  //  int pinNumber1 = 21;
  //  int pinNumber2 = 22;
  //  clearBuffer();
  //  pinMode(pinNumber1, OUTPUT);
  //  pinMode(pinNumber2, OUTPUT);
  //
  //  Serial.printf("\n\rToggling pin %d. Press x to exit\n\r", pinNumber1);
  //  Serial.printf("\n\rToggling pin %d. Press x to exit\n\r", pinNumber2);
  //
  //  while (Serial.available() == 0)
  //  {
  //    digitalWrite(pinNumber1, HIGH);
  //    digitalWrite(pinNumber2, HIGH);
  //    for (int x = 0 ; x < 100 ; x++)
  //    {
  //      delay(30);
  //      if (Serial.available()) break;
  //    }
  //
  //    digitalWrite(pinNumber1, LOW);
  //    digitalWrite(pinNumber2, LOW);
  //    for (int x = 0 ; x < 100 ; x++)
  //    {
  //      delay(30);
  //      if (Serial.available()) break;
  //    }
  //  }
  //  pinMode(pinNumber1, INPUT);
  //  pinMode(pinNumber2, INPUT);
  //
  //  Serial.println("Done");

  Wire.begin();

  //begin/end wire transmission to see if bus is responding correctly
  //All good: 0ms, response 2
  //SDA/SCL shorted: 1000ms timeout, response 5
  //SCL/VCC shorted: 14ms, response 5
  //SCL/GND shorted: 1000ms, response 5
  //SDA/VCC shorted: 1000ms, reponse 5
  //SDA/GND shorted: 14ms, response 5
  unsigned long startTime = millis();
  Wire.beginTransmission(0x15); //Dummy address
  int endValue = Wire.endTransmission();
  Serial.printf("Response time: %d endValue: %d\n\r", millis() - startTime, endValue);
  if (endValue == 2)
    online.i2c = true;
  else if (endValue == 5)
    Serial.println("It appears something is shorting the I2C lines.");

  beginBoard(); //Determine what hardware platform we are running on and check on button

  beginSD(); //Test if SD is present

  if (online.i2c == true)
  {
    beginGNSS(); //Connect to GNSS to get module type

    beginDisplay(); //Start display first to be able to display any errors

    beginAccelerometer();

    beginFuelGauge(); //Configure battery fuel guage monitor

    oled.erase();

    uint8_t fontHeight = 8;
    uint8_t yPos = 0;

    if (online.accelerometer)
      printTextCenter("Accel: OK", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted
    else
      printTextCenter("Accel: BAD", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted

    yPos += fontHeight;

    if (online.microSD)
      printTextCenter("SD: OK", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted
    else
      printTextCenter("SD: BAD", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted

    yPos += fontHeight;

    if (online.gnss)
      printTextCenter("GNSS: OK", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted
    else
      printTextCenter("GNSS: BAD", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted

    yPos += fontHeight;

    if (online.battery)
      printTextCenter("Batt: OK", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted
    else
      printTextCenter("Batt: BAD", yPos, QW_FONT_5X7, 1, false);  //text, y, font type, kerning, inverted

    yPos += fontHeight;

    oled.display();
  }

  //Select port 0 on MUX so UART1 is connected to ZED if bootloading is needed
  if(pin_muxA >= 0)
  {
    pinMode(pin_muxA, OUTPUT);
    digitalWrite(pin_muxA, LOW);
  }
  if(pin_muxB >= 0)
  {
    pinMode(pin_muxB, OUTPUT);
    digitalWrite(pin_muxB, LOW);
    Serial.println("MUX should now be UART");
  }

  menuSystem();
}

void loop()
{
  delay(10);

  if(pin_powerSenseAndControl >= 0)
  {
    if(digitalRead(pin_powerSenseAndControl) == LOW)
    {
      Serial.println("Power button pressed");
      ESP.restart(); //Use the power button as a reset
    }
    
  }
}
