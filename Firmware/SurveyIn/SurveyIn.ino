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

*/

#include <Wire.h> //Needed for I2C to GPS

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

//Base status goes from Rover-Mode (LED off), surveying in (blinking), to survey is complete/trasmitting RTCM (solid)
enum BaseState
{
  BASE_OFF = 0,
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
};
volatile byte baseState = BASE_OFF;
unsigned long baseStateBlinkTime = 0;
const unsigned long maxSurveyInWait_s = 60L * 5L; //Re-start survey-in after X seconds

const int positionAccuracyLED_20mm = 13; //POSACC1
const int positionAccuracyLED_100mm = 15; //POSACC2
const int positionAccuracyLED_1000mm = 2; //POSACC3
const int baseStatusLED = 4;
const int baseSwitch = 5;


void setup()
{
  Serial.begin(115200);
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
    while (1);
  }

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

  Serial.println(F("Set switch to 1 for Base, 0 for Rover"));
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
    //delay(100); //Don't pound the I2C bus too hard

    bool response = myGPS.getSurveyStatus(2000); //Query module for SVIN status with 2000ms timeout (req can take a long time)
    if (response == true)
    {
      if (myGPS.svin.valid == true)
      {
        Serial.println(F("Survey valid!"));
        Serial.println(F("Base survey complete! RTCM now broadcasting."));
        baseState = BASE_TRANSMITTING;

        digitalWrite(baseStatusLED, HIGH); //Turn on LED
      }
      else
      {
        Serial.print(F("Time elapsed: "));
        Serial.print((String)myGPS.svin.observationTime);

        Serial.print(F(" Accuracy: "));
        Serial.print((String)myGPS.svin.meanAccuracy);

        byte SIV = myGPS.getSIV();
        Serial.print(F(" SIV: "));
        Serial.print(SIV);

        Serial.println();

        if (myGPS.svin.meanAccuracy > 6.0)
          baseState = BASE_SURVEYING_IN_SLOW;
        else
          baseState = BASE_SURVEYING_IN_FAST;

        if (myGPS.svin.observationTime > maxSurveyInWait_s)
        {
          Serial.println(F("Survey-In took more than 15 minutes. Restarting survey in."));

          resetSurvey();

          surveyIn();
        }
      }
    }
    else
    {
      Serial.println(F("SVIN request failed"));
    }
  }

  if (baseState == BASE_SURVEYING_IN_SLOW)
  {
    if (millis() - baseStateBlinkTime > 500)
    {
      baseStateBlinkTime += 500;
      Serial.println(F("Slow blink"));

      if (digitalRead(baseStatusLED) == LOW)
        digitalWrite(baseStatusLED, HIGH);
      else
        digitalWrite(baseStatusLED, LOW);
    }
  }
  else if (baseState == BASE_SURVEYING_IN_FAST)
  {
    if (millis() - baseStateBlinkTime > 100)
    {
      baseStateBlinkTime += 100;
      Serial.println(F("Fast blink"));

      if (digitalRead(baseStatusLED) == LOW)
        digitalWrite(baseStatusLED, HIGH);
      else
        digitalWrite(baseStatusLED, LOW);
    }
  }
  else if (baseState == BASE_OFF)
  {
    //We're in rover mode so update the accuracy LEDs
    uint32_t accuracy = myGPS.getHorizontalAccuracy();

    // Convert the horizontal accuracy (mm * 10^-1) to a float
    float f_accuracy = accuracy;
    // Now convert to m
    f_accuracy = f_accuracy / 10000.0; // Convert from mm * 10^-1 to m

    Serial.print("Rover Accuracy (m): ");
    Serial.print(f_accuracy, 4); // Print the accuracy with 4 decimal places

    if (f_accuracy <= 0.02)
    {
      Serial.print(" 0.02m LED");
      digitalWrite(positionAccuracyLED_20mm, HIGH);
      digitalWrite(positionAccuracyLED_100mm, HIGH);
      digitalWrite(positionAccuracyLED_1000mm, HIGH);
    }
    else if (f_accuracy <= 0.100)
    {
      Serial.print(" 0.1m LED");
      digitalWrite(positionAccuracyLED_20mm, LOW);
      digitalWrite(positionAccuracyLED_100mm, HIGH);
      digitalWrite(positionAccuracyLED_1000mm, HIGH);
    }
    else if (f_accuracy <= 1.0000)
    {
      Serial.print(" 1m LED");
      digitalWrite(positionAccuracyLED_20mm, LOW);
      digitalWrite(positionAccuracyLED_100mm, LOW);
      digitalWrite(positionAccuracyLED_1000mm, HIGH);
    }
    else if (f_accuracy > 1.0)
    {
      Serial.print(" No LEDs");
      digitalWrite(positionAccuracyLED_20mm, LOW);
      digitalWrite(positionAccuracyLED_100mm, LOW);
      digitalWrite(positionAccuracyLED_1000mm, LOW);
    }
    Serial.println();
  }
}
