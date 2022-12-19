/*
  October 20, 2022
  By: Nathan Seidle

  This example demonstrates a simplified version of reading the ZED-F9P UART1 as quickly as possible
  then recording that serial data to the SD card. Two tasks are used because the SD write task is blocking
  up to ~150ms, with up to 1100ms write times seen on some apparently good cards. While the SD is recording,
  we continue to harvest data from the UART into the circular buffer.

  Not all SD cards are equal. A Kingston 16GB card performed terribly with multiple >500ms writes,
  where as a 8GB Kingston card did flawlessly. If you see poor performance, try
  formatting the card with the SD Formatter: https://www.sdcard.org/downloads/formatter/

  At 230400bps, we receive 23040 bytes/s, or a byte every 43.4us. If the ESP32 UART FIFO is ~127 bytes, we must check
  Serial.available every 5.5ms or less to avoid data loss. Additionally, if SD blocks for 250ms, we need a
  23040 * 0.25 = 5760 byte buffer.

  This sketch creates a file called myTest.txt is created and GNSS NMEA and another other messages (RAWX, etc) are logged.

  This sketch does not configure the ZED-F9P. The ZED's settings are loaded from NVM at POR so we assume
  the module has been previously configured for the desired test (usually this is 4Hz fix rate, NMEAx5 + RAWx2 messages).

  We can modify the various buffer sizes and verify log integrity. We can also modify the task priorities.
  Additionally, we can switch between the built-in SD library and sdFat for ease of testing. In general,
  sdFat seems to perform better than the built-in SD library.

  SD writes that take longer than 100ms are shown. This helps troubleshoot an SD card that has poor performance.
  Low buffer space is displayed. When the buffer reaches 0 bytes left, we can assume the log will have a corruption.

  Use the UBX Integrity checker to check the validity of the generated log:
    https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library/blob/main/Utils/UBX_Integrity_Checker.py
*/

#include "settings.h"

#define USE_SDFAT //Comment out to us the built-in SD library

#ifdef USE_SDFAT

#include "SdFat.h" //http://librarymanager/All#sdfat_exfat by Bill Greiman. Currently uses v2.1.1
SdFat SD;

#else

#include "SD.h" //Built-in SD library
SPIClass spi = SPIClass(VSPI);

#endif

HardwareSerial serialGNSS(2); //TX on 17, RX on 16

const int uartReceiveBufferSize = (1024 * 2); //This buffer is filled automatically as the UART receives characters.
const int gnssHandlerBufferSize = (1024 * 4); //This buffer is filled from the UART receive buffer, and is then written to SD

uint8_t * rBuffer;

int pin_SCK = 18;
int pin_MISO = 19;
int pin_MOSI = 23;
int pin_microSD_CS = 25;

File ubxFile;

char fileName[] = "/myTest.txt";
long fileSize = 0;
unsigned long lastFileReport = 0;

unsigned long lastUBXLogSyncTime = 0;

TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
const uint8_t F9PSerialReadTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int readTaskStackSize = 2000;

TaskHandle_t SDWriteTaskHandle = NULL;
const uint8_t SDWriteTaskPriority = 1; //3 being the highest, and 0 being the lowest
const int SDWriteTaskStackSize = 2000;

void setup()
{
  Serial.begin(115200);
  delay(250);

  Serial.println("Start basic UART test");
  Serial.println("b) Begin logging");
  Serial.println("c) Close log before extracting card");
  Serial.println("r) Reset");
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'r')
    {
      ESP.restart();
    }
    else if (incoming == 'c')
    {
      Serial.println("Closing file...");
      online.logging = false;
      stopTasks();

      delay(1000);

      ubxFile.close();
      Serial.println("File closed");
    }
    else if (incoming == 'b')
    {

      rBuffer = (uint8_t*)malloc(gnssHandlerBufferSize);

#ifdef USE_SDFAT
      //      if (SD.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(32))) == false) { //This fails on some SD cards
      if (SD.begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(16))) == false) {
#else
      //if (!SD.begin(pin_microSD_CS)) { //Works
      spi.begin(pin_SCK, pin_MISO, pin_MOSI, pin_microSD_CS); //Needed when definiing SPI bus speed
      if (!SD.begin(pin_microSD_CS, spi, 80000000)) { //80MHz bus is not actually achieved
#endif

        Serial.println("Card Mount Failed");
        while (1)
        {
          if (Serial.available()) ESP.restart();
          delay(1);
        }
      }

      if (SD.exists(fileName))
        SD.remove(fileName);

#ifdef USE_SDFAT
      ubxFile.open(fileName, O_CREAT | O_APPEND | O_WRITE);
#else
      ubxFile = SD.open(fileName, FILE_APPEND);
#endif

      online.microSD = true;
      online.logging = true;

      serialGNSS.setRxBufferSize(uartReceiveBufferSize); //Relatively small but is serviced by high priority task
      serialGNSS.setTimeout(0); //Zero disables timeout, thus, onReceive callback will only be called when RX FIFO reaches 120 bytes: https://github.com/espressif/arduino-esp32/blob/dca1a1e6b3d766aae8d0a6b8305b8273ccf5077c/cores/esp32/HardwareSerial.cpp#L233
      serialGNSS.begin(115200 * 2); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

      //Start task for harvesting bytes from UART2
      if (F9PSerialReadTaskHandle == NULL)
        xTaskCreate(
          F9PSerialReadTask, //Function to call
          "F9Read", //Just for humans
          readTaskStackSize, //Stack Size
          NULL, //Task input parameter
          F9PSerialReadTaskPriority, //Priority
          &F9PSerialReadTaskHandle); //Task handle

      //Start task for writing to SD card
      if (SDWriteTaskHandle == NULL)
        xTaskCreate(
          SDWriteTask, //Function to call
          "SDWrite", //Just for humans
          SDWriteTaskStackSize, //Stack Size
          NULL, //Task input parameter
          SDWriteTaskPriority, //Priority
          &SDWriteTaskHandle); //Task handle

      Serial.println("Recording to log...");
    }
  }

  //Report file sizes to show recording is working
  if ((millis() - lastFileReport) > 5000)
  {
    lastFileReport = millis();
    Serial.printf("UBX file size: %ld\n\r", fileSize);
  }

  delay(1); //Yield to other tasks
}
