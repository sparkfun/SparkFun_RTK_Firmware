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
    if (zedModuleType == PLATFORM_F9P) productVariant = RTK_EXPRESS;
    else if (zedModuleType == PLATFORM_F9R) productVariant = RTK_EXPRESS_PLUS;
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

    //Bug in ZED-F9P v1.13 firmware causes RTK LED to not light when RTK Floating with SBAS on.
    //The following changes the POR default but will be overwritten by settings in NVM or settings file
    settings.ubxConstellations[1].enabled = false;

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

    if (esp_reset_reason() == ESP_RST_POWERON)
    {
      powerOnCheck(); //Only do check if we POR start
    }

    pinMode(pin_setupButton, INPUT_PULLUP);

    setMuxport(settings.dataPortChannel); //Set mux to user's choice: NMEA, I2C, PPS, or DAC

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

    if (esp_reset_reason() == ESP_RST_POWERON)
    {
      powerOnCheck(); //Only do check if we POR start
    }

    pinMode(pin_peripheralPowerControl, OUTPUT);
    digitalWrite(pin_peripheralPowerControl, HIGH); //Turn on SD, ZED, etc

    setMuxport(settings.dataPortChannel); //Set mux to user's choice: NMEA, I2C, PPS, or DAC

    //CTS is active low. ESP32 pin 5 has pullup at POR. We must drive it low.
    pinMode(pin_radio_cts, OUTPUT);
    digitalWrite(pin_radio_cts, LOW);

    strcpy(platformFilePrefix, "SFE_Facet");
    strcpy(platformPrefix, "Facet");
  }

  Serial.printf("SparkFun RTK %s v%d.%d-%s\r\n", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

  //For all boards, check reset reason. If reset was due to wdt or panic, append last log
  if (esp_reset_reason() == ESP_RST_POWERON)
  {
    reuseLastLog = false; //Start new log

    loadSettingsPartial();
    if (settings.enableResetDisplay == true)
    {
      settings.resetCount = 0;
      recordSystemSettings(); //Record to NVM
    }
  }
  else
  {
    reuseLastLog = true; //Attempt to reuse previous log

    loadSettingsPartial();
    if (settings.enableResetDisplay == true)
    {
      settings.resetCount++;
      recordSystemSettings(); //Record to NVM
      Serial.printf("resetCount: %d\n\r", settings.resetCount);
    }

    Serial.print("Reset reason: ");
    switch (esp_reset_reason())
    {
      case ESP_RST_UNKNOWN: Serial.println(F("ESP_RST_UNKNOWN")); break;
      case ESP_RST_POWERON : Serial.println(F("ESP_RST_POWERON")); break;
      case ESP_RST_SW : Serial.println(F("ESP_RST_SW")); break;
      case ESP_RST_PANIC : Serial.println(F("ESP_RST_PANIC")); break;
      case ESP_RST_INT_WDT : Serial.println(F("ESP_RST_INT_WDT")); break;
      case ESP_RST_TASK_WDT : Serial.println(F("ESP_RST_TASK_WDT")); break;
      case ESP_RST_WDT : Serial.println(F("ESP_RST_WDT")); break;
      case ESP_RST_DEEPSLEEP : Serial.println(F("ESP_RST_DEEPSLEEP")); break;
      case ESP_RST_BROWNOUT : Serial.println(F("ESP_RST_BROWNOUT")); break;
      case ESP_RST_SDIO : Serial.println(F("ESP_RST_SDIO")); break;
      default : Serial.println(F("Unknown"));
    }
  }
}

