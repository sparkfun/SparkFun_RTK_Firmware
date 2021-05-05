//Regularly update subsystems. Called from main loop (aka not tasks).

//Change between Rover or Base depending on switch state
void checkSetupButton()
{
  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == LOW) //Switch is set to base mode
  {
    if (systemState == STATE_ROVER_NO_FIX ||
        systemState == STATE_ROVER_FIX ||
        systemState == STATE_ROVER_RTK_FLOAT ||
        systemState == STATE_ROVER_RTK_FIX)
    {
      displayBaseStart();

      //Configure for base mode
      Serial.println(F("Base Mode"));

      //Restart Bluetooth with 'Base' name
      //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
      beginBluetooth();

      if (settings.fixedBase == false)
      {
        //Don't configure base if we are going to do a survey in. We need to wait for surveyInStartingAccuracy to be achieved in state machine
        changeState(STATE_BASE_TEMP_SURVEY_NOT_STARTED);
      }
      else if (settings.fixedBase == true)
      {
        if (configureUbloxModuleBase() == false)
        {
          Serial.println(F("Base config failed!"));
          displayBaseFail();
          delay(1000);
          return;
        }

        bool response = startFixedBase();
        if (response == true)
        {
          changeState(STATE_BASE_FIXED_TRANSMITTING);
        }
        else
        {
          Serial.println(F("Fixed base start failed"));
          displayBaseFail();
          delay(1000);
          return;
        }
      }

      digitalWrite(baseStatusLED, HIGH);
      displayBaseSuccess();
      delay(500);
    }
  }
  else if (digitalRead(baseSwitch) == HIGH) //Switch is set to Rover
  {
    if (systemState == STATE_BASE_TEMP_SURVEY_NOT_STARTED ||
        systemState == STATE_BASE_TEMP_SURVEY_STARTED ||
        systemState == STATE_BASE_TEMP_TRANSMITTING ||
        systemState == STATE_BASE_TEMP_WIFI_STARTED ||
        systemState == STATE_BASE_TEMP_WIFI_CONNECTED ||
        systemState == STATE_BASE_TEMP_CASTER_STARTED ||
        systemState == STATE_BASE_TEMP_CASTER_CONNECTED ||
        systemState == STATE_BASE_FIXED_TRANSMITTING ||
        systemState == STATE_BASE_FIXED_WIFI_STARTED ||
        systemState == STATE_BASE_FIXED_WIFI_CONNECTED ||
        systemState == STATE_BASE_FIXED_CASTER_STARTED ||
        systemState == STATE_BASE_FIXED_CASTER_CONNECTED)
    {
      displayRoverStart();

      //Configure for rover mode
      Serial.println(F("Rover Mode"));

      //If we are survey'd in, but switch is rover then disable survey
      if (configureUbloxModuleRover() == false)
      {
        Serial.println(F("Rover config failed!"));
        displayRoverFail();
        return;
      }

      beginBluetooth(); //Restart Bluetooth with 'Rover' name

      changeState(STATE_ROVER_NO_FIX);

      digitalWrite(baseStatusLED, LOW);
      displayRoverSuccess();
      delay(500);
    }
  }
}
