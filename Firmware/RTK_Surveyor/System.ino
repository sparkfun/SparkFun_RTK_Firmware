//Setup the u-blox module for any setup (base or rover)
//In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare occasion, become
//corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked.
bool configureUbloxModule()
{
  if (online.gnss == false) return (false);

  boolean response = true;

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
  if (i2cGNSS.getNavigationFrequency(maxWait) != 1)
    response &= i2cGNSS.setNavigationFrequency(1, maxWait);
  if (response == false)
    Serial.println("Set rate failed");

  //Survey mode is only available on ZED-F9P modules
  if (zedModuleType == PLATFORM_F9P)
  {
    response = i2cGNSS.setSurveyMode(0, 0, 0); //Disable Survey-In or Fixed Mode
    if (response == false)
      Serial.println("Disable TMODE3 failed");
  }

#define OUTPUT_SETTING 14
#define INPUT_SETTING 12

  //UART1 will primarily be used to pass NMEA and UBX from ZED to ESP32 (eventually to cell phone)
  //but the phone can also provide RTCM data and a user may want to configure the ZED over Bluetooth.
  //So let's be sure to enable UBX+NMEA+RTCM on the input
  getPortSettings(COM_PORT_UART1); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_NMEA | COM_TYPE_UBX | COM_TYPE_RTCM3))
    response &= i2cGNSS.setPortOutput(COM_PORT_UART1, COM_TYPE_NMEA | COM_TYPE_UBX | COM_TYPE_RTCM3); //Set the UART1 to output UBX+NMEA+RTCM

  if (settingPayload[INPUT_SETTING] != (COM_TYPE_NMEA | COM_TYPE_UBX | COM_TYPE_RTCM3))
    response &= i2cGNSS.setPortInput(COM_PORT_UART1, COM_TYPE_NMEA | COM_TYPE_UBX | COM_TYPE_RTCM3); //Set the UART1 to input UBX+NMEA+RTCM

  //Disable SPI port - This is just to remove some overhead by ZED
  getPortSettings(COM_PORT_SPI); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != 0)
    response &= i2cGNSS.setPortOutput(COM_PORT_SPI, 0); //Disable all protocols
  if (settingPayload[INPUT_SETTING] != 0)
    response &= i2cGNSS.setPortInput(COM_PORT_SPI, 0); //Disable all protocols

  getPortSettings(COM_PORT_UART2); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != COM_TYPE_RTCM3)
    response &= i2cGNSS.setPortOutput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to output RTCM (in case this device goes into base mode)
  if (settingPayload[INPUT_SETTING] != COM_TYPE_RTCM3)
    response &= i2cGNSS.setPortInput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to input RTCM

  if (settingPayload[INPUT_SETTING] != COM_TYPE_UBX)
    response &= i2cGNSS.setPortInput(COM_PORT_I2C, (COM_TYPE_NMEA | COM_TYPE_UBX | COM_TYPE_RTCM3)); //We don't want NMEA, but we will want to deliver RTCM over I2C

  //The USB port on the ZED may be used for RTCM to/from the computer (as an NTRIP caster or client)
  //So let's be sure all protocols are on for the USB port
  getPortSettings(COM_PORT_USB); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3))
    response &= i2cGNSS.setPortOutput(COM_PORT_USB, (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3)); //Set the USB port to everything

  if (settingPayload[INPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3))
    response &= i2cGNSS.setPortInput(COM_PORT_USB, (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3)); //Set the USB port to everything

  response &= configureConstellations(); //Enable the constellations the user has set

  response &= configureGNSSMessageRates(COM_PORT_UART1, settings.ubxMessages); //Make sure the appropriate messages are enabled

  response &= disableNMEASentences(COM_PORT_I2C); //Disable NMEA messages on all but UART1
  response &= disableNMEASentences(COM_PORT_UART2);
  response &= disableNMEASentences(COM_PORT_SPI);

  if (zedModuleType == PLATFORM_F9R)
    response &= i2cGNSS.setAutoESFSTATUS(true, false); //Tell the GPS to "send" each ESF Status, but do not update stale data when accessed

  if (getSerialRate(COM_PORT_UART1) != settings.dataPortBaud)
  {
    Serial.println("Updating UART1 rate");
    i2cGNSS.setSerialRate(settings.dataPortBaud, COM_PORT_UART1); //Defaults to 460800 to maximize message output support
  }
  if (getSerialRate(COM_PORT_UART2) != settings.radioPortBaud)
  {
    Serial.println("Updating UART2 rate");
    i2cGNSS.setSerialRate(settings.radioPortBaud, COM_PORT_UART2); //Defaults to 57600 to match SiK telemetry radio firmware default
  }

  if (response == false)
  {
    Serial.println("Module failed initial config.");
  }

  response &= i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  if (response == false)
    Serial.println("Module failed to save.");

  //Turn on/off debug messages
  if (settings.enableI2Cdebug)
    i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
  else
    i2cGNSS.disableDebugging();

  return (response);
}

