//Get MAC, start radio
//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool startBluetooth()
{
  //Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  if (buttonPreviousState == BUTTON_ROVER)
    sprintf(deviceName, "Surveyor Rover-%02X%02X", unitMACAddress[4], unitMACAddress[5]); //Rover mode
  else
    sprintf(deviceName, "Surveyor Base-%02X%02X", unitMACAddress[4], unitMACAddress[5]); //Base mode

  if (SerialBT.begin(deviceName) == false)
  {
    Serial.println(F("An error occurred initializing Bluetooth"));
    radioState = RADIO_OFF;

    if (productVariant == RTK_SURVEYOR)
      digitalWrite(pin_bluetoothStatusLED, LOW);
    return (false);
  }

  //Set PIN to 1234 so we can connect to older BT devices, but not require a PIN for modern device pairing
  //See issue: https://github.com/sparkfun/SparkFun_RTK_Surveyor/issues/5
  //https://github.com/espressif/esp-idf/issues/1541
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;

  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE; //Requires pin 1234 on old BT dongle, No prompt on new BT dongle
  //esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_OUT; //Works but prompts for either pin (old) or 'Does this 6 pin appear on the device?' (new)

  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
  esp_bt_pin_code_t pin_code;
  pin_code[0] = '1';
  pin_code[1] = '2';
  pin_code[2] = '3';
  pin_code[3] = '4';
  esp_bt_gap_set_pin(pin_type, 4, pin_code);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  SerialBT.register_callback(btCallback);
  SerialBT.setTimeout(250);

  Serial.print(F("Bluetooth broadcasting as: "));
  Serial.println(deviceName);

  radioState = BT_ON_NOCONNECTION;

  if (productVariant == RTK_SURVEYOR)
    digitalWrite(pin_bluetoothStatusLED, HIGH);

  //Start the tasks for handling incoming and outgoing BT bytes to/from ZED-F9P
  if (F9PSerialReadTaskHandle == NULL)
    xTaskCreate(
      F9PSerialReadTask,
      "F9Read", //Just for humans
      readTaskStackSize, //Stack Size
      NULL, //Task input parameter
      0, //Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
      &F9PSerialReadTaskHandle); //Task handle

  if (F9PSerialWriteTaskHandle == NULL)
    xTaskCreate(
      F9PSerialWriteTask,
      "F9Write", //Just for humans
      writeTaskStackSize, //Stack Size
      NULL, //Task input parameter
      0, //Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
      &F9PSerialWriteTaskHandle); //Task handle

  //Start task for controlling Bluetooth pair LED
  if (productVariant == RTK_SURVEYOR)
    btLEDTask.attach(btLEDTaskPace, updateBTled); //Rate in seconds, callback

  return (true);
}

//This function stops BT so that it can be restarted later
//It also releases as much system resources as possible so that WiFi/caster is more stable
void endBluetooth()
{
  //Delete tasks if running
  if (F9PSerialReadTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialReadTaskHandle);
    F9PSerialReadTaskHandle = NULL;
  }
  if (F9PSerialWriteTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialWriteTaskHandle);
    F9PSerialWriteTaskHandle = NULL;
  }

  SerialBT.flush(); //Complete any transfers
  SerialBT.disconnect(); //Drop any clients
  SerialBT.end(); //SerialBT.end() will release significant RAM (~100k!) but a SerialBT.start will crash.

  //The following code releases the BT hardware so that it can be restarted with a SerialBT.begin
  customBTstop();
  Serial.println(F("Bluetooth turned off"));

  radioState = RADIO_OFF;
}

