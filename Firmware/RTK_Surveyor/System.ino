//Starting and restarting BT is a problem. See issue: https://github.com/espressif/arduino-esp32/issues/2718
//To work around the bug without modifying the core we create our own btStop() function with
//the patch from github
//bool customBTstop() {
//  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
//    return true;
//  }
//  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
//    if (esp_bt_controller_disable()) {
//      log_e("BT Disable failed");
//      return false;
//    }
//    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
//  }
//  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
//  {
//    log_i("inited");
//    if (esp_bt_controller_deinit())
//    {
//      log_e("BT deint failed");
//      return false;
//    }
//    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
//      ;
//    return true;
//  }
//  log_e("BT Stop failed");
//  return false;
//}


//Setup the u-blox module for any setup (base or rover)
//In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare occasion, become
//corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked.
bool configureUbloxModule()
{
  boolean response = true;
  int maxWait = 2000;

  i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  //The first thing we do is go to 1Hz to lighten any I2C traffic from a previous configuration
  if (i2cGNSS.getNavigationFrequency(maxWait) != 1)
    response &= i2cGNSS.setNavigationFrequency(1, maxWait);
  if (response == false)
    Serial.println(F("Set rate failed"));

  response = i2cGNSS.disableSurveyMode(); //Disable survey
  if (response == false)
    Serial.println(F("Disable Survey failed"));

#define OUTPUT_SETTING 14
#define INPUT_SETTING 12

  //UART1 will primarily be used to pass NMEA and UBX from ZED to ESP32 (eventually to cell phone)
  //but the phone can also provide RTCM data. So let's be sure to enable RTCM on UART1 input.
  //Protocol out = NMEA, protocol in = RTCM
  getPortSettings(COM_PORT_UART1); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_NMEA | COM_TYPE_UBX) || settingPayload[INPUT_SETTING] != COM_TYPE_RTCM3)
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_UART1, COM_TYPE_NMEA | COM_TYPE_UBX); //Set the UART1 to output NMEA and UBX
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

  //Turn on RTCM over I2C port so that we can harvest RTCM over I2C and send out over WiFi
  //This is easier than parsing over UART because the library handles the frame detection
  getPortSettings(COM_PORT_I2C); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3) || settingPayload[INPUT_SETTING] != COM_TYPE_UBX)
  {
    response &= i2cGNSS.setPortOutput(COM_PORT_I2C, COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3); //Set the I2C port to output UBX (config), NMEA (logging), and RTCM3 (casting)
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

  response &= enableMessages(COM_PORT_UART1, settings.broadcast); //Make sure the appropriate messages are enabled
  response &= enableMessages(COM_PORT_I2C, settings.log); //Make sure the appropriate messages are enabled

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
  }

  //Based on current settings, update the logging options within the GNSS library
  i2cGNSS.logRXMRAWX(settings.log.rawx);
  i2cGNSS.logRXMSFRBX(settings.log.sfrbx);

  if (logNMEAMessages() == true)
    i2cGNSS.setNMEALoggingMask(SFE_UBLOX_FILTER_NMEA_ALL); // Enable logging of all enabled NMEA messages
  else
    i2cGNSS.setNMEALoggingMask(0); // Disable logging of all enabled NMEA messages

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH)
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
    if (configureUbloxModuleBase() == false)
    {
      Serial.println(F("Base config failed!"));
      return (false);
    }
  }

  response &= i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  if (response == false)
    Serial.println(F("Module failed to save."));

  return (response);
}

