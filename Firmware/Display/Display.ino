/*
   MicroOLED_Clock.ino

   Distributed as-is; no warranty is given.
*/
#include <Wire.h>  // Include Wire if you're using I2C
#include <SFE_MicroOLED.h>  // Include the SFE_MicroOLED library

#define PIN_RESET 9  // Connect RST to pin 9 (SPI & I2C)
#define DC_JUMPER 1 // Set to either 0 (SPI, default) or 1 (I2C) based on jumper, matching the value of the DC Jumper
MicroOLED oled(PIN_RESET, DC_JUMPER);

uint8_t Battery_0 [] = {
0xFF, 0x01, 0xFD, 0xFD, 0xFD, 0x01, 0x01, 0xFD, 0xFD, 0xFD, 0x01, 0x01, 0xFD, 0xFD, 0xFD, 0x01,
0x0F, 0x08, 0xF8, 0x0F, 0x08, 0x0B, 0x0B, 0x0B, 0x08, 0x08, 0x0B, 0x0B, 0x0B, 0x08, 0x08, 0x0B,
0x0B, 0x0B, 0x08, 0x0F, 0x01, 0x01, 
};

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

void loop()
{
  if (displayDetected)
  {
    oled.drawIcon(0, 0, 19, 16, Battery_0, sizeof(Battery_0), true);
    oled.display();

    while(1);
  }

  Serial.print(".");
  delay(100);
}
