//Given the system state, display the appropriate information
void updateDisplay()
{
  //Update the display if connected
  if (online.display == true)
  {
    if (millis() - lastDisplayUpdate > 500) //Update display at 2Hz
    {
      lastDisplayUpdate = millis();

      oled.clear(PAGE); // Clear the display's internal buffer

      switch (systemState)
      {
        case (STATE_ROVER_NO_FIX):
          paintRoverNoFix();
          break;
        case (STATE_ROVER_FIX):
          paintRoverFix();
          break;
        case (STATE_ROVER_RTK_FLOAT):
          paintRoverRTKFloat();
          break;
        case (STATE_ROVER_RTK_FIX):
          paintRoverRTKFix();
          break;
        case (STATE_BASE_TEMP_SURVEY_NOT_STARTED):
          paintBaseTempSurveyNotStarted();
          break;
        case (STATE_BASE_TEMP_SURVEY_STARTED):
          paintBaseTempSurveyStarted();
          break;
        case (STATE_BASE_TEMP_TRANSMITTING):
          paintBaseTempTransmitting();
          break;
        case (STATE_BASE_TEMP_WIFI_STARTED):
          paintBaseTempWiFiStarted();
          break;
        case (STATE_BASE_TEMP_WIFI_CONNECTED):
          paintBaseTempWiFiConnected();
          break;
        case (STATE_BASE_TEMP_CASTER_STARTED):
          paintBaseTempCasterStarted();
          break;
        case (STATE_BASE_TEMP_CASTER_CONNECTED):
          paintBaseTempCasterConnected();
          break;
        case (STATE_BASE_FIXED_TRANSMITTING):
          paintBaseFixedTransmitting();
          break;
        case (STATE_BASE_FIXED_WIFI_STARTED):
          paintBaseFixedWiFiStarted();
          break;
        case (STATE_BASE_FIXED_WIFI_CONNECTED):
          paintBaseFixedWiFiConnected();
          break;
        case (STATE_BASE_FIXED_CASTER_STARTED):
          paintBaseFixedCasterStarted();
          break;
        case (STATE_BASE_FIXED_CASTER_CONNECTED):
          paintBaseFixedCasterConnected();
          break;
      }

      oled.display(); //Push internal buffer to display
    }
  }
}

void displaySplash()
{
  if (online.display == true)
  {
    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(10, 2); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("SparkFun"));

    oled.setCursor(21, 13);
    oled.setFontType(1);
    oled.print(F("RTK"));

    int textX = 3;
    int textY = 25;
    int textKerning = 9;
    oled.setFontType(1);
    printTextwithKerning((char*)"Express", textX, textY, textKerning);

    oled.setCursor(20, 41);
    oled.setFontType(0); //Set font to smallest
    oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    oled.display();
  }
}

void displayShutdown()
{
  if (online.display == true)
  {
    //Show shutdown text
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 2;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Shutting", textX, textY, textKerning);

    textX = 4;
    textY = 25;
    textKerning = 9;
    oled.setFontType(1);

    printTextwithKerning((char*)"Down...", textX, textY, textKerning);

    oled.display();
  }
}

//Displays a small error message then hard freeze
//Text wraps and is small but legible
void displayError(char * errorMessage)
{
  if (online.display == true)
  {
    oled.clear(PAGE); // Clear the display's internal buffer

    oled.setCursor(0, 0); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("Error:"));

    oled.setCursor(2, 10);
    //oled.setFontType(1);
    oled.print(errorMessage);

    oled.display(); //Push internal buffer to display

    while (1) delay(10); //Hard freeze
  }
}

