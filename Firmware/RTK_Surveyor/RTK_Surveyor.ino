/*
  September 1st, 2020
  SparkFun Electronics
  Nathan Seidle

  This firmware runs the core of the SparkFun RTK Surveyor product. It runs on an ESP32
  and communicates with the ZED-F9P.

  Select the ESP32 Dev Module from the boards list. This maps the same pins to the ESP32-WROOM module.

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
    (Done) Base - Enter fixed coordinates, survey-in settings
    (Done) Ports - Configure Radio and Data port baud rates
    (Done) Test menu
    Enable various debug outputs sent over BT

*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 1;

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

//EEPROM for storing settings
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <EEPROM.h>
#define EEPROM_SIZE 2048 //ESP32 emulates EEPROM in non-volatile storage (external flash IC). Max is 508k.
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
SdFat sd;
SdFile gnssDataFile; //File that all gnss data is written to

char settingsFileName[40] = "SFE_Surveyor_Settings.txt"; //File to read/write system settings to

unsigned long lastDataLogSyncTime = 0; //Used to record to SD every half second
long startLogTime_minutes = 0; //Mark when we start logging so we can stop logging after maxLogTime_minutes
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include <Wire.h> //Needed for I2C to GNSS

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

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
char deviceName[20]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'
const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds
bool inTestMode = false; //Used to re-route BT traffic while in test sub menu
int battLevel = 0; //SOC measured from fuel gauge, in %
long systemTime_minutes = 0; //Used to test if logging is less than max minutes

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

  Wire.begin(); //Start I2C

  beginLEDs(); //LED and PWM setup

  //Start EEPROM and SD for settings, and display for output
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  beginEEPROM();

  //eepromErase();

  beginSD(); //Test if SD is present

  loadSettings(); //Attempt to load settings after SD is started so we can read the settings file if available

  beginDisplay(); //Check if an external Qwiic OLED is attached
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  beginFuelGauge(); //Configure battery fuel guage monitor
  checkBatteryLevels(); //Force display so you see battery level immediately at power on

  beginBT(); //Get MAC, start radio

  beginGNSS(); //Connect and configure ZED-F9P

  if (online.microSD == true)
    Serial.println(F("microSD card online"));

  Serial.flush(); //Complete any previous prints

  danceLEDs(); //Turn on LEDs like a car dashboard

  if (online.microSD == true && settings.zedOutputLogging == true) startLogTime_minutes = 0; //Mark now as start of logging

  //myGPS.enableDebugging(); //Enable debug messages over Serial (default)
}

void loop()
{
  //Update Bluetooth LED status
  if (bluetoothState == BT_ON_NOCONNECTION)
  {
    if (millis() - lastBluetoothLEDBlink > 500)
    {
      if (digitalRead(bluetoothStatusLED) == LOW)
        digitalWrite(bluetoothStatusLED, HIGH);
      else
        digitalWrite(bluetoothStatusLED, LOW);
      lastBluetoothLEDBlink = millis();
    }
  }

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH && baseState != BASE_OFF)
  {
    //Configure for rover mode
    Serial.println(F("Rover Mode"));

    baseState = BASE_OFF;
    startBluetooth(); //Restart Bluetooth with new name

    //If we are survey'd in, but switch is rover then disable survey
    if (configureUbloxModuleRover() == false)
    {
      Serial.println("Rover config failed!");
    }

    digitalWrite(baseStatusLED, LOW);
  }
  else if (digitalRead(baseSwitch) == LOW && baseState == BASE_OFF)
  {
    //Configure for base mode
    Serial.println(F("Base Mode"));

    if (configureUbloxModuleBase() == false)
    {
      Serial.println("Base config failed!");
    }

    startBluetooth(); //Restart Bluetooth with new name

    baseState = BASE_SURVEYING_IN_NOTSTARTED; //Switch to new state
  }

  if (baseState == BASE_SURVEYING_IN_NOTSTARTED || baseState == BASE_SURVEYING_IN_SLOW || baseState == BASE_SURVEYING_IN_FAST)
  {
    updateSurveyInStatus();
  }
  else if (baseState == BASE_OFF)
  {
    updateRoverStatus();
  }

  updateBattLEDs();

  updateDisplay();

  //Menu system via ESP32 USB connection
  if (Serial.available()) menuMain(); //Present user menu

  //Create files or close files as needed
  if (settings.zedOutputLogging == true && online.dataLogging == false)
  {
    beginDataLogging();
  }
  else if (settings.zedOutputLogging == false && online.dataLogging == true)
  {
    //Close down file
    gnssDataFile.sync();
    gnssDataFile.close();
    online.dataLogging = false;
  }

  //Convert current system time to minutes. This is used in F9PSerialReadTask() to see if we are within max log window.
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
      if (bluetoothState == BT_CONNECTED)
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
      float hpa = myGPS.getHorizontalAccuracy() / 10000.0;
      if (hpa > 30.0)
      {
        oled.print(">30");
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

      if (myGPS.getFixType() == 0) //0 = No Fix
      {
        oled.print("0");
      }
      else
      {
        oled.print(myGPS.getSIV());
      }

      oled.display();
    }
  }
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

    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(10, 2); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print("SparkFun");

    oled.setCursor(21, 13);
    oled.setFontType(1);
    oled.print("RTK");

    int surveyorTextY = 25;
    int surveyorTextX = 2;
    int surveyorTextKerning = 8;
    oled.setFontType(1);

    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("S");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("u");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("v");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("e");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("y");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("o");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    oled.setCursor(20, 41);
    oled.setFontType(0); //Set font to smallest
    oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    oled.display();
  }
}

//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (myGPS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGPS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      blinkError(ERROR_NO_I2C);
    }
  }

  //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
  //  if (myGPS.getModuleInfo(1100) == true) // Try to get the module info
  //  {
  //    if (strcmp(myGPS.minfo.extension[1], latestZEDFirmware) != 0)
  //    {
  //      Serial.print("The ZED-F9P appears to have outdated firmware. Found: ");
  //      Serial.println(myGPS.minfo.extension[1]);
  //      Serial.print("The Surveyor works best with ");
  //      Serial.println(latestZEDFirmware);
  //      Serial.print("Please upgrade using u-center.");
  //      Serial.println();
  //    }
  //    else
  //    {
  //      Serial.println("ZED-F9P firmware is current");
  //    }
  //  }

  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure module. Trying again."));
    delay(1000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure module. Hard stop."));
      blinkError(ERROR_GPS_CONFIG_FAIL);
    }
  }
  Serial.println(F("GNSS configuration complete"));
}

//Get MAC, start radio
void beginBT()
{
  //Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  SerialBT.register_callback(btCallback);
  if (startBluetooth() == false)
  {
    Serial.println("An error occurred initializing Bluetooth");
    bluetoothState = BT_OFF;
    digitalWrite(bluetoothStatusLED, LOW);
  }
  else
  {
    bluetoothState = BT_ON_NOCONNECTION;
    digitalWrite(bluetoothStatusLED, HIGH);
    lastBluetoothLEDBlink = millis();
  }
}

//Set LEDs for output and configure PWM
void beginLEDs()
{
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

  ledcSetup(ledRedChannel, freq, resolution);
  ledcSetup(ledGreenChannel, freq, resolution);

  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);

  ledcWrite(ledRedChannel, 0);
  ledcWrite(ledGreenChannel, 0);
}