//Starting and restarting BT is a problem. See issue: https://github.com/espressif/arduino-esp32/issues/2718
//To work around the bug without modifying the core we create our own btStop() function with
//the patch from github
bool customBTstop() {

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
    return true;
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    if (esp_bt_controller_disable()) {
      log_e("BT Disable failed");
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
  {
    log_i("inited");
    if (esp_bt_controller_deinit())
    {
      log_e("BT deint failed");
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
      ;
    return true;
  }
  log_e("BT Stop failed");
  return false;
}

//Start WiFi assuming it was previously fully released
//See WiFiBluetoothSwitch sketch for more info
void startWiFi()
{
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_init_config); //Restart WiFi resources

  Serial.printf("Connecting to local WiFi: %s\n\r", settings.wifiSSID);
  WiFi.begin(settings.wifiSSID, settings.wifiPW);

  radioState = WIFI_ON_NOCONNECTION;
}

//Stop WiFi and release all resources
//See WiFiBluetoothSwitch sketch for more info
void stopWiFi()
{
  caster.stop();
  WiFi.mode(WIFI_OFF);
  esp_wifi_deinit(); //Free all resources
  Serial.println("WiFi Stopped");

  radioState = RADIO_OFF;
}

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

  response = i2cGNSS.disableSurveyMode(maxWait); //Disable survey
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
  if (settings.log.rawx == true)
  {
    i2cGNSS.setAutoRXMRAWX(true, false); // Enable automatic RXM RAWX messages: without callback; without implicit update
    i2cGNSS.logRXMRAWX(true);
  }
  else
  {
    i2cGNSS.setAutoRXMRAWX(false); // Disable automatic RXM RAWX messages
    i2cGNSS.logRXMRAWX(false);
  }

  if (settings.log.sfrbx == true)
  {
    i2cGNSS.setAutoRXMSFRBX(true, false); // Enable automatic RXM SFRBX messages: without callback; without implicit update
    i2cGNSS.logRXMSFRBX(true);
  }
  else
  {
    i2cGNSS.setAutoRXMSFRBX(false); // Disable automatic RXM SFRBX messages
    i2cGNSS.logRXMSFRBX(false);
  }

  if (logNMEAMessages() == true)
    i2cGNSS.setNMEALoggingMask(SFE_UBLOX_FILTER_NMEA_ALL); // Enable logging of all enabled NMEA messages
  else
    i2cGNSS.setNMEALoggingMask(0); // Disable logging of all enabled NMEA messages

  response &= i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  if (response == false)
    Serial.println(F("Module failed to save."));

  //Turn on/off debug messages
  if (settings.enableI2Cdebug)
    i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
  else
    i2cGNSS.disableDebugging();

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
    Serial.println(F("getPortSettings failed"));
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
    Serial.println(F("getNMEASettings failed"));
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
    Serial.println(F("getRTCMSettings failed"));
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
    Serial.println(F("getSerialRate failed"));
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
      if (productVariant == RTK_SURVEYOR)
      {
        digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
        digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
        digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
        digitalWrite(pin_baseStatusLED, HIGH);
        digitalWrite(pin_bluetoothStatusLED, HIGH);
        delay(200);
        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
        digitalWrite(pin_positionAccuracyLED_100cm, LOW);
        digitalWrite(pin_baseStatusLED, LOW);
        digitalWrite(pin_bluetoothStatusLED, LOW);
        delay(200);
      }
    }

    delay(2000);
  }
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
    if (productVariant == RTK_SURVEYOR)
      digitalWrite(pin_bluetoothStatusLED, HIGH);
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println(F("Client disconnected"));
    radioState = BT_ON_NOCONNECTION;
    if (productVariant == RTK_SURVEYOR)
      digitalWrite(pin_bluetoothStatusLED, LOW);
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
    sprintf(tempStr, "Red");
  else if (battLevel < 50)
    sprintf(tempStr, "Yellow");
  else if (battLevel >= 50)
    sprintf(tempStr, "Green");
  else
    sprintf(tempStr, "No batt");

  Serial.printf("%s\n\r", tempStr);

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

//If debug option is on, print available heap
void reportHeap()
{
  if (settings.enableHeapReport == true)
  {
    if (millis() - lastHeapReport > 1000)
    {
      lastHeapReport = millis();
      Serial.printf("freeHeap: %d\n\r", ESP.getFreeHeap());
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
  if (productVariant == RTK_EXPRESS)
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
