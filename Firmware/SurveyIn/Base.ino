//Configure specific aspects of the receiver for base mode
bool configureUbloxModuleBase()
{
  bool response = true;

  response &= myGPS.setDynamicModel(DYN_MODEL_STATIONARY); // Set the dynamic model to STATIONARY
  if (response == false) // Set the dynamic model to STATIONARY
  {
    Serial.println(F("Warning: setDynamicModel failed!"));
    return (false);
  }

  // Let's read the new dynamic model to see if it worked
  uint8_t newDynamicModel = myGPS.getDynamicModel();
  if (newDynamicModel != 2)
  {
    Serial.println(F("setDynamicModel failed!"));
    return (false);
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

  return (response);
}

void surveyIn()
{
  boolean response = true;

  resetSurvey();

//  //Check if Survey is in Progress before initiating one
//  response = myGPS.getSurveyStatus(2000); //Query module for SVIN status with 2000ms timeout (request can take a long time)
//  if (response == false)
//  {
//    Serial.println(F("Failed to get Survey In status"));
//    return;
//  }
//
//  if (myGPS.svin.active == true)
//  {
//    Serial.print(F("Survey already in progress. Restarting."));
//  }

  response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_I2C, 1); //Enable message 1005 to output through I2C port, message every second
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_I2C, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_I2C, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_I2C, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_I2C, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_I2C, 10); //Enable message every 10 seconds

  //Use COM_PORT_UART1 for the above six messages to direct RTCM messages out UART1
  //COM_PORT_UART2, COM_PORT_USB, COM_PORT_SPI are also available
  //For example: response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART1, 10);

  if (response == false)
  {
    Serial.println(F("RTCM failed to enable. Are you sure you have an ZED-F9P?"));
    return;
  }
  Serial.println(F("RTCM messages enabled"));

  //Start survey
  //The ZED-F9P is slightly different than the NEO-M8P. See the Integration manual 3.5.8 for more info.
  //response = myGPS.enableSurveyMode(300, 2.000); //Enable Survey in on NEO-M8P, 300 seconds, 2.0m
  response = myGPS.enableSurveyMode(60, 5.000); //Enable Survey in, 60 seconds, 5.0m
  if (response == false)
  {
    Serial.println(F("Survey start failed"));
    return;
  }
  Serial.println(F("Survey started. This will run until 60s has passed and less than 5m accuracy is achieved."));

  baseState = BASE_SURVEYING_IN_SLOW;

  myGPS.setI2COutput(COM_TYPE_UBX | COM_TYPE_RTCM3); //Set the I2C port to output UBX and RTCM sentences (not really an option, turns on NMEA as well)
}

void resetSurvey()
{
  //Slightly modified method for restarting survey-in from: https://portal.u-blox.com/s/question/0D52p00009IsVoMCAV/restarting-surveyin-on-an-f9p
  bool response = myGPS.disableSurveyMode(); //Disable survey
  delay(500);
  response = myGPS.enableSurveyMode(1000, 400.000); //Enable Survey in with bogus values
  delay(500);
  response = myGPS.disableSurveyMode(); //Disable survey
  delay(500);
}
