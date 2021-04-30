//Configure the basic GNSS reception settings
//Update rate, constellations, etc
void menuGNSS()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: GNSS Menu"));

    float currentHz = getMeasurementFrequency(); //Get measRate and navRate from module.
    uint16_t currentHzInt = (uint16_t)currentHz;
    float currentSeconds = 1.0 / currentHz;

    Serial.print(F("1) Set measurement rate in Hz: "));
    Serial.println(currentHz, 4);

    Serial.print(F("2) Set measurement rate in seconds between measurements: "));
    Serial.println(currentSeconds, 2);

    Serial.print(F("3) Toggle GxGGA sentence: "));
    if (getNMEASettings(UBX_NMEA_GGA, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Toggle GxGSA sentence: "));
    if (getNMEASettings(UBX_NMEA_GSA, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("5) Toggle GxGSV sentence: "));
    if (getNMEASettings(UBX_NMEA_GSV, COM_PORT_UART1) == currentHzInt) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("6) Toggle GxRMC sentence: "));
    if (getNMEASettings(UBX_NMEA_RMC, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("7) Toggle GxGST sentence: "));
    if (getNMEASettings(UBX_NMEA_GST, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("8) Toggle GNSS RAWX sentence: "));
    if (getRAWXSettings(COM_PORT_I2C) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("9) Toggle GNSS SFRBX sentence: "));
    if (getSFRBXSettings(COM_PORT_I2C) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("10) Toggle SBAS: "));
    if (getSBAS() == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    int incoming = getNumber(30); //Timeout after x seconds

    if (incoming == 1)
    {
      Serial.print(F("Enter GNSS measurement rate in Hz: "));
      double rate = getDouble(menuTimeout); //Timeout after x seconds
      if (rate < 0.0 || rate > 10.0) //Arbitrary 10Hz limit. We need to limit based on enabled constellations.
      {
        Serial.println(F("Error: measurement rate out of range"));
      }
      else
      {
        setMeasurementRates(1.0 / rate); //Convert Hz to seconds. This will set settings.measurementRate and settings.navigationRate
        //Settings recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 2)
    {
      Serial.print(F("Enter GNSS measurement rate in seconds between measurements: "));
      double rate = getDouble(menuTimeout); //Timeout after x seconds
      if (rate < 0.0 || rate > 8255.0) //Limit of 127 (navRate) * 65000ms (measRate) = 137 minute limit.
      {
        Serial.println(F("Error: measurement rate out of range"));
      }
      else
      {
        setMeasurementRates(rate); //This will set settings.measurementRate and settings.navigationRate
        //Settings recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 3)
    {
      if (getNMEASettings(UBX_NMEA_GGA, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (i2cGNSS.disableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1) == true)
          settings.outputSentenceGGA = false;
      }
      else
      {
        //Enable the sentence
        if (i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1) == true)
          settings.outputSentenceGGA = true;
      }
    }
    else if (incoming == 4)
    {
      if (getNMEASettings(UBX_NMEA_GSA, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (i2cGNSS.disableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1) == true)
          settings.outputSentenceGSA = false;
      }
      else
      {
        //Enable the sentence
        if (i2cGNSS.enableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1) == true)
          settings.outputSentenceGSA = true;
      }
    }
    else if (incoming == 5)
    {
      if (getNMEASettings(UBX_NMEA_GSV, COM_PORT_UART1) == currentHzInt)
      {
        //Disable the sentence
        if (i2cGNSS.disableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1) == true)
          settings.outputSentenceGSV = false;
      }
      else
      {
        //Enable the sentence at measurement rate to make sentence transmit at 1Hz to avoid stressing BT SPP buffers with 25+ SIV
        if (i2cGNSS.enableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1, currentHzInt) == true)
          settings.outputSentenceGSV = true;
      }
    }
    else if (incoming == 6)
    {
      if (getNMEASettings(UBX_NMEA_RMC, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (i2cGNSS.disableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1) == true)
          settings.outputSentenceRMC = false;
      }
      else
      {
        //Enable the sentence
        if (i2cGNSS.enableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1) == true)
          settings.outputSentenceRMC = true;
      }
    }
    else if (incoming == 7)
    {
      if (getNMEASettings(UBX_NMEA_GST, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (i2cGNSS.disableNMEAMessage(UBX_NMEA_GST, COM_PORT_UART1) == true)
          settings.outputSentenceGST = false;
      }
      else
      {
        //Enable the sentence
        if (i2cGNSS.enableNMEAMessage(UBX_NMEA_GST, COM_PORT_UART1) == true)
          settings.outputSentenceGST = true;
      }
    }
    else if (incoming == 8)
    {
      if (getRAWXSettings(COM_PORT_I2C) == 1)
      {
        //Disable
        if (i2cGNSS.disableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, COM_PORT_I2C) == true)
        {
          i2cGNSS.logRXMRAWX(false); // Disable RXM RAWX data logging
          settings.logRAWX = false;
        }
      }
      else
      {
        //Enable
        if (i2cGNSS.enableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, COM_PORT_I2C) == true)
        {
          i2cGNSS.logRXMRAWX(); // Enable RXM RAWX data logging
          settings.logRAWX = true;
        }
      }
    }
    else if (incoming == 9)
    {
      if (getSFRBXSettings(COM_PORT_I2C) == 1)
      {
        //Disable
        if (i2cGNSS.disableMessage(UBX_CLASS_RXM, UBX_RXM_SFRBX, COM_PORT_I2C) == true)
        {
          i2cGNSS.logRXMSFRBX(false); // Disable RXM SFRBX data logging
          settings.logSFRBX = false;
        }
      }
      else
      {
        //Enable
        if (i2cGNSS.enableMessage(UBX_CLASS_RXM, UBX_RXM_SFRBX, COM_PORT_I2C) == true)
        {
          i2cGNSS.logRXMSFRBX(); // Enable RXM SFRBX data logging
          settings.logSFRBX = true;
        }
      }
    }
    else if (incoming == 10)
    {
      if (getSBAS() == true)
      {
        //Disable it
        if (setSBAS(false) == true)
          settings.enableSBAS = false;
      }
      else
      {
        //Enable it
        if (setSBAS(true) == true)
          settings.enableSBAS = true;
      }

    }

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  beginGNSS(); //Push any new settings to GNSS module

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Given the number of seconds between desired solution reports, determine measurementRate and navigationRate
//measurementRate > 25 & <= 65535
//navigationRate >= 1 && <= 127
//We give preference to limiting a measurementRate to 30s or below due to reported problems with measRates above 30.
void setMeasurementRates(float secondsBetweenSolutions)
{
  uint16_t measRate = 0; //Calculate these locally and then attempt to apply them to ZED at completion
  uint16_t navRate = 0;

  //If we have more than an hour between readings, increase mesaurementRate to near max of 65,535
  if (secondsBetweenSolutions > 3600.0)
  {
    measRate = 65000;
  }

  //If we have more than 30s, but less than 3600s between readings, use 30s measurement rate
  else if (secondsBetweenSolutions > 30.0)
  {
    measRate = 30000;
  }

  //User wants measurements less than 30s (most common), set measRate to match user request
  //This will make navRate = 1.
  else
  {
    measRate = secondsBetweenSolutions * 1000.0;
  }

  navRate = secondsBetweenSolutions * 1000.0 / measRate; //Set navRate to nearest int value
  measRate = secondsBetweenSolutions * 1000.0 / navRate; //Adjust measurement rate to match actual navRate

  //Serial.printf("measurementRate / navRate: %d / %d\n", measRate, navRate);

  //If we successfully set rates, only then record to settings
  if (i2cGNSS.setMeasurementRate(measRate) == true && i2cGNSS.setNavigationRate(navRate) == true)
  {
    settings.measurementRate = measRate;
    settings.navigationRate = navRate;
  }
  else
  {
    Serial.println(F("menuGNSS: Failed to set measurement and navigation rates"));
  }
}

//We need to know our overall measurement frequency for things like setting the GSV NMEA sentence rate.
//This returns a float of the rate based on settings that is the readings per second (Hz).
float getMeasurementFrequency()
{
  uint16_t currentMeasurementRate = i2cGNSS.getMeasurementRate();
  uint16_t currentNavigationRate = i2cGNSS.getNavigationRate();

  currentNavigationRate = i2cGNSS.getNavigationRate();
  //The ZED-F9P will report an incorrect nav rate if we have rececently changed it.
  //Reading a second time insures a correct read.

  //Serial.printf("currentMeasurementRate / currentNavigationRate: %d / %d\n", currentMeasurementRate, currentNavigationRate);

  float measurementFrequency = (1000.0 / currentMeasurementRate) / currentNavigationRate;
  return (measurementFrequency);
}
