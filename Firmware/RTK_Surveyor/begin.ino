//Initial startup functions for GNSS, SD, display, radio, etc

void beginSD()
{
  //Setup FAT file access semaphore
  if (xFATSemaphore == NULL)
  {
    xFATSemaphore = xSemaphoreCreateMutex();
    if (xFATSemaphore != NULL)
      xSemaphoreGive(xFATSemaphore);  //Make the file system available for use
  }

  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

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
        //if (SD.begin(PIN_MICROSD_CHIP_SELECT, spi, spiFreq) == true) break;
      }

      if (tries == maxTries)
      {
        Serial.println(F("SD init failed. Is card present? Formatted?"));
        digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
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
      delay(5000);
      online.microSD = false;
      return;
    }

    online.microSD = true;
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

    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(10, 2); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("SparkFun"));

    oled.setCursor(21, 13);
    oled.setFontType(1);
    oled.print(F("RTK"));

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
      blinkError(ERROR_NO_I2C);
    }
  }

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

  online.gnss = true;

  Serial.println(F("GNSS configuration complete"));
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
