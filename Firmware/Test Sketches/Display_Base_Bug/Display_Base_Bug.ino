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

void setup()
{
  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  delay(100);
  Serial.println("OLED example");

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

int counter = 1;

void loop()
{
  if (displayDetected)
  {
//    oled.clear(PAGE); // Clear the display's internal memory
//    oled.clear(ALL);  // Clear the library's display buffer
    //    oled.drawIcon(0, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);
    //    oled.drawIcon(4, 4, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
    //    oled.drawIcon(4, 4, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
    //    oled.drawIcon(4, 4, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);


    //HPA
//    oled.setCursor(0, 21); //x, y
//    oled.print("HPA:");
//    oled.print("125");

    //3D Mean Accuracy
    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(17, 19); //x, y: Squeeze against the colon
    oled.print(":");
    oled.print(counter++);
//
//    //SIV
    oled.setCursor(16, 35); //x, y
    oled.print(":24");
//
//    //Bluetooth Address
    oled.setFontType(0); //Set font to type 0: 
    oled.setCursor(0, 4); //x, y
    oled.print("BC5D");

    oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);
    oled.drawIcon(28, 0, Base_Width, Base_Height, Base, sizeof(Base), true);
    oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);

    oled.drawIcon(1, 38, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true); //fails
//    oled.drawIcon(1, 32, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true); //good

    oled.line(0, 46, 0, 48);
    oled.display();

    //while(1);
  }

  //Fix type
  //None
  //3D
  //RTK Fix
  //RTK Float
  //TIME - Base thing

  Serial.print(".");
  delay(250);
}
