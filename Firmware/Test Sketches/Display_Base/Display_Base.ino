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

const int FIRMWARE_VERSION_MAJOR = 2;
const int FIRMWARE_VERSION_MINOR = 7;

void setup()
{
  Wire.begin();
  Wire.setClock(100000);

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

    oled.setCursor(10, 2); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print("SparkFun");

    oled.setCursor(21, 13);
    oled.setFontType(1);
    oled.print("RTK");

    int surveyorTextY = 25;
    int surveyorTextX = 2;
    int surveyorTextKerning = 8;
    oled.setFontType(1);

    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("S");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("u");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("v");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("e");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("y");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("o");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    oled.setCursor(20, 41);
    oled.setFontType(0); //Set font to smallest
    oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    oled.display();

    //while(1) delay(10);
  }
}

float counter = 1.042;

void loop()
{
  if (displayDetected)
  {
    oled.clear(PAGE); // Clear the display's internal memory

    oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);

    oled.setFontType(1); //Set font to type 1: 8x16

    //Rover = Horizontal positional accuracy, Base = 3D Mean Accuracy
    float hpa;

    //hpa = counter + 0.123;
    hpa = 10.7161;

    oled.setCursor(16, 20); //x, y
    oled.print(":");

    if (hpa > 30)
    {
      oled.print(">30");
    }
    else if (hpa > 9.9)
    {
      oled.print(hpa, 1); //Print down to decimeter
    }
    else if (hpa > 1.0)
    {
      oled.print(hpa, 2); //Print down to centimeter
    }
    else
    {
      oled.print("."); //Remove leading zero
      oled.printf("%03d", (int)(hpa * 1000)); //Print down to millimeter
    }

    //SIV
    oled.setCursor(16, 36); //x, y
    oled.print(":24");

    //Bluetooth Address
//    oled.setFontType(0); //Set font to type 0:
//    oled.setCursor(0, 4); //x, y
//    oled.print("BC5D");
    oled.drawIcon(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol, sizeof(BT_Symbol), true);

        oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);
    //    oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true);

    oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);

    oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);

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