//Disable all the NMEA sentences on a given com port
bool disableNMEASentences(uint8_t portType)
{
  bool response = true;
  if (getNMEASettings(UBX_NMEA_GGA, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GGA, portType);
  if (getNMEASettings(UBX_NMEA_GSA, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GSA, portType);
  if (getNMEASettings(UBX_NMEA_GSV, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GSV, portType);
  if (getNMEASettings(UBX_NMEA_RMC, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_RMC, portType);
  if (getNMEASettings(UBX_NMEA_GST, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GST, portType);
  if (getNMEASettings(UBX_NMEA_GLL, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GLL, portType);
  if (getNMEASettings(UBX_NMEA_VTG, portType) != 0)
    response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_VTG, portType);

  return (response);
}

//Enable RTCM sentences for a given com port
//This function is needed when switching from rover to base
//We over-ride user settings in base mode
bool enableRTCMSentences(uint8_t portType)
{
  bool response = true;
  int maxWait = 1000; //When I2C traffic is large during logging, give extra time

  if (getRTCMSettings(UBX_RTCM_1005, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1005, portType, 1, maxWait); //Enable message 1005 to output through UART2, message every second
  if (getRTCMSettings(UBX_RTCM_1074, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1074, portType, 1, maxWait);
  if (getRTCMSettings(UBX_RTCM_1084, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1084, portType, 1, maxWait);
  if (getRTCMSettings(UBX_RTCM_1094, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1094, portType, 1, maxWait);
  if (getRTCMSettings(UBX_RTCM_1124, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1124, portType, 1, maxWait);
  if (getRTCMSettings(UBX_RTCM_1230, portType) != 10)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1230, portType, 10, maxWait); //Enable message every 10 seconds

  return (response);
}

//Disable RTCM sentences for a given com port
//This function is needed when switching from base to rover
//It's used for turning off RTCM on USB, UART2, and I2C ports.
//UART1 should be re-enabled with user settings using the configureGNSSMessageRates() function
bool disableRTCMSentences(uint8_t portType)
{
  bool response = true;
  int maxWait = 1000; //When I2C traffic is large during logging, give extra time

  if (getRTCMSettings(UBX_RTCM_1005, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1005, portType, maxWait);
  if (getRTCMSettings(UBX_RTCM_1074, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1074, portType, maxWait);
  if (getRTCMSettings(UBX_RTCM_1084, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1084, portType, maxWait);
  if (getRTCMSettings(UBX_RTCM_1094, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1094, portType, maxWait);
  if (getRTCMSettings(UBX_RTCM_1124, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1124, portType, maxWait);
  if (getRTCMSettings(UBX_RTCM_1230, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1230, portType, maxWait);
  return (response);
}

//Given a portID, load the settings associated
bool getPortSettings(uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_PRT; // This is the message ID
  customCfg.len = 1;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = portID; //Request the caller's portID from GPS module

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("getPortSettings failed");
    return (false);
  }

  return (true);
}

//Given a portID and a NMEA message type, load the settings associated
uint8_t getNMEASettings(uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_CLASS_NMEA;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("getNMEASettings failed");
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}

//Given a portID and a RTCM message type, load the settings associated
uint8_t getRTCMSettings(uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_RTCM_MSB;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("getRTCMSettings failed");
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}

//Given a portID and a NMEA message type, load the settings associated
uint32_t getSerialRate(uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_PRT; // This is the message ID
  customCfg.len = 1;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = portID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println("getSerialRate failed");
    return (false);
  }

  return (((uint32_t)settingPayload[10] << 16) | ((uint32_t)settingPayload[9] << 8) | settingPayload[8]);
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
