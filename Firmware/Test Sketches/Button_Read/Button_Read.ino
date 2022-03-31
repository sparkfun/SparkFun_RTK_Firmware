/*
  Example showing how to read the buttons or rocker switches on the RTK product line

  The platform is autodetected at power on (RTK Surveyor vs Express vs Express Plus vs Facet).
  A task is spun up to monitor buttons and prints when a button is pressed/released
*/

#include "settings.h"

#include <Wire.h>

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 11;

char platformPrefix[40] = "Surveyor"; //Sets the prefix for broadcast names
char platformFilePrefix[40] = "SFE_Surveyor"; //Sets the prefix for logs and settings files

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//These pins are set in beginBoard()
int pin_batteryLevelLED_Red;
int pin_batteryLevelLED_Green;
int pin_positionAccuracyLED_1cm;
int pin_positionAccuracyLED_10cm;
int pin_positionAccuracyLED_100cm;
int pin_baseStatusLED;
int pin_bluetoothStatusLED;
int pin_microSD_CS;
int pin_zed_tx_ready;
int pin_zed_reset;
int pin_batteryLevel_alert;

int pin_muxA;
int pin_muxB;
int pin_powerSenseAndControl;
int pin_setupButton;
int pin_powerFastOff;
int pin_dac26;
int pin_adc39;
int pin_peripheralPowerControl;

int pin_radio_rx;
int pin_radio_tx;
int pin_radio_rst;
int pin_radio_pwr;
int pin_radio_cts;
int pin_radio_rts;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Buttons - Interrupt driven and debounce
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <JC_Button.h> // http://librarymanager/All#JC_Button
Button *setupBtn = NULL; //We can't instantiate the buttons here because we don't yet know what pin numbers to use
Button *powerBtn = NULL;

TaskHandle_t ButtonCheckTaskHandle = NULL;
const uint8_t ButtonCheckTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int buttonTaskStackSize = 2000;

const int shutDownButtonTime = 2000; //ms press and hold before shutdown
unsigned long lastRockerSwitchChange = 0; //If quick toggle is detected (less than 500ms), enter WiFi AP Config mode
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint32_t powerPressedStartTime = 0; //Times how long user has been holding power button, used for power down
uint8_t debounceDelay = 20; //ms to delay between button reads

uint64_t lastLogSize = 0;
bool logIncreasing = false; //Goes true when log file is greater than lastLogSize
bool reuseLastLog = false; //Goes true if we have a reset due to software (rather than POR)

bool setupByPowerButton = false; //We can change setup via tapping power button

unsigned long startTime = 0; //Used for checking longest running functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("RTK Button example");

  Wire.begin();

  beginBoard(); //Determine what hardware platform we are running on and check on button

  beginSystemState(); //Determine initial system state. Start task for button monitoring.
}

void loop()
{
  delay(10); //A small delay prevents panic if no other I2C or functions are called
}
