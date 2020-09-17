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
  Decrease LED resistors to increase brightness

  Can we add NTRIP reception over Wifi to the ESP32 to aid in survey in time?

  Menu System:
    Test system? Connection to GPS?
    Enable various debug output over BT?
    Display MAC address / broadcast name

  ESP32 crashing when in rover mode, when LEDs change?
  Seems very stable in base mode, during and after full survey in

H
Lat: .01804188
Long: .28259348
Stable on Arudino 1.8.9 and default ublox lib


*/

#define RXD2 16
#define TXD2 17

#include <Wire.h> //Needed for I2C to GPS

#include "SparkFun_Ublox_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

//Hardware connections v11
const int positionAccuracyLED_20mm = 2; //POSACC1
const int positionAccuracyLED_100mm = 15; //POSACC2
const int positionAccuracyLED_1000mm = 13; //POSACC3
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 12;

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
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
} BaseState;
volatile BaseState baseState = BASE_OFF;
unsigned long baseStateBlinkTime = 0;
const unsigned long maxSurveyInWait_s = 60L * 5L; //Re-start survey-in after X seconds

uint32_t lastBluetoothLEDBlink = 0;
uint32_t lastRoverUpdate = 0;
uint32_t lastBaseUpdate = 0;

uint32_t lastTime = 0;

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected");
    bluetoothState = BT_CONNECTED;
    digitalWrite(bluetoothStatusLED, HIGH);
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("Client disconnected");
    bluetoothState = BT_ON_NOCONNECTION;
    digitalWrite(bluetoothStatusLED, LOW);
    lastBluetoothLEDBlink = millis();
  }

  //  if (event == ESP_SPP_WRITE_EVT ) {
  //    Serial.println("W");
  //    if(Serial1.available()) SerialBT.write(Serial1.read());
  //  }
}

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging
  Serial2.begin(115200); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at 115200bps.
  Serial.println("SparkFun RTK Surveyor v1.0");

  pinMode(positionAccuracyLED_20mm, OUTPUT);
  pinMode(positionAccuracyLED_100mm, OUTPUT);
  pinMode(positionAccuracyLED_1000mm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  digitalWrite(positionAccuracyLED_20mm, LOW);
  digitalWrite(positionAccuracyLED_100mm, LOW);
  digitalWrite(positionAccuracyLED_1000mm, LOW);
  digitalWrite(baseStatusLED, LOW);
  digitalWrite(bluetoothStatusLED, LOW);

  SerialBT.register_callback(callback);
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

  Wire.begin();
  if (myGPS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGPS.begin() == false)
    {
      Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring."));
      blinkError(ERROR_NO_I2C);
    }
    else
      Serial.println(F("Ublox GPS detected"));
  }
  else
    Serial.println(F("Ublox GPS detected"));

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

  myGPS.enableDebugging(); //Enable debug messages over Serial (default)
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

  if (bluetoothState == BT_CONNECTED)
  {
    //If we are actively survey-in then do not pass NMEA data
    if (baseState == BASE_SURVEYING_IN_SLOW || baseState == BASE_SURVEYING_IN_FAST)
    {
      //Do nothing
    }
    else
    {
      //If the ZED has any new NMEA data, pass it out over Bluetooth
      while(Serial2.available())
      {
        SerialBT.write(Serial2.read());
        yield();
      }

      //If the phone has any new data (NTRIP RTCM, etc), pass it out over Bluetooth
      while(SerialBT.available())
      {
        Serial2.write(SerialBT.read());
        yield();
      }
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

    //Begin Survey in
    surveyIn();
  }

  if (baseState == BASE_SURVEYING_IN_SLOW || baseState == BASE_SURVEYING_IN_FAST)
  {
    updateSurveyInStatus();
  }
  else if (baseState == BASE_OFF)
  {
    updateRoverStatus();
  }

  //delay(1);
}
