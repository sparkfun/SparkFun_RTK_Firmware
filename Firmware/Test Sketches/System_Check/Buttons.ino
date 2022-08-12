//If we have a power button tap, or if the display is not yet started (no I2C!)
//then don't display a shutdown screen
void powerDown(bool displayInfo)
{
  //Disable SD card use
  //endSD(false, false);

  //Prevent other tasks from logging, even if access to the microSD card was denied
  online.logging = false;

  if (displayInfo == true)
  {
    //displayShutdown();
    //delay(2000);
  }

  pinMode(pin_powerSenseAndControl, OUTPUT);
  digitalWrite(pin_powerSenseAndControl, LOW);

  pinMode(pin_powerFastOff, OUTPUT);
  digitalWrite(pin_powerFastOff, LOW);

  while (1)
    delay(1);
}
