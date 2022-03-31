/*
  Demonstrates reading an SD card and showing files on card.

  Uses semaphores to prevent hardware access collisions.
*/

#include "settings.h"

int pin_microSD_CS = 25;

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include "SdFat.h" //http://librarymanager/All#sdfat_exfat by Bill Greiman. Currently uses v2.1.1

SdFat sd;

char platformFilePrefix[40] = "SFE_Surveyor"; //Sets the prefix for logs and settings files

SdFile ubxFile; //File that all gnss ubx messages setences are written to
unsigned long lastUBXLogSyncTime = 0; //Used to record to SD every half second
int startLogTime_minutes = 0; //Mark when we start any logging so we can stop logging after maxLogTime_minutes
int startCurrentLogTime_minutes = 0; //Mark when we start this specific log file so we can close it after x minutes and start a new one

SdFile newFirmwareFile; //File that is available if user uploads new firmware via web gui

//System crashes if two tasks access a file at the same time
//So we use a semaphore to see if file system is available
SemaphoreHandle_t xFATSemaphore;
const TickType_t fatSemaphore_shortWait_ms = 10 / portTICK_PERIOD_MS;
const TickType_t fatSemaphore_longWait_ms = 200 / portTICK_PERIOD_MS;

//Display used/free space in menu and config page
uint32_t sdCardSizeMB = 0;
uint32_t sdFreeSpaceMB = 0;
uint32_t sdUsedSpaceMB = 0;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("SD File Listing");

  beginSD();

  if (online.microSD == false)
    Serial.println("SD not detected. Please check card. Is it formatted?");
  else
    sd.ls(LS_R | LS_DATE | LS_SIZE); //Print files on card
}

void loop()
{
  Serial.println("Press a key to reset");

  if (Serial.available()) ESP.restart();

  delay(1000);
}
