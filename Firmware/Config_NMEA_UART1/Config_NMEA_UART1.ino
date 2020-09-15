/*
  Using I2C, setup UART1 to be NMEA+UBX, at 115200bps
  This will pipe into the NINA module that is 115200bps by default

  Setup UART2 to be RTCM3, at 57600bps
  This will pipe correction data to/from radio at default rate for SiK firmware

  Set update rate to 1Hz

  This example does not demonstrate high precision position over I2C but it is available
  over UBX packets on UART1 using UBX. This means you should be able to connect over Bluetooth
  to UART1 and use U-center to see high-precision positions.

  TODO:

  Try I2C again if init fails
  We may need to cold start to re-start survey in?
  Accuracy of position
  Does 57600bps support update rates of RTK at 4Hz?
  Press/hold to activate survey in
*/

#include <Wire.h>

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

long lastTime = 0; //Simple local timer. Limits amount of I2C traffic to Ublox module.

void setup()
{
  Serial.begin(115200);
  Serial.println(F("SparkFun Ublox Example"));

  Wire.begin();

  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  bool response = configureUbloxModule();

  if(response == false)
  {
    //Try once more
      Serial.println(F("Failed to configure module. Trying again."));
    delay(2000);
    response = configureUbloxModule();

    if(response == false)
    {
      Serial.println(F("Failed to configure module. Power cycle? Freezing..."));
      while(1);
    }
  }

  Serial.println(F("GPS configuration complete"));
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'r')
    {
      myGPS.factoryReset();
      Serial.println(F("Module reset"));
    }
    else if (incoming == 'c')
    {
      configureModule();
    }
    else if (incoming == 's')
    {
      bool success = surveyIn();

      if (success == true)
        Serial.println(F("Base survey complete! RTCM now broadcasting."));
      else
        Serial.println(F("Survey failed"));
    }
  }

  // Calling getPVT returns true if there actually is a fresh navigation solution available.
  if (myGPS.getPVT())
  {
    //lastTime = millis(); //Update the timer

    long latitude = myGPS.getLatitude();
    long longitude = myGPS.getLongitude();
    long altitude = myGPS.getAltitude();
    byte SIV = myGPS.getSIV();
    long accuracy = myGPS.getPositionAccuracy();
    uint32_t horizontalAccuracy = myGPS.getHorizontalAccuracy();
    byte RTK = myGPS.getCarrierSolutionType();

    Serial.print(F("Lat: "));
    Serial.print(latitude);

    Serial.print(F(" Long: "));
    Serial.print(longitude);
    Serial.print(F(" (degrees * 10^-7)"));

    Serial.print(F(" Alt: "));
    Serial.print(altitude);
    Serial.print(F(" (mm)"));


    Serial.print(F(" SIV: "));
    Serial.print(SIV);

    Serial.print(F(" 3D Acc: "));
    Serial.print(accuracy);
    Serial.print(F("mm"));

    float f_horizontalAccuracy = horizontalAccuracy;
    f_horizontalAccuracy /= 10.0;

    Serial.print(F(" Horz Acc: "));
    Serial.print(f_horizontalAccuracy, 1);
    Serial.print(F("mm"));

    Serial.print(" RTK: ");
    Serial.print(RTK);
    if (RTK == 1) Serial.print(F(" High precision float fix!"));
    if (RTK == 2) Serial.print(F(" High precision fix!"));

    Serial.println();
  }
}

bool configureUbloxModule()
{
  boolean response = true;

  response &= myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  //myGPS.setNavigationFrequency(10); //Set output to 10 times a second
  response &= myGPS.setNavigationFrequency(4); //Set output in Hz

  myGPS.setSerialRate(115200); //Set UART1 to 115200
  response &= myGPS.setUART1Output(COM_TYPE_NMEA | COM_TYPE_UBX); //Set the UART1 to output NMEA and UBX so we can get high precision over Bluetooth

  myGPS.setSerialRate(57600, 2); //Set UART2 to 57600 to match SiK firmware default
  response &= myGPS.setUART2Output(COM_TYPE_RTCM3); //Set the UART2 to output RTCM3 for radio link

  response &= myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

  response &= myGPS.setAutoPVT(true); //Tell the GPS to "send" each solution

  if (response == false)
  {
    Serial.println(F("Module failed initial config."));
    return (false);
  }

  response &= myGPS.setDynamicModel(DYN_MODEL_STATIONARY); // Set the dynamic model to STATIONARY
  if (response == false) // Set the dynamic model to STATIONARY
  {
    Serial.println(F("Warning: setDynamicModel failed!"));
    return(false);
  }

  // Let's read the new dynamic model to see if it worked
  uint8_t newDynamicModel = myGPS.getDynamicModel();
  if (newDynamicModel != 2)
  {
    Serial.println(F("setDynamicModel failed!"));
    return(false);
  }

  response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART2, 1); //Enable message 1005 to output through UART2, message every second
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 10); //Enable message every 10 seconds

  if (response == false)
  {
    Serial.println(F("RTCM failed to enable."));
    return (false);
  }

  response &= myGPS.saveConfiguration(); //Save the current settings to flash and BBR

  if (response == false)
  {
    Serial.println(F("Module failed to save."));
    return (false);
  }

  return(true);
}

bool surveyIn()
{
  boolean response = true;

  //Check if Survey is in Progress before initiating one
  response = myGPS.getSurveyStatus(2000); //Query module for SVIN status with 2000ms timeout (request can take a long time)
  if (response == false)
  {
    Serial.println(F("Failed to get Survey In status"));
    return (false);
  }

  if (myGPS.svin.active == true)
  {
    Serial.println(F("Survey already in progress."));
  }
  else
  {
    //Start survey
    //The ZED-F9P is slightly different than the NEO-M8P. See the Integration manual 3.5.8 for more info.
    //response = myGPS.enableSurveyMode(300, 2.000); //Enable Survey in on NEO-M8P, 300 seconds, 2.0m
    response = myGPS.enableSurveyMode(60, 5.000); //Enable Survey in, 60 seconds, 5.0m
    if (response == false)
    {
      Serial.println("Survey start failed");
      return (false);
    }
    Serial.println("Survey started. This will run until 60s has passed and less than 5m accuracy is achieved.");
  }

  while (Serial.available()) Serial.read(); //Clear buffer

  //Begin waiting for survey to complete
  while (myGPS.svin.valid == false)
  {
    if (Serial.available())
    {
      byte incoming = Serial.read();
      if (incoming == 'x')
      {
        //Stop survey mode
        response = myGPS.disableSurveyMode(); //Disable survey
        Serial.println(F("Survey stopped"));
        return (false);
      }
    }

    response = myGPS.getSurveyStatus(2000); //Query module for SVIN status with 2000ms timeout (req can take a long time)
    if (response == true)
    {
      Serial.print(F("Press x to end survey - "));
      Serial.print(F("Time elapsed: "));
      Serial.print((String)myGPS.svin.observationTime);

      Serial.print(F(" Accuracy: "));
      Serial.print((String)myGPS.svin.meanAccuracy);

      uint32_t horizontalAccuracy = myGPS.getHorizontalAccuracy();
      float f_horizontalAccuracy = horizontalAccuracy;
      f_horizontalAccuracy /= 10.0;

      Serial.print(F(" Horz Acc: "));
      Serial.print(f_horizontalAccuracy, 1);
      Serial.print(F("mm"));

      Serial.println();
    }
    else
    {
      Serial.println(F("SVIN request failed"));
    }

    delay(1000);
  }
  return (true);
}
