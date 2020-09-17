
//Wait for survey in to complete
bool updateSurveyInStatus()
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
        Serial.println(F("Survey-In took more than 5 minutes. Restarting survey in."));

        resetSurvey();

        surveyIn();
      }
    }
  }
  else
  {
    Serial.println(F("SVIN request failed"));
  }

  //Update the Base LED accordingly
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
}

//Configure specific aspects of the receiver for base mode
bool configureUbloxModuleBase()
{
  bool response = true;

  // Set dynamic model
  if (myGPS.getDynamicModel() != DYN_MODEL_STATIONARY)
  {
    if (myGPS.setDynamicModel(DYN_MODEL_STATIONARY) == false)
      Serial.println(F("Warning: setDynamicModel failed!"));
    return (false);
  }
}

if (getRTCMSettings(UBX_RTCM_1005, COM_PORT_UART2) != 1)
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART2, 1); //Enable message 1005 to output through UART2, message every second
if (getRTCMSettings(UBX_RTCM_1074, COM_PORT_UART2) != 1)
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_UART2, 1);
if (getRTCMSettings(UBX_RTCM_1084, COM_PORT_UART2) != 1)
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_UART2, 1);
if (getRTCMSettings(UBX_RTCM_1094, COM_PORT_UART2) != 1)
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_UART2, 1);
if (getRTCMSettings(UBX_RTCM_1124, COM_PORT_UART2) != 1)
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_UART2, 1);
if (getRTCMSettings(UBX_RTCM_1230, COM_PORT_UART2) != 10)
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 10); //Enable message every 10 seconds

if (response == false)
{
  Serial.println(F("RTCM failed to enable."));
  return (false);
}

return (response);
}

//Start survey
//The ZED-F9P is slightly different than the NEO-M8P. See the Integration manual 3.5.8 for more info.
void surveyIn()
{
  boolean response = true;

  resetSurvey();

  if (response == false)
  {
    Serial.println(F("RTCM failed to enable. Are you sure you have an ZED-F9P?"));
    return;
  }
  Serial.println(F("RTCM messages enabled"));

  response = myGPS.enableSurveyMode(60, 5.000); //Enable Survey in, 60 seconds, 5.0m
  if (response == false)
  {
    Serial.println(F("Survey start failed"));
    return;
  }
  Serial.println(F("Survey started. This will run until 60s has passed and less than 5m accuracy is achieved."));

  baseState = BASE_SURVEYING_IN_SLOW;
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
