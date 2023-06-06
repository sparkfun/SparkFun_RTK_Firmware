//  RTK Display Test
//  Configures the OLED display for Hookup Guide photos

//  Select one of the provided systemState's below
//  Optionally select one of the antenna status (open / short)

//  Press and hold the MODE button to update / advance the display. Release to make the display static.
//  I.e. push and hold MODE until the display looks nice! ;-)


#define HAS_BATTERY false
#define HAS_ANTENNA_SHORT_OPEN true

#include "settings.h"



//SystemState systemState = STATE_BASE_FIXED_TRANSMITTING; // <--- Use this one for "Base Casting"

//SystemState systemState = STATE_ROVER_RTK_FIX; // <--- Use this one for "Rover Fixed"

SystemState systemState = STATE_NTPSERVER_SYNC; // <--- Use this one for "NTP"




//GNSS configuration for Dynamic Model
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3 v3.0.5

const int aStatus = 0; // <--- Change to SFE_UBLOX_ANTENNA_STATUS_SHORT or SFE_UBLOX_ANTENNA_STATUS_OPEN if desired

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



struct
{
  bool display = true;
  bool gnss = true;
  bool logging = true;
  bool accelerometer = false;
  ethernetStatus_e ethernetStatus = ETH_CONNECTED;
} online;

SystemState setupState = STATE_MARK_EVENT;
const int displayProfile = 0;
const float battLevel = 70;
const float horizontalAccuracy = 0.0141;
const uint8_t tAcc = 26;
const int dynamicModel = DYN_MODEL_PORTABLE;
const int PLATFORM_F9P = 0;
const int PLATFORM_F9R = 1;
int zedModuleType = PLATFORM_F9P;
const int numSV = 30;
const int fusionMode = 1;
const int fixType = 3;
const bool lbandCorrectionsReceived = false;
uint8_t loggingIconDisplayed = 0; //Increases every 500ms while logging
const bool logIncreasing = true;
const float svinMeanAccuracy = 0.97;
const int svinObservationTime = 57;
const int rtcmPacketsSent = 123;

struct
{
  bool linkUp = true;
  IPAddress localIP = { 192, 168, 0, 45 };
} ETH;

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int pin_peripheralPowerControl = 32; //Reference Station power control is on pin 32. Set to -1 if not needed.
int pin_setupButton = 0; //Reference Station MODE is on pin 0. Set to -1 if not needed.
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//I2C for GNSS, battery gauge, display, accelerometer
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_Graphic_OLED
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

uint32_t lastDisplayUpdate = 0;
bool firstRadioSpotBlink = false; //Controls when the shared icon space is toggled
unsigned long firstRadioSpotTimer = 0;
bool secondRadioSpotBlink = false; //Controls when the shared icon space is toggled
unsigned long secondRadioSpotTimer = 0;
bool thirdRadioSpotBlink = false; //Controls when the shared icon space is toggled
unsigned long thirdRadioSpotTimer = 0;
uint32_t lastBaseIconUpdate = 0;
bool baseIconDisplayed = false; //Toggles as lastBaseIconUpdate goes above 1000ms

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging

  if (pin_peripheralPowerControl >= 0)
  {
    pinMode(pin_peripheralPowerControl, OUTPUT);
    digitalWrite(pin_peripheralPowerControl, HIGH); //Turn on SD, W5500, etc
  }

  if (pin_setupButton >= 0)
  {
    pinMode(pin_setupButton, INPUT_PULLUP);
  }

  Wire.begin();

  beginDisplay(); //Start display to be able to display any errors

}

void loop()
{
  updateDisplay((pin_setupButton >= 0) ? digitalRead(pin_setupButton) == LOW : true); //Only 'increment' the display if the MODE / Setup button is pressed
}