//Print the classic battery icon with levels
void paintBatteryLevel()
{
  if (online.display == true)
  {
    //Current battery charge level
    if (battLevel < 25)
      oled.drawIcon(45, 0, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
    else if (battLevel < 50)
      oled.drawIcon(45, 0, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
    else if (battLevel < 75)
      oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);
    else //batt level > 75
      oled.drawIcon(45, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);
  }
}

//Display Bluetooth icon, Bluetooth MAC, or WiFi depending on connection state
void paintWirelessIcon()
{
  if (online.display == true)
  {
    //Bluetooth icon if paired, or Bluetooth MAC address if not paired
    if (radioState == BT_CONNECTED)
    {
      oled.drawIcon(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol, sizeof(BT_Symbol), true);
    }
    else if (radioState == WIFI_ON_NOCONNECTION)
    {
      //Blink WiFi icon
      if (millis() - lastWifiIconUpdate > 500)
      {
        lastWifiIconUpdate = millis();
        if (wifiIconDisplayed == false)
        {
          wifiIconDisplayed = true;

          //Draw the icon
          oled.drawIcon(6, 1, WiFi_Symbol_Width, WiFi_Symbol_Height, WiFi_Symbol, sizeof(WiFi_Symbol), true);
        }
        else
          wifiIconDisplayed = false;
      }
    }
    else if (radioState == WIFI_CONNECTED)
    {
      //Solid WiFi icon
      oled.drawIcon(6, 1, WiFi_Symbol_Width, WiFi_Symbol_Height, WiFi_Symbol, sizeof(WiFi_Symbol), true);
    }
    else
    {
      char macAddress[5];
      sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
      oled.setFontType(0); //Set font to smallest
      oled.setCursor(0, 4);
      oled.print(macAddress);
    }
  }
}

//Display cross hairs and horizontal accuracy
//Display double circle if we have RTK (blink = float, solid = fix)
void paintHorizontalAccuracy()
{
  if (online.display == true)
  {
    //Blink crosshair icon until we achieve <5m horz accuracy (user definable)
    if (systemState == STATE_BASE_TEMP_SURVEY_NOT_STARTED)
    {
      if (millis() - lastCrosshairIconUpdate > 500)
      {
        lastCrosshairIconUpdate = millis();
        if (crosshairIconDisplayed == false)
        {
          crosshairIconDisplayed = true;

          //Draw the icon
          oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
        }
        else
          crosshairIconDisplayed = false;
      }
    }
    else if (systemState == STATE_ROVER_RTK_FLOAT)
    {
      if (millis() - lastCrosshairIconUpdate > 500)
      {
        lastCrosshairIconUpdate = millis();
        if (crosshairIconDisplayed == false)
        {
          crosshairIconDisplayed = true;

          //Draw dual crosshair
          oled.drawIcon(0, 18, CrossHairDual_Width, CrossHairDual_Height, CrossHairDual, sizeof(CrossHairDual), true);
        }
        else
          crosshairIconDisplayed = false;
      }
    }
    else if (systemState == STATE_ROVER_RTK_FIX)
    {
      //Draw dual crosshair
      oled.drawIcon(0, 18, CrossHairDual_Width, CrossHairDual_Height, CrossHairDual, sizeof(CrossHairDual), true);
    }
    else
    {
      //Draw crosshair
      oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
    }

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(16, 20); //x, y
    oled.print(":");
    float hpa = i2cGNSS.getHorizontalAccuracy() / 10000.0;
    if (hpa > 30.0)
    {
      oled.print(F(">30m"));
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
  }
}

//Draw either a rover or base icon depending on screen
//Draw a different base if we have fixed coordinate base type
void paintBaseState()
{
  if (online.display == true)
  {
    if (systemState == STATE_ROVER_NO_FIX ||
        systemState == STATE_ROVER_FIX ||
        systemState == STATE_ROVER_RTK_FLOAT ||
        systemState == STATE_ROVER_RTK_FIX)
    {
      oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);
    }
    else if (systemState == STATE_BASE_TEMP_SURVEY_STARTED ||
             systemState == STATE_BASE_TEMP_SURVEY_NOT_STARTED //Turn on base icon solid (blink crosshair in paintHorzAcc)
            )
    {
      //Blink base icon until survey is complete
      if (millis() - lastBaseIconUpdate > 500)
      {
        lastBaseIconUpdate = millis();
        if (baseIconDisplayed == false)
        {
          baseIconDisplayed = true;

          //Draw the icon
          oled.drawIcon(27, 0, BaseTemporary_Width, BaseTemporary_Height, BaseTemporary, sizeof(BaseTemporary), true); //true - blend with other pixels
        }
        else
          baseIconDisplayed = false;
      }
    }
    else if (systemState == STATE_BASE_TEMP_TRANSMITTING ||
             systemState == STATE_BASE_TEMP_WIFI_STARTED ||
             systemState == STATE_BASE_TEMP_WIFI_CONNECTED ||
             systemState == STATE_BASE_TEMP_CASTER_STARTED ||
             systemState == STATE_BASE_TEMP_CASTER_CONNECTED)
    {
      //Draw the icon
      oled.drawIcon(27, 0, BaseTemporary_Width, BaseTemporary_Height, BaseTemporary, sizeof(BaseTemporary), true); //true - blend with other pixels
    }
    else if (systemState == STATE_BASE_FIXED_TRANSMITTING ||
             systemState == STATE_BASE_FIXED_WIFI_STARTED ||
             systemState == STATE_BASE_FIXED_WIFI_CONNECTED ||
             systemState == STATE_BASE_FIXED_CASTER_STARTED ||
             systemState == STATE_BASE_FIXED_CASTER_CONNECTED)
    {
      //Draw the icon
      oled.drawIcon(27, 0, BaseFixed_Width, BaseFixed_Height, BaseFixed, sizeof(BaseFixed), true); //true - blend with other pixels
    }
  }
}

//Draw satellite icon and sats in view
//Blink icon if no fix
void paintSIV()
{
  if (online.display == true)
  {
    //Blink satellite dish icon if we don't have a fix
    if (i2cGNSS.getFixType() == 3 || i2cGNSS.getFixType() == 5) //3D or Time
    {
      //Fix, turn on icon
      oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
    }
    else
    {
      if (millis() - lastSatelliteDishIconUpdate > 500)
      {
        //Serial.println("SIV Blink");
        lastSatelliteDishIconUpdate = millis();
        if (satelliteDishIconDisplayed == false)
        {
          satelliteDishIconDisplayed = true;

          //Draw the icon
          oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
        }
        else
          satelliteDishIconDisplayed = false;
      }
    }

    oled.setFontType(1); //Set font to type 1: 8x16
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
  }
}

//Draw log icon
//Turn off icon if log file fails to get bigger
void paintLogging()
{
  if (online.display == true)
  {
    if (logIncreasing == true)
    {
      oled.drawIcon(64 - Logging_Width, 48 - Logging_Height, Logging_Width, Logging_Height, Logging, sizeof(Logging), true); //Draw the icon
    }
  }
}

//Base screen. Display BLE, rover, battery, HorzAcc and SIV
//Blink SIV until fix
void paintRoverNoFix()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    paintHorizontalAccuracy();

    paintSIV();

    paintLogging();
  }
}

