//Control the messages that get broadcast over Bluetooth (NMEA and/or UBX)
void menuBroadcast()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Broadcast Menu"));

    Serial.print(F("1) Broadcast NMEA GGA: "));
    if (settings.broadcast.gga == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Broadcast NMEA GSA: "));
    if (settings.broadcast.gsa == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Broadcast NMEA GSV: "));
    if (settings.broadcast.gsv == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Broadcast NMEA RMC: "));
    if (settings.broadcast.rmc == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("5) Broadcast NMEA GST: "));
    if (settings.broadcast.gst == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("6) Broadcast UBX RAWX: "));
    if (settings.broadcast.rawx == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("7) Broadcast UBX SFRBX: "));
    if (settings.broadcast.sfrbx == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.broadcast.gga ^= 1;
    }
    else if (incoming == '2')
    {
      settings.broadcast.gsa ^= 1;
    }
    else if (incoming == '3')
    {
      settings.broadcast.gsv ^= 1;
    }
    else if (incoming == '4')
    {
      settings.broadcast.rmc ^= 1;
    }
    else if (incoming == '5')
    {
      settings.broadcast.gst ^= 1;
    }
    else if (incoming == '6')
    {
      settings.broadcast.rawx ^= 1;
    }
    else if (incoming == '7')
    {
      settings.broadcast.sfrbx ^= 1;
    }

    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars

  bool response = enableMessages(COM_PORT_UART1, settings.broadcast); //Make sure the appropriate messages are enabled
  if (response == false)
  {
    Serial.println(F("menuBroadcast: Failed to enable UART1 messages - Try 1"));
    //Try again
    response = enableMessages(COM_PORT_UART1, settings.broadcast); //Make sure the appropriate messages are enabled
    if (response == false)
      Serial.println(F("menuBroadcast: Failed to enable UART1 messages - Try 2"));
    else
      Serial.println(F("menuBroadcast: UART1 messages successfully enabled"));
  }
}
