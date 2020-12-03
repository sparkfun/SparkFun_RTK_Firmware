
uint8_t settingPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

//Setup the Ublox module for any setup (base or rover)
//In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare occasion, become
//corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked. We should only have to configure
//a fresh Ublox module once and never again.
bool configureUbloxModule()
{
  boolean response = true;

#define OUTPUT_SETTING 14
#define INPUT_SETTING 12

  //UART1 will primarily be used to pass NMEA from ZED to ESP32/Cell phone but the phone
  //can also provide RTCM data. So let's be sure to enable RTCM on UART1 input.
  getPortSettings(COM_PORT_UART1); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != COM_TYPE_NMEA || settingPayload[INPUT_SETTING] != COM_TYPE_RTCM3)
  {
    Serial.println("Updating UART1 configuration");
    response &= myGPS.setPortOutput(COM_PORT_UART1, COM_TYPE_NMEA); //Set the UART1 to output NMEA
    response &= myGPS.setPortInput(COM_PORT_UART1, COM_TYPE_RTCM3); //Set the UART1 to input RTCM
  }

  //Disable SPI port - This is just to remove some overhead by ZED
  getPortSettings(COM_PORT_SPI); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != 0 || settingPayload[INPUT_SETTING] != 0)
  {
    Serial.println("Updating SPI configuration");
    response &= myGPS.setPortOutput(COM_PORT_SPI, 0); //Disable all protocols
    response &= myGPS.setPortInput(COM_PORT_SPI, 0); //Disable all protocols
  }

  getPortSettings(COM_PORT_UART2); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != COM_TYPE_RTCM3 || settingPayload[INPUT_SETTING] != COM_TYPE_RTCM3)
  {
    response &= myGPS.setPortOutput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to output RTCM (in case this device goes into base mode)
    response &= myGPS.setPortInput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to input RTCM
  }

  getPortSettings(COM_PORT_I2C); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != COM_TYPE_UBX || settingPayload[INPUT_SETTING] != COM_TYPE_UBX)
  {
    response &= myGPS.setPortOutput(COM_PORT_I2C, COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
    response &= myGPS.setPortInput(COM_PORT_I2C, COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  }

  //The USB port on the ZED may be used for RTCM to/from the computer (as an NTRIP caster or client)
  //So let's be sure all protocols are on for the USB port
  getPortSettings(COM_PORT_USB); //Load the settingPayload with this port's settings
  if (settingPayload[OUTPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3) || settingPayload[INPUT_SETTING] != (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3))
  {
    response &= myGPS.setPortOutput(COM_PORT_USB, (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3)); //Set the USB port to everything
    response &= myGPS.setPortInput(COM_PORT_USB, (COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3)); //Set the USB port to everything
  }


  //myGPS.setNavigationFrequency(10); //Set output to 10 times a second
  if (myGPS.getNavigationFrequency() != 4)
  {
    response &= myGPS.setNavigationFrequency(4); //Set output in Hz
  }

  //Make sure the appropriate NMEA sentences are enabled
  if (getNMEASettings(UBX_NMEA_GGA, COM_PORT_UART1) != 1)
    response &= myGPS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1);
  if (getNMEASettings(UBX_NMEA_GSA, COM_PORT_UART1) != 1)
    response &= myGPS.enableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1);
  if (getNMEASettings(UBX_NMEA_GSV, COM_PORT_UART1) != 1)
    response &= myGPS.enableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1);
  if (getNMEASettings(UBX_NMEA_RMC, COM_PORT_UART1) != 1)
    response &= myGPS.enableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1);
  if (getNMEASettings(UBX_NMEA_GST, COM_PORT_UART1) != 1)
    response &= myGPS.enableNMEAMessage(UBX_NMEA_GST, COM_PORT_UART1);

  //response &= myGPS.setAutoPVT(true); //Tell the GPS to "send" each solution
  response &= myGPS.setAutoPVT(false); //Turn off PVT

  if (getSerialRate(COM_PORT_UART1) != 115200)
  {
    Serial.println("Updating UART1 rate");
    myGPS.setSerialRate(115200, COM_PORT_UART1); //Set UART1 to 115200
  }
  if (getSerialRate(COM_PORT_UART2) != 57600)
  {
    Serial.println("Updating UART2 rate");
    myGPS.setSerialRate(57600, COM_PORT_UART2); //Set UART2 to 57600 to match SiK firmware default
  }

  if (response == false)
  {
    Serial.println(F("Module failed initial config."));
    return (false);
  }

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH)
  {
    //Configure for rover mode
    if (configureUbloxModuleRover() == false)
    {
      Serial.println("Rover config failed!");
      return (false);
    }
  }
  else
  {
    //Configure for base mode
    if (configureUbloxModuleBase() == false)
    {
      Serial.println("Base config failed!");
      return (false);
    }
  }

  response &= myGPS.saveConfiguration(); //Save the current settings to flash and BBR

  if (response == false)
  {
    Serial.println(F("Module failed to save."));
    return (false);
  }

  return (true);
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
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
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
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
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
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
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
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
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

