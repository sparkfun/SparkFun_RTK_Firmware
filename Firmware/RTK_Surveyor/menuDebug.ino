//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Debug Menu"));

    printModuleInfo();

    Serial.print(F("1) I2C Debugging Output: "));
    if (settings.enableI2Cdebug == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Heap Reporting: "));
    if (settings.enableHeapReport == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("3) Task Highwater Reporting: "));
    if (settings.enableTaskReports == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("4) Set SPI/SD Interface Frequency: "));
    Serial.print(settings.spiFrequency);
    Serial.println(" MHz");

    Serial.print(F("5) Set SPP RX Buffer Size: "));
    Serial.println(settings.sppRxQueueSize);

    Serial.print(F("6) Set SPP TX Buffer Size: "));
    Serial.println(settings.sppTxQueueSize);

    Serial.print(F("7) Throttle BT Transmissions During SPP Congestion: "));
    if (settings.throttleDuringSPPCongestion == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enableI2Cdebug ^= 1;

      if (settings.enableI2Cdebug)
        i2cGNSS.enableDebugging(Serial, true); //Enable only the critical debug messages over Serial
      else
        i2cGNSS.disableDebugging();
    }
    else if (incoming == '2')
    {
      settings.enableHeapReport ^= 1;
    }
    else if (incoming == '3')
    {
      settings.enableTaskReports ^= 1;
    }
    else if (incoming == '4')
    {
      Serial.print(F("Enter SPI frequency in MHz (1 to 48): "));
      int freq = getNumber(menuTimeout); //Timeout after x seconds
      if (freq < 1 || freq > 48) //Arbitrary 48 hour limit
      {
        Serial.println(F("Error: SPI frequency out of range"));
      }
      else
      {
        settings.spiFrequency = freq; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '5')
    {
      Serial.print(F("Enter SPP RX Queue Size in Bytes (32 to 16384): "));
      uint16_t queSize = getNumber(menuTimeout); //Timeout after x seconds
      if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
      {
        Serial.println(F("Error: Queue size out of range"));
      }
      else
      {
        settings.sppRxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '6')
    {
      Serial.print(F("Enter SPP TX Queue Size in Bytes (32 to 16384): "));
      uint16_t queSize = getNumber(menuTimeout); //Timeout after x seconds
      if (queSize < 32 || queSize > 16384) //Arbitrary 16k limit
      {
        Serial.println(F("Error: Queue size out of range"));
      }
      else
      {
        settings.sppTxQueueSize = queSize; //Recorded to NVM and file at main menu exit
      }
    }
    else if (incoming == '7')
    {
      settings.throttleDuringSPPCongestion ^= 1;
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
}
