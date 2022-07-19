void beginBoard()
{
  //Use ADC to check 50% resistor divider
  int pin_adc_rtk_facet = 35;
  uint16_t idValue = analogReadMilliVolts(pin_adc_rtk_facet);
  log_d("Board ADC ID: %d", idValue);

  if (idValue > (3300 / 2 * 0.9) && idValue < (3300 / 2 * 1.1))
  {
    productVariant = RTK_FACET;
  }
  else if (idValue > (3300 * 2 / 3 * 0.9) && idValue < (3300 * 2 / 3 * 1.1))
  {
    productVariant = RTK_FACET_LBAND;
  }
  else if (idValue > (3300 * 3.3 / 13.3 * 0.9) && idValue < (3300 * 3.3 / 13.3 * 1.1))
  {
    productVariant = RTK_EXPRESS;
  }
  else if (idValue > (3300 * 10 / 13.3 * 0.9) && idValue < (3300 * 10 / 13.3 * 1.1))
  {
    productVariant = RTK_EXPRESS_PLUS;
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
    //
    //    //Bug in ZED-F9P v1.13 firmware causes RTK LED to not light when RTK Floating with SBAS on.
    //    //The following changes the POR default but will be overwritten by settings in NVM or settings file
    //    settings.ubxConstellations[1].enabled = false;

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
    //
    //    pinMode(pin_powerSenseAndControl, INPUT_PULLUP);
    //    pinMode(pin_powerFastOff, INPUT);
    //
    //    if (esp_reset_reason() == ESP_RST_POWERON)
    //    {
    //      powerOnCheck(); //Only do check if we POR start
    //    }
    //
    //    pinMode(pin_setupButton, INPUT_PULLUP);
    //
    //    setMuxport(settings.dataPortChannel); //Set mux to user's choice: NMEA, I2C, PPS, or DAC

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
  else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
  {
    //    //v11
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
    //
    //    pinMode(pin_powerSenseAndControl, INPUT_PULLUP);
    //    pinMode(pin_powerFastOff, INPUT);
    //
    //    if (esp_reset_reason() == ESP_RST_POWERON)
    //    {
    //      powerOnCheck(); //Only do check if we POR start
    //    }
    //
    //    pinMode(pin_peripheralPowerControl, OUTPUT);
    //    digitalWrite(pin_peripheralPowerControl, HIGH); //Turn on SD, ZED, etc
    //
    //    setMuxport(settings.dataPortChannel); //Set mux to user's choice: NMEA, I2C, PPS, or DAC
    //
    //    //CTS is active low. ESP32 pin 5 has pullup at POR. We must drive it low.
    //    pinMode(pin_radio_cts, OUTPUT);
    //    digitalWrite(pin_radio_cts, LOW);

    if (productVariant == RTK_FACET)
    {
      strcpy(platformFilePrefix, "SFE_Facet");
      strcpy(platformPrefix, "Facet");
    }
    else if (productVariant == RTK_FACET_LBAND)
    {
      strcpy(platformFilePrefix, "SFE_Facet_LBand");
      strcpy(platformPrefix, "Facet L-Band");
    }
  }

  //Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  //  //For all boards, check reset reason. If reset was due to wdt or panic, append last log
  //  if (esp_reset_reason() == ESP_RST_POWERON)
  //  {
  //    reuseLastLog = false; //Start new log
  //
  //    loadSettingsPartial();
  //    if (settings.enableResetDisplay == true)
  //    {
  //      settings.resetCount = 0;
  //      recordSystemSettings(); //Record to NVM
  //    }
  //  }
  //  else
  //  {
  //    reuseLastLog = true; //Attempt to reuse previous log
  //
  //    loadSettingsPartial();
  //    if (settings.enableResetDisplay == true)
  //    {
  //      settings.resetCount++;
  //      Serial.printf("resetCount: %d\n\r", settings.resetCount);
  //      recordSystemSettings(); //Record to NVM
  //    }
  //
  //    Serial.print("Reset reason: ");
  //    switch (esp_reset_reason())
  //    {
  //      case ESP_RST_UNKNOWN: Serial.println(F("ESP_RST_UNKNOWN")); break;
  //      case ESP_RST_POWERON : Serial.println(F("ESP_RST_POWERON")); break;
  //      case ESP_RST_SW : Serial.println(F("ESP_RST_SW")); break;
  //      case ESP_RST_PANIC : Serial.println(F("ESP_RST_PANIC")); break;
  //      case ESP_RST_INT_WDT : Serial.println(F("ESP_RST_INT_WDT")); break;
  //      case ESP_RST_TASK_WDT : Serial.println(F("ESP_RST_TASK_WDT")); break;
  //      case ESP_RST_WDT : Serial.println(F("ESP_RST_WDT")); break;
  //      case ESP_RST_DEEPSLEEP : Serial.println(F("ESP_RST_DEEPSLEEP")); break;
  //      case ESP_RST_BROWNOUT : Serial.println(F("ESP_RST_BROWNOUT")); break;
  //      case ESP_RST_SDIO : Serial.println(F("ESP_RST_SDIO")); break;
  //      default : Serial.println(F("Unknown"));
  //    }
  //  }
}


//Connect to ZED module and identify particulars
void beginGNSS()
{
  if (i2cGNSS.begin() == false)
  {
    log_d("GNSS Failed to begin. Trying again.");

    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (i2cGNSS.begin() == false)
    {
      //displayGNSSFail(1000);
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
    else if (strstr(zedFirmwareVersion, "1.32") != NULL) //ZED-F9P released May, 2022
      zedFirmwareVersionInt = 132;
    else
    {
      Serial.printf("Unknown firmware version: %s\n\r", zedFirmwareVersion);
      zedFirmwareVersionInt = 99; //0.99 invalid firmware version
    }

    //Determine if we have a ZED-F9P (Express/Facet) or an ZED-F9R (Express Plus/Facet Plus)
    if (strstr(i2cGNSS.minfo.extension[3], "ZED-F9P") != NULL)
      zedModuleType = PLATFORM_F9P;
    else if (strstr(i2cGNSS.minfo.extension[3], "ZED-F9R") != NULL)
      zedModuleType = PLATFORM_F9R;
    else
    {
      Serial.printf("Unknown ZED module: %s\n\r", i2cGNSS.minfo.extension[3]);
      zedModuleType = PLATFORM_F9P;
    }

    printZEDInfo(); //Print module type and firmware version
  }

  online.gnss = true;
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

void beginSD()
{
  bool gotSemaphore;

  online.microSD = false;
  gotSemaphore = false;
  while (settings.enableSD == true)
  {
    //Setup SD card access semaphore
    if (sdCardSemaphore == NULL)
      sdCardSemaphore = xSemaphoreCreateMutex();
    else if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_shortWait_ms) != pdPASS)
    {
      //This is OK since a retry will occur next loop
      log_d("sdCardSemaphore failed to yield, Begin.ino line %d\r\n", __LINE__);
      break;
    }
    gotSemaphore = true;

    pinMode(pin_microSD_CS, OUTPUT);
    digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected

    //Allocate the data structure that manages the microSD card
    if (!sd)
    {
      sd = new SdFat();
      if (!sd)
      {
        log_d("Failed to allocate the SdFat structure!");
        break;
      }
    }

    //Do a quick test to see if a card is present
    int tries = 0;
    int maxTries = 5;
    while (tries < maxTries)
    {
      if (sdPresent() == true) break;
      log_d("SD present failed. Trying again %d out of %d", tries + 1, maxTries);

      //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
      //Max current is 200mA average across 1s, peak 300mA
      delay(10);
      tries++;
    }
    if (tries == maxTries) break;

    //If an SD card is present, allow SdFat to take over
    log_d("SD card detected");

    if (settings.spiFrequency > 16)
    {
      Serial.println("Error: SPI Frequency out of range. Default to 16MHz");
      settings.spiFrequency = 16;
    }

    if (sd->begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == false)
    {
      tries = 0;
      maxTries = 1;
      for ( ; tries < maxTries ; tries++)
      {
        log_d("SD init failed. Trying again %d out of %d", tries + 1, maxTries);

        delay(250); //Give SD more time to power up, then try again
        if (sd->begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == true) break;
      }

      if (tries == maxTries)
      {
        Serial.println(F("SD init failed. Is card present? Formatted?"));
        digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected
        break;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd->chdir() == false)
    {
      Serial.println(F("SD change directory failed"));
      break;
    }

    //    if (createTestFile() == false)
    //    {
    //      Serial.println(F("Failed to create test file. Format SD card with 'SD Card Formatter'."));
    //      displaySDFail(5000);
    //      break;
    //    }
    //
    //    //Load firmware file from the microSD card if it is present
    //    scanForFirmware();

    Serial.println(F("microSD: Online"));
    online.microSD = true;
    break;
  }

  //Free the semaphore
  if (sdCardSemaphore && gotSemaphore)
    xSemaphoreGive(sdCardSemaphore);  //Make the file system available for use
}
