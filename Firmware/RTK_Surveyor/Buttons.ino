//User has pressed the power button to turn on the system
//Was it an accidental bump or do they really want to turn on?
//Let's make sure they continue to press for 500ms
void powerOnCheck()
{
  powerPressedStartTime = millis();
  if (digitalRead(pin_powerSenseAndControl) == LOW)
    delay(500);

#ifndef ENABLE_DEVELOPER
  if (digitalRead(pin_powerSenseAndControl) != LOW)
    powerDown(false); //Power button tap. Returning to off state.
#endif

  powerPressedStartTime = 0; //Reset var to return to normal 'on' state
}

//If we have a power button tap, or if the display is not yet started (no I2C!)
//then don't display a shutdown screen
void powerDown(bool displayInfo)
{
  //Disable SD card use
  endSD(false, false);

  //Prevent other tasks from logging, even if access to the microSD card was denied
  online.logging = false;

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
