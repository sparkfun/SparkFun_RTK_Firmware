//If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
//Task for writing to the GNSS receiver
void F9PSerialWriteTask(void *e)
{
  while (true)
  {
    //Receive corrections from either the ESP32 USB or bluetooth
    //and write to the GPS
    //    if (Serial.available())
    //    {
    //      auto s = Serial.readBytes(wBuffer, SERIAL_SIZE_RX);
    //      serialGNSS.write(wBuffer, s);
    //    }

    if (SerialBT.available())
    {
      while (SerialBT.available())
      {
        if (inTestMode == false)
        {
          //Pass bytes tp GNSS receiver
          auto s = SerialBT.readBytes(wBuffer, SERIAL_SIZE_RX);
          serialGNSS.write(wBuffer, s);
        }
        else
        {
          Serial.printf("I heard: %c\n", SerialBT.read());
        }
      }
    }

    taskYIELD();
  }
}

//If the ZED has any new NMEA data, pass it out over Bluetooth
//Task for reading data from the GNSS receiver.
void F9PSerialReadTask(void *e)
{
  while (true)
  {
    if (serialGNSS.available())
    {
      auto s = serialGNSS.readBytes(rBuffer, SERIAL_SIZE_RX);

      //If we are actively survey-in then do not pass NMEA data from ZED to phone
      if (baseState == BASE_SURVEYING_IN_SLOW || baseState == BASE_SURVEYING_IN_FAST)
      {
        //Do nothing
      }
      else if (SerialBT.connected())
      {
        SerialBT.write(rBuffer, s);
      }

      //If user wants to log, record to SD
      if (settings.logNMEA == true)
      {
        if (online.nmeaLogging == true)
        {
          //Check if we are inside the max time window for logging
          if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
          {
            taskYIELD();
            nmeaFile.write(rBuffer, s);
            taskYIELD();

            //Force sync every 500ms
            if (millis() - lastDataLogSyncTime > 500)
            {
              lastDataLogSyncTime = millis();

              taskYIELD();
              nmeaFile.sync();
              taskYIELD();

              if (settings.frequentFileAccessTimestamps == true)
                updateDataFileAccess(&nmeaFile); // Update the file access time & date
            }
          }
        }
      }
    }
    taskYIELD();
  }
}

//Call back for when BT connection event happens (connected/disconnect)
//Used for updating the radioState state machine
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println(F("Client Connected"));
    radioState = BT_CONNECTED;
    digitalWrite(bluetoothStatusLED, HIGH);
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println(F("Client disconnected"));
    radioState = BT_ON_NOCONNECTION;
    digitalWrite(bluetoothStatusLED, LOW);
  }
}

//Setup the u-blox module for any setup (base or rover)
//In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare occasion, become
//corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked. We should only have to configure
//a fresh u-blox module once and never again.
bool configureUbloxModule()
{
  boolean response = true;

#define OUTPUT_SETTING 14
#define INPUT_SETTING 12

  //UART1 will primarily be used to pass NMEA from ZED to ESP32 (eventually to cell phone)
  //but the phone can also provide RTCM data. So let's be sure to enable RTCM on UART1 input.
  //Protocol out = NMEA, protocol in = RTCM
  getPortSettings(COM_PORT_UART1); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_NMEA) || settingPayload[INPUT_SETTING] != COM_TYPE_RTCM3)
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_UART1, COM_TYPE_NMEA); //Set the UART1 to output NMEA
    response &= i2cGNSS.setPortInput(COM_PORT_UART1, COM_TYPE_RTCM3); //Set the UART1 to input RTCM
  }

  //Disable SPI port - This is just to remove some overhead by ZED
  getPortSettings(COM_PORT_SPI); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != 0 || settingPayload[INPUT_SETTING] != 0)
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_SPI, 0); //Disable all protocols
    response &= i2cGNSS.setPortInput(COM_PORT_SPI, 0); //Disable all protocols
  }

  getPortSettings(COM_PORT_UART2); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != COM_TYPE_RTCM3 || settingPayload[INPUT_SETTING] != COM_TYPE_RTCM3)
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to output RTCM (in case this device goes into base mode)
    response &= i2cGNSS.setPortInput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to input RTCM
  }

  getPortSettings(COM_PORT_I2C); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != COM_TYPE_UBX || settingPayload[INPUT_SETTING] != COM_TYPE_UBX)
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_I2C, COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
    response &= i2cGNSS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX); //Set the I2C port to input UBX only
  }

  //The USB port on the ZED may be used for RTCM to/from the computer (as an NTRIP caster or client)
  //So let's be sure all protocols are on for the USB port
  getPortSettings(COM_PORT_USB); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3) || settingPayload[INPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3))
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_USB, (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3)); //Set the USB port to everything
    response &= i2cGNSS.setPortInput(COM_PORT_USB, (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3)); //Set the USB port to everything
  }

  response &= enableNMEASentences(COM_PORT_UART1); //Make sure the appropriate NMEA sentences are enabled

  response &= enableRXMSentences(COM_PORT_I2C); //Make sure the appropriate RXM (RAXW and SFXBR) are enabled

  response &= i2cGNSS.setAutoPVT(true, false); //Tell the GPS to "send" each solution, but do not update stale data when accessed
  response &= i2cGNSS.setAutoHPPOSLLH(true, false); //Tell the GPS to "send" each high res solution, but do not update stale data when accessed

  if (getSerialRate(COM_PORT_UART1) != settings.dataPortBaud)
  {
    Serial.println(F("Updating UART1 rate"));
    i2cGNSS.setSerialRate(settings.dataPortBaud, COM_PORT_UART1); //Set UART1 to 115200
  }
  if (getSerialRate(COM_PORT_UART2) != settings.radioPortBaud)
  {
    Serial.println(F("Updating UART2 rate"));
    i2cGNSS.setSerialRate(settings.radioPortBaud, COM_PORT_UART2); //Set UART2 to 57600 to match SiK telemetry radio firmware default
  }

  if (response == false)
  {
    Serial.println(F("Module failed initial config."));
    return (false);
  }

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  //  if (digitalRead(baseSwitch) == HIGH)
  if (1)
  {
    //Configure for rover mode
    if (configureUbloxModuleRover() == false)
    {
      Serial.println(F("Rover config failed!"));
      return (false);
    }
  }
  else
  {
    //Configure for base mode
    //    if (configureUbloxModuleBase() == false)
    //    {
    //      Serial.println(F("Base config failed!"));
    //      return (false);
    //    }
  }

  response &= i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  if (response == false)
    Serial.println(F("Module failed to save."));

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
    Serial.println(F("getPortSettings failed!"));
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
    Serial.println(F("getNMEASettings failed!"));
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

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_RTCM_MSB;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getRTCMSettings failed!"));
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
    Serial.println(F("getSerialRate failed!"));
    return (false);
  }

  return (((uint32_t)settingPayload[10] << 16) | ((uint32_t)settingPayload[9] << 8) | settingPayload[8]);
}

