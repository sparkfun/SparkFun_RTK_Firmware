
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

  //myGPS.setNavigationFrequency(10); //Set output to 10 times a second
  if (myGPS.getNavigationFrequency() != 4)
  {
    response &= myGPS.setNavigationFrequency(4); //Set output in Hz
  }

  //Make sure the appropriate sentences are enabled
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

  response &= myGPS.setAutoPVT(true); //Tell the GPS to "send" each solution

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
      digitalWrite(positionAccuracyLED_20mm, HIGH);
      digitalWrite(positionAccuracyLED_100mm, HIGH);
      digitalWrite(positionAccuracyLED_1000mm, HIGH);
      digitalWrite(baseStatusLED, HIGH);
      digitalWrite(bluetoothStatusLED, HIGH);
      delay(200);
      digitalWrite(positionAccuracyLED_20mm, LOW);
      digitalWrite(positionAccuracyLED_100mm, LOW);
      digitalWrite(positionAccuracyLED_1000mm, LOW);
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
    digitalWrite(positionAccuracyLED_20mm, HIGH);
    digitalWrite(positionAccuracyLED_100mm, HIGH);
    digitalWrite(positionAccuracyLED_1000mm, HIGH);
    digitalWrite(baseStatusLED, HIGH);
    digitalWrite(bluetoothStatusLED, HIGH);
    delay(100);
    digitalWrite(positionAccuracyLED_20mm, LOW);
    digitalWrite(positionAccuracyLED_100mm, LOW);
    digitalWrite(positionAccuracyLED_1000mm, LOW);
    digitalWrite(baseStatusLED, LOW);
    digitalWrite(bluetoothStatusLED, LOW);
    delay(100);
  }

  digitalWrite(positionAccuracyLED_20mm, HIGH);
  digitalWrite(positionAccuracyLED_100mm, HIGH);
  digitalWrite(positionAccuracyLED_1000mm, HIGH);
  digitalWrite(baseStatusLED, HIGH);
  digitalWrite(bluetoothStatusLED, HIGH);

  delay(250);
  digitalWrite(positionAccuracyLED_20mm, LOW);
  delay(250);
  digitalWrite(positionAccuracyLED_100mm, LOW);
  delay(250);
  digitalWrite(positionAccuracyLED_1000mm, LOW);

  delay(250);
  digitalWrite(baseStatusLED, LOW);
  delay(250);
  digitalWrite(bluetoothStatusLED, LOW);
}
