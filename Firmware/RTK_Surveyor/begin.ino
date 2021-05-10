//Initial startup functions for GNSS, SD, display, radio, etc

//Based on hardware features, determine if this is RTK Surveyor or RTK Express hardware
//Must be called after Wire.begin so that we can do I2C tests
void beginBoard()
{
  productVariant = RTK_SURVEYOR;
  if (isConnected(0x19) == true) //Check for accelerometer
  {
    productVariant = RTK_EXPRESS;
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
    pin_baseSwitch = 5;
    pin_microSD_CS = 25;
    pin_zed_tx_ready = 26;
    pin_zed_reset = 27;
    pin_batteryLevel_alert = 36;

    strcpy(platformFilePrefix, "SFE_Surveyor");
  }
  else if (productVariant == RTK_EXPRESS)
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

    //Check reset reason. If system rese
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
      powerOnCheck(); //Only do check if we POR start
      reuseLastLog = false; //Start new log
    }
    else
    {
      reuseLastLog = true; //Atempt to reuse previous log
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

    pinMode(pin_setupButton, INPUT_PULLUP);

    setMuxport(settings.dataPortChannel); //Set mux to user's choice: NMEA, I2C, PPS, or DAC

    reuseLastLog = true; //Atempt to reuse previous log

    strcpy(platformFilePrefix, "SFE_Express");
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

    if (sd.begin(SD_CONFIG) == false)
    {
      int tries = 0;
      int maxTries = 2;
      for ( ; tries < maxTries ; tries++)
      {
        Serial.printf("SD init failed. Trying again %d out of %d\n\r", tries + 1, maxTries);

        delay(250); //Give SD more time to power up, then try again
        if (sd.begin(SD_CONFIG) == true) break;
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

    if (createTestFile() == false)
    {
      Serial.println(F("Failed to create test file. Format SD card with 'SD Card Formatter'."));
      displaySDFail(5000);
      online.microSD = false;
      return;
    }

    online.microSD = true;

    //Setup FAT file access semaphore
    if (xFATSemaphore == NULL)
    {
      xFATSemaphore = xSemaphoreCreateMutex();
      if (xFATSemaphore != NULL)
        xSemaphoreGive(xFATSemaphore);  //Make the file system available for use
    }
  }
  else
  {
    online.microSD = false;
  }
}

//We do not start the UART2 for GNSS->BT reception here because the interrupts would be pinned to core 1
//competing with I2C interrupts
//See issue: https://github.com/espressif/arduino-esp32/issues/3386
//We instead start a task that runs on core 0, that then begins serial
void beginUART2()
{
  if (startUART2TaskHandle == NULL) xTaskCreatePinnedToCore(
      startUART2Task,
      "UARTStart", //Just for humans
      2000, //Stack Size
      NULL, //Task input parameter
      0, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
      &startUART2TaskHandle, //Task handle
      0); //Core where task should run, 0=core, 1=Arduino

  while (uart2Started == false) //Wait for task to run once
    delay(1);
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

    displaySplash();
  }
}

//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (logUBXMessages() == true)
  {
    i2cGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C
  }

  // RAWX messages can be over 2KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
  // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
  // to hold the backlog of data if/when this happens.
  // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
  i2cGNSS.setFileBufferSize(gnssFileBufferSize); // setFileBufferSize must be called _before_ .begin

  if (i2cGNSS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (i2cGNSS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      displayGNSSFail(0);
      blinkError(ERROR_NO_I2C);
    }
  }

  //Increase transactions to reduce transfer time
  i2cGNSS.i2cTransactionSize = 128;

  //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
  //  if (i2cGNSS.getModuleInfo(1100) == true) // Try to get the module info
  //  {
  //    if (strcmp(i2cGNSS.minfo.extension[1], latestZEDFirmware) != 0)
  //    {
  //      Serial.print(F("The ZED-F9P appears to have outdated firmware. Found: "));
  //      Serial.println(i2cGNSS.minfo.extension[1]);
  //      Serial.print(F("The Surveyor works best with "));
  //      Serial.println(latestZEDFirmware);
  //      Serial.print(F("Please upgrade using u-center."));
  //      Serial.println();
  //    }
  //    else
  //    {
  //      Serial.println(F("ZED-F9P firmware is current"));
  //    }
  //  }

  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure module. Trying again."));
    delay(1000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure module. Hard stop."));
      blinkError(ERROR_GPS_CONFIG_FAIL);
    }
  }

  Serial.println(F("GNSS configuration complete"));

  online.gnss = true;
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
    pinMode(pin_baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

    digitalWrite(pin_positionAccuracyLED_1cm, LOW);
    digitalWrite(pin_positionAccuracyLED_10cm, LOW);
    digitalWrite(pin_positionAccuracyLED_100cm, LOW);
    digitalWrite(pin_baseStatusLED, LOW);
    digitalWrite(pin_bluetoothStatusLED, LOW);

    ledcSetup(ledRedChannel, freq, resolution);
    ledcSetup(ledGreenChannel, freq, resolution);

    ledcAttachPin(pin_batteryLevelLED_Red, ledRedChannel);
    ledcAttachPin(pin_batteryLevelLED_Green, ledGreenChannel);

    ledcWrite(ledRedChannel, 0);
    ledcWrite(ledGreenChannel, 0);
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

//Begin onboard accelerometer if available
void beginAccelerometer()
{
  if (productVariant == RTK_EXPRESS)
  {
    if (accel.begin() == false)
    {
      Serial.println("Accelerometer not detected.");
      online.accelerometer = false;
      return;
    }

    //The larger the avgAmount the faster we should read the sensor
    //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
    accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second

    online.accelerometer = true;
  }
}