//Currently identical to RoverNoFix because paintSIV and paintHorizontalAccuracy takes into account system states
void paintRoverFix()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    paintHorizontalAccuracy();

    paintSIV();

    paintLogging();
  }
}

//Currently identical to RoverNoFix because paintSIV and paintHorizontalAccuracy takes into account system states
void paintRoverRTKFloat()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    paintHorizontalAccuracy();

    paintSIV();

    paintLogging();
  }
}

void paintRoverRTKFix()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    paintHorizontalAccuracy();

    paintSIV();

    paintLogging();
  }
}

//Start of base / survey in / NTRIP mode
//Screen is displayed while we are waiting for horz accuracy to drop to appropriate level
//Blink crosshair icon until we have we have horz accuracy < user defined level
void paintBaseTempSurveyNotStarted()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    paintHorizontalAccuracy(); //2nd line

    paintSIV();

    paintLogging();
  }
}

//Survey in is running. Show 3D Mean and elapsed time.
void paintBaseTempSurveyStarted()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    float meanAccuracy = i2cGNSS.getSurveyInMeanAccuracy(100);
    int elapsedTime = i2cGNSS.getSurveyInObservationTime(100);

    oled.setFontType(0);
    oled.setCursor(0, 23); //x, y
    oled.print("Mean:");

    oled.setCursor(29, 20); //x, y
    oled.setFontType(1);
    if (meanAccuracy < 10.0) //Error check
      oled.print(meanAccuracy, 2);
    else
      oled.print(">10");

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("Time:");

    oled.setCursor(30, 36); //x, y
    oled.setFontType(1);
    if (elapsedTime < 1000) //Error check
      oled.print(elapsedTime);
    else
      oled.print("0");

    paintLogging();
  }
}

//Show transmission of RTCM packets
void paintBaseTempTransmitting()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 1;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Xmitting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show transmission of RTCM packets
//Blink WiFi icon
void paintBaseTempWiFiStarted()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 1;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Xmitting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show transmission of RTCM packets
//Solid WiFi icon
//This is identical to paintBaseTempWiFiStarted
void paintBaseTempWiFiConnected()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 1;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Xmitting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show connecting to caster service
//Solid WiFi icon
void paintBaseTempCasterStarted()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 11;
    int textY = 17;
    int textKerning = 8;

    printTextwithKerning((char*)"Caster", textX, textY, textKerning);

    textX = 3;
    textY = 33;
    textKerning = 6;
    oled.setFontType(1);

    printTextwithKerning((char*)"Connecting", textX, textY, textKerning);
  }
}

