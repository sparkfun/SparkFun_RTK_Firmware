//Production testing
//Allow operator to output NMEA on radio port for connector testing
//Scan for display
void menuTest()
{
  inTestMode = true; //Reroutes bluetooth bytes

  //Enable RTCM
  myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART2, 1); //Enable message 1005 to output through UART2, message every second
  myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_UART2, 1);
  myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_UART2, 1);
  myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_UART2, 1);
  myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_UART2, 1);
  myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 10); //Enable message every 10 seconds

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Test Menu"));

    Serial.print("Bluetooth broadcasting as: ");
    Serial.println(deviceName);

    Serial.println("Radio Port is now outputting RTCM");

    if (settings.enableSD && online.microSD)
    {
      Serial.println("microSD card is successfully detected");
    }

    //0x3D is default on Qwiic board
    if (isConnected(0x3D) == true || isConnected(0x3C) == true)
      Serial.println("Qwiic Good. Display detected.");
    else
      Serial.println("Qwiic port failed! No display detected.");
      
    Serial.println(F("Any character received over Blueooth connection will be displayed here"));

    Serial.println(F("1) Display microSD contents"));

    Serial.println(F("2) Scan for Qwiic OLED display"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      if (settings.enableSD && online.microSD)
      {
        Serial.println(F("Files found (date time size name):\n"));
        sd.ls(LS_R | LS_DATE | LS_SIZE);
      }
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      Serial.println("time out");
      //      break;
    }
    else
      printUnknown(incoming);
  }

  inTestMode = false; //Reroutes bluetooth bytes

  //Disable RTCM sentences
  myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART2, 0);
  myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_UART2, 0);
  myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_UART2, 0);
  myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_UART2, 0);
  myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_UART2, 0);
  myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 0);

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}
