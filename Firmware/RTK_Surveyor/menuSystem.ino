//Display current system status
void menuSystem()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: System Menu"));

    Serial.print(F("GNSS: "));
    if (online.gnss == true)
    {
      Serial.print(F("Online - "));

      printModuleInfo();

      printCurrentConditions();
    }
    else Serial.println(F("Offline"));

    Serial.print(F("Display: "));
    if (online.display == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    Serial.print(F("Accelerometer: "));
    if (online.display == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    Serial.print(F("Fuel Gauge: "));
    if (online.battery == true)
    {
      Serial.print(F("Online - "));

      battLevel = lipo.getSOC();
      battVoltage = lipo.getVoltage();
      battChangeRate = lipo.getChangeRate();

      Serial.printf("Batt (%d%%) / Voltage: %0.02fV", battLevel, battVoltage);
      Serial.println();
    }
    else Serial.println(F("Offline"));

    Serial.print(F("microSD: "));
    if (online.microSD == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    //Display MAC address
    char macAddress[5];
    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);

    Serial.print(F("MAC: "));
    Serial.print(macAddress);
    Serial.print(F(" - "));

    //Verify the ESP UART2 can communicate TX/RX to ZED UART1
    if (online.gnss == true)
    {
      if (zedUartPassed == false)
      {
        stopUART2Tasks(); //Stop absoring ZED serial via task

        //Clear out buffer before starting
        while (serialGNSS.available()) serialGNSS.read();
        serialGNSS.flush();

        //begin() attempts 3 connections with X timeout per attempt
        SFE_UBLOX_GNSS myGNSS;
        if (myGNSS.begin(serialGNSS) == true)
        {
          zedUartPassed = true;
          Serial.print(F("BT Online"));
        }
        else
          Serial.print(F("BT Offline"));

        startUART2Tasks(); //Return to normal operation
      }
      else
        Serial.print(F("BT Online"));
    }
    else
    {
      Serial.print("GNSS Offline");
    }
    Serial.println();

    if (settings.enableSD == true && online.microSD == true)
    {
      Serial.println(F("f) Display microSD Files"));
    }

    Serial.println(F("d) Configure Debug"));

    Serial.println(F("r) Reset all settings to default"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == 'd')
      menuDebug();
    else if (incoming == 'r')
    {
      Serial.println(F("\r\nResetting to factory defaults. Press 'y' to confirm:"));
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        factoryReset();
      }
      else
        Serial.println(F("Reset aborted"));
    }
    else if (incoming == 'f' && settings.enableSD == true && online.microSD == true)
    {
      //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
      if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
      {
        Serial.println(F("Files found (date time size name):\n\r"));
        sd.ls(LS_R | LS_DATE | LS_SIZE);

        xSemaphoreGive(xFATSemaphore);
      }
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars

}

//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Debug Menu"));

    Serial.print(F("1) I2C Debugging Output: "));
    if (settings.enableI2Cdebug == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Heap Reporting: "));
    if (settings.enableHeapReport == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Task Highwater Reporting: "));
    if (settings.enableTaskReports == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Set SPI/SD Interface Frequency: "));
    Serial.print(settings.spiFrequency);
    Serial.println(" MHz");

    Serial.print(F("5) Set SPP RX Buffer Size: "));
    Serial.println(settings.sppRxQueueSize);

    Serial.print(F("6) Set SPP TX Buffer Size: "));
    Serial.println(settings.sppTxQueueSize);

    Serial.print(F("7) Throttle BT Transmissions During SPP Congestion: "));
    if (settings.throttleDuringSPPCongestion == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("8) Display Reset Counter: "));
    if (settings.enableResetDisplay == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("e) Erase LittleFS"));

    Serial.println(F("r) Force system reset"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enableI2Cdebug ^= 1;

      if (settings.enableI2Cdebug)
        i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
      else
        i2cGNSS.disableDebugging();
    }
    else if (incoming == '2')
    {
      settings.enableHeapReport ^= 1;
    }
    else if (incoming == '3')
    {
      settings.enableTaskReports ^= 1;
    }
    else if (incoming == '4')
    {
      Serial.print(F("Enter SPI frequency in MHz (1 to 16): "));
      int freq = getNumber(menuTimeout); //Timeout after x seconds
      if (freq < 1 || freq > 16) //Arbitrary 16MHz limit
      {
        Serial.println(F("Error: SPI frequency out of range"));
      }
      else
      {
        settings.spiFrequency = freq; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '5')
    {
      Serial.print(F("Enter SPP RX Queue Size in Bytes (32 to 16384): "));
      uint16_t queSize = getNumber(menuTimeout); //Timeout after x seconds
      if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
      {
        Serial.println(F("Error: Queue size out of range"));
      }
      else
      {
        settings.sppRxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '6')
    {
      Serial.print(F("Enter SPP TX Queue Size in Bytes (32 to 16384): "));
      uint16_t queSize = getNumber(menuTimeout); //Timeout after x seconds
      if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
      {
        Serial.println(F("Error: Queue size out of range"));
      }
      else
      {
        settings.sppTxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '7')
    {
      settings.throttleDuringSPPCongestion ^= 1;
    }
    else if (incoming == '8')
    {
      settings.enableResetDisplay ^= 1;
      if (settings.enableResetDisplay == true)
      {
        settings.resetCount = 0;
        recordSystemSettings(); //Record to NVM
      }
    }
    else if (incoming == 'e')
    {
      Serial.println("Erasing LittleFS and resetting");
      LittleFS.format();
      ESP.restart();
    }
    else if (incoming == 'r')
    {
      ESP.restart();
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Print the current long/lat/alt/HPA/SIV
//From Example11_GetHighPrecisionPositionUsingDouble
void printCurrentConditions()
{
  if (online.gnss == true)
  {
    Serial.print(F("SIV: "));
    Serial.print(numSV);

    Serial.print(", HPA (m): ");
    Serial.print(horizontalAccuracy, 3);

    Serial.print(", Lat: ");
    Serial.print(latitude, 9);
    Serial.print(", Lon: ");
    Serial.print(longitude, 9);

    Serial.print(", Altitude (m): ");
    Serial.print(altitude, 1);

    Serial.println();
  }
}

void printCurrentConditionsNMEA()
{
  if (online.gnss == true)
  {
    char systemStatus[100];
    sprintf(systemStatus, "%02d%02d%02d.%02d,%02d%02d%02d,%0.3f,%d,%0.9f,%0.9f,%0.2f,%d,%d,%d",
            gnssHour, gnssMinute, gnssSecond, mseconds,
            gnssDay, gnssMonth, gnssYear % 2000, //Limit to 2 digits
            horizontalAccuracy, numSV,
            latitude, longitude,
            altitude,
            fixType, carrSoln,
            battLevel
           );

    char nmeaMessage[100]; //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, systemStatus); //textID, buffer, text
    Serial.println(nmeaMessage);
  }
  else
  {
    char nmeaMessage[100]; //Max NMEA sentence length is 82
    createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, (char *)"OFFLINE"); //textID, buffer, text
    Serial.println(nmeaMessage);
  }
}
