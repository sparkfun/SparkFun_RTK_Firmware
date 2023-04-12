//Setup the u-blox module for any setup (base or rover)
//In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare occasion, become
//corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked.
bool configureUbloxModule()
{
  if (online.gnss == false) return (false);

  bool response = true;

  //Turn on/off debug messages
  if (settings.enableI2Cdebug)
  {
#if defined(REF_STN_GNSS_DEBUG)
    if (ENABLE_DEVELOPER && productVariant == REFERENCE_STATION)
      theGNSS.enableDebugging(serialGNSS); //Output all debug messages over serialGNSS
    else
#endif
    theGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
  }
  else
    theGNSS.disableDebugging();

  //Wait for initial report from module
  int maxWait = 2000;
  startTime = millis();
  while (pvtUpdated == false)
  {
    theGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
    theGNSS.checkCallbacks(); //Process any callbacks: ie, eventTriggerReceived
    delay(10);
    if ((millis() - startTime) > maxWait)
    {
      log_d("PVT Update failed");
      break;
    }
  }

  //The first thing we do is go to 1Hz to lighten any I2C traffic from a previous configuration
  response &= theGNSS.newCfgValset();
  response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, 1000);
  response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, 1);

  //Survey mode is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
    response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0); //Disable survey-in mode

  //UART1 will primarily be used to pass NMEA and UBX from ZED to ESP32 (eventually to cell phone)
  //but the phone can also provide RTCM data and a user may want to configure the ZED over Bluetooth.
  //So let's be sure to enable UBX+NMEA+RTCM on the input
  if (USE_I2C_GNSS)
  {
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 1);
    if (zedModuleType == PLATFORM_F9P)
      response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 1);
    if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
      response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);
  
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1_BAUDRATE, settings.dataPortBaud); //Defaults to 230400 to maximize message output support
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, settings.radioPortBaud); //Defaults to 57600 to match SiK telemetry radio firmware default
  
    //Disable SPI port - This is just to remove some overhead by ZED
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_NMEA, 0);
    if (zedModuleType == PLATFORM_F9P)
      response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_RTCM3X, 0);
  
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_NMEA, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_RTCM3X, 0);
    if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
      response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_SPARTN, 0);
  }
  else //SPI GNSS
  {
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_UBX, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_NMEA, 1);
    if (zedModuleType == PLATFORM_F9P)
      response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_RTCM3X, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_UBX, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_NMEA, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_RTCM3X, 1);
    if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
      response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_SPARTN, 0);
  
    //Disable I2C and UART1 ports - This is just to remove some overhead by ZED
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_NMEA, 0);
    if (zedModuleType == PLATFORM_F9P)
      response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_RTCM3X, 0);
  
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_NMEA, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_RTCM3X, 0);
    if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
      response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_SPARTN, 0);

    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 0);
    if (zedModuleType == PLATFORM_F9P)
      response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 0);
  
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 0);
    if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
      response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);
  }

  //Set the UART2 to only do RTCM (in case this device goes into base mode)
  response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_NMEA, 0);
  if (zedModuleType == PLATFORM_F9P)
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_RTCM3X, 1);
  response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_UBX, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_NMEA, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_RTCM3X, 1);
  if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_SPARTN, 0);

  //We don't want NMEA over I2C, but we will want to deliver RTCM, and UBX+RTCM is not an option
  if (USE_I2C_GNSS)
  {
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_UBX, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_NMEA, 1);
    if (zedModuleType == PLATFORM_F9P)
      response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_RTCM3X, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_UBX, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_NMEA, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_RTCM3X, 1);
    if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
    {
      if (productVariant == RTK_FACET_LBAND)
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_SPARTN, 1); //We push NEO-D9S correction data (SPARTN) to ZED-F9P over the I2C interface
      else
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_SPARTN, 0);
    }
  }

  //The USB port on the ZED may be used for RTCM to/from the computer (as an NTRIP caster or client)
  //So let's be sure all protocols are on for the USB port
  response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_UBX, 1);
  response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_NMEA, 1);
  if (zedModuleType == PLATFORM_F9P)
    response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_RTCM3X, 1);
  response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_UBX, 1);
  response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_NMEA, 1);
  response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_RTCM3X, 1);
  if (zedModuleType == PLATFORM_F9P && zedFirmwareVersionInt >= 120)
    response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_SPARTN, 0);

  response &= theGNSS.sendCfgValset();

  if (response == false)
    systemPrintln("Module failed config block 0");
  response = true; //Reset

  //Enable the constellations the user has set
  response &= setConstellations(true); //19 messages. Send newCfg or sendCfg with value set
  if (response == false)
    systemPrintln("Module failed config block 1");
  response = true; //Reset

  //Make sure the appropriate messages are enabled
  response &= setMessages(MAX_SET_MESSAGES_RETRIES); //Does a complete open/closed val set
  if (response == false)
    systemPrintln("Module failed config block 2");
  response = true; //Reset

  //Disable NMEA messages on all but UART1
  response &= theGNSS.newCfgValset();

  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_I2C, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_I2C, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_I2C, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_I2C, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_I2C, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_I2C, 0);

  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART2, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART2, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART2, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART2, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART2, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART2, 0);
  response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART2, 0);

  if (USE_I2C_GNSS) //Don't disable NMEA on SPI if the GNSS is SPI!
  {
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_SPI, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_SPI, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_SPI, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_SPI, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_SPI, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI, 0);
  }

  if (USE_SPI_GNSS) //If the GNSS is SPI, _do_ disable NMEA on UART1
  {
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART1, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1, 0);
  }
  
  
  response &= theGNSS.sendCfgValset();

  if (response == false)
    systemPrintln("Module failed config block 3");

  if (zedModuleType == PLATFORM_F9R)
  {
    response &= theGNSS.setAutoESFSTATUS(true, false); //Tell the GPS to "send" each ESF Status, but do not update stale data when accessed
  }

  return (response);
}

