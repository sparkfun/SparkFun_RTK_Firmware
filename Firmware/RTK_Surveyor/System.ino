//Setup the u-blox module for any setup (base or rover)
//In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare occasion, become
//corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked.
bool configureUbloxModule()
{
  if (online.gnss == false) return (false);

  bool response = true;

  //Wait for initial report from module
  int maxWait = 2000;
  startTime = millis();
  while (pvtUpdated == false)
  {
    i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
    i2cGNSS.checkCallbacks(); //Process any callbacks: ie, eventTriggerReceived
    delay(10);
    if (millis() - startTime > maxWait)
    {
      log_d("PVT Update failed");
      break;
    }
  }

  //The first thing we do is go to 1Hz to lighten any I2C traffic from a previous configuration
  response &= i2cGNSS.newCfgValset16(UBLOX_CFG_RATE_MEAS, 1000);
  response &= i2cGNSS.addCfgValset16(UBLOX_CFG_RATE_NAV, 1);

  //Survey mode is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_TMODE_MODE, 0); //Disable survey-in mode

  //UART1 will primarily be used to pass NMEA and UBX from ZED to ESP32 (eventually to cell phone)
  //but the phone can also provide RTCM data and a user may want to configure the ZED over Bluetooth.
  //So let's be sure to enable UBX+NMEA+RTCM on the input
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART1OUTPROT_UBX, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART1OUTPROT_NMEA, 1);
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART1OUTPROT_RTCM3X, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART1INPROT_UBX, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART1INPROT_NMEA, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART1INPROT_RTCM3X, 1);

  response &= i2cGNSS.addCfgValset32(UBLOX_CFG_UART1_BAUDRATE, settings.dataPortBaud); //Defaults to 460800 to maximize message output support
  response &= i2cGNSS.addCfgValset32(UBLOX_CFG_UART2_BAUDRATE, settings.radioPortBaud); //Defaults to 57600 to match SiK telemetry radio firmware default

  //Disable SPI port - This is just to remove some overhead by ZED
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SPIOUTPROT_UBX, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SPIOUTPROT_NMEA, 0);
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SPIOUTPROT_RTCM3X, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SPIINPROT_UBX, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SPIINPROT_NMEA, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SPIINPROT_RTCM3X, 0);

  //Set the UART2 to only do RTCM (in case this device goes into base mode)
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART2OUTPROT_UBX, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART2OUTPROT_NMEA, 0);
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART2OUTPROT_RTCM3X, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART2INPROT_UBX, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART2INPROT_NMEA, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_UART2INPROT_RTCM3X, 1);

  //We don't want NMEA over I2C, but we will want to deliver RTCM, and UBX+RTCM is not an option
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_I2COUTPROT_UBX, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_I2COUTPROT_NMEA, 1);
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_I2COUTPROT_RTCM3X, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_I2CINPROT_UBX, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_I2CINPROT_NMEA, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_I2CINPROT_RTCM3X, 1);

  //The USB port on the ZED may be used for RTCM to/from the computer (as an NTRIP caster or client)
  //So let's be sure all protocols are on for the USB port
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_USBOUTPROT_UBX, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_USBOUTPROT_NMEA, 1);
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_USBOUTPROT_RTCM3X, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_USBINPROT_UBX, 1);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_USBINPROT_NMEA, 1);
  response &= i2cGNSS.sendCfgValset8(UBLOX_CFG_USBINPROT_RTCM3X, 1);

  //Enable the constellations the user has set
  response &= setConstellations(false); //Do not apply newCfg or sendCfg to value set

  if (response == false)
    Serial.println("Module failed config block 1");

  //Make sure the appropriate messages are enabled
  response &= enableMessages(); //Does a complete open/closed val set
  if (response == false)
    Serial.println("Module failed config block 2");

  //Disable NMEA messages on all but UART1
  response &= i2cGNSS.newCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_I2C, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_I2C, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_I2C, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GST_I2C, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_I2C, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_I2C, 0);

  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART2, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART2, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART2, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART2, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART2, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART2, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART2, 0);

  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_SPI, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_SPI, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_SPI, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_SPI, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GST_SPI, 0);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI, 0);
  response &= i2cGNSS.sendCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI, 0);

  if (response == false)
    Serial.println("Module failed config block 3");

  if (zedModuleType == PLATFORM_F9R)
    response &= i2cGNSS.setAutoESFSTATUS(true, false); //Tell the GPS to "send" each ESF Status, but do not update stale data when accessed

  //Turn on/off debug messages
  if (settings.enableI2Cdebug)
    i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
  else
    i2cGNSS.disableDebugging();

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
    while (millis() - splashStart < 2000) delay(1);
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
    Serial.printf("Batt (%d%%): Voltage: %0.02fV", battLevel, battVoltage);

    char tempStr[25];
    if (battChangeRate > 0)
      sprintf(tempStr, "C");
    else
      sprintf(tempStr, "Disc");
    Serial.printf(" %sharging: %0.02f%%/hr ", tempStr, battChangeRate);

    if (battLevel < 10)
      sprintf(tempStr, "Red");
    else if (battLevel < 50)
      sprintf(tempStr, "Yellow");
    else if (battLevel >= 50)
      sprintf(tempStr, "Green");
    else
      sprintf(tempStr, "No batt");

    Serial.printf("%s\r\n", tempStr);
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
  SdFile testFile;
  char testFileName[40] = "testfile.txt";

  //Attempt to write to the file system
  if (testFile.open(testFileName, O_CREAT | O_APPEND | O_WRITE) != true)
    return (false);

  //File successfully created
  testFile.close();
  if (sd->exists(testFileName))
    sd->remove(testFileName);
  return (true);
}

