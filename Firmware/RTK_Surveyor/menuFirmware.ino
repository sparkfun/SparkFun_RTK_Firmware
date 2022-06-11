//Update firmware if bin files found
void menuFirmware()
{
  if (online.microSD == false)
  {
    Serial.println(F("No SD card detected"));
  }

  if (binCount == 0)
  {
    Serial.println(F("No valid binary files found."));
    delay(2000);
    return;
  }

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Update Firmware Menu"));

    for (int x = 0 ; x < binCount ; x++)
    {
      Serial.printf("%d) Load %s\n\r", x + 1, binFileNames[x]);
    }

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming > 0 && incoming < (binCount + 1))
    {
      //Adjust incoming to match array
      incoming--;
      updateFromSD(binFileNames[incoming]);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      Serial.printf("Bad value: %d\n\r", incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Looks for matching binary files in root
//Loads a global called binCount
void scanForFirmware()
{
  if (online.microSD == true)
  {
    //Attempt to access file system. This avoids collisions with file writing in F9PSerialReadTask()
    //Wait up to 5s, this is important
    if (xSemaphoreTake(sdCardSemaphore, 5000 / portTICK_PERIOD_MS) == pdPASS)
    {
      //Count available binaries
      SdFile tempFile;
      SdFile dir;
      const char* BIN_EXT = "bin";
      const char* BIN_HEADER = "RTK_Surveyor_Firmware";

      char fname[50]; //Handle long file names

      dir.open("/"); //Open root

      while (tempFile.openNext(&dir, O_READ) && binCount < maxBinFiles)
      {
        if (tempFile.isFile())
        {
          tempFile.getName(fname, sizeof(fname));

          if (strcmp(forceFirmwareFileName, fname) == 0)
          {
            Serial.println(F("Forced firmware detected. Loading..."));
            displayForcedFirmwareUpdate();
            updateFromSD(forceFirmwareFileName);
          }

          //Check 'bin' extension
          if (strcmp(BIN_EXT, &fname[strlen(fname) - strlen(BIN_EXT)]) == 0)
          {
            //Check for 'RTK_Surveyor_Firmware' start of file name
            if (strncmp(fname, BIN_HEADER, strlen(BIN_HEADER)) == 0)
            {
              strcpy(binFileNames[binCount++], fname); //Add this to the array
            }
            else
              Serial.printf("Unknown: %s\n\r", fname);
          }
        }
        tempFile.close();
      }

      xSemaphoreGive(sdCardSemaphore);
    }
    else
    {
      //This is an error when a firmware file is on the microSD card
      Serial.printf("sdCardSemaphore failed to yield, %s line %d\r\n", __FILE__, __LINE__);
    }
  }
}

//Look for firmware file on SD card and update as needed
void updateFromSD(const char *firmwareFileName)
{
  //Turn off any tasks so that we are not disrupted
  wifiStop();
  stopBluetooth();

  //Delete tasks if running
  stopUART2Tasks();

  Serial.printf("Loading %s\n\r", firmwareFileName);
  if (sd.exists(firmwareFileName))
  {
    SdFile firmwareFile;
    firmwareFile.open(firmwareFileName, O_READ);

    size_t updateSize = firmwareFile.fileSize();
    if (updateSize == 0)
    {
      Serial.println(F("Error: Binary is empty"));
      firmwareFile.close();
      return;
    }

    if (Update.begin(updateSize) == false)
    {
      Serial.println(F("Update begin failed. Not enough partition space available."));
      firmwareFile.close();
      return;
    }

    Serial.println(F("Moving file to OTA section"));
    Serial.print(F("Bytes to write: "));
    Serial.print(updateSize);

    const int pageSize = 512 * 4;
    byte dataArray[pageSize];
    int bytesWritten = 0;

    //Indicate progress
    int barWidthInCharacters = 20; //Width of progress bar, ie [###### % complete
    long portionSize = updateSize / barWidthInCharacters;
    int barWidth = 0;

    //Bulk write from the SD file to flash
    while (firmwareFile.available())
    {
      if (productVariant == RTK_SURVEYOR)
        digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Toggle LED to indcate activity

      int bytesToWrite = pageSize; //Max number of bytes to read
      if (firmwareFile.available() < bytesToWrite) bytesToWrite = firmwareFile.available(); //Trim this read size as needed

      firmwareFile.read(dataArray, bytesToWrite); //Read the next set of bytes from file into our temp array

      if (Update.write(dataArray, bytesToWrite) != bytesToWrite)
      {
        Serial.println(F("\nWrite failed. Binary may be incorrectly aligned."));
        break;
      }
      else
        bytesWritten += bytesToWrite;

      //Indicate progress
      if (bytesWritten > barWidth * portionSize)
      {
        //Advance the bar
        barWidth++;
        Serial.print(F("\n["));
        for (int x = 0 ; x < barWidth ; x++)
          Serial.print("=");
        Serial.printf("%d%%", bytesWritten * 100 / updateSize);
        if (bytesWritten == updateSize) Serial.println("]");

        displayFirmwareUpdateProgress(bytesWritten * 100 / updateSize);
      }
    }
    Serial.println(F("\nFile move complete"));

    if (Update.end())
    {
      if (Update.isFinished())
      {
        displayFirmwareUpdateProgress(100);

        //Clear all settings from LittleFS
        LittleFS.format();

        Serial.println(F("Firmware updated successfully. Rebooting. Goodbye!"));

        //If forced firmware is detected, do a full reset of config as well
        if (strcmp(forceFirmwareFileName, firmwareFileName) == 0)
        {
          Serial.println(F("Removing firmware file"));

          //Remove forced firmware file to prevent endless loading
          firmwareFile.close();
          sd.remove(firmwareFileName);

          i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.
        }

        delay(1000);
        ESP.restart();
      }
      else
        Serial.println(F("Update not finished? Something went wrong!"));
    }
    else
    {
      Serial.print(F("Error Occurred. Error #: "));
      Serial.println(String(Update.getError()));
    }

    firmwareFile.close();

    displayMessage("Update Failed", 0);

    Serial.println(F("Firmware update failed. Please try again."));
  }
  else
  {
    Serial.println(F("No firmware file found"));
  }
}
