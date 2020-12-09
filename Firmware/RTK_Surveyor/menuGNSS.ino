//Configure the basic GNSS reception settings
//Update rate, constellations, etc
void menuGNSS()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: GNSS Menu"));

    Serial.print(F("1) Set measurement frequency: "));
    Serial.println(settings.gnssMeasurementFrequency);

    Serial.print(F("2) Toggle GxGGA sentence: "));
    if (getNMEASettings(UBX_NMEA_GGA, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Toggle GxGSA sentence: "));
    if (getNMEASettings(UBX_NMEA_GSA, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Toggle GxGSV sentence: "));
    if (getNMEASettings(UBX_NMEA_GSV, COM_PORT_UART1) == settings.gnssMeasurementFrequency) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("5) Toggle GxRMC sentence: "));
    if (getNMEASettings(UBX_NMEA_RMC, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("6) Toggle GxGST sentence: "));
    if (getNMEASettings(UBX_NMEA_GST, COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("7) Toggle GNSS RAWX sentence: "));
    if (getRAWXSettings(COM_PORT_UART1) == 1) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("8) Toggle SBAS: "));
    if (getSBAS() == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      Serial.print(F("Enter GNSS measurement rate: "));
      int rate = getNumber(menuTimeout); //Timeout after x seconds
      if (rate < 1 || rate > 20) //Arbitrary 10Hz limit. We need to limit based on enabled constellations.
      {
        Serial.println(F("Error: measurement rate out of range"));
      }
      else
      {
        settings.gnssMeasurementFrequency = rate; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '2')
    {
      if (getNMEASettings(UBX_NMEA_GGA, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (myGPS.disableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1) == true)
          settings.outputSentenceGGA = false;
      }
      else
      {
        //Enable the sentence
        if (myGPS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1) == true)
          settings.outputSentenceGGA = true;
      }
    }
    else if (incoming == '3')
    {
      if (getNMEASettings(UBX_NMEA_GSA, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (myGPS.disableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1) == true)
          settings.outputSentenceGSA = false;
      }
      else
      {
        //Enable the sentence
        if (myGPS.enableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1) == true)
          settings.outputSentenceGSA = true;
      }
    }
    else if (incoming == '4')
    {
      if (getNMEASettings(UBX_NMEA_GSV, COM_PORT_UART1) == settings.gnssMeasurementFrequency)
      {
        //Disable the sentence
        if (myGPS.disableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1) == true)
          settings.outputSentenceGSV = false;
      }
      else
      {
        //Enable the sentence at measurement rate to make sentence transmit at 1Hz to avoid stressing BT SPP buffers with 25+ SIV
        if (myGPS.enableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1, settings.gnssMeasurementFrequency) == true)
          settings.outputSentenceGSV = true;
      }
    }
    else if (incoming == '5')
    {
      if (getNMEASettings(UBX_NMEA_RMC, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (myGPS.disableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1) == true)
          settings.outputSentenceRMC = false;
      }
      else
      {
        //Enable the sentence
        if (myGPS.enableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1) == true)
          settings.outputSentenceRMC = true;
      }
    }
    else if (incoming == '6')
    {
      if (getNMEASettings(UBX_NMEA_GST, COM_PORT_UART1) == 1)
      {
        //Disable the sentence
        if (myGPS.disableNMEAMessage(UBX_NMEA_GST, COM_PORT_UART1) == true)
          settings.outputSentenceGST = false;
      }
      else
      {
        //Enable the sentence
        if (myGPS.enableNMEAMessage(UBX_NMEA_GST, COM_PORT_UART1) == true)
          settings.outputSentenceGST = true;
      }
    }
    else if (incoming == '7')
    {
      if (getRAWXSettings(COM_PORT_UART1) == 1)
      {
        //Disable
        if (myGPS.disableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, COM_PORT_UART1) == true)
          settings.gnssRAWOutput = false;
      }
      else
      {
        //Enable
        if (myGPS.enableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, COM_PORT_UART1) == true)
          settings.gnssRAWOutput = true;
      }
    }
    else if (incoming == '8')
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

    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  myGPS.saveConfiguration(); //Save the current settings to flash and BBR
  beginGNSS(); //Push any new settings to GNSS module

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
