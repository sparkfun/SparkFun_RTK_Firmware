static QwiicMicroOLED oled;
static uint32_t blinking_icons;
static uint32_t icons;

void beginDisplay()
{
  blinking_icons = 0;
  if (oled.begin() == true)
  {
    online.display = true;

    Serial.println(F("Display started"));
    //displaySplash();
    splashStart = millis();
  }
  else
  {
    if (productVariant == RTK_SURVEYOR)
    {
      Serial.println(F("Display not detected"));
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
    {
      Serial.println(F("Display Error: Not detected."));
    }
  }
}
