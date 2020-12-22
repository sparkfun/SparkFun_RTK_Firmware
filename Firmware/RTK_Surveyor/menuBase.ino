//Configure the survey in settings (time and 3D dev max)
//Set the ECEF coordinates for a known location
void menuBase()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Base Menu"));

    Serial.print(F("1) Toggle Base Mode: "));
    if (settings.fixedBase == true) Serial.println(F("Fixed/Static Position"));
    else Serial.println(F("Use Survey-In"));

    if (settings.fixedBase == true)
    {
      Serial.print(F("2) Toggle Coordinate System: "));
      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF) Serial.println(F("ECEF"));
      else Serial.println(F("Geographic"));

      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
      {
        Serial.print(F("3) Set ECEF X/Y/Z coordinates: "));
        Serial.print(settings.fixedEcefX, 4);
        Serial.print(F("m, "));
        Serial.print(settings.fixedEcefY, 4);
        Serial.print(F("m, "));
        Serial.print(settings.fixedEcefZ, 4);
        Serial.println(F("m"));
      }
      else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEOGRAPHIC)
      {
        Serial.print(F("3) Set Lat/Long/Altitude coordinates: "));
        Serial.print(settings.fixedLat, 9);
        Serial.print(F("°, "));
        Serial.print(settings.fixedLong, 9);
        Serial.print(F("°, "));
        Serial.print(settings.fixedAltitude, 4);
        Serial.println(F("m"));
      }
    }
    else
    {
      Serial.print(F("2) Set minimum observation time: "));
      Serial.print(settings.observationSeconds);
      Serial.println(F(" seconds"));

      Serial.print(F("3) Set required Mean 3D Standard Deviation: "));
      Serial.print(settings.observationPositionAccuracy, 3);
      Serial.println(F(" meters"));
    }

    Serial.print(F("4) Toggle NTRIP Server: "));
    if (settings.enableNtripServer == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (settings.enableNtripServer == true)
    {
      Serial.print(F("5) Set WiFi SSID: "));
      Serial.println(settings.wifiSSID);

      Serial.print(F("6) Set WiFi PW: "));
      Serial.println(settings.wifiPW);

      Serial.print(F("7) Set Caster Address: "));
      Serial.println(settings.casterHost);

      Serial.print(F("8) Set Caster Port: "));
      Serial.println(settings.casterPort);

      Serial.print(F("9) Set Mountpoint: "));
      Serial.println(settings.mountPoint);

      Serial.print(F("10) Set Mountpoint PW: "));
      Serial.println(settings.mountPointPW);
    }

    Serial.println(F("x) Exit"));

    int incoming = getNumber(30); //Timeout after x seconds

    if (incoming == 1)
    {
      settings.fixedBase ^= 1;
    }
    else if (settings.fixedBase == true && incoming == 2)
    {
      settings.fixedBaseCoordinateType ^= 1;
    }
    else if (settings.fixedBase == true && incoming == 3)
    {
      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
      {
        Serial.println(F("Enter the fixed ECEF coordinates that will be used in Base mode:"));

        Serial.print(F("ECEF X in meters (ex: -1280182.920): "));
        double fixedEcefX = getDouble(menuTimeout); //Timeout after x seconds

        //Progress with additional prompts only if the user enters valid data
        if (fixedEcefX != STATUS_GETNUMBER_TIMEOUT && fixedEcefX != STATUS_PRESSED_X)
        {
          settings.fixedEcefX = fixedEcefX;

          Serial.print(F("\nECEF Y in meters (ex: -4716808.5807): "));
          double fixedEcefY = getDouble(menuTimeout);
          if (fixedEcefY != STATUS_GETNUMBER_TIMEOUT && fixedEcefY != STATUS_PRESSED_X)
          {
            settings.fixedEcefY = fixedEcefY;

            Serial.print(F("\nECEF Z in meters (ex: 4086669.6393): "));
            double fixedEcefZ = getDouble(menuTimeout);
            if (fixedEcefZ != STATUS_GETNUMBER_TIMEOUT && fixedEcefZ != STATUS_PRESSED_X)
              settings.fixedEcefZ = fixedEcefZ;
          }
        }
      }
      else  if (settings.fixedBaseCoordinateType == COORD_TYPE_GEOGRAPHIC)
      {
        Serial.println(F("Enter the fixed Lat/Long/Altitude coordinates that will be used in Base mode:"));

        Serial.print(F("Lat in degrees (ex: -105.184774720): "));
        double fixedLat = getDouble(menuTimeout); //Timeout after x seconds

        //Progress with additional prompts only if the user enters valid data
        if (fixedLat != STATUS_GETNUMBER_TIMEOUT && fixedLat != STATUS_PRESSED_X)
        {
          settings.fixedLat = fixedLat;

          Serial.print(F("\nLong in degrees (ex: 40.090335429): "));
          double fixedLong = getDouble(menuTimeout);
          if (fixedLong != STATUS_GETNUMBER_TIMEOUT && fixedLong != STATUS_PRESSED_X)
          {
            settings.fixedLong = fixedLong;

            Serial.print(F("\nAltitude in meters (ex: 1560.2284): "));
            double fixedAltitude = getDouble(menuTimeout);
            if (fixedAltitude != STATUS_GETNUMBER_TIMEOUT && fixedAltitude != STATUS_PRESSED_X)
              settings.fixedAltitude = fixedAltitude;
          }
        }
      }
    }
    else if (settings.fixedBase == false && incoming == 2)
    {
      Serial.print(F("Enter the number of seconds for survey-in obseration time (60 to 600s): "));
      int observationSeconds = getNumber(menuTimeout); //Timeout after x seconds
      if (observationSeconds < 60 || observationSeconds > 60 * 10) //Arbitrary 10 minute limit
      {
        Serial.println(F("Error: observation seconds out of range"));
      }
      else
      {
        settings.observationSeconds = observationSeconds; //Recorded to NVM and file at main menu exit
      }
    }
    else if (settings.fixedBase == false && incoming == 3)
    {
      Serial.print(F("Enter the number of meters for survey-in required position accuracy (1.0 to 5.0m): "));
      float observationPositionAccuracy = getDouble(menuTimeout); //Timeout after x seconds
      if (observationPositionAccuracy < 1.0 || observationPositionAccuracy > 5.0) //Arbitrary 1m minimum
      {
        Serial.println(F("Error: observation positional accuracy requirement out of range"));
      }
      else
      {
        settings.observationPositionAccuracy = observationPositionAccuracy; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == 4)
    {
      settings.enableNtripServer ^= 1;
    }
    else if (incoming == 5 && settings.enableNtripServer == true)
    {
      Serial.print(F("Enter local WiFi SSID: "));
      readLine(settings.wifiSSID, sizeof(settings.wifiSSID), menuTimeout);
    }
    else if (incoming == 6 && settings.enableNtripServer == true)
    {
      Serial.printf("Enter password for WiFi network %s: ", settings.wifiSSID);
      readLine(settings.wifiPW, sizeof(settings.wifiPW), menuTimeout);
    }
    else if (incoming == 7 && settings.enableNtripServer == true)
    {
      Serial.print(F("Enter new Caster Address: "));
      readLine(settings.casterHost, sizeof(settings.casterHost), menuTimeout);
    }
    else if (incoming == 8 && settings.enableNtripServer == true)
    {
      Serial.print(F("Enter new Caster Port: "));

      int casterPort = getNumber(menuTimeout); //Timeout after x seconds
      if (casterPort < 1 || casterPort > 99999) //Arbitrary 99k max port #
        Serial.println(F("Error: Caster Port out of range"));
      else
        settings.casterPort = casterPort; //Recorded to NVM and file at main menu exit
    }
    else if (incoming == 9 && settings.enableNtripServer == true)
    {
      Serial.print(F("Enter new Mount Point: "));
      readLine(settings.mountPoint, sizeof(settings.mountPoint), menuTimeout);
    }
    else if (incoming == 10 && settings.enableNtripServer == true)
    {
      Serial.printf("Enter password for Mount Point %s: ", settings.mountPoint);
      readLine(settings.mountPointPW, sizeof(settings.mountPointPW), menuTimeout);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
