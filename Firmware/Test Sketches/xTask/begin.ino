//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (myGPS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGPS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      while(1);
    }
  }

  //myGPS.enableDebugging();

  bool response = true;
  
  response &= myGPS.setAutoPVT(true, false); //Tell the GPS to "send" each solution, but do not update stale data when accessed
  response &= myGPS.setAutoHPPOSLLH(true, false); //Tell the GPS to "send" each high res solution, but do not update stale data when accessed

  response &= myGPS.setNavigationFrequency(4); //Set output in Hz

  if(response == false)
  {
    Serial.println("PVT failed!");
    while(1);
  }

  Serial.println(F("GNSS configuration complete"));
}

//Configure the on board MAX17048 fuel gauge
void beginFuelGauge()
{
  // Set up the MAX17048 LiPo fuel gauge
  if (lipo.begin() == false)
  {
    Serial.println(F("MAX17048 not detected. Continuing."));
    return;
  }

  //Always use hibernate mode
  if (lipo.getHIBRTActThr() < 0xFF) lipo.setHIBRTActThr((uint8_t)0xFF);
  if (lipo.getHIBRTHibThr() < 0xFF) lipo.setHIBRTHibThr((uint8_t)0xFF);

  Serial.println(F("MAX17048 configuration complete"));
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    online.display = true;

    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(10, 2); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print("SparkFun");

    oled.setCursor(21, 13);
    oled.setFontType(1);
    oled.print("RTK");

    int surveyorTextY = 25;
    int surveyorTextX = 2;
    int surveyorTextKerning = 8;
    oled.setFontType(1);

    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("S");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("u");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("v");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("e");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("y");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("o");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    oled.setCursor(20, 41);
    oled.setFontType(0); //Set font to smallest
    oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    oled.display();
  }
}

//Set LEDs for output and configure PWM
void beginLEDs()
{
  pinMode(positionAccuracyLED_1cm, OUTPUT);
  pinMode(positionAccuracyLED_10cm, OUTPUT);
  pinMode(positionAccuracyLED_100cm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  digitalWrite(positionAccuracyLED_1cm, LOW);
  digitalWrite(positionAccuracyLED_10cm, LOW);
  digitalWrite(positionAccuracyLED_100cm, LOW);
  digitalWrite(baseStatusLED, LOW);
  digitalWrite(bluetoothStatusLED, LOW);

  ledcSetup(ledRedChannel, freq, resolution);
  ledcSetup(ledGreenChannel, freq, resolution);

  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);

  ledcWrite(ledRedChannel, 0);
  ledcWrite(ledGreenChannel, 0);
}
