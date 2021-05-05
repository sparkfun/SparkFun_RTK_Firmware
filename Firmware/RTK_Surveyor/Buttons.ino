//Change between Rover or Base depending on switch state
void checkSetupButton()
{
  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == LOW) //Switch is set to base mode
  {
    if (buttonPreviousState == BUTTON_ROVER)
    {
      buttonPreviousState = BUTTON_BASE;
      changeState(STATE_BASE_NOT_STARTED);
    }
  }
  else if (digitalRead(baseSwitch) == HIGH) //Switch is set to Rover
  {
    if (buttonPreviousState == BUTTON_BASE)
    {
      buttonPreviousState = BUTTON_ROVER;
      changeState(STATE_ROVER_NOT_STARTED);
    }
  }
}