//Enable the NMEA/UBX messages, based on given log or broadcast settings, for a given port
bool enableMessages(uint8_t portType, gnssMessages messageSetting)
{
  bool response = true;
  if (messageSetting.gga == true)
  {
    if (getNMEASettings(UBX_NMEA_GGA, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GGA, portType);
  }
  else if (messageSetting.gga == false)
  {
    if (getNMEASettings(UBX_NMEA_GGA, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GGA, portType);
  }

  if (messageSetting.gsa == true)
  {
    if (getNMEASettings(UBX_NMEA_GSA, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GSA, portType);
  }
  else if (messageSetting.gsa == false)
  {
    if (getNMEASettings(UBX_NMEA_GSA, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GSA, portType);
  }

  //When receiving 15+ satellite information, the GxGSV sentences can be a large amount of data
  //If the update rate is >1Hz then this data can overcome the BT capabilities causing timeouts and lag
  //So we set the GSV sentence to 1Hz regardless of update rate
  uint16_t measurementFrequency = (uint16_t)getMeasurementFrequency(); //Force to int
  if (messageSetting.gsv == true)
  {
    if (getNMEASettings(UBX_NMEA_GSV, portType) != measurementFrequency)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GSV, portType, measurementFrequency);
  }
  else if (messageSetting.gsv == false)
  {
    if (getNMEASettings(UBX_NMEA_GSV, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GSV, portType);
  }

  if (messageSetting.rmc == true)
  {
    if (getNMEASettings(UBX_NMEA_RMC, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_RMC, portType);
  }
  else if (messageSetting.rmc == false)
  {
    if (getNMEASettings(UBX_NMEA_RMC, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_RMC, portType);
  }

  if (messageSetting.gst == true)
  {
    if (getNMEASettings(UBX_NMEA_GST, portType) != 1)
      response &= i2cGNSS.enableNMEAMessage(UBX_NMEA_GST, portType);
  }
  else if (messageSetting.gst == false)
  {
    if (getNMEASettings(UBX_NMEA_GST, portType) != 0)
      response &= i2cGNSS.disableNMEAMessage(UBX_NMEA_GST, portType);
  }

  if (messageSetting.rawx == true)
  {
    if (getRAWXSettings(portType) != 1)
      response &= i2cGNSS.enableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, portType);
  }
  else if (messageSetting.rawx == false)
  {
    if (getRAWXSettings(portType) != 0)
      response &= i2cGNSS.disableMessage(UBX_CLASS_RXM, UBX_RXM_RAWX, portType);
  }

  if (messageSetting.sfrbx == true)
  {
    if (getSFRBXSettings(portType) != 1)
      response &= i2cGNSS.enableMessage(UBX_CLASS_RXM, UBX_RXM_SFRBX, portType);
  }
  else if (messageSetting.sfrbx == false)
  {
    if (getSFRBXSettings(portType) != 0)
      response &= i2cGNSS.disableMessage(UBX_CLASS_RXM, UBX_RXM_SFRBX, portType);
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

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

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

//Freeze displaying a given error code
void blinkError(t_errorNumber errorNumber)
{
  while (1)
  {
    for (int x = 0 ; x < errorNumber ; x++)
    {
      digitalWrite(positionAccuracyLED_1cm, HIGH);
      digitalWrite(positionAccuracyLED_10cm, HIGH);
      digitalWrite(positionAccuracyLED_100cm, HIGH);
      digitalWrite(baseStatusLED, HIGH);
      digitalWrite(bluetoothStatusLED, HIGH);
      delay(200);
      digitalWrite(positionAccuracyLED_1cm, LOW);
      digitalWrite(positionAccuracyLED_10cm, LOW);
      digitalWrite(positionAccuracyLED_100cm, LOW);
      digitalWrite(baseStatusLED, LOW);
      digitalWrite(bluetoothStatusLED, LOW);
      delay(200);
    }

    delay(2000);
  }
}

//Turn on indicator LEDs to verify LED function and indicate setup sucess
void danceLEDs()
{
  for (int x = 0 ; x < 2 ; x++)
  {
    digitalWrite(positionAccuracyLED_1cm, HIGH);
    digitalWrite(positionAccuracyLED_10cm, HIGH);
    digitalWrite(positionAccuracyLED_100cm, HIGH);
    digitalWrite(baseStatusLED, HIGH);
    digitalWrite(bluetoothStatusLED, HIGH);
    delay(100);
    digitalWrite(positionAccuracyLED_1cm, LOW);
    digitalWrite(positionAccuracyLED_10cm, LOW);
    digitalWrite(positionAccuracyLED_100cm, LOW);
    digitalWrite(baseStatusLED, LOW);
    digitalWrite(bluetoothStatusLED, LOW);
    delay(100);
  }

  digitalWrite(positionAccuracyLED_1cm, HIGH);
  digitalWrite(positionAccuracyLED_10cm, HIGH);
  digitalWrite(positionAccuracyLED_100cm, HIGH);
  digitalWrite(baseStatusLED, HIGH);
  digitalWrite(bluetoothStatusLED, HIGH);

  delay(250);
  digitalWrite(positionAccuracyLED_1cm, LOW);
  delay(250);
  digitalWrite(positionAccuracyLED_10cm, LOW);
  delay(250);
  digitalWrite(positionAccuracyLED_100cm, LOW);

  delay(250);
  digitalWrite(baseStatusLED, LOW);
  delay(250);
  digitalWrite(bluetoothStatusLED, LOW);
}

//Get the confirmed current date
//bool getConfirmedDate(uint16_t maxWait)
//{
//  if (i2cGNSS.packetUBXNAVPVT == NULL) i2cGNSS.initPacketUBXNAVPVT(); //Check that RAM has been allocated for the PVT data
//  if (i2cGNSS.packetUBXNAVPVT == NULL) //Bail if the RAM allocation failed
//    return (false);
//
////  if (packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.confirmedDate == false)
////    getPVT(maxWait);
////  packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.confirmedDate = false; //Since we are about to give this to user, mark this data as stale
////  packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.all = false;
////  return ((bool)packetUBXNAVPVT->data.flags2.bits.confirmedDate);
//return(true);
//}


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

//Update Battery level LEDs every 5s
void updateBattLEDs()
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
  battLevel = lipo.getSOC();
  battVoltage = lipo.getVoltage();
  battChangeRate = lipo.getChangeRate();

  Serial.printf("Batt (%d%%): Voltage: %0.02fV", battLevel, battVoltage);

  char tempStr[25];
  if (battChangeRate > 0)
    sprintf(tempStr, "C");
  else
    sprintf(tempStr, "Disc");
  Serial.printf(" %sharging: %0.02f%%/hr ", tempStr, battChangeRate);

  if (battLevel < 10)
  {
    sprintf(tempStr, "RED uh oh!");
    ledcWrite(ledRedChannel, 255);
    ledcWrite(ledGreenChannel, 0);
  }
  else if (battLevel < 50)
  {
    sprintf(tempStr, "Yellow ok");
    ledcWrite(ledRedChannel, 128);
    ledcWrite(ledGreenChannel, 128);
  }
  else if (battLevel >= 50)
  {
    sprintf(tempStr, "Green all good");
    ledcWrite(ledRedChannel, 0);
    ledcWrite(ledGreenChannel, 255);
  }
  else
  {
    sprintf(tempStr, "No batt");
    ledcWrite(ledRedChannel, 10);
    ledcWrite(ledGreenChannel, 0);
  }

  Serial.printf("%s\n", tempStr);
}

//Ping an I2C device and see if it responds
bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

//Given text, a position, and kerning, print text to display
//This is helpful for squishing or stretching a string to appropriately fill the display
void printTextwithKerning(char *newText, uint8_t xPos, uint8_t yPos, uint8_t kerning)
{
  for (int x = 0 ; x < strlen(newText) ; x++)
  {
    oled.setCursor(xPos, yPos);
    oled.print(newText[x]);
    xPos += kerning;
  }
}
//Create a test file in file structure to make sure we can
bool createTestFile()
{
  SdFile testFile;
  char testFileName[40] = "testfile.txt";

  if (testFile.open(testFileName, O_CREAT | O_APPEND | O_WRITE) == true)
  {
    testFile.close();

    if (sd.exists(testFileName))
      sd.remove(testFileName);
    return (true);
  }

  return (false);
}

//Returns true if any messages are enabled for logging
bool logMessages()
{
  if (logNMEAMessages())
    return (true);
  if (logUBXMessages())
    return (true);
  return (false);
}

//Returns true if any of the NMEA messages are enabled for logging
bool logNMEAMessages()
{
  if (settings.log.gga == true || settings.log.gsa == true || settings.log.gsv == true || settings.log.rmc == true || settings.log.gst == true)
    return (true);
  return (false);
}

//Returns true if any of the UBX messages are enabled for logging
bool logUBXMessages()
{
  if (settings.log.rawx == true || settings.log.sfrbx == true)
    return (true);
  return (false);
}
