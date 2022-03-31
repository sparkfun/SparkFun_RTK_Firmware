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

  //online.battery = true;
}

//Set LEDs for output and configure PWM
void beginLEDs()
{
//  if (productVariant == RTK_SURVEYOR)
//  {
//    pinMode(pin_positionAccuracyLED_1cm, OUTPUT);
//    pinMode(pin_positionAccuracyLED_10cm, OUTPUT);
//    pinMode(pin_positionAccuracyLED_100cm, OUTPUT);
//    pinMode(pin_baseStatusLED, OUTPUT);
//    pinMode(pin_bluetoothStatusLED, OUTPUT);
//    pinMode(pin_setupButton, INPUT_PULLUP); //HIGH = rover, LOW = base
//
//    digitalWrite(pin_positionAccuracyLED_1cm, LOW);
//    digitalWrite(pin_positionAccuracyLED_10cm, LOW);
//    digitalWrite(pin_positionAccuracyLED_100cm, LOW);
//    digitalWrite(pin_baseStatusLED, LOW);
//    digitalWrite(pin_bluetoothStatusLED, LOW);
//
//    ledcSetup(ledRedChannel, pwmFreq, pwmResolution);
//    ledcSetup(ledGreenChannel, pwmFreq, pwmResolution);
//    ledcSetup(ledBTChannel, pwmFreq, pwmResolution);
//
//    ledcAttachPin(pin_batteryLevelLED_Red, ledRedChannel);
//    ledcAttachPin(pin_batteryLevelLED_Green, ledGreenChannel);
//    ledcAttachPin(pin_bluetoothStatusLED, ledBTChannel);
//
//    ledcWrite(ledRedChannel, 0);
//    ledcWrite(ledGreenChannel, 0);
//    ledcWrite(ledBTChannel, 0);
//  }
}
