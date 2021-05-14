void checkButtons()
{
  if (productVariant == RTK_SURVEYOR)
  {
    //Check rover switch and configure module accordingly
    //When switch is set to '1' = BASE, pin will be shorted to ground
    if (digitalRead(pin_baseSwitch) == LOW) //Switch is set to base mode
    {
      if (buttonPreviousState == BUTTON_ROVER)
      {
        buttonPreviousState = BUTTON_BASE;
        changeState(STATE_BASE_NOT_STARTED);
      }
    }
    else if (digitalRead(pin_baseSwitch) == HIGH) //Switch is set to Rover
    {
      if (buttonPreviousState == BUTTON_BASE)
      {
        buttonPreviousState = BUTTON_ROVER;
        changeState(STATE_ROVER_NOT_STARTED);
      }
    }
  }
  else if (productVariant == RTK_EXPRESS)
  {
    //Check power button
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    if (digitalRead(pin_powerSenseAndControl) == LOW && powerPressedStartTime == 0)
    {
      //Debounce check
      delay(debounceDelay);
      if (digitalRead(pin_powerSenseAndControl) == LOW)
      {
        powerPressedStartTime = millis();
      }
    }
    else if (digitalRead(pin_powerSenseAndControl) == LOW && powerPressedStartTime > 0)
    {
      //Debounce check
      delay(debounceDelay);
      if (digitalRead(pin_powerSenseAndControl) == LOW)
      {
        if ((millis() - powerPressedStartTime) > 2000)
        {
          powerDown(true);
        }
      }
    }
    else if (digitalRead(pin_powerSenseAndControl) == HIGH && powerPressedStartTime > 0)
    {
      //Debounce check
      delay(debounceDelay);
      if (digitalRead(pin_powerSenseAndControl) == HIGH)
      {
        Serial.print("Power button released after ms: ");
        Serial.println(millis() - powerPressedStartTime);
        powerPressedStartTime = 0; //Reset var to return to normal 'on' state

        setupByPowerButton = true; //Notify base/rover setup
      }
    }
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    //Check setup button and configure module accordingly
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    if (digitalRead(pin_setupButton) == LOW || setupByPowerButton == true)
    {
      delay(debounceDelay); //Debounce
      if (digitalRead(pin_setupButton) == LOW || setupByPowerButton == true)
      {
        //User is pressing button
        if (setupButtonState == BUTTON_RELEASED)
        {
          setupButtonState = BUTTON_PRESSED;

          setupByPowerButton = false;

          //Toggle between Rover and Base system states
          if (buttonPreviousState == BUTTON_ROVER)
          {
            buttonPreviousState = BUTTON_BASE;
            changeState(STATE_BASE_NOT_STARTED);
          }
          else if (buttonPreviousState == BUTTON_BASE)
          {
            buttonPreviousState = BUTTON_ROVER;
            changeState(STATE_ROVER_NOT_STARTED);
          }
        } //End button state check
      } //End debounce button check
    } //End first button check
    else if (digitalRead(pin_setupButton) == HIGH && setupButtonState == BUTTON_PRESSED)
    {
      //Return to unpressed state
      setupButtonState = BUTTON_RELEASED;
    }

    //Check to see if user is pressing both buttons simultaneously - show test screen
    if (digitalRead(pin_powerSenseAndControl) == LOW && digitalRead(pin_setupButton) == LOW)
    {
      delay(debounceDelay); //Debounce
      if (digitalRead(pin_powerSenseAndControl) == LOW && digitalRead(pin_setupButton) == LOW)
      {
        displayTest();
        setupButtonState = BUTTON_RELEASED;
        buttonPreviousState = BUTTON_ROVER;
        changeState(STATE_ROVER_NOT_STARTED);
      }
    }
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  }//end productVariant = Express

  else if (productVariant == RTK_FACET)
  {
    //Check power button
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    if (digitalRead(pin_powerSenseAndControl) == LOW && powerPressedStartTime == 0)
    {
      //Debounce check
      delay(debounceDelay);
      if (digitalRead(pin_powerSenseAndControl) == LOW)
      {
        powerPressedStartTime = millis();
      }
    }
    else if (digitalRead(pin_powerSenseAndControl) == LOW && powerPressedStartTime > 0)
    {
      //Debounce check
      delay(debounceDelay);
      if (digitalRead(pin_powerSenseAndControl) == LOW)
      {
        if ((millis() - powerPressedStartTime) > 2000)
        {
          powerDown(true);
        }
      }
    }
    else if (digitalRead(pin_powerSenseAndControl) == HIGH && powerPressedStartTime > 0)
    {
      //Debounce check
      delay(debounceDelay);
      if (digitalRead(pin_powerSenseAndControl) == HIGH)
      {
        Serial.print("Power button released after ms: ");
        Serial.println(millis() - powerPressedStartTime);
        powerPressedStartTime = 0; //Reset var to return to normal 'on' state
      }
    }
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    //Check setup button and configure module accordingly
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    if (digitalRead(pin_powerSenseAndControl) == LOW)
    {
      delay(debounceDelay); //Debounce
      if (digitalRead(pin_powerSenseAndControl) == LOW)
      {
        //User is pressing button
        if (setupButtonState == BUTTON_RELEASED)
        {
          setupButtonState = BUTTON_PRESSED;

          //Toggle between Rover and Base system states
          if (buttonPreviousState == BUTTON_ROVER)
          {
            buttonPreviousState = BUTTON_BASE;
            changeState(STATE_BASE_NOT_STARTED);
          }
          else if (buttonPreviousState == BUTTON_BASE)
          {
            buttonPreviousState = BUTTON_ROVER;
            changeState(STATE_ROVER_NOT_STARTED);
          }
        } //End button state check
      } //End debounce button check
    } //End first button check
    else if (digitalRead(pin_powerSenseAndControl) == HIGH && setupButtonState == BUTTON_PRESSED)
    {
      //Return to unpressed state
      setupButtonState = BUTTON_RELEASED;
    }

    //Check to see if user is pressing both buttons simultaneously - show test screen
    //    if (digitalRead(pin_powerSenseAndControl) == LOW && digitalRead(pin_setupButton) == LOW)
    //    {
    //      delay(debounceDelay); //Debounce
    //      if (digitalRead(pin_powerSenseAndControl) == LOW && digitalRead(pin_setupButton) == LOW)
    //      {
    //        displayTest();
    //        setupButtonState = BUTTON_RELEASED;
    //        buttonPreviousState = BUTTON_ROVER;
    //        changeState(STATE_ROVER_NOT_STARTED);
    //      }
    //    }
    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  }//end productVariant = Facet
}

//User has pressed the power button to turn on the system
//Was it an accidental bump or do they really want to turn on?
//Let's make sure they continue to press for two seconds
void powerOnCheck()
{
  powerPressedStartTime = millis();
  while (digitalRead(pin_powerSenseAndControl) == LOW)
  {
    delay(100); //Wait for user to stop pressing button.

    if (millis() - powerPressedStartTime > 500)
      break;
  }

  if (millis() - powerPressedStartTime < 500)
    powerDown(false); //Power button tap. Returning to off state.

  powerPressedStartTime = 0; //Reset var to return to normal 'on' state
}

//If we have a power button tap, or if the display is not yet started (no I2C!)
//then don't display a shutdown screen
void powerDown(bool displayInfo)
{
  if (online.logging == true)
  {
    //Close down file system
    ubxFile.sync();
    ubxFile.close();
    online.logging = false;
  }

  if (displayInfo == true)
  {
    displayShutdown();
    delay(2000);
  }

  pinMode(pin_powerSenseAndControl, OUTPUT);
  digitalWrite(pin_powerSenseAndControl, LOW);

  pinMode(pin_powerFastOff, OUTPUT);
  digitalWrite(pin_powerFastOff, LOW);

  while (1)
    delay(1);
}
