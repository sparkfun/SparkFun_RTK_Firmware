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

    Serial.println(F("r) Reset GNSS Receiver"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      Serial.print(F("Enter GNSS measurement rate: "));
      int rate = getNumber(menuTimeout); //Timeout after x seconds
      if (rate < 1 || rate > 10) //Arbitrary 10Hz limit. We need to limit based on enabled constellations.
      {
        Serial.println(F("Error: measurement rate out of range"));
      }
      else
      {
        settings.gnssMeasurementFrequency = rate; //Recorded to NVM and file at main menu exit
      }
    }

    else if (incoming == 'r')
    {
      Serial.println(F("\r\nResetting ZED-F9P to factory defaults. Press 'y' to confirm:"));
      byte bContinue = getByteChoice(menuTimeout);
      if (bContinue == 'y')
      {
        myGPS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

        Serial.println(F("ZED-F9P settings reset. Please reset RTK Surveyor. Freezing."));
        while (1) 
          delay(1); //Prevent CPU freakout
      }
      else
        Serial.println(F("Reset aborted"));
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  beginGNSS(); //Push any new settings to GNSS module

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
