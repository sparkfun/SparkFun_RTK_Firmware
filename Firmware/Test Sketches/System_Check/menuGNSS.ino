//Print the module type and firmware version
void printZEDInfo()
{
  if (zedModuleType == PLATFORM_F9P)
    Serial.printf("ZED-F9P firmware: %s\n\r", zedFirmwareVersion);
  else if (zedModuleType == PLATFORM_F9R)
    Serial.printf("ZED-F9R firmware: %s\n\r", zedFirmwareVersion);
  else
    Serial.printf("Unknown module with firmware: %s\n\r", zedFirmwareVersion);
}


//Print the NEO firmware version
void printNEOInfo()
{
  if (productVariant == RTK_FACET_LBAND)
    Serial.printf("NEO-D9S firmware: %s\n\r", neoFirmwareVersion);
}
