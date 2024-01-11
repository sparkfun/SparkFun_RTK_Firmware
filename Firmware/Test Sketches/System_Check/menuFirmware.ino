// Format the firmware version
void formatFirmwareVersion(uint8_t major, uint8_t minor, char *buffer, int bufferLength, bool includeDate)
{
  char prefix;

  // Construct the full or release candidate version number
  prefix = 'd';
  //    if (enableRCFirmware && (bufferLength >= 21))
  //        // 123456789012345678901
  //        // pxxx.yyy-dd-mmm-yyyy0
  //        snprintf(buffer, bufferLength, "%c%d.%d-%s", prefix, major, minor, __DATE__);

  // Construct a truncated version number
  if (bufferLength >= 9)
    // 123456789
    // pxxx.yyy0
    snprintf(buffer, bufferLength, "%c%d.%d", prefix, major, minor);

  // The buffer is too small for the version number
  else
  {
    Serial.printf("ERROR: Buffer too small for version number!\r\n");
    if (bufferLength > 0)
      *buffer = 0;
  }
}

// Get the current firmware version
void getFirmwareVersion(char *buffer, int bufferLength, bool includeDate)
{
  formatFirmwareVersion(FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, buffer, bufferLength, includeDate);
}
