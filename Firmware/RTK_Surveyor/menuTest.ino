//Production testing
//Allow operator to output NMEA on radio port for connector testing
//Scan for display
void menuTest()
{
  inTestMode = true; //Reroutes bluetooth bytes

  //Enable RTCM 1230. This is the GLONASS bias sentence and is transmitted
  //even if there is no GPS fix. We use it to test serial output.
  myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 1); //Enable message every second

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Test Menu"));

    Serial.print(F("Bluetooth broadcasting as: "));
    Serial.println(deviceName);

    Serial.println(F("Radio Port is now outputting RTCM"));

    if (settings.enableSD && online.microSD)
    {
      Serial.println(F("microSD card is successfully detected"));
    }

    //0x3D is default on Qwiic board
    if (isConnected(0x3D) == true || isConnected(0x3C) == true)
      Serial.println(F("Qwiic Good. Display detected."));
    else
      Serial.println(F("Qwiic port failed! No display detected."));
      
    Serial.println(F("Any character received over Blueooth connection will be displayed here"));

    Serial.println(F("1) Display microSD contents"));

    Serial.println(F("2) Scan for Qwiic OLED display"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

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
      Serial.println(F("time out"));
      //      break;
    }
    else
      printUnknown(incoming);
  }

  inTestMode = false; //Reroutes bluetooth bytes

  //Disable RTCM sentences
  myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 0);

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
