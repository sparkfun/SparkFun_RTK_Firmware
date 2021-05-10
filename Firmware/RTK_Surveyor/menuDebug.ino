//Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
  int maxWait = 2000;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Debug Menu"));

    //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
    if (i2cGNSS.getModuleInfo(maxWait) == true) // Try to get the module info
    {
      Serial.print(F("ZED-F9P firmware: "));
      Serial.println(i2cGNSS.minfo.extension[1]);
    }

    Serial.print(F("1) Toggle I2C Debugging Output: "));
    if (settings.enableI2Cdebug == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.print(F("2) Toggle Heap Reporting: "));
    if (settings.enableHeapReport == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(30); //Timeout after x seconds

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
