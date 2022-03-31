void beginSD()
{
  pinMode(pin_microSD_CS, OUTPUT);
  digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if (settings.spiFrequency > 16)
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
        log_d("SD init failed. Trying again %d out of %d", tries + 1, maxTries);

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
      //displaySDFail(5000);
      online.microSD = false;
      return;
    }

    online.microSD = true;

    Serial.println(F("microSD online"));
    //scanForFirmware(); //See if SD card contains new firmware that should be loaded at startup
  }
  else
  {
    online.microSD = false;
  }
}