//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool startBluetooth()
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  mac[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  char deviceName[20];
  if (digitalRead(baseSwitch) == HIGH)
  {
    //Rover mode
    sprintf(deviceName, "Surveyor Rover-%02X%02X", mac[4], mac[5]);
  }
  else
  {
    //Base mode
    sprintf(deviceName, "Surveyor Base-%02X%02X", mac[4], mac[5]);
  }

  if (SerialBT.begin(deviceName) == false)
    return (false);
  Serial.print("Bluetooth broadcasting as: ");
  Serial.println(deviceName);

  //Start the tasks.
  //Can also use xTaskCreatePinnedToCore to pin a task to one of the two cores
  //on the ESP32
  xTaskCreate(F9PSerialReadTask, "F9Read", 10000, NULL, 0, NULL);
  xTaskCreate(F9PSerialWriteTask, "F9Write", 10000, NULL, 0, NULL);
  
  return (true);
}

boolean SFE_UBLOX_GPS_ADD::getModuleInfo(uint16_t maxWait)
{
    myGPS.minfo.hwVersion[0] = 0;
    myGPS.minfo.swVersion[0] = 0;
    for (int i = 0; i < 10; i++)
        myGPS.minfo.extension[i][0] = 0;
    myGPS.minfo.extensionNo = 0;

    // Let's create our custom packet
    uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

    // The next line creates and initialises the packet information which wraps around the payload
    ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

    // The structure of ubxPacket is:
    // uint8_t cls           : The message Class
    // uint8_t id            : The message ID
    // uint16_t len          : Length of the payload. Does not include cls, id, or checksum bytes
    // uint16_t counter      : Keeps track of number of overall bytes received. Some responses are larger than 255 bytes.
    // uint16_t startingSpot : The counter value needed to go past before we begin recording into payload array
    // uint8_t *payload      : The payload
    // uint8_t checksumA     : Given to us by the module. Checked against the rolling calculated A/B checksums.
    // uint8_t checksumB
    // sfe_ublox_packet_validity_e valid            : Goes from NOT_DEFINED to VALID or NOT_VALID when checksum is checked
    // sfe_ublox_packet_validity_e classAndIDmatch  : Goes from NOT_DEFINED to VALID or NOT_VALID when the Class and ID match the requestedClass and requestedID

    // sendCommand will return:
    // SFE_UBLOX_STATUS_DATA_RECEIVED if the data we requested was read / polled successfully
    // SFE_UBLOX_STATUS_DATA_SENT     if the data we sent was writted successfully (ACK'd)
    // Other values indicate errors. Please see the sfe_ublox_status_e enum for further details.

    // Referring to the u-blox M8 Receiver Description and Protocol Specification we see that
    // the module information can be read using the UBX-MON-VER message. So let's load our
    // custom packet with the correct information so we can read (poll / get) the module information.

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
