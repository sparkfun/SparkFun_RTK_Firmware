//Wait for survey in to complete
bool updateSurveyInStatus()
{
  //Update the LEDs only every second or so
  if (millis() - lastBaseUpdate > 1000)
  {
    lastBaseUpdate += 1000;

    if (baseState == BASE_SURVEYING_IN_NOTSTARTED)
    {
      //Check for <5m horz accuracy
      uint32_t accuracy = myGPS.getHorizontalAccuracy(250); //This call defaults to 1100ms and can cause the core to crash with WDT timeout

      float f_accuracy = accuracy;
      f_accuracy = f_accuracy / 10000.0; // Convert the horizontal accuracy (mm * 10^-1) to a float

      if (f_accuracy > 0.0 && f_accuracy < 5.0)
      {
        //We've got a good positional accuracy, start survey
        surveyIn();
      }
      else
      {
        Serial.print("Waiting for Horz Accuracy < 5m: ");
        Serial.print(f_accuracy, 2); // Print the accuracy with 2 decimal places
        Serial.println();
      }

    } //baseState = Surveying In Not started
    else
    {

      bool response = myGPS.getSurveyStatus(2000); //Query module for SVIN status with 2000ms timeout (req can take a long time)
      if (response == true)
      {
        if (myGPS.svin.valid == true)
        {
          Serial.println(F("Base survey complete! RTCM now broadcasting."));
          baseState = BASE_TRANSMITTING;

          digitalWrite(baseStatusLED, HIGH); //Turn on LED
        }
        else
        {
          byte SIV = myGPS.getSIV();

          Serial.print(F("Time elapsed: "));
          Serial.print((String)myGPS.svin.observationTime);
          Serial.print(F(" Accuracy: "));
          Serial.print((String)myGPS.svin.meanAccuracy);
          Serial.print(F(" SIV: "));
          Serial.print(SIV);
          Serial.println();

          SerialBT.print(F("Time elapsed: "));
          SerialBT.print((String)myGPS.svin.observationTime);
          SerialBT.print(F(" Accuracy: "));
          SerialBT.print((String)myGPS.svin.meanAccuracy);
          SerialBT.print(F(" SIV: "));
          SerialBT.print(SIV);
          SerialBT.println();

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
    } //baseState = Surveying In Slow or fast
  } //Check every second

  //Update the Base LED accordingly
  if (baseState == BASE_SURVEYING_IN_NOTSTARTED || baseState == BASE_SURVEYING_IN_SLOW)
  {
    if (millis() - baseStateBlinkTime > 2000)
    {
      baseStateBlinkTime += 2000;
      Serial.println(F("Slow blink"));

      if (digitalRead(baseStatusLED) == LOW)
        digitalWrite(baseStatusLED, HIGH);
      else
        digitalWrite(baseStatusLED, LOW);
    }
  }
  else if (baseState == BASE_SURVEYING_IN_FAST)
  {
    if (millis() - baseStateBlinkTime > 500)
    {
      baseStateBlinkTime += 500;
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

  digitalWrite(positionAccuracyLED_1cm, LOW);
  digitalWrite(positionAccuracyLED_10cm, LOW);
  digitalWrite(positionAccuracyLED_100cm, LOW);

  // Set dynamic model
  if (myGPS.getDynamicModel() != DYN_MODEL_STATIONARY)
  {
    if (myGPS.setDynamicModel(DYN_MODEL_STATIONARY) == false)
    {
      Serial.println(F("setDynamicModel failed!"));
      return (false);
    }
  }

  //In base mode the Surveyor should output RTCM over UART1, UART2, and USB ports:
  //(Primary) UART2 in case the Surveyor is connected via radio to rover
  //(Seconday) USB in case the Surveyor is used as an NTRIP caster
  //(Tertiary) UART1 in case Surveyor is sending RTCM to phone that is then NTRIP caster
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

  if (getRTCMSettings(UBX_RTCM_1005, COM_PORT_UART1) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART1, 1); //Enable message 1005 to output through UART2, message every second
  if (getRTCMSettings(UBX_RTCM_1074, COM_PORT_UART1) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_UART1, 1);
  if (getRTCMSettings(UBX_RTCM_1084, COM_PORT_UART1) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_UART1, 1);
  if (getRTCMSettings(UBX_RTCM_1094, COM_PORT_UART1) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_UART1, 1);
  if (getRTCMSettings(UBX_RTCM_1124, COM_PORT_UART1) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_UART1, 1);
  if (getRTCMSettings(UBX_RTCM_1230, COM_PORT_UART1) != 10)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART1, 10); //Enable message every 10 seconds

  if (getRTCMSettings(UBX_RTCM_1005, COM_PORT_USB) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_USB, 1); //Enable message 1005 to output through UART2, message every second
  if (getRTCMSettings(UBX_RTCM_1074, COM_PORT_USB) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_USB, 1);
  if (getRTCMSettings(UBX_RTCM_1084, COM_PORT_USB) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_USB, 1);
  if (getRTCMSettings(UBX_RTCM_1094, COM_PORT_USB) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_USB, 1);
  if (getRTCMSettings(UBX_RTCM_1124, COM_PORT_USB) != 1)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_USB, 1);
  if (getRTCMSettings(UBX_RTCM_1230, COM_PORT_USB) != 10)
    response &= myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_USB, 10); //Enable message every 10 seconds

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
  resetSurvey();

  bool response = myGPS.enableSurveyMode(60, 5.000); //Enable Survey in, 60 seconds, 5.0m
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
  response &= myGPS.enableSurveyMode(1000, 400.000); //Enable Survey in with bogus values
  delay(500);
  response &= myGPS.disableSurveyMode(); //Disable survey
  delay(500);
}
