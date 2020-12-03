/*
   MicroOLED_Clock.ino

   Distributed as-is; no warranty is given.
*/
#include <Wire.h>
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);

bool displayDetected = false;

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  delay(100);
  Serial.println("OLED example");

  //Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    Serial.println("Display detected");
    displayDetected = true;
  }
  else
    Serial.println("No display detected");

  if (displayDetected)
  {
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory
    oled.clear(ALL);  // Clear the library's display buffer
  }
}

void loop()
{
  if (displayDetected)
  {
    //    oled.drawIcon(0, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);
    //    oled.drawIcon(4, 4, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
    //    oled.drawIcon(4, 4, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
    //    oled.drawIcon(4, 4, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);

    oled.setFontType(1); //Set font to type 1: 8x16

    //HPA
    //    oled.setCursor(0, 21); //x, y
    //    oled.print("HPA:");
    //    oled.print("125");

    //3D Mean Accuracy
    oled.setCursor(17, 19); //x, y: Squeeze against the colon
    oled.print(":12.1");

    //SIV
    oled.setCursor(16, 35); //x, y
    oled.print(":24");

    //Bluetooth Address
//    oled.setFontType(0); //Set font to type 0:
//    oled.setCursor(0, 4); //x, y
//    oled.print("BC5D");

    //Bluetooth Address
    char macAddress[5];
    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
    Serial.printf("MAC: %s", macAddress);

    oled.setFontType(0); //Set font type to smallest
    oled.setCursor(0, 4);
    oled.print(macAddress);

    oled.drawIcon(45, 0, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
    oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);
    oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
    oled.drawIcon(1, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);

    oled.display();

    while (1);
  }

  //Fix type
  //None
  //3D
  //RTK Fix
  //RTK Float
  //TIME - Base thing

  Serial.print(".");
  delay(100);
}
