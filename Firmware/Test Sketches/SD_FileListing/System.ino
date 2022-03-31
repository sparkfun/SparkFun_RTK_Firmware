//Create a test file in file structure to make sure we can
bool createTestFile()
{
  SdFile testFile;
  char testFileName[40] = "testfile.txt";

  if (xFATSemaphore == NULL)
  {
    log_d("xFATSemaphore is Null");
    return (false);
  }

  //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
  if (xSemaphoreTake(xFATSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
  {
    if (testFile.open(testFileName, O_CREAT | O_APPEND | O_WRITE) == true)
    {
      testFile.close();

      if (sd.exists(testFileName))
        sd.remove(testFileName);
      xSemaphoreGive(xFATSemaphore);
      return (true);
    }
    xSemaphoreGive(xFATSemaphore);
  }

  return (false);
}
