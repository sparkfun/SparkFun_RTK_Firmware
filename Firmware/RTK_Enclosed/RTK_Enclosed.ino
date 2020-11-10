/*
  September 1st, 2020
  SparkFun Electronics
  Nathan Seidle

  This firmware runs the core of the SparkFun RTK Surveyor product. It runs on an ESP32
  and communicates with the ZED-F9P.

  Select the ESP32 Dev Module from the boards list. This maps the same pins to the ESP32-WROOM module.

  (Done) Bluetooth NMEA works but switch label is reverse
  (Done) Fix BT reconnect failure - Better but still fails sometimes
  (Done) Implement survey in and out based on switch
  (Done) Try better survey in setup: https://portal.u-blox.com/s/question/0D52p00009IsVoMCAV/restarting-surveyin-on-an-f9p
  (Done) Startup sequence LEDs like a car (all on, then count down quickly)
  (Done) Make the Base LED blink faster/slower (for real) depending on base accuracy
  (Done) Enable the Avinab suggested NMEA sentences:
  - Enable GST
  - Enable the high accuracy mode and NMEA 4.0+ in the NMEA configuration
  - Set the satellite numbering to Extended (3 Digits) for full skyplot support
  (Done) Add ESP32 ID to end of Bluetooth broadcast ID
  (Done) Decrease LED resistors to increase brightness
  (Done) Enable RTCM sentences for USB and UART1, in addition to UART2
  (Fixed) Something wierd happened with v1.13. Fix type changed? LED is not turning on even in float mode. (Disable SBAS)
  (Done) Check for v1.13 of ZED firmware. Display warning if not matching.
  (Fixed) ESP32 crashing when in rover mode, when LEDs change? https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/issues/124
  Set static position based on user input
  Wait for better pos accuracy before starting a survey in
  Can we add NTRIP reception over Wifi to the ESP32 to aid in survey in time?

  BT cast batt, RTK status, etc

  Menu System:
    Test system? Connection to GPS?
    Enable various debug output over BT?
    Display MAC address / broadcast name
    Change broadcast name + MAC
    Change max survey in time before cold start
    Allow user to enter permanent coordinates.
    Allow user to enable/disable detection of permanent base
    Set radius (5m default) for auto-detection of base
    Set nav rate. 4Hz is fun but may drown BT connection. 1Hz seems to be more stable.
    If more than 1Hz, turn off SV sentences.
*/

#include <Wire.h> //Needed for I2C to GPS

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//All the I2C and GPS stuff
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

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Bits for the battery level LEDs
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048); // Create a MAX17048

// setting PWM properties
const int freq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int resolution = 8;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Setup hardware serial and BT buffers
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

HardwareSerial GPS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 16384 //Using a large buffer. This might be much bigger than needed but the ESP32 has enough RAM
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Hardware connections v11
const int positionAccuracyLED_1cm = 2;
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 12;
const int positionAccuracyLED_100cm = 13;
const int positionAccuracyLED_10cm = 15;
const int sd_cs = 25;
const int zed_tx_ready = 26;
const int zed_reset = 27;
const int batteryLevelLED_Red = 32;
const int batteryLevelLED_Green = 33;
const int batteryLevel_alert = 36;
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

uint32_t lastBluetoothLEDBlink = 0;
uint32_t lastRoverUpdate = 0;
uint32_t lastBaseUpdate = 0;
uint32_t lastBattUpdate = 0;

uint32_t lastTime = 0;

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

  ledcSetup(ledRedChannel, freq, resolution);
  ledcSetup(ledGreenChannel, freq, resolution);

  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);

  ledcWrite(ledRedChannel, 0);
  ledcWrite(ledGreenChannel, 0);

  setupLiPo(); //Configure battery fuel guage monitor
  checkBatteryLevels(); //Display initial battery level

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

  if (myGPS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGPS.begin() == false)
    {
      Serial.println(F("u-blox GPS not detected at default I2C address. Please check wiring."));
      blinkError(ERROR_NO_I2C);
    }
    else
      Serial.println(F("u-blox GPS detected"));
  }
  else
    Serial.println(F("u-blox GPS detected"));

  //Based on Example21_ModuleInfo
  if (myGPS.getModuleInfo(1100) == true) // Try to get the module info
  {
    if (strcmp(myGPS.minfo.extension[1], latestZEDFirmware) != 0)
    {
      Serial.print("The ZED-F9P appears to have outdated firmware. Found: ");
      Serial.println(myGPS.minfo.extension[1]);
      Serial.print("The Surveyor works best with ");
      Serial.println(latestZEDFirmware);
      Serial.print("Please upgrade using u-center.");
      Serial.println();
    }
    else
    {
      Serial.println("ZED-F9P firmware is current");
    }
  }

  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure module. Trying again."));
    delay(1000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure module. Power cycle? Freezing..."));
      blinkError(ERROR_GPS_CONFIG_FAIL);
    }
  }
  Serial.println(F("GPS configuration complete"));

  danceLEDs(); //Turn on LEDs like a car dashboard

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

}
