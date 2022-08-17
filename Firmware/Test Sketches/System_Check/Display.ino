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

void displayHelloWorld()
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Hello", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("World", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();
  }
}

//Given text, and location, print text center of the screen
void printTextCenter(const char *text, uint8_t yPos, QwiicFont & fontType, uint8_t kerning, bool highlight) //text, y, font type, kearning, inverted
{
  oled.setFont(fontType);
  oled.setDrawMode(grROPXOR);

  uint8_t fontWidth = fontType.width;
  if (fontWidth == 8) fontWidth = 7; //8x16, but widest character is only 7 pixels.

  uint8_t xStart = (oled.getWidth() / 2) - ((strlen(text) * (fontWidth + kerning)) / 2) + 1;

  uint8_t xPos = xStart;
  for (int x = 0 ; x < strlen(text) ; x++)
  {
    oled.setCursor(xPos, yPos);
    oled.print(text[x]);
    xPos += fontWidth + kerning;
  }

  if (highlight) //Draw a box, inverted over text
  {
    uint8_t textPixelWidth = strlen(text) * (fontWidth + kerning);

    //Error check
    int xBoxStart = xStart - 5;
    if (xBoxStart < 0) xBoxStart = 0;
    int xBoxEnd = textPixelWidth + 9;
    if (xBoxEnd > oled.getWidth() - 1) xBoxEnd = oled.getWidth() - 1;

    oled.rectangleFill(xBoxStart, yPos, xBoxEnd, 12, 1); //x, y, width, height, color
  }
}
