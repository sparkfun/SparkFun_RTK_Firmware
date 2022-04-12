//Initial startup functions for GNSS, SD, display, radio, etc

//Based on hardware features, determine if this is RTK Surveyor or RTK Express hardware
//Must be called after Wire.begin so that we can do I2C tests
void beginBoard()
{
  //Use ADC to check 50% resistor divider
  int pin_adc_rtk_facet = 35;
  if (analogReadMilliVolts(pin_adc_rtk_facet) > (3300 / 2 * 0.9) && analogReadMilliVolts(pin_adc_rtk_facet) < (3300 / 2 * 1.1))
  {
    productVariant = RTK_FACET;
  }
  else if (isConnected(0x19) == true) //Check for accelerometer
  {
    productVariant = RTK_EXPRESS;
  }
  else
  {
    productVariant = RTK_SURVEYOR;
  }

  //Setup hardware pins
  if (productVariant == RTK_SURVEYOR)
  {
    pin_batteryLevelLED_Red = 32;
    pin_batteryLevelLED_Green = 33;
    pin_positionAccuracyLED_1cm = 2;
    pin_positionAccuracyLED_10cm = 15;
    pin_positionAccuracyLED_100cm = 13;
    pin_baseStatusLED = 4;
    pin_bluetoothStatusLED = 12;
    pin_setupButton = 5;
    pin_microSD_CS = 25;
    pin_zed_tx_ready = 26;
    pin_zed_reset = 27;
    pin_batteryLevel_alert = 36;

    strcpy(platformFilePrefix, "SFE_Surveyor");
    strcpy(platformPrefix, "Surveyor");
  }
  else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
  {
    pin_muxA = 2;
    pin_muxB = 4;
    pin_powerSenseAndControl = 13;
    pin_setupButton = 14;
    pin_microSD_CS = 25;
    pin_dac26 = 26;
    pin_powerFastOff = 27;
    pin_adc39 = 39;

    pinMode(pin_powerSenseAndControl, INPUT_PULLUP);
    pinMode(pin_powerFastOff, INPUT);

    pinMode(pin_setupButton, INPUT_PULLUP);

    if (productVariant == RTK_EXPRESS)
    {
      strcpy(platformFilePrefix, "SFE_Express");
      strcpy(platformPrefix, "Express");
    }
    else if (productVariant == RTK_EXPRESS_PLUS)
    {
      strcpy(platformFilePrefix, "SFE_Express_Plus");
      strcpy(platformPrefix, "Express Plus");
    }
  }
  else if (productVariant == RTK_FACET)
  {
    //v11
    pin_muxA = 2;
    pin_muxB = 0;
    pin_powerSenseAndControl = 13;
    pin_peripheralPowerControl = 14;
    pin_microSD_CS = 25;
    pin_dac26 = 26;
    pin_powerFastOff = 27;
    pin_adc39 = 39;

    pin_radio_rx = 33;
    pin_radio_tx = 32;
    pin_radio_rst = 15;
    pin_radio_pwr = 4;
    pin_radio_cts = 5;
    //pin_radio_rts = 255; //Not implemented

    pinMode(pin_powerSenseAndControl, INPUT_PULLUP);
    pinMode(pin_powerFastOff, INPUT);

    pinMode(pin_peripheralPowerControl, OUTPUT);
    digitalWrite(pin_peripheralPowerControl, HIGH); //Turn on SD, ZED, etc

    //CTS is active low. ESP32 pin 5 has pullup at POR. We must drive it low.
    pinMode(pin_radio_cts, OUTPUT);
    digitalWrite(pin_radio_cts, LOW);

    strcpy(platformFilePrefix, "SFE_Facet");
    strcpy(platformPrefix, "Facet");
  }

  Serial.printf("SparkFun RTK %s v%d.%d-%s\r\n", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
}

//Depending on platform and previous power down state, set system state
void beginSystemState()
{
  if (productVariant == RTK_SURVEYOR)
  {
    setupBtn = new Button(pin_setupButton); //Create the button in memory
  }
  else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
  {
    setupBtn = new Button(pin_setupButton); //Create the button in memory
    powerBtn = new Button(pin_powerSenseAndControl); //Create the button in memory
  }
  else if (productVariant == RTK_FACET)
  {
    powerBtn = new Button(pin_powerSenseAndControl); //Create the button in memory
  }

  //Starts task for monitoring button presses
  if (ButtonCheckTaskHandle == NULL)
    xTaskCreate(
      ButtonCheckTask,
      "BtnCheck", //Just for humans
      buttonTaskStackSize, //Stack Size
      NULL, //Task input parameter
      ButtonCheckTaskPriority,
      &ButtonCheckTaskHandle); //Task handle
}
