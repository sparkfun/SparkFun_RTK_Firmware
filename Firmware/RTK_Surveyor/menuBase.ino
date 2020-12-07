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
        Serial.print("m, ");
        Serial.print(settings.fixedEcefY, 4);
        Serial.print("m, ");
        Serial.print(settings.fixedEcefZ, 4);
        Serial.println(F("m"));
      }
      else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEOGRAPHIC)
      {
        Serial.print(F("3) Set Lat/Long/Altitude coordinates: "));
        Serial.print(settings.fixedLat, 9);
        Serial.print("°, ");
        Serial.print(settings.fixedLong, 9);
        Serial.print("°, ");
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

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.fixedBase ^= 1;
    }
    else if (settings.fixedBase == true)
    {
      if (incoming == '2')
      {
        settings.fixedBaseCoordinateType ^= 1;
      }
      if (incoming == '3')
      {
        if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
        {
          Serial.println(F("Enter the fixed ECEF coordinates that will be used in Base mode:"));

          Serial.print(F("ECEF X in meters (ex: -1280182.920): "));
          double fixedEcefX = getDouble(menuTimeout); //Timeout after x seconds
          settings.fixedEcefX = fixedEcefX;

          Serial.print(F("\nECEF Y in meters (ex: -4716808.5807): "));
          double fixedEcefY = getDouble(menuTimeout);
          settings.fixedEcefY = fixedEcefY;

          Serial.print(F("\nECEF Z in meters (ex: 4086669.6393): "));
          double fixedEcefZ = getDouble(menuTimeout);
          settings.fixedEcefZ = fixedEcefZ;

          Serial.printf("\nX: %f\n", settings.fixedEcefX);
          Serial.printf("Y: %f\n", settings.fixedEcefY);
          Serial.printf("Z: %f\n", settings.fixedEcefZ);
        }
        else  if (settings.fixedBaseCoordinateType == COORD_TYPE_GEOGRAPHIC)
        {
          Serial.println(F("Enter the fixed Lat/Long/Altitude coordinates that will be used in Base mode:"));

          Serial.print(F("Lat in degrees (ex: -105.184774720): "));
          double fixedLat = getDouble(menuTimeout); //Timeout after x seconds
          settings.fixedLat = fixedLat;

          Serial.print(F("\nLong in degrees (ex: 40.090335429): "));
          double fixedLong = getDouble(menuTimeout);
          settings.fixedLong = fixedLong;

          Serial.print(F("\nAltitude in meters (ex: 1560.2284): "));
          double fixedAltitude = getDouble(menuTimeout);
          settings.fixedAltitude = fixedAltitude;

          Serial.printf("\nlat: %f\n", settings.fixedLat);
          Serial.printf("long: %f\n", settings.fixedLong);
          Serial.printf("altitude: %f\n", settings.fixedAltitude);
        }
      }
      else if (incoming == 'x')
        break;
      else if (incoming == STATUS_GETBYTE_TIMEOUT)
        break;
      else
        printUnknown(incoming);
    }
    else if (settings.fixedBase == false)
    {
      if (incoming == '2')
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
      else if (incoming == '3')
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
      else if (incoming == 'x')
        break;
      else if (incoming == STATUS_GETBYTE_TIMEOUT)
        break;
      else
        printUnknown(incoming);
    }

    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