//Show transmission of RTCM packets to caster service
//Solid WiFi icon
void paintBaseTempCasterConnected()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 4;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Casting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show transmission of RTCM packets
void paintBaseFixedTransmitting()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 1;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Xmitting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show transmission of RTCM packets
//Blink WiFi icon
void paintBaseFixedWiFiStarted()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 1;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Xmitting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show transmission of RTCM packets
//Solid WiFi icon
//This is identical to paintBaseTempWiFiStarted
void paintBaseFixedWiFiConnected()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 1;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Xmitting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show connecting to caster service
//Solid WiFi icon
void paintBaseFixedCasterStarted()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 11;
    int textY = 18;
    int textKerning = 8;

    printTextwithKerning((char*)"Caster", textX, textY, textKerning);

    textX = 3;
    textY = 33;
    textKerning = 6;
    oled.setFontType(1);

    printTextwithKerning((char*)"Connecting", textX, textY, textKerning);
  }
}

//Show transmission of RTCM packets to caster service
//Solid WiFi icon
void paintBaseFixedCasterConnected()
{
  if (online.display == true)
  {
    paintBatteryLevel(); //Top right corner

    paintWirelessIcon(); //Top left corner

    paintBaseState(); //Top center

    int textX = 4;
    int textY = 17;
    int textKerning = 8;
    oled.setFontType(1);
    printTextwithKerning((char*)"Casting", textX, textY, textKerning);

    oled.setCursor(0, 39); //x, y
    oled.setFontType(0);
    oled.print("RTCM:");

    if (rtcmPacketsSent < 100)
      oled.setCursor(30, 36); //x, y - Give space for two digits
    else
      oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

    paintLogging();
  }
}

//Show error, 15 minutes elapsed without sufficient environment
//void paintBaseFailed()
//{
//  oled.setFontType(0);
//  oled.setCursor(0, 22); //x, y
//  oled.print("Base Fail Please    Reset");
//}

void displayBaseStart()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 18;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Base", textX, textY, textKerning);

    oled.display();
  }
}

void displayBaseSuccess()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 18;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Base", textX, textY, textKerning);

    textX = 5;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"Started", textX, textY, textKerning);
    oled.display();
  }
}

void displayBaseFail()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 18;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Base", textX, textY, textKerning);

    textX = 10;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"Failed", textX, textY, textKerning);
    oled.display();
  }
}

void displayRoverStart()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 14;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Rover", textX, textY, textKerning);

    oled.display();
  }
}

void displayRoverSuccess()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 14;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Rover", textX, textY, textKerning);

    textX = 5;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"Started", textX, textY, textKerning);
    oled.display();
  }
}

void displayRoverFail()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 14;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Rover", textX, textY, textKerning);

    textX = 10;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"Failed", textX, textY, textKerning);
    oled.display();
  }
}

//When user enter serial config menu the display will freeze so show splash while config happens
void displaySerialConfig()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 10;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Serial", textX, textY, textKerning);

    textX = 10;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"Config", textX, textY, textKerning);
    oled.display();
  }
}

void displaySurveyStart()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 10;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Survey", textX, textY, textKerning);

    oled.display();
  }
}

void displaySurveyStarted()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 10;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Survey", textX, textY, textKerning);

    textX = 6;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"Started", textX, textY, textKerning);
    oled.display();
  }
}

//Draw a frame at outside edge
void drawFrame()
{
  //Init and draw box at edge to see screen alignment
  int xMax = 63;
  int yMax = 47;
  oled.line(0, 0, xMax, 0); //Top
  oled.line(0, 0, 0, yMax); //Left
  oled.line(0, yMax, xMax, yMax); //Bottom
  oled.line(xMax, 0, xMax, yMax); //Right
}

//If the SD card is detected but is not formatted correctly, display warning
void displaySDFail()
{
  if (online.display == true)
  {
    oled.clear(PAGE);

    oled.setCursor(21, 13);
    oled.setFontType(1);

    int textX = 11;
    int textY = 10;
    int textKerning = 8;

    printTextwithKerning((char*)"Format", textX, textY, textKerning);

    textX = 7;
    textY = 25;
    textKerning = 8;
    oled.setFontType(1);

    printTextwithKerning((char*)"SD Card", textX, textY, textKerning);
    oled.display();
  }
}
