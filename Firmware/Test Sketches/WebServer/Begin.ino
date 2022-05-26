const int pin_microSD_CS = 25;

typedef struct struct_settings {
  bool enableSD = true;
  uint16_t spiFrequency = 16; //By default, use 16MHz SPI
} Settings;

Settings settings;

const TickType_t fatSemaphore_shortWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_longWait_ms = 200 / portTICK_PERIOD_MS;
SemaphoreHandle_t xFATSemaphore;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void displaySDFail(uint16_t displayTime)
{
  Serial.println ("SD card failed to initialize!");
}

//Create a test file in file structure to make sure we can
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
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

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void beginSD()
{
  pinMode(pin_microSD_CS, OUTPUT);
  digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Do a quick test to see if a card is present
    int tries = 0;
    int maxTries = 5;
    while (tries < maxTries)
    {
      if (sdPresent() == true) break;
      log_d("SD present failed. Trying again %d out of %d", tries + 1, maxTries);

      //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
      //Max current is 200mA average across 1s, peak 300mA
      delay(10);
      tries++;
    }
    if (tries == maxTries) return;

    //If an SD card is present, allow SdFat to take over
    log_d("SD card detected");

    if (settings.spiFrequency > 16)
    {
      Serial.println("Error: SPI Frequency out of range. Default to 16MHz");
      settings.spiFrequency = 16;
    }

    if (sd.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == false)
    {
      tries = 0;
      maxTries = 1;
      for ( ; tries < maxTries ; tries++)
      {
        log_d("SD init failed. Trying again %d out of %d", tries + 1, maxTries);

        delay(250); //Give SD more time to power up, then try again
        if (sd.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == true) break;
      }

      if (tries == maxTries)
      {
        Serial.println(F("SD init failed. Is card present? Formatted?"));
        digitalWrite(pin_microSD_CS, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      Serial.println(F("SD change directory failed"));
      online.microSD = false;
      return;
    }

    //Setup FAT file access semaphore
    if (xFATSemaphore == NULL)
    {
      xFATSemaphore = xSemaphoreCreateMutex();
      if (xFATSemaphore != NULL)
        xSemaphoreGive(xFATSemaphore);  //Make the file system available for use
    }

    if (createTestFile() == false)
    {
      Serial.println(F("Failed to create test file. Format SD card with 'SD Card Formatter'."));
      displaySDFail(5000);
      online.microSD = false;
      return;
    }

    online.microSD = true;

    Serial.println(F("microSD online"));
  }
  else
  {
    online.microSD = false;
  }
}