void beginSD()
{
  pinMode(pin_microSD_CS, OUTPUT);
  digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if(settings.spiFrequency > 16)
    {
      Serial.println(("Error: SPI Frequency out of range. Default to 16MHz"));
      settings.spiFrequency = 16;
    }

    if (sd.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == false)
    {
      int tries = 0;
      int maxTries = 1;
      for ( ; tries < maxTries ; tries++)
      {
        Serial.printf("SD init failed. Trying again %d out of %d\n\r", tries + 1, maxTries);

        delay(250); //Give SD more time to power up, then try again
        if (sd.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == true) break;
      }

      if (tries == maxTries)
      {
        Serial.println(F("SD init failed. Is card present? Formatted?"));
        digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      Serial.println(F("SD change directory failed"));
      online.microSD = false;
      return;
    }

    //Setup FAT file access semaphore
    if (xFATSemaphore == NULL)
    {
      xFATSemaphore = xSemaphoreCreateMutex();
      if (xFATSemaphore != NULL)
        xSemaphoreGive(xFATSemaphore);  //Make the file system available for use
    }

    if (createTestFile() == false)
    {
      Serial.println(F("Failed to create test file. Format SD card with 'SD Card Formatter'."));
      displaySDFail(5000);
      online.microSD = false;
      return;
    }

    online.microSD = true;

    Serial.println(F("microSD online"));
    scanForFirmware(); //See if SD card contains new firmware that should be loaded at startup
  }
  else
  {
    online.microSD = false;
  }
}

//We want the UART2 interrupts to be pinned to core 0 to avoid competing with I2C interrupts
//We do not start the UART2 for GNSS->BT reception here because the interrupts would be pinned to core 1
//We instead start a task that runs on core 0, that then begins serial
//See issue: https://github.com/espressif/arduino-esp32/issues/3386
void beginUART2()
{
  if (pinUART2TaskHandle == NULL) xTaskCreatePinnedToCore(
      pinUART2Task,
      "UARTStart", //Just for humans
      2000, //Stack Size
      NULL, //Task input parameter
      0, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
      &pinUART2TaskHandle, //Task handle
      0); //Core where task should run, 0=core, 1=Arduino

  while (uart2pinned == false) //Wait for task to run once
    delay(1);
}

//Assign UART2 interrupts to the core 0. See: https://github.com/espressif/arduino-esp32/issues/3386
void pinUART2Task( void *pvParameters )
{
  serialGNSS.setRxBufferSize(SERIAL_SIZE_RX);
  serialGNSS.setTimeout(0);
  serialGNSS.begin(settings.dataPortBaud); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

  uart2pinned = true;

  vTaskDelete( NULL ); //Delete task once it has run once
}

//Serial Read/Write tasks for the F9P must be started after BT is up and running otherwise SerialBT.available will cause reboot
void startUART2Tasks()
{
  //Start the tasks for handling incoming and outgoing BT bytes to/from ZED-F9P
  if (F9PSerialReadTaskHandle == NULL)
    xTaskCreate(
      F9PSerialReadTask,
      "F9Read", //Just for humans
      readTaskStackSize, //Stack Size
      NULL, //Task input parameter
      F9PSerialReadTaskPriority, //Priority
      &F9PSerialReadTaskHandle); //Task handle

  if (F9PSerialWriteTaskHandle == NULL)
    xTaskCreate(
      F9PSerialWriteTask,
      "F9Write", //Just for humans
      writeTaskStackSize, //Stack Size
      NULL, //Task input parameter
      F9PSerialWriteTaskPriority, //Priority
      &F9PSerialWriteTaskHandle); //Task handle
}

//Stop tasks - useful when running firmware update or WiFi AP is running
void stopUART2Tasks()
{
  //Delete tasks if running
  if (F9PSerialReadTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialReadTaskHandle);
    F9PSerialReadTaskHandle = NULL;
  }
  if (F9PSerialWriteTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialWriteTaskHandle);
    F9PSerialWriteTaskHandle = NULL;
  }
}

//ESP32 requires the creation of an EEPROM space
void beginEEPROM()
{
  if (EEPROM.begin(EEPROM_SIZE) == false)
    Serial.println(F("beginEEPROM: Failed to initialize EEPROM"));
  else
    online.eeprom = true;
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    online.display = true;

    oled.setI2CTransactionSize(64); //Increase to page size of 64. Slight speed improvement over 32 bytes.

    displaySplash();
  }
  else
  {
    if (productVariant == RTK_SURVEYOR)
    {
      Serial.println(F("Display not detected"));
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET)
    {
      Serial.println(F("Display Error: Not detected."));
    }
  }
}

