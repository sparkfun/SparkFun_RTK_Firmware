//Wait for survey in to complete
bool updateSurveyInStatus()
{
  //If user wants to use fixed coordinates, do so immediately
  if (settings.fixedBase == true)
  {
    bool response = startFixedBase();
    if (response == true)
    {
      baseState = BASE_TRANSMITTING;
      return (true);
    }
    else
    {
      return (false);
    }
  }

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
        //Current positional accuracy is within 5m so start survey
        surveyIn();
      }
      else
      {
        Serial.print("Waiting for Horz Accuracy < 5 meters: ");
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

          //If we are greater than a meter out of our objective, blink slow
          if (myGPS.svin.meanAccuracy > (settings.observationPositionAccuracy + 1.0))
            baseState = BASE_SURVEYING_IN_SLOW;
          else
            baseState = BASE_SURVEYING_IN_FAST;

          if (myGPS.svin.observationTime > maxSurveyInWait_s)
          {
            Serial.printf("Survey-In took more than %d minutes. Restarting survey-in process.\n", maxSurveyInWait_s / 60);

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

  bool response = myGPS.enableSurveyMode(settings.observationSeconds, settings.observationPositionAccuracy); //Enable Survey in, with user parameters
  if (response == false)
  {
    Serial.println(F("Survey start failed"));
    return;
  }
  Serial.printf("Survey started. This will run until %d seconds have passed and less than %0.03f meter accuracy is achieved.\n",
                settings.observationSeconds,
                settings.observationPositionAccuracy
               );

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

//Start the base using fixed coordinates
bool startFixedBase()
{
  bool response = false;

  if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
  {
    //Break ECEF into main and high precision parts
    //The type casting should not effect rounding of original double cast coordinate
    long majorEcefX = settings.fixedEcefX * 100;
    long minorEcefX = ((settings.fixedEcefX * 100.0) - majorEcefX) * 100.0;
    long majorEcefY = settings.fixedEcefY * 100;
    long minorEcefY = ((settings.fixedEcefY * 100.0) - majorEcefY) * 100.0;
    long majorEcefZ = settings.fixedEcefZ * 100;
    long minorEcefZ = ((settings.fixedEcefZ * 100.0) - majorEcefZ) * 100.0;

    //    Serial.printf("fixedEcefY (should be -4716808.5807): %0.04f\n", settings.fixedEcefY);
    //    Serial.printf("major (should be -471680858): %d\n", majorEcefY);
    //    Serial.printf("minor (should be -7): %d\n", minorEcefY);

    //Units are cm with a high precision extension so -1234.5678 should be called: (-123456, -78)
    //-1280208.308,-4716803.847,4086665.811 is SparkFun HQ so...
    response = myGPS.setStaticPosition(majorEcefX, minorEcefX,
                                       majorEcefY, minorEcefY,
                                       majorEcefZ, minorEcefZ
                                      ); //With high precision 0.1mm parts
  }
  else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEOGRAPHIC)
  {
    //Break coordinates into main and high precision parts
    //The type casting should not effect rounding of original double cast coordinate
    int64_t majorLat = settings.fixedLat * 10000000;
    int64_t minorLat = ((settings.fixedLat * 10000000) - majorLat) * 100;
    int64_t majorLong = settings.fixedLong * 10000000;
    int64_t minorLong = ((settings.fixedLong * 10000000) - majorLong) * 100;
    int32_t majorAlt = settings.fixedAltitude * 100;
    int32_t minorAlt = ((settings.fixedAltitude * 100) - majorAlt) * 100;

    Serial.printf("fixedLat (should be -105.184774720): %0.09f\n", settings.fixedLat);
    Serial.printf("major (should be -1051847747): %lld\n", majorLat);
    Serial.printf("minor (should be -20): %lld\n", minorLat);

    Serial.printf("fixedLong (should be 40.090335429): %0.09f\n", settings.fixedLong);
    Serial.printf("major (should be 400903354): %lld\n", majorLong);
    Serial.printf("minor (should be 29): %lld\n", minorLong);

    Serial.printf("fixedAlt (should be 1560.2284): %0.04f\n", settings.fixedAltitude);
    Serial.printf("major (should be 156022): %ld\n", majorAlt);
    Serial.printf("minor (should be 84): %ld\n", minorAlt);

    response = myGPS.setStaticPosition(
                 majorLong, minorLong,
                 majorLat, minorLat,
                 majorAlt, minorAlt,
                 true);
  }

  if (response == true)
  {
    Serial.println("Static Base Started Successfully");
  }

  return (response);
}