//Turn on indicator LEDs to verify LED function and indicate setup sucess
void danceLEDs()
{
  if (productVariant == RTK_SURVEYOR)
  {
    for (int x = 0 ; x < 2 ; x++)
    {
      digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
      digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
      digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
      digitalWrite(pin_baseStatusLED, HIGH);
      digitalWrite(pin_bluetoothStatusLED, HIGH);
      delay(100);
      digitalWrite(pin_positionAccuracyLED_1cm, LOW);
      digitalWrite(pin_positionAccuracyLED_10cm, LOW);
      digitalWrite(pin_positionAccuracyLED_100cm, LOW);
      digitalWrite(pin_baseStatusLED, LOW);
      digitalWrite(pin_bluetoothStatusLED, LOW);
      delay(100);
    }

    digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
    digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
    digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
    digitalWrite(pin_baseStatusLED, HIGH);
    digitalWrite(pin_bluetoothStatusLED, HIGH);

    delay(250);
    digitalWrite(pin_positionAccuracyLED_1cm, LOW);
    delay(250);
    digitalWrite(pin_positionAccuracyLED_10cm, LOW);
    delay(250);
    digitalWrite(pin_positionAccuracyLED_100cm, LOW);

    delay(250);
    digitalWrite(pin_baseStatusLED, LOW);
    delay(250);
    digitalWrite(pin_bluetoothStatusLED, LOW);
  }
  else
  {
    //Units can boot under 1s. Keep splash screen up for at least 2s.
    while ((millis() - splashStart) < 2000) delay(1);
  }
}

//Update Battery level LEDs every 5s
void updateBattery()
{
  if (millis() - lastBattUpdate > 5000)
  {
    lastBattUpdate = millis();

    checkBatteryLevels();
  }
}

//When called, checks level of battery and updates the LED brightnesses
//And outputs a serial message to USB
void checkBatteryLevels()
{
  if (online.battery == true)
  {
    battLevel = lipo.getSOC();
    battVoltage = lipo.getVoltage();
    battChangeRate = lipo.getChangeRate();
  }
  else
  {
    //False numbers but above system cut-off level
    battLevel = 10;
    battVoltage = 3.7;
    battChangeRate = 0;
  }

  if (settings.enablePrintBatteryMessages)
  {
    systemPrintf("Batt (%d%%): Voltage: %0.02fV", battLevel, battVoltage);

    char tempStr[25];
    if (battChangeRate >= -0.01)
    {
      snprintf(tempStr, sizeof(tempStr), "C");
      externalPowerConnected = true;
    }
    else
    {
      snprintf(tempStr, sizeof(tempStr), "Disc");
      externalPowerConnected = false;
    }

    systemPrintf(" %sharging: %0.02f%%/hr ", tempStr, battChangeRate);

    if (battLevel < 10)
      snprintf(tempStr, sizeof(tempStr), "Red");
    else if (battLevel < 50)
      snprintf(tempStr, sizeof(tempStr), "Yellow");
    else if (battLevel >= 50)
      snprintf(tempStr, sizeof(tempStr), "Green");
    else
      snprintf(tempStr, sizeof(tempStr), "No batt");

    systemPrintf("%s\r\n", tempStr);
  }

  if (productVariant == RTK_SURVEYOR)
  {
    if (battLevel < 10)
    {
      ledcWrite(ledRedChannel, 255);
      ledcWrite(ledGreenChannel, 0);
    }
    else if (battLevel < 50)
    {
      ledcWrite(ledRedChannel, 128);
      ledcWrite(ledGreenChannel, 128);
    }
    else if (battLevel >= 50)
    {
      ledcWrite(ledRedChannel, 0);
      ledcWrite(ledGreenChannel, 255);
    }
    else
    {
      ledcWrite(ledRedChannel, 10);
      ledcWrite(ledGreenChannel, 0);
    }
  }
}

