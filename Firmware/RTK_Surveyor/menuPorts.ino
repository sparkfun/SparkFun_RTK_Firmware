//Set the baud rates for the radio and data ports
void menuPorts()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Port Menu"));

    Serial.print(F("1) Set serial baud rate for Radio Port: "));
    Serial.print(getSerialRate(COM_PORT_UART2));
    Serial.println(F(" bps"));

    Serial.print(F("2) Set serial baud rate for Data Port: "));
    Serial.print(getSerialRate(COM_PORT_UART1));
    Serial.println(F(" bps"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      Serial.print(F("Enter baud rate (4800 to 921600) for Radio Port: "));
      int newBaud = getNumber(menuTimeout); //Timeout after x seconds
      if (newBaud < 4800 || newBaud > 921600)
      {
        Serial.println(F("Error: baud rate out of range"));
      }
      else
      {
        settings.radioPortBaud = newBaud;
        myGPS.setSerialRate(newBaud, COM_PORT_UART2); //Set Radio Port
      }
    }
    else if (incoming == '2')
    {
      Serial.print(F("Enter baud rate (4800 to 921600) for Data Port: "));
      int newBaud = getNumber(menuTimeout); //Timeout after x seconds
      if (newBaud < 4800 || newBaud > 921600)
      {
        Serial.println(F("Error: baud rate out of range"));
      }
      else
      {
        settings.dataPortBaud = newBaud;
        myGPS.setSerialRate(newBaud, COM_PORT_UART1); //Set Data Port
      }
    }

    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}
