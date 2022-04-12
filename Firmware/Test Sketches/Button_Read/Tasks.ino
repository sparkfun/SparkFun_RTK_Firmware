
//Monitor momentary buttons or rocker switches, depending on platform
void ButtonCheckTask(void *e)
{
  if (setupBtn != NULL) setupBtn->begin();
  if (powerBtn != NULL) powerBtn->begin();

  while (true)
  {
    if (productVariant == RTK_SURVEYOR)
    {
      setupBtn->read();

      //When switch is set to '1' = BASE, pin will be shorted to ground
      if (setupBtn->isPressed()) //Switch is set to base mode
      {
        Serial.println("Rocker: Base");
        delay(100);
      }
      else if (setupBtn->wasReleased()) //Switch is set to Rover
      {
        Serial.println("Rocker: Rover");
      }
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS) //Express: Check both of the momentary switches
    {
      setupBtn->read();
      powerBtn->read();

      if (setupBtn->isPressed())
      {
        Serial.println("Setup button pressed");
        delay(100);
      }
      else if (setupBtn->wasReleased())
      {
        Serial.println("Setup button released");
      }
      if (powerBtn->isPressed())
      {
        Serial.println("Power button pressed");
        delay(100);
      }
      else if (powerBtn->wasReleased())
      {
        Serial.println("Power button released");
      }
    } //End Platform = RTK Express

    else if (productVariant == RTK_FACET) //Check one momentary button
    {
      powerBtn->read();

      if (powerBtn->isPressed())
      {
        Serial.println("Power button pressed");
        delay(100);
      }
      else if (powerBtn->wasReleased())
      {
        Serial.println("Power button released");
      }
    } //End Platform = RTK Facet

    delay(1); //Poor man's way of feeding WDT. Required to prevent Priority 1 tasks from causing WDT reset
    taskYIELD();
  }
}
