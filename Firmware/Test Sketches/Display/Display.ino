/*
   MicroOLED_Clock.ino

   Distributed as-is; no warranty is given.
*/
#include <Wire.h>

#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_Graphic_OLED
QwiicMicroOLED oled;

// Fonts
#include <res/qw_fnt_5x7.h>
#include <res/qw_fnt_8x16.h>

//#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
#include "icons.h"

//#define PIN_RESET 9
//#define DC_JUMPER 1
//MicroOLED oled(PIN_RESET, DC_JUMPER);

bool displayDetected = false;

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("OLED example");

  Wire.begin();
  Wire.setClock(400000);

  if (oled.begin() == false)
  {
    Serial.println("Device begin failed. Freezing...");
    while (true)
      ;
  }
  Serial.println("Begin success");
  displayDetected = true;
}

void loop()
{
  if (displayDetected)
  {
    //    oled.drawIcon(0, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);
    //    oled.drawIcon(4, 4, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
    //    oled.drawIcon(4, 4, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
    //    oled.drawIcon(4, 4, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);

    oled.setFont(QW_FONT_8X16);

    //HPA
    //    oled.setCursor(0, 21); //x, y
    //    oled.print("HPA:");
    //    oled.print("125");

    //3D Mean Accuracy
    oled.setCursor(17, 19); //x, y: Squeeze against the colon
    oled.print(":12.1");

    //SIV
    oled.setCursor(16, 35); //x, y
    printText(":24", QW_FONT_5X7);
    //oled.print(":24");

    //Bluetooth Address
    oled.setFont(QW_FONT_5X7);
    oled.setCursor(0, 4); //x, y
    oled.print("BC5D");

    displayBitmap(45, 0, Battery_2_Width, Battery_2_Height, Battery_2);
    displayBitmap(28, 0, Base_Width, Base_Height, Base);
    displayBitmap(0, 18, CrossHair_Width, CrossHair_Height, CrossHair);
    displayBitmap(1, 35, Antenna_Width, Antenna_Height, Antenna);
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

//Wrapper to avoid needing to pass width/height data twice
void displayBitmap(uint8_t x, uint8_t y, uint8_t imageWidth, uint8_t imageHeight, uint8_t *imageData)
{
  oled.bitmap(x, y, x + imageWidth, y + imageHeight, imageData, imageWidth, imageHeight);
}

void printText(const char *text, QwiicFont &fontType)
{
  oled.setFont(fontType);
  oled.setDrawMode(grROPXOR);

  oled.print(text);
}