//Connect to ZED module and identify particulars
void beginGNSS()
{
  if (i2cGNSS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (i2cGNSS.begin() == false)
    {
      displayGNSSFail(1000);
      online.gnss = false;
      return;
    }
  }

  //Increase transactions to reduce transfer time
  i2cGNSS.i2cTransactionSize = 128;

  //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
  if (i2cGNSS.getModuleInfo(1100) == true) // Try to get the module info
  {
    //i2cGNSS.minfo.extension[1] looks like 'FWVER=HPG 1.12'
    strcpy(zedFirmwareVersion, i2cGNSS.minfo.extension[1]);

    //Remove 'FWVER='. It's extraneous and = causes settings file parsing issues
    char *ptr = strstr(zedFirmwareVersion, "FWVER=");
    if (ptr != NULL)
      strcpy(zedFirmwareVersion, ptr + strlen("FWVER="));

    //Convert version to a uint8_t
    if (strstr(zedFirmwareVersion, "1.00") != NULL)
      zedFirmwareVersionInt = 100;
    else if (strstr(zedFirmwareVersion, "1.12") != NULL)
      zedFirmwareVersionInt = 112;
    else if (strstr(zedFirmwareVersion, "1.13") != NULL)
      zedFirmwareVersionInt = 113;
    else if (strstr(zedFirmwareVersion, "1.20") != NULL) //Mostly for F9R HPS 1.20, but also F9P HPG v1.20 Spartan future support
      zedFirmwareVersionInt = 120;
    else if (strstr(zedFirmwareVersion, "1.21") != NULL) //Future F9R HPS v1.21
      zedFirmwareVersionInt = 121;
    else if (strstr(zedFirmwareVersion, "1.30") != NULL) //ZED-F9P released Dec, 2021
      zedFirmwareVersionInt = 130;
    else
      Serial.printf("Unknown firmware version: %s\n\r", zedFirmwareVersion);

    //Determine if we have a ZED-F9P (Express/Facet) or an ZED-F9R (Express Plus/Facet Plus)
    if (strstr(i2cGNSS.minfo.extension[3], "ZED-F9P") != NULL)
    {
      zedModuleType = PLATFORM_F9P;
    }
    else if (strstr(i2cGNSS.minfo.extension[3], "ZED-F9R") != NULL)
    {
      zedModuleType = PLATFORM_F9R;
    }

    printModuleInfo(); //Print module type and firmware version
  }

  online.gnss = true;
}

//Configuration can take >1s so configure during splash
void configureGNSS()
{
  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure GNSS module. Trying again."));
    delay(1000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure GNSS module."));
      displayGNSSFail(1000);
      online.gnss = false;
      return;
    }
  }

  Serial.println(F("GNSS configuration complete"));
}

//Set LEDs for output and configure PWM
void beginLEDs()
{
  if (productVariant == RTK_SURVEYOR)
  {
    pinMode(pin_positionAccuracyLED_1cm, OUTPUT);
    pinMode(pin_positionAccuracyLED_10cm, OUTPUT);
    pinMode(pin_positionAccuracyLED_100cm, OUTPUT);
    pinMode(pin_baseStatusLED, OUTPUT);
    pinMode(pin_bluetoothStatusLED, OUTPUT);
    pinMode(pin_setupButton, INPUT_PULLUP); //HIGH = rover, LOW = base

    digitalWrite(pin_positionAccuracyLED_1cm, LOW);
    digitalWrite(pin_positionAccuracyLED_10cm, LOW);
    digitalWrite(pin_positionAccuracyLED_100cm, LOW);
    digitalWrite(pin_baseStatusLED, LOW);
    digitalWrite(pin_bluetoothStatusLED, LOW);

    ledcSetup(ledRedChannel, pwmFreq, pwmResolution);
    ledcSetup(ledGreenChannel, pwmFreq, pwmResolution);
    ledcSetup(ledBTChannel, pwmFreq, pwmResolution);

    ledcAttachPin(pin_batteryLevelLED_Red, ledRedChannel);
    ledcAttachPin(pin_batteryLevelLED_Green, ledGreenChannel);
    ledcAttachPin(pin_bluetoothStatusLED, ledBTChannel);

    ledcWrite(ledRedChannel, 0);
    ledcWrite(ledGreenChannel, 0);
    ledcWrite(ledBTChannel, 0);
  }
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

  online.battery = true;
}

