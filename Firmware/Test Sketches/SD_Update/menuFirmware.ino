//Update firmware if bin files found
void menuFirmware()
{
  if (online.microSD == false)
  {
    Serial.println(F("No SD card detected"));
  }

  if (binCount == 0)
  {
    Serial.println("No valid binary files found.");
    delay(2000);
    return;
  }

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Update Firmware Menu"));

    for (int x = 0 ; x < binCount ; x++)
    {
      Serial.printf("%d) Load %s\n", x + 1, binFileNames[x]);
    }

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming > 0 && incoming < (binCount + 1))
    {
      //Adjust incoming to match array
      incoming--;
      Serial.printf("Loading %s\n", binFileNames[incoming]);
      updateFromSD(binFileNames[incoming]);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      Serial.printf("Bad value: %d\n", incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Looks for matching binary files in root
//Loads a global called binCount
void scanForFirmware()
{
  if (online.microSD == true)
  {
    //Count available binaries
    SdFile tempFile;
    SdFile dir;
    const char* BIN_EXT = "bin";
    const char* BIN_HEADER = "RTK_Surveyor_Firmware";
    char fname[50]; //Handle long file names

    dir.open("/"); //Open root

    while (tempFile.openNext(&dir, O_READ))
    {
      if (tempFile.isFile())
      {
        tempFile.getName(fname, sizeof(fname));

        if (strcmp(forceFirmwareFileName, fname) == 0)
          updateFromSD((char *)forceFirmwareFileName);

        //Check for 'sfe_rtk' and 'bin' extension
        if (strcmp(BIN_EXT, &fname[strlen(fname) - strlen(BIN_EXT)]) == 0)
        {
          if (strstr(fname, BIN_HEADER) != NULL)
          {
            strcpy(binFileNames[binCount++], fname); //Add this to the array
          }
          else
            Serial.printf("Unknown: %s\n", fname);
        }
      }
      tempFile.close();
    }
  }
}

//Look for firmware file on SD card and update as needed
void updateFromSD(char *firmwareFileName)
{
  Serial.printf("Loading %s\n", firmwareFileName);
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

    Serial.print(F("Moving file to OTA section"));

    const int pageSize = 512;
    byte dataArray[pageSize];
    int bytesWritten = 0;

    //Indicate progress
    int barWidthInCharacters = 20; //Width of progress bar, ie [###### % complete
    long portionSize = updateSize / barWidthInCharacters;
    int barWidth = 0;

    //Bulk write from the SD file to the EEPROM
    while (firmwareFile.available())
    {
      int bytesToWrite = pageSize; //Max number of bytes to read
      if (firmwareFile.available() < bytesToWrite) bytesToWrite = firmwareFile.available(); //Trim this read size as needed

      firmwareFile.read(dataArray, bytesToWrite); //Read the next set of bytes from file into our temp array

      if (Update.write(dataArray, bytesToWrite) != bytesToWrite)
        Serial.println(F("Write failed"));
      else
        bytesWritten += bytesToWrite;

      //Indicate progress
      if (bytesWritten > barWidth * portionSize)
      {
        //Advance the bar
        barWidth++;
        Serial.print("\n[");
        for (int x = 0 ; x < barWidth ; x++)
          Serial.print("=");
        Serial.printf("%d%%", bytesWritten * 100 / updateSize);
        if (bytesWritten == updateSize) Serial.println("]");
      }
    }

    Serial.println(F("\nFile move complete"));

    if (Update.end())
    {
      if (Update.isFinished())
      {
        Serial.println(F("Firmware updated successfully. Rebooting. Good bye!"));
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
  }
  else
  {
    Serial.println(F("No firmware file found"));
  }
}
