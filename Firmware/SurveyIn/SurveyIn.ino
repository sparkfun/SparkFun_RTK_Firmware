/*
  When flipping from Rover to Base, we'll have to cold start.
  How long does it take to survey in from cold start?

  9-12-20: 174s
  1000+ is the antenna in a bad position?
  Passed 5000. After a cold start, base after 782s. Perhaps we auto-cold start after 1000s (900s = 15 minutes).
  Then 297.
  Then 408.
  176.
  222
  184

  (Done) Create local function to read the port settings using the custom command so we can compare against it and only change settings
  as required. This is an attempt to prevent the module from getting its I2C address overwritten

  (Done) Factory reset a device then make sure it configs correctly

*/

#include <Wire.h> //Needed for I2C to GPS

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

typedef enum 
{
  ERROR_NO_I2C = 2,
} t_errorNumber;


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

const int positionAccuracyLED_20mm = 2; //POSACC1
const int positionAccuracyLED_100mm = 15; //POSACC2
const int positionAccuracyLED_1000mm = 13; //POSACC3
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 12;

void setup()
{
  Serial.begin(115200);
  delay(2000); //Wait for USB port to show up
  Serial.println(F("Ublox Base station example"));

  pinMode(positionAccuracyLED_20mm, OUTPUT);
  pinMode(positionAccuracyLED_100mm, OUTPUT);
  pinMode(positionAccuracyLED_1000mm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  Wire.begin();

  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
    blinkError(ERROR_NO_I2C);
  }

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

  Serial.println(F("Set switch to 1 for Base, 0 for Rover"));

  //By default, module should be configured for rover but we'll check the switch during config
  configureUbloxModule();

  danceLEDs(); //Turn on LEDs like a car dashboard
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'x')
    {
      Serial.println(F("Restart Survey in"));

      resetSurvey();

      surveyIn();
    }
    else if (incoming == 'r')
    {
      //Configure for rover mode
      Serial.println(F("Rover Mode"));

      baseState = BASE_OFF;

      //If we are survey'd in, but switch is rover then disable survey
      if (configureUbloxModuleRover() == false)
      {
        Serial.println("Rover config failed!");
      }

    }
    else if (incoming == 'b')
    {
      //Configure for base mode
      Serial.println(F("Base Mode"));

      if (configureUbloxModuleBase() == false)
      {
        Serial.println("Base config failed!");
      }

      //Begin Survey in
      surveyIn();

    }
  }

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH && baseState != BASE_OFF)
  {
    //Configure for rover mode
    Serial.println(F("Rover Mode"));

    baseState = BASE_OFF;

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
}
