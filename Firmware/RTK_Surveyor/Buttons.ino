//Regularly update subsystems. Called from main loop (aka not tasks).

//Change between Rover or Base depending on switch state
void checkSetupButton()
{
  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == LOW) //Switch is set to base mode
  {
    if (systemState == STATE_ROVER_NO_FIX ||
        systemState == STATE_ROVER_FIX ||
        systemState == STATE_ROVER_RTK_FLOAT ||
        systemState == STATE_ROVER_RTK_FIX)
    {
      displayBaseStart();

      //Configure for base mode
      Serial.println(F("Base Mode"));

      //Configure for base mode
      Serial.println(F("Base Mode"));

      //Restart Bluetooth with 'Base' name
      //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
      beginBluetooth();

      if (settings.fixedBase == false)
      {
        //Don't configure base if we are going to do a survey in. We need to wait for surveyInStartingAccuracy to be achieved in state machine
        changeState(STATE_BASE_TEMP_SURVEY_NOT_STARTED);
      }
      else if (settings.fixedBase == true)
      {
        if (configureUbloxModuleBase() == false)
        {
          Serial.println(F("Base config failed!"));
          displayBaseFail();
          return;
        }

        bool response = startFixedBase();
        if (response == true)
        {
          changeState(STATE_BASE_FIXED_TRANSMITTING);
        }
        else
        {
          //TODO maybe create a custom fixed base fail screen
          Serial.println(F("Fixed base start failed!"));
          displayBaseFail();
          return;
        }
      }

      displayBaseSuccess();
      delay(500);
    }
  }
  else if (digitalRead(baseSwitch) == HIGH) //Switch is set to Rover
  {
    if (systemState == STATE_BASE_TEMP_SURVEY_NOT_STARTED ||
        systemState == STATE_BASE_TEMP_SURVEY_STARTED ||
        systemState == STATE_BASE_TEMP_TRANSMITTING ||
        systemState == STATE_BASE_TEMP_WIFI_STARTED ||
        systemState == STATE_BASE_TEMP_WIFI_CONNECTED ||
        systemState == STATE_BASE_TEMP_CASTER_STARTED ||
        systemState == STATE_BASE_TEMP_CASTER_CONNECTED ||
        systemState == STATE_BASE_FIXED_TRANSMITTING ||
        systemState == STATE_BASE_FIXED_WIFI_STARTED ||
        systemState == STATE_BASE_FIXED_WIFI_CONNECTED ||
        systemState == STATE_BASE_FIXED_CASTER_STARTED ||
        systemState == STATE_BASE_FIXED_CASTER_CONNECTED)
    {
      displayRoverStart();


      //Configure for rover mode
      Serial.println(F("Rover Mode"));

      //If we are survey'd in, but switch is rover then disable survey
      if (configureUbloxModuleRover() == false)
      {
        Serial.println(F("Rover config failed!"));
        displayRoverFail();
        return;
      }

      beginBluetooth(); //Restart Bluetooth with 'Rover' name

      digitalWrite(baseStatusLED, LOW);

      changeState(STATE_ROVER_NO_FIX);
      displayRoverSuccess();
      delay(500);
    }
  }
}

//Create or close UBX/NMEA files as needed (startup or as user changes settings)
//Push new UBX packets to log as needed
void updateLogs()
{
  if (online.nmeaLogging == false && settings.logNMEA == true)
  {
    beginLoggingNMEA();
  }
  else if (online.nmeaLogging == true && settings.logNMEA == false)
  {
    //Close down file
    nmeaFile.sync();
    nmeaFile.close();
    online.nmeaLogging = false;
  }

  if (online.ubxLogging == false && settings.logUBX == true)
  {
    beginLoggingUBX();
  }
  else if (online.ubxLogging == true && settings.logUBX == false)
  {
    //Close down file
    ubxFile.sync();
    ubxFile.close();
    online.ubxLogging = false;
  }

  if (online.ubxLogging == true)
  {
    //Check if we are inside the max time window for logging
    if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
    {
      while (i2cGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
      {
        //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
        if (xSemaphoreTake(xFATSemaphore, fatSemaphore_maxWait) == pdPASS)
        {
          uint8_t myBuffer[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

          i2cGNSS.extractFileBufferData((uint8_t *)&myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

          int bytesWritten = ubxFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

          //Force sync every 1000ms
          if (millis() - lastUBXLogSyncTime > 1000)
          {
            lastUBXLogSyncTime = millis();
            digitalWrite(baseStatusLED, !digitalRead(baseStatusLED)); //Blink LED to indicate logging activity
            ubxFile.sync();
            digitalWrite(baseStatusLED, !digitalRead(baseStatusLED)); //Return LED to previous state
          }

          xSemaphoreGive(xFATSemaphore);
        }

        // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
        i2cGNSS.checkUblox(); // Check for the arrival of new data and process it.
      }
    }
  }

  //Report file sizes to show recording is working
  if (online.nmeaLogging == true || online.ubxLogging == true)
  {
    if (millis() - lastFileReport > 5000)
    {
      lastFileReport = millis();
      if (online.nmeaLogging == true && online.ubxLogging == true)
        Serial.printf("UBX and NMEA file size: %d / %d", ubxFile.fileSize(), nmeaFile.fileSize());
      else if (online.nmeaLogging == true)
        Serial.printf("NMEA file size: %d", nmeaFile.fileSize());
      else if (online.ubxLogging == true)
        Serial.printf("UBX file size: %d", ubxFile.fileSize());

      if ((systemTime_minutes - startLogTime_minutes) > settings.maxLogTime_minutes)
        Serial.printf(" reached max log time %d / System time %d",
                      settings.maxLogTime_minutes,
                      (systemTime_minutes - startLogTime_minutes));

      Serial.println();
    }
  }
}

//Once we have a fix, sync system clock to GNSS
//All SD writes will use the system date/time
void updateRTC()
{
  if (online.rtc == false)
  {
    if (online.gnss == true)
    {
      if (i2cGNSS.getConfirmedDate() == true && i2cGNSS.getConfirmedTime() == true)
      {
        //For the ESP32 SD library, the date/time stamp of files is set using the internal system time
        //This is normally set with WiFi NTP but we will rarely have WiFi
        rtc.setTime(i2cGNSS.getSecond(), i2cGNSS.getMinute(), i2cGNSS.getHour(), i2cGNSS.getDay(), i2cGNSS.getMonth(), i2cGNSS.getYear());  // 17th Jan 2021 15:24:30

        online.rtc = true;

        Serial.print(F("System time set to: "));
        Serial.println(rtc.getTime("%B %d %Y %H:%M:%S")); //From ESP32Time library example
      }
    }
  }
}