//Ping an I2C device and see if it responds
bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

//Create a test file in file structure to make sure we can
bool createTestFile()
{
  FileSdFatMMC testFile;

  //TODO: double-check that SdFat tollerates preceeding slashes
  char testFileName[40] = "/testfile.txt";

  //Attempt to write to the file system
  if (testFile.open(testFileName, O_CREAT | O_APPEND | O_WRITE) != true)
  {
    systemPrintln("createTestFile: failed to create (open) test file");
    return (false);
  }

  testFile.println("Testing...");

  //File successfully created
  testFile.close();

  if (USE_SPI_MICROSD)
  {
    if (sd->exists(testFileName))
      sd->remove(testFileName);
    return (!sd->exists(testFileName));
  }
#ifdef COMPILE_SD_MMC
  else
  {
    if (SD_MMC.exists(testFileName))
      SD_MMC.remove(testFileName);
    return (!SD_MMC.exists(testFileName));
  }
#endif
}

//If debug option is on, print available heap
void reportHeapNow()
{
  if (settings.enableHeapReport == true)
  {
    lastHeapReport = millis();
    systemPrintf("FreeHeap: %d / HeapLowestPoint: %d / LargestBlock: %d\r\n", ESP.getFreeHeap(), xPortGetMinimumEverFreeHeapSize(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  }
}

//If debug option is on, print available heap
void reportHeap()
{
  if (settings.enableHeapReport == true)
  {
    if (millis() - lastHeapReport > 1000)
    {
      reportHeapNow();
    }
  }
}

//Based on current LED state, blink upwards fashion
//Used to indicate casting
void cyclePositionLEDs()
{
  if (productVariant == RTK_SURVEYOR)
  {
    //Cycle position LEDs to indicate casting
    if (millis() - lastCasterLEDupdate > 500)
    {
      lastCasterLEDupdate = millis();
      if (digitalRead(pin_positionAccuracyLED_100cm) == HIGH)
      {
        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
        digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
        digitalWrite(pin_positionAccuracyLED_100cm, LOW);
      }
      else if (digitalRead(pin_positionAccuracyLED_10cm) == HIGH)
      {
        digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
        digitalWrite(pin_positionAccuracyLED_100cm, LOW);
      }
      else //Catch all
      {
        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
        digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
      }
    }
  }
}

//Set the port of the 1:4 dual channel analog mux
//This allows NMEA, I2C, PPS/Event, and ADC/DAC to be routed through data port via software select
void setMuxport(int channelNumber)
{
  if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
  {
    pinMode(pin_muxA, OUTPUT);
    pinMode(pin_muxB, OUTPUT);

    if (channelNumber > 3) return; //Error check

    switch (channelNumber)
    {
      case 0:
        digitalWrite(pin_muxA, LOW);
        digitalWrite(pin_muxB, LOW);
        break;
      case 1:
        digitalWrite(pin_muxA, HIGH);
        digitalWrite(pin_muxB, LOW);
        break;
      case 2:
        digitalWrite(pin_muxA, LOW);
        digitalWrite(pin_muxB, HIGH);
        break;
      case 3:
        digitalWrite(pin_muxA, HIGH);
        digitalWrite(pin_muxB, HIGH);
        break;
    }
  }
}

//Create $GNTXT, type message complete with CRC
//https://www.nmea.org/Assets/20160520%20txt%20amendment.pdf
//Used for recording system events (boot reason, event triggers, etc) inside the log
void createNMEASentence(customNmeaType_e textID, char *nmeaMessage, size_t sizeOfNmeaMessage, char *textMessage)
{
  //Currently we don't have messages longer than 82 char max so we hardcode the sentence numbers
  const uint8_t totalNumberOfSentences = 1;
  const uint8_t sentenceNumber = 1;

  char nmeaTxt[200]; //Max NMEA sentence length is 82
  snprintf(nmeaTxt, sizeof(nmeaTxt), "$GNTXT,%02d,%02d,%02d,%s*", totalNumberOfSentences, sentenceNumber, textID, textMessage);

  //From: http://engineeringnotes.blogspot.com/2015/02/generate-crc-for-nmea-strings-arduino.html
  byte CRC = 0; //XOR chars between '$' and '*'
  for (byte x = 1 ; x < strlen(nmeaTxt) - 1; x++)
    CRC = CRC ^ nmeaTxt[x];

  snprintf(nmeaMessage, sizeOfNmeaMessage, "%s%02X", nmeaTxt, CRC);
}

//Reset settings struct to default initializers
void settingsToDefaults()
{
  settings = defaultSettings;
}

//Enable all the valid messages for this platform
//There are ~89 messages so split into batches. VALSET is limited to 64 max per batch
//Uses dummy newCfg and sendCfg values to be sure we open/close a complete set
bool setMessages(int maxRetries)
{
  uint32_t spiOffset = 0; //Set to 3 if using SPI to convert UART1 keys to SPI. This is brittle and non-perfect, but works.
  if (USE_SPI_GNSS)
    spiOffset = 3;

  bool success = false;
  int tryNo = -1;
  while ((++tryNo < maxRetries) && !success)
  {
    bool response = true;
    
    int x = 0;
    while (x < MAX_UBX_MSG)
    {
      response &= theGNSS.newCfgValset();
      
      do
      {
        if (ubxMessages[x].supported & zedModuleType)
        {
          uint8_t rate = settings.ubxMessageRates[x];
  
          //If the GNSS is SPI, we need to make sure that NAV_PVT, NAV_HPPOSLLH, ESF_STATUS and TIM_TP remained enabled
          //(but not enabled for logging)
          if (USE_SPI_GNSS)
          {
            if (ubxMessages[x].msgClass == UBX_CLASS_NAV)
              if ((ubxMessages[x].msgID == UBX_NAV_PVT) || (ubxMessages[x].msgID == UBX_NAV_HPPOSLLH))
                if (rate == 0)
                  rate = 1;
            if (ubxMessages[x].msgClass == UBX_CLASS_ESF)
              if (ubxMessages[x].msgID ==  UBX_ESF_STATUS)
                if (zedModuleType == PLATFORM_F9R)
                  if (rate == 0)
                    rate = 1;
            if (ubxMessages[x].msgClass == UBX_CLASS_TIM)
            {
              if (ubxMessages[x].msgID ==  UBX_TIM_TM2)
                if (rate == 0)
                  rate = 1;
              if (ubxMessages[x].msgID ==  UBX_TIM_TP)
                if (HAS_GNSS_TP_INT)
                  if (rate == 0)
                    rate = 1;
            }
            if (ubxMessages[x].msgClass == UBX_CLASS_RXM)
              if (ubxMessages[x].msgID ==  UBX_RXM_COR)
                if (rate == 0)
                  rate = 1;
            if (ubxMessages[x].msgClass == UBX_CLASS_NMEA)
              if (ubxMessages[x].msgID ==  UBX_NMEA_GGA)
                if (rate == 0)
                  rate = 1;
          }
          
          response &= theGNSS.addCfgValset(ubxMessages[x].msgConfigKey + spiOffset, rate);
        }
        x++;
      }
      while (((x % 43) < 42) && (x < MAX_UBX_MSG)); //Limit 1st batch to 42. Batches after that will be (up to) 43 in size. It's a HHGTTG thing.
      
      response &= theGNSS.sendCfgValset();
    }
  
    //For SPI GNSS products, we need to add each message to the GNSS Library logging buffer
    //to mimic UART1
    if (USE_SPI_GNSS)
    {
      uint32_t logRTCMMessages = 0;
      uint32_t logNMEAMessages = 0;
      
      for (x = 0; x < MAX_UBX_MSG; x++)
      {
        if (ubxMessages[x].msgClass == UBX_RTCM_MSB) //RTCM messages
        {
          if ((settings.ubxMessageRates[x] > 0) && (ubxMessages[x].supported & zedModuleType))
            logRTCMMessages |= ubxMessages[x].filterMask;
        }
        else if (ubxMessages[x].msgClass == UBX_CLASS_NMEA) //NMEA messages
        {
          if ((settings.ubxMessageRates[x] > 0) && (ubxMessages[x].supported & zedModuleType))
            logNMEAMessages |= ubxMessages[x].filterMask;
        }
        else //UBX messages
        {
          if (ubxMessages[x].supported & zedModuleType)
            theGNSS.enableUBXlogging(ubxMessages[x].msgClass, ubxMessages[x].msgID, settings.ubxMessageRates[x] > 0);
        }
      }
  
      theGNSS.setRTCMLoggingMask(logRTCMMessages);
      theGNSS.setNMEALoggingMask(logNMEAMessages);
    }
  
  if (response)
    success = true;
  }

  return (success);
}

//Enable all the valid messages for this platform over the USB port
//Add 2 to every UART1 key. This is brittle and non-perfect, but works.
bool setMessagesUSB(int maxRetries)
{
  bool success = false;
  int tryNo = -1;
  while ((++tryNo < maxRetries) && !success)
  {
    bool response = true;
    
    int x = 0;
    while (x < MAX_UBX_MSG)
    {
      response &= theGNSS.newCfgValset();
      
      do
      {
        if (ubxMessages[x].supported & zedModuleType)
          response &= theGNSS.addCfgValset(ubxMessages[x].msgConfigKey + 2, settings.ubxMessageRates[x]);
        x++;
      }
      while (((x % 43) < 42) && (x < MAX_UBX_MSG)); //Limit 1st batch to 42. Batches after that will be (up to) 43 in size. It's a HHGTTG thing.
      
      response &= theGNSS.sendCfgValset();
    }

  if (response)
    success = true;
  }

  return (success);
}

//Enable all the valid constellations and bands for this platform
//Band support varies between platforms and firmware versions
//We open/close a complete set if sendCompleteBatch = true
//19 messages
bool setConstellations(bool sendCompleteBatch)
{
  bool response = true;

  if (sendCompleteBatch)
    response &= theGNSS.newCfgValset();

  bool enableMe = settings.ubxConstellations[0].enabled;
  response &= theGNSS.addCfgValset(settings.ubxConstellations[0].configKey, enableMe); //GPS

  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GPS_L1CA_ENA, settings.ubxConstellations[0].enabled);
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GPS_L2C_ENA, settings.ubxConstellations[0].enabled);

  //v1.12 ZED-F9P firmware does not allow for SBAS control
  //Also, if we can't identify the version (99), skip SBAS enable
  if ((zedModuleType == PLATFORM_F9P) && ((zedFirmwareVersionInt == 112) || (zedFirmwareVersionInt == 99)))
  {
    //Skip
  }
  else
  {
    response &= theGNSS.addCfgValset(settings.ubxConstellations[1].configKey, settings.ubxConstellations[1].enabled); //SBAS
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_SBAS_L1CA_ENA, settings.ubxConstellations[1].enabled);
  }

  response &= theGNSS.addCfgValset(settings.ubxConstellations[2].configKey, settings.ubxConstellations[2].enabled); //GAL
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GAL_E1_ENA, settings.ubxConstellations[2].enabled);
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GAL_E5B_ENA, settings.ubxConstellations[2].enabled);

  response &= theGNSS.addCfgValset(settings.ubxConstellations[3].configKey, settings.ubxConstellations[3].enabled); //BDS
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_BDS_B1_ENA, settings.ubxConstellations[3].enabled);
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_BDS_B2_ENA, settings.ubxConstellations[3].enabled);

  response &= theGNSS.addCfgValset(settings.ubxConstellations[4].configKey, settings.ubxConstellations[4].enabled); //QZSS
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L1CA_ENA, settings.ubxConstellations[4].enabled);

  //UBLOX_CFG_SIGNAL_QZSS_L1S_ENA not supported on F9R in v1.21 and below
  if (zedModuleType == PLATFORM_F9P)
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L1S_ENA, settings.ubxConstellations[4].enabled);
  else if ((zedModuleType == PLATFORM_F9R) && (zedFirmwareVersionInt > 121))
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L1S_ENA, settings.ubxConstellations[4].enabled);

  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L2C_ENA, settings.ubxConstellations[4].enabled);

  response &= theGNSS.addCfgValset(settings.ubxConstellations[5].configKey, settings.ubxConstellations[5].enabled); //GLO
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GLO_L1_ENA, settings.ubxConstellations[5].enabled);
  response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GLO_L2_ENA, settings.ubxConstellations[5].enabled);

  if (sendCompleteBatch)
    response &= theGNSS.sendCfgValset();

  return (response);
}

//Periodically print position if enabled
void printPosition()
{
  //Periodically print the position
  if (settings.enablePrintPosition && ((millis() - lastPrintPosition) > 15000))
  {
    printCurrentConditions();
    lastPrintPosition = millis();
  }
}
