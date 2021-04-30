//Regularly update subsystems. Called from main loop (aka not tasks).

//Change between Rover or Base depending on switch state
void checkSetupSwitch()
{
  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH && baseState != BASE_OFF)
  {
    //Configure for rover mode
    Serial.println(F("Rover Mode"));

    baseState = BASE_OFF;

    beginBluetooth(); //Restart Bluetooth with 'Rover' name

    //If we are survey'd in, but switch is rover then disable survey
    if (configureUbloxModuleRover() == false)
    {
      Serial.println(F("Rover config failed!"));
    }

    digitalWrite(baseStatusLED, LOW);
  }
  else if (digitalRead(baseSwitch) == LOW && baseState == BASE_OFF)
  {
    //Configure for base mode
    Serial.println(F("Base Mode"));

    if (configureUbloxModuleBase() == false)
    {
      Serial.println(F("Base config failed!"));
    }

    //Restart Bluetooth with 'Base' name
    //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
    beginBluetooth();

    baseState = BASE_SURVEYING_IN_NOTSTARTED; //Switch to new state
  }  
}

void updateDisplay()
{
  //Update the display if connected
  if (online.display == true)
  {
    if (millis() - lastDisplayUpdate > 1000)
    {
      lastDisplayUpdate = millis();

      oled.clear(PAGE); // Clear the display's internal buffer

      //Current battery charge level
      if (battLevel < 25)
        oled.drawIcon(45, 0, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
      else if (battLevel < 50)
        oled.drawIcon(45, 0, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
      else if (battLevel < 75)
        oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);
      else //batt level > 75
        oled.drawIcon(45, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);

      //Bluetooth Address or RSSI
      if (radioState == BT_CONNECTED)
      {
        oled.drawIcon(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol, sizeof(BT_Symbol), true);
      }
      else
      {
        char macAddress[5];
        sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
        oled.setFontType(0); //Set font to smallest
        oled.setCursor(0, 4);
        oled.print(macAddress);
      }

      if (digitalRead(baseSwitch) == LOW)
        oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true); //true - blend with other pixels
      else
        oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);

      //Horz positional accuracy
      oled.setFontType(1); //Set font to type 1: 8x16
      oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
      oled.setCursor(16, 20); //x, y
      oled.print(":");
      float hpa = i2cGNSS.getHorizontalAccuracy() / 10000.0;
      if (hpa > 30.0)
      {
        oled.print(F(">30"));
      }
      else if (hpa > 9.9)
      {
        oled.print(hpa, 1); //Print down to decimeter
      }
      else if (hpa > 1.0)
      {
        oled.print(hpa, 2); //Print down to centimeter
      }
      else
      {
        oled.print("."); //Remove leading zero
        oled.printf("%03d", (int)(hpa * 1000)); //Print down to millimeter
      }

      //SIV
      oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
      oled.setCursor(16, 36); //x, y
      oled.print(":");

      if (i2cGNSS.getFixType() == 0) //0 = No Fix
      {
        oled.print("0");
      }
      else
      {
        oled.print(i2cGNSS.getSIV());
      }

      oled.display();
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