//Enable the RAWX and SFRBX message based on user's settings, on a given com port
bool enableRXMSentences(uint8_t portType)
{
  bool response = true;

  if (settings.logRAWX == true)
  {
    if (getRAWXSettings(portType) != 1)
      response &= i2cGNSS.enableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, portType);
  }
  else if (settings.logRAWX == false)
  {
    if (getRAWXSettings(portType) != 0)
      response &= i2cGNSS.disableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, portType);
  }

  if (settings.logSFRBX == true)
  {
    if (getSFRBXSettings(portType) != 1)
      response &= i2cGNSS.enableMessage(UBX_CLASS_RXM, UBX_RXM_SFRBX, portType);
  }
  else if (settings.logSFRBX == false)
  {
    if (getSFRBXSettings(portType) != 0)
      response &= i2cGNSS.disableMessage(UBX_CLASS_RXM, UBX_RXM_SFRBX, portType);
  }

  return (response);
}

//Enable the NMEA messages, based on user's settings, on a given com port
bool enableNMEASentences(uint8_t portType)
{
  bool response = true;
  if (settings.outputSentenceGGA == true)
  {
    if (getNMEASettings(UBX_NMEA_GGA, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, portType);
  }
  else if (settings.outputSentenceGGA == false)
  {
    if (getNMEASettings(UBX_NMEA_GGA, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GGA, portType);
  }

  if (settings.outputSentenceGSA == true)
  {
    if (getNMEASettings(UBX_NMEA_GSA, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GSA, portType);
  }
  else if (settings.outputSentenceGSA == false)
  {
    if (getNMEASettings(UBX_NMEA_GSA, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GSA, portType);
  }

  //When receiving 15+ satellite information, the GxGSV sentences can be a large amount of data
  //If the update rate is >1Hz then this data can overcome the BT capabilities causing timeouts and lag
  //So we set the GSV sentence to 1Hz regardless of update rate
  if (settings.outputSentenceGSV == true)
  {
    if (getNMEASettings(UBX_NMEA_GSV, portType) != settings.gnssMeasurementFrequency)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GSV, portType, settings.gnssMeasurementFrequency);
  }
  else if (settings.outputSentenceGSV == false)
  {
    if (getNMEASettings(UBX_NMEA_GSV, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GSV, portType);
  }

  if (settings.outputSentenceRMC == true)
  {
    if (getNMEASettings(UBX_NMEA_RMC, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_RMC, portType);
  }
  else if (settings.outputSentenceRMC == false)
  {
    if (getNMEASettings(UBX_NMEA_RMC, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_RMC, portType);
  }

  if (settings.outputSentenceGST == true)
  {
    if (getNMEASettings(UBX_NMEA_GST, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GST, portType);
  }
  else if (settings.outputSentenceGST == false)
  {
    if (getNMEASettings(UBX_NMEA_GST, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GST, portType);
  }

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
bool enableRTCMSentences(uint8_t portType)
{
  bool response = true;
  if (getRTCMSettings(UBX_RTCM_1005, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1005, portType, 1); //Enable message 1005 to output through UART2, message every second
  if (getRTCMSettings(UBX_RTCM_1074, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1074, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1084, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1084, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1094, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1094, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1124, portType) != 1)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1124, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1230, portType) != 10)
    response &= i2cGNSS.enableRTCMmessage(UBX_RTCM_1230, portType, 10); //Enable message every 10 seconds

  return (response);
}

//Disable RTCM sentences for a given com port
bool disableRTCMSentences(uint8_t portType)
{
  bool response = true;
  if (getRTCMSettings(UBX_RTCM_1005, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1005, portType);
  if (getRTCMSettings(UBX_RTCM_1074, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1074, portType);
  if (getRTCMSettings(UBX_RTCM_1084, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1084, portType);
  if (getRTCMSettings(UBX_RTCM_1094, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1094, portType);
  if (getRTCMSettings(UBX_RTCM_1124, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1124, portType);
  if (getRTCMSettings(UBX_RTCM_1230, portType) != 0)
    response &= i2cGNSS.disableRTCMmessage(UBX_RTCM_1230, portType);
  return (response);
}
