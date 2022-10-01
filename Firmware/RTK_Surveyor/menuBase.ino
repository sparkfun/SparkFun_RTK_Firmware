//Configure the survey in settings (time and 3D dev max)
//Set the ECEF coordinates for a known location
void menuBase()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Base");

    Serial.print("1) Toggle Base Mode: ");
    if (settings.fixedBase == true) Serial.println("Fixed/Static Position");
    else Serial.println("Use Survey-In");

    if (settings.fixedBase == true)
    {
      Serial.print("2) Toggle Coordinate System: ");
      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF) Serial.println("ECEF");
      else Serial.println("Geodetic");

      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
      {
        Serial.print("3) Set ECEF X/Y/Z coordinates: ");
        Serial.print(settings.fixedEcefX, 4);
        Serial.print("m, ");
        Serial.print(settings.fixedEcefY, 4);
        Serial.print("m, ");
        Serial.print(settings.fixedEcefZ, 4);
        Serial.println("m");
      }
      else if (settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC)
      {
        Serial.print("3) Set Lat/Long/Altitude coordinates: ");
        Serial.print(settings.fixedLat, haeNumberOfDecimals);
        Serial.write(167); //°
        Serial.print(", ");
        Serial.print(settings.fixedLong, haeNumberOfDecimals);
        Serial.write(167); //°
        Serial.print(", ");
        Serial.print(settings.fixedAltitude, 4);
        Serial.println("m");

        Serial.printf("4) Set Antenna Height: %dmm\r\n", settings.antennaHeight);

        Serial.printf("5) Set Antenna Reference Point: %0.1fmm\r\n", settings.antennaReferencePoint);
      }
    }
    else
    {
      Serial.print("2) Set minimum observation time: ");
      Serial.print(settings.observationSeconds);
      Serial.println(" seconds");

      Serial.print("3) Set required Mean 3D Standard Deviation: ");
      Serial.print(settings.observationPositionAccuracy, 3);
      Serial.println(" meters");

      Serial.print("4) Set required initial positional accuracy before Survey-In: ");
      Serial.print(settings.surveyInStartingAccuracy, 3);
      Serial.println(" meters");
    }

    Serial.print("6) Toggle NTRIP Server: ");
    if (settings.enableNtripServer == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    if (settings.enableNtripServer == true)
    {
      Serial.print("7) Set WiFi SSID: ");
      Serial.println(settings.ntripServer_wifiSSID);

      Serial.print("8) Set WiFi PW: ");
      Serial.println(settings.ntripServer_wifiPW);

      Serial.print("9) Set Caster Address: ");
      Serial.println(settings.ntripServer_CasterHost);

      Serial.print("10) Set Caster Port: ");
      Serial.println(settings.ntripServer_CasterPort);

      Serial.print("11) Set Mountpoint: ");
      Serial.println(settings.ntripServer_MountPoint);

      Serial.print("12) Set Mountpoint PW: ");
      Serial.println(settings.ntripServer_MountPointPW);
    }

    if (!settings.fixedBase) {
      Serial.print("13) Select survey-in radio: ");
      Serial.printf("%s\r\n", settings.ntripServer_StartAtSurveyIn ? "WiFi" : "Bluetooth");
    }

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      settings.fixedBase ^= 1;
      restartBase = true;
    }
    else if (settings.fixedBase == true && incoming == 2)
    {
      settings.fixedBaseCoordinateType ^= 1;
    }

    else if (settings.fixedBase == true && incoming == 3)
    {
      if (settings.fixedBaseCoordinateType == COORD_TYPE_ECEF)
      {
        Serial.println("Enter the fixed ECEF coordinates that will be used in Base mode:");

        Serial.print("ECEF X in meters (ex: -1280182.9200): ");
        double fixedEcefX = getDouble();

        //Progress with additional prompts only if the user enters valid data
        if (fixedEcefX != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedEcefX != INPUT_RESPONSE_GETNUMBER_EXIT)
        {
          settings.fixedEcefX = fixedEcefX;

          Serial.print("\nECEF Y in meters (ex: -4716808.5807): ");
          double fixedEcefY = getDouble();
          if (fixedEcefY != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedEcefY != INPUT_RESPONSE_GETNUMBER_EXIT)
          {
            settings.fixedEcefY = fixedEcefY;

            Serial.print("\nECEF Z in meters (ex: 4086669.6393): ");
            double fixedEcefZ = getDouble();
            if (fixedEcefZ != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedEcefZ != INPUT_RESPONSE_GETNUMBER_EXIT)
              settings.fixedEcefZ = fixedEcefZ;
          }
        }
      }
      else  if (settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC)
      {
        Serial.println("Enter the fixed Lat/Long/Altitude coordinates that will be used in Base mode:");

        Serial.print("Lat in degrees (ex: 40.090335429): ");
        double fixedLat = getDouble();

        //Progress with additional prompts only if the user enters valid data
        if (fixedLat != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedLat != INPUT_RESPONSE_GETNUMBER_EXIT)
        {
          settings.fixedLat = fixedLat;

          Serial.print("\nLong in degrees (ex: -105.184774720): ");
          double fixedLong = getDouble();
          if (fixedLong != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedLong != INPUT_RESPONSE_GETNUMBER_EXIT)
          {
            settings.fixedLong = fixedLong;

            Serial.print("\nAltitude in meters (ex: 1560.2284): ");
            double fixedAltitude = getDouble();
            if (fixedAltitude != INPUT_RESPONSE_GETNUMBER_TIMEOUT && fixedAltitude != INPUT_RESPONSE_GETNUMBER_EXIT)
              settings.fixedAltitude = fixedAltitude;
          }
        }
      }
    }
    else if (settings.fixedBase == true && settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC && incoming == 4)
    {
      Serial.print("Enter the antenna height (a.k.a. pole length) in millimeters (-15000 to 15000mm): ");
      int antennaHeight = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((antennaHeight != INPUT_RESPONSE_GETNUMBER_EXIT) && (antennaHeight != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (antennaHeight < -15000 || antennaHeight > 15000) //Arbitrary 15m max
          Serial.println("Error: Antenna Height out of range");
        else
          settings.antennaHeight = antennaHeight; //Recorded to NVM and file at main menu exit
      }
    }
    else if (settings.fixedBase == true && settings.fixedBaseCoordinateType == COORD_TYPE_GEODETIC && incoming == 5)
    {
      Serial.print("Enter the antenna reference point (a.k.a. ARP) in millimeters (-200.0 to 200.0mm): ");
      float antennaReferencePoint = getDouble();
      if (antennaReferencePoint < -200.0 || antennaReferencePoint > 200.0) //Arbitrary 200mm max
        Serial.println("Error: Antenna Reference Point out of range");
      else
        settings.antennaReferencePoint = antennaReferencePoint; //Recorded to NVM and file at main menu exit
    }

    else if (settings.fixedBase == false && incoming == 2)
    {
      Serial.print("Enter the number of seconds for survey-in obseration time (60 to 600s): ");
      int observationSeconds = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((observationSeconds != INPUT_RESPONSE_GETNUMBER_EXIT) && (observationSeconds != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (observationSeconds < 60 || observationSeconds > 60 * 10) //Arbitrary 10 minute limit
          Serial.println("Error: Observation seconds out of range");
        else
          settings.observationSeconds = observationSeconds; //Recorded to NVM and file at main menu exit
      }
    }
    else if (settings.fixedBase == false && incoming == 3)
    {
      Serial.print("Enter the number of meters for survey-in required position accuracy (1.0 to 5.0m): ");
      float observationPositionAccuracy = getDouble();
#ifdef ENABLE_DEVELOPER
      if (observationPositionAccuracy < 1.0 || observationPositionAccuracy > 10.0) //Arbitrary 1m minimum
#else
      if (observationPositionAccuracy < 1.0 || observationPositionAccuracy > 5.0) //Arbitrary 1m minimum
#endif
        Serial.println("Error: Observation positional accuracy requirement out of range");
      else
        settings.observationPositionAccuracy = observationPositionAccuracy; //Recorded to NVM and file at main menu exit
    }
    else if (settings.fixedBase == false && incoming == 4)
    {
      Serial.print("Enter the positional accuracy required before Survey-In begins (0.1 to 5.0m): ");
      float surveyInStartingAccuracy = getDouble();
      if (surveyInStartingAccuracy < 0.1 || surveyInStartingAccuracy > 5.0) //Arbitrary 0.1m minimum
        Serial.println("Error: Starting accuracy out of range");
      else
        settings.surveyInStartingAccuracy = surveyInStartingAccuracy; //Recorded to NVM and file at main menu exit
    }

    else if (incoming == 6)
    {
      settings.enableNtripServer ^= 1;
      restartBase = true;
    }
    else if (incoming == 7 && settings.enableNtripServer == true)
    {
      Serial.print("Enter local WiFi SSID: ");
      getString(settings.ntripServer_wifiSSID, sizeof(settings.ntripServer_wifiSSID));
      restartBase = true;
    }
    else if (incoming == 8 && settings.enableNtripServer == true)
    {
      Serial.printf("Enter password for WiFi network %s: ", settings.ntripServer_wifiSSID);
      getString(settings.ntripServer_wifiPW, sizeof(settings.ntripServer_wifiPW));
      restartBase = true;
    }
    else if (incoming == 9 && settings.enableNtripServer == true)
    {
      Serial.print("Enter new Caster Address: ");
      getString(settings.ntripServer_CasterHost, sizeof(settings.ntripServer_CasterHost));
      restartBase = true;
    }
    else if (incoming == 10 && settings.enableNtripServer == true)
    {
      Serial.print("Enter new Caster Port: ");

      int ntripServer_CasterPort = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((ntripServer_CasterPort != INPUT_RESPONSE_GETNUMBER_EXIT) && (ntripServer_CasterPort != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (ntripServer_CasterPort < 1 || ntripServer_CasterPort > 99999) //Arbitrary 99k max port #
          Serial.println("Error: Caster port out of range");
        else
          settings.ntripServer_CasterPort = ntripServer_CasterPort; //Recorded to NVM and file at main menu exit
        restartBase = true;
      }
    }
    else if (incoming == 11 && settings.enableNtripServer == true)
    {
      Serial.print("Enter new Mount Point: ");
      getString(settings.ntripServer_MountPoint, sizeof(settings.ntripServer_MountPoint));
      restartBase = true;
    }
    else if (incoming == 12 && settings.enableNtripServer == true)
    {
      Serial.printf("Enter password for Mount Point %s: ", settings.ntripServer_MountPoint);
      getString(settings.ntripServer_MountPointPW, sizeof(settings.ntripServer_MountPointPW));
      restartBase = true;
    }
    else if ((!settings.fixedBase) && (incoming == 13))
    {
      settings.ntripServer_StartAtSurveyIn ^= 1;
      restartBase = true;
    }
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Configure ESF settings
void menuSensorFusion()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Sensor Fusion");

    Serial.print("Fusion Mode: ");
    Serial.print(i2cGNSS.packetUBXESFSTATUS->data.fusionMode);
    Serial.print(" - ");
    if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 0)
      Serial.println("Initializing");
    else if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 1)
      Serial.println("Calibrated");
    else if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 2)
      Serial.println("Suspended");
    else if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 3)
      Serial.println("Disabled");

    if (settings.enableSensorFusion == true && settings.dynamicModel != DYN_MODEL_AUTOMOTIVE)
      Serial.println("Warning: Dynamic Model not set to Automotive. Sensor Fusion is best used with the Automotive Dynamic Model.");

    Serial.print("1) Toggle Sensor Fusion: ");
    if (settings.enableSensorFusion == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.print("2) Toggle Automatic IMU-mount Alignment: ");
    if (settings.autoIMUmountAlignment == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      settings.enableSensorFusion ^= 1;
      setSensorFusion(settings.enableSensorFusion); //Enable/disable sensor fusion
    }
    else if (incoming == 2)
    {
      settings.autoIMUmountAlignment ^= 1;
    }

    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  i2cGNSS.setESFAutoAlignment(settings.autoIMUmountAlignment); //Configure UBX-CFG-ESFALG Automatic IMU-mount Alignment

  clearBuffer(); //Empty buffer of any newline chars
}

//Enable or disable sensor fusion using keys
void setSensorFusion(bool enable)
{
  if (getSensorFusion() != enable)
    i2cGNSS.setVal8(UBLOX_CFG_SFCORE_USE_SF, enable, VAL_LAYER_ALL);
}

bool getSensorFusion()
{
  return (i2cGNSS.getVal8(UBLOX_CFG_SFCORE_USE_SF, VAL_LAYER_RAM, 1200));
}
