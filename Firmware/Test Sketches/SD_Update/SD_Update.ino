/*
  Demonstrates reading a binary file from SD and reprogramming ESP32.

  To work, we must the use a partition scheme that includes OTA.
  For RTK Surveyor, 'Minimal SPIFFS (1.9MB with OTA/190KB SPIFFS)' works well.
*/

#include <Update.h>

#include "settings.h"

const byte PIN_MICROSD_CHIP_SELECT = 25;

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
SdFat sd;
SdFile gnssDataFile; //File that all gnss data is written to

char settingsFileName[40] = "SFE_Surveyor_Settings.txt"; //File to read/write system settings to

unsigned long lastDataLogSyncTime = 0; //Used to record to SD every half second
long startLogTime_minutes = 0; //Mark when we start logging so we can stop logging after maxLogTime_minutes
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds

int binCount = 0;
char binFileNames[10][50];
const char* forceFirmwareFileName = "RTK_Surveyor_Firmware_Force.bin";

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("SD Update");

  beginSD(); //Test if SD is present

  if (online.microSD == true)
  {
    Serial.println("SD online");
    scanForFirmware();
  }
}


void loop() {
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == '1')
    {
      menuFirmware();
    }
    else
    {
      Serial.println("Unknown command");
    }

    delay(10);
    while (Serial.available()) Serial.read(); //Remote extra chars
  }

}