//If debug option is on, print available heap
void reportHeapNow()
{
  if (settings.enableHeapReport == true)
  {
    lastHeapReport = millis();
    Serial.printf("FreeHeap: %d / HeapLowestPoint: %d / LargestBlock: %d\r\n", ESP.getFreeHeap(), xPortGetMinimumEverFreeHeapSize(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
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

boolean SFE_UBLOX_GNSS_ADD::getModuleInfo(uint16_t maxWait)
{
  i2cGNSS.minfo.hwVersion[0] = 0;
  i2cGNSS.minfo.swVersion[0] = 0;
  for (int i = 0; i < 10; i++)
    i2cGNSS.minfo.extension[i][0] = 0;
  i2cGNSS.minfo.extensionNo = 0;

  // Let's create our custom packet
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

  // The next line creates and initialises the packet information which wraps around the payload
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_MON; // This is the message Class
  customCfg.id = UBX_MON_VER;    // This is the message ID
  customCfg.len = 0;             // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0;    // Always set the startingSpot to zero (unless you really know what you are doing)

  // Now let's send the command. The module info is returned in customPayload

  if (sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
    return (false); //If command send fails then bail

  // Now let's extract the module info from customPayload

  uint16_t position = 0;
  for (int i = 0; i < 30; i++)
  {
    minfo.swVersion[i] = customPayload[position];
    position++;
  }
  for (int i = 0; i < 10; i++)
  {
    minfo.hwVersion[i] = customPayload[position];
    position++;
  }

  while (customCfg.len >= position + 30)
  {
    for (int i = 0; i < 30; i++)
    {
      minfo.extension[minfo.extensionNo][i] = customPayload[position];
      position++;
    }
    minfo.extensionNo++;
    if (minfo.extensionNo > 9)
      break;
  }

  return (true); //Success!
}

//Create $GNTXT, type message complete with CRC
//https://www.nmea.org/Assets/20160520%20txt%20amendment.pdf
//Used for recording system events (boot reason, event triggers, etc) inside the log
void createNMEASentence(customNmeaType_e textID, char *nmeaMessage, char *textMessage)
{
  //Currently we don't have messages longer than 82 char max so we hardcode the sentence numbers
  const uint8_t totalNumberOfSentences = 1;
  const uint8_t sentenceNumber = 1;

  char nmeaTxt[200]; //Max NMEA sentence length is 82
  sprintf(nmeaTxt, "$GNTXT,%02d,%02d,%02d,%s*", totalNumberOfSentences, sentenceNumber, textID, textMessage);

  //From: http://engineeringnotes.blogspot.com/2015/02/generate-crc-for-nmea-strings-arduino.html
  byte CRC = 0; // XOR chars between '$' and '*'
  for (byte x = 1 ; x < strlen(nmeaTxt) - 1; x++)
    CRC = CRC ^ nmeaTxt[x];

  sprintf(nmeaMessage, "%s%02X", nmeaTxt, CRC);
}

//Reset settings struct to default initializers
void settingsToDefaults()
{
  settings = defaultSettings;
}

//Enable all the valid messages for this platform
//There are 73 messages so split in two batches, limited to 64 a batch
//Uses dummy newCfg and sendCfg values to be sure we open/close a complete set
bool enableMessages()
{
  bool response = true;

  response &= i2cGNSS.newCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI, 0); //Dummy opening value - #1
  for (int x = 0 ; x < 62 ; x++)
  {
    if (settings.ubxMessages[x].supported & zedModuleType)
      response &= i2cGNSS.addCfgValset8(settings.ubxMessages[x].msgConfigKey, settings.ubxMessages[x].msgRate);
  }
  response &= i2cGNSS.sendCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI, 0); //Dummy closing value - #64

  //Final 11 messages
  response &= i2cGNSS.newCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI, 0); //Dummy opening value - #1
  for (int x = 62 ; x < MAX_UBX_MSG ; x++)
  {
    if (settings.ubxMessages[x].supported & zedModuleType)
      response &= i2cGNSS.addCfgValset8(settings.ubxMessages[x].msgConfigKey, settings.ubxMessages[x].msgRate);
  }
  response &= i2cGNSS.sendCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI, 0); //Dummy closing value - #13

  return (response);
}

//Enable all the valid messages for this platform over the USB port
bool enableMessagesUSB()
{
  bool response = true;

  //TODO

  return (response);
}

//Enable all the valid constellations and bands for this platform
//Band support varies between platforms and firmware versions
//We open/close a complete set if sendCompleteBatch = true
bool setConstellations(bool sendCompleteBatch)
{
  bool response = true;

  if (sendCompleteBatch)
    response &= i2cGNSS.newCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI, 0); //Dummy opening value

  response &= i2cGNSS.addCfgValset8(settings.ubxConstellations[0].configKey, settings.ubxConstellations[0].enabled); //GPS
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GPS_L1CA_ENA, settings.ubxConstellations[0].enabled);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GPS_L2C_ENA, settings.ubxConstellations[0].enabled);

  //v1.12 ZED-F9P firmware does not allow for SBAS control
  //Also, if we can't identify the version (99), skip SBAS enable
  if (zedModuleType == PLATFORM_F9P && (zedFirmwareVersionInt == 112 || zedFirmwareVersionInt == 99))
  {
    //Skip
  }
  else
  {
    response &= i2cGNSS.addCfgValset8(settings.ubxConstellations[1].configKey, settings.ubxConstellations[1].enabled); //SBAS
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_SBAS_L1CA_ENA, settings.ubxConstellations[1].enabled);
  }

  response &= i2cGNSS.addCfgValset8(settings.ubxConstellations[2].configKey, settings.ubxConstellations[2].enabled); //GAL
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_E1_ENA, settings.ubxConstellations[2].enabled);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GAL_E5B_ENA, settings.ubxConstellations[2].enabled);

  response &= i2cGNSS.addCfgValset8(settings.ubxConstellations[3].configKey, settings.ubxConstellations[3].enabled); //BDS
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_B1_ENA, settings.ubxConstellations[3].enabled);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_BDS_B2_ENA, settings.ubxConstellations[3].enabled);

  response &= i2cGNSS.addCfgValset8(settings.ubxConstellations[4].configKey, settings.ubxConstellations[4].enabled); //QZSS
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_QZSS_L1CA_ENA, settings.ubxConstellations[4].enabled);

  //UBLOX_CFG_SIGNAL_QZSS_L1S_ENA not supported on F9R in v1.21 and below
  if (zedModuleType == PLATFORM_F9P)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_QZSS_L1S_ENA, settings.ubxConstellations[4].enabled);
  else if (zedModuleType == PLATFORM_F9R && zedFirmwareVersionInt > 121)
    response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_QZSS_L1S_ENA, settings.ubxConstellations[4].enabled);

  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_QZSS_L2C_ENA, settings.ubxConstellations[4].enabled);

  response &= i2cGNSS.addCfgValset8(settings.ubxConstellations[5].configKey, settings.ubxConstellations[5].enabled); //GLO
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_L1_ENA, settings.ubxConstellations[5].enabled);
  response &= i2cGNSS.addCfgValset8(UBLOX_CFG_SIGNAL_GLO_L2_ENA, settings.ubxConstellations[5].enabled); //53 keys

  if (sendCompleteBatch)
    response &= i2cGNSS.sendCfgValset8(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI, 0); //Dummy closing value

  return (response);
}
