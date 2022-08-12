//Display current system status
void menuSystem()
{
  bool sdCardAlreadyMounted;

  while (1)
  {
    Serial.println();
    Serial.println(F("SparkFun RTK System Check Tool"));

    //Use ADC to check resistor divider
    int pin_adc_rtk_facet = 35;
    uint16_t idValue = analogReadMilliVolts(pin_adc_rtk_facet);

    Serial.print("Board ID: ");
    if (idValue > (3300 / 2 * 0.9) && idValue < (3300 / 2 * 1.1))
      Serial.print("I think this is a RTK Facet because resistor divider ID matches.");
    else if (idValue > (3300 * 2 / 3 * 0.9) && idValue < (3300 * 2 / 3 * 1.1))
      Serial.print("I think this is a RTK Facet L-Band because resistor divider ID matches.");
    else if (idValue > (3300 * 3.3 / 13.3 * 0.9) && idValue < (3300 * 3.3 / 13.3 * 1.1))
      Serial.print("I think this is a RTK Express because resistor divider ID matches.");
    else if (idValue > (3300 * 10 / 13.3 * 0.9) && idValue < (3300 * 10 / 13.3 * 1.1))
      Serial.print("I think this is a RTK Express Plus because resistor divider ID matches.");
    else if (isConnected(0x19) == true) //Check for accelerometer
    {
      if (zedModuleType == PLATFORM_F9P)
        Serial.print("I think this is a RTK Express because the accelerometer is present and the ZED is F9P.");
      else if (zedModuleType == PLATFORM_F9R)
        Serial.print("I think this is a RTK Express Plus because the accelerometer is present and the ZED is F9R.");
    }
    else
    {
      Serial.print("I think this is a RTK Surveyor because there is no accelerometer detected. ");
      Serial.printf("Board ADC ID: %d", idValue);
    }
    Serial.println();

    if (online.i2c == false)
    {
      Serial.println("I2C: Offline - Something is causing bus problems");
    }
    else
    {
      Serial.println("I2C: Online");

      Serial.print(F("GNSS: "));
      if (online.gnss == true)
      {
        Serial.print(F("Online - "));

        printZEDInfo();

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
    }

    Serial.print(F("microSD: "));
    if (online.microSD == true) Serial.println(F("Online"));
    else Serial.println(F("Offline"));

    //    if (online.lband == true)
    //    {
    //      Serial.print(F("L-Band: Online - "));
    //
    //      if (online.lbandCorrections == true) Serial.print(F("Keys Good"));
    //      else Serial.print(F("No Keys"));
    //
    //      Serial.print(F(" / Corrections Received"));
    //      if (lbandCorrectionsReceived == false) Serial.print(F(" Failed"));
    //
    //      Serial.printf(" / Eb/N0[dB] (>9 is good): %0.2f", lBandEBNO);
    //
    //      Serial.print(F(" - "));
    //
    //      printNEOInfo();
    //    }

    //Display MAC address
    char macAddress[5];
    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);

    Serial.print(F("Bluetooth ("));
    Serial.print(macAddress);
    Serial.print(F("): "));

    //Verify the ESP UART2 can communicate TX/RX to ZED UART1
    if (online.gnss == true)
    {
      if (zedUartPassed == false)
      {
        //stopUART2Tasks(); //Stop absoring ZED serial via task

        i2cGNSS.setSerialRate(460800, COM_PORT_UART1); //Defaults to 460800 to maximize message output support
        serialGNSS.begin(460800); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

        SFE_UBLOX_GNSS myGNSS;
        if (myGNSS.begin(serialGNSS) == true) //begin() attempts 3 connections
        {
          zedUartPassed = true;
          Serial.print(F("Online"));
        }
        else
          Serial.print(F("Offline"));

        i2cGNSS.setSerialRate(settings.dataPortBaud, COM_PORT_UART1); //Defaults to 460800 to maximize message output support
        serialGNSS.begin(settings.dataPortBaud); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

        //startUART2Tasks(); //Return to normal operation
      }
      else
        Serial.print(F("Online"));
    }
    else
    {
      Serial.print("Can't check (GNSS offline)");
    }
    Serial.println();

    Serial.println(F("s) Scan I2C"));
    Serial.println(F("S) Verbose scan of I2C"));
    Serial.println(F("t) Toggle pin"));
    Serial.println(F("r) Reset"));
    Serial.println(F("p) Power Down"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == 'r')
    {
      ESP.restart();
    }
    else if (incoming == 's')
    {
      Serial.println("Scan I2C bus:");
      for (byte address = 0; address < 127; address++ )
      {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
          Serial.print("Device found at address 0x");
          if (address < 0x10)
            Serial.print("0");
          Serial.print(address, HEX);

          if (address == 0x42) Serial.print(" GNSS");
          if (address == 0x43) Serial.print(" NEO");
          if (address == 0x2C) Serial.print(" USB Hub");
          if (address == 0x36) Serial.print(" Fuel Gauge");
          if (address == 0x19) Serial.print(" Accelerometer");
          if (address == 0x3D) Serial.print(" Display Main");
          if (address == 0x3C) Serial.print(" Display Alternate");
          Serial.println();
        }
      }
      Serial.println("Done");
    }
    else if (incoming == 'S')
    {
      Serial.println("Verbose scan I2C bus:");
      for (byte address = 0; address < 127; address++ )
      {
        Serial.print("Address 0x");
        if (address < 0x10)
          Serial.print("0");
        Serial.print(address, HEX);
        Serial.print(" ");

        unsigned long startTime = millis();

        //begin/end wire transmission should take a few ms. If it's taking longer,
        //it's likely the I2C bus being shorted or pulled in

        Wire.beginTransmission(address);
        int endValue = Wire.endTransmission();

        if (millis() - startTime > 100)
          Serial.print("(The I2C bus is borked! Something is shorting SDA/SCL pins. Check accelerometer.) ");

        if (endValue != 0)
        {
          Serial.print("Nothing");
        }
        else if (endValue == 0)
        {
          Serial.print("Found ");

          if (address == 0x42) Serial.print("GNSS");
          if (address == 0x43) Serial.print("NEO");
          if (address == 0x2C) Serial.print("USB Hub");
          if (address == 0x36) Serial.print("Fuel Gauge");
          if (address == 0x19) Serial.print("Accelerometer");
          if (address == 0x3D) Serial.print("Dsiplay Main");
          if (address == 0x3C) Serial.print("Dsiplay Alternate");
        }
        Serial.println();
      }
      Serial.println("Done");
    }
    else if (incoming == 't')
    {
      Serial.println("Select pin to toggle:");
      Serial.println("0 - Stat LED");
      Serial.println("21 - SDA");
      Serial.println("22 - SCL");
      Serial.println("23 - SD COPI");
      Serial.println("13 - Power Control");
      int pinNumber = getNumber(menuTimeout); //Timeout after x seconds

      if (pinNumber >= 0 && pinNumber < STATUS_PRESSED_X)
      {
        Wire.end();
        delete sd;

        clearBuffer();
        pinMode(pinNumber, OUTPUT);

        Serial.printf("\n\rToggling pin %d. Press x to exit\n\r", pinNumber);

        while (Serial.available() == 0)
        {
          digitalWrite(pinNumber, HIGH);
          for (int x = 0 ; x < 100 ; x++)
          {
            delay(30);
            if (Serial.available()) break;
          }

          digitalWrite(pinNumber, LOW);
          for (int x = 0 ; x < 100 ; x++)
          {
            delay(30);
            if (Serial.available()) break;
          }
        }
        pinMode(pinNumber, INPUT);

        Serial.println("Done");
        ESP.restart();
      }
    }
    else if (incoming == 'p')
    {
      Serial.println("Power down");

      powerDown(false); //No display
    }
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      //break;
    }
    else
      printUnknown(incoming);
  }
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