//Begin accelerometer if available
void beginAccelerometer()
{
  if (accel.begin() == false)
  {
    online.accelerometer = false;

    displayAccelFail(1000);

    return;
  }

  //The larger the avgAmount the faster we should read the sensor
  //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
  accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second

  Serial.println(F("Accelerometer configuration complete"));

  online.accelerometer = true;
}

//Depending on platform and previous power down state, set system state
void beginSystemState()
{
  if (productVariant == RTK_SURVEYOR)
  {
    systemState = STATE_ROVER_NOT_STARTED; //Assume Rover. ButtonCheckTask_Switch() will correct as needed.

    setupBtn = new Button(pin_setupButton); //Create the button in memory
  }
  else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
  {
    systemState = settings.lastState; //Return to system state previous to power down.

    if (systemState > STATE_SHUTDOWN)
    {
      Serial.println("Unknown state - factory reset");
      factoryReset();
    }

    setupBtn = new Button(pin_setupButton); //Create the button in memory
    powerBtn = new Button(pin_powerSenseAndControl); //Create the button in memory
  }
  else if (productVariant == RTK_FACET)
  {
    systemState = settings.lastState; //Return to system state previous to power down.

    if (systemState > STATE_SHUTDOWN)
    {
      Serial.println("Unknown state - factory reset");
      factoryReset();
    }

    if (systemState == STATE_ROVER_NOT_STARTED)
      firstRoverStart = true; //Allow user to enter test screen during first rover start

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

//Setup the timepulse output on the PPS pin for external triggering
//Setup TM2 time stamp input as need
void beginExternalTriggers()
{
  if(online.gnss == false)
    return;
    
  UBX_CFG_TP5_data_t timePulseParameters;

  if (i2cGNSS.getTimePulseParameters(&timePulseParameters) == false)
    log_e("getTimePulseParameters failed!");

  timePulseParameters.tpIdx = 0; // Select the TIMEPULSE pin

  // While the module is _locking_ to GNSS time, turn off pulse
  timePulseParameters.freqPeriod = 1000000; //Set the period between pulses in us
  timePulseParameters.pulseLenRatio = 0; //Set the pulse length in us

  // When the module is _locked_ to GNSS time, make it generate 1kHz
  timePulseParameters.freqPeriodLock = settings.externalPulseTimeBetweenPulse_us; //Set the period between pulses is us
  timePulseParameters.pulseLenRatioLock = settings.externalPulseLength_us; //Set the pulse length in us

  timePulseParameters.flags.bits.active = settings.enableExternalPulse; //Make sure the active flag is set to enable the time pulse. (Set to 0 to disable.)
  timePulseParameters.flags.bits.lockedOtherSet = 1; //Tell the module to use freqPeriod while locking and freqPeriodLock when locked to GNSS time
  timePulseParameters.flags.bits.isFreq = 0; //Tell the module that we want to set the period
  timePulseParameters.flags.bits.isLength = 1; //Tell the module that pulseLenRatio is a length (in us)
  timePulseParameters.flags.bits.polarity = (uint8_t)settings.externalPulsePolarity; //Rising or failling edge type pulse

  if (i2cGNSS.setTimePulseParameters(&timePulseParameters, 1000) == false)
    log_e("setTimePulseParameters failed!");

  if (settings.enableExternalHardwareEventLogging == true)
    i2cGNSS.setAutoTIMTM2callback(&eventTriggerReceived); //Enable automatic TIM TM2 messages with callback to eventTriggerReceived
  else
    i2cGNSS.setAutoTIMTM2callback(NULL);
}
