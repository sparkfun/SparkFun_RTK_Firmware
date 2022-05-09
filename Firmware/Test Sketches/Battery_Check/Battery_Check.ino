/*
  Basic demonstration of the MAX17048 fuel gauge IC used in the RTK line.

  On the RTK Surveyor, one LED is used to indicate status. Battery level LED goes
  from Green (50-100%) to Yellow (10-50%) to Red (<10%)
*/

//Battery fuel gauge and PWM LEDs
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048);

// setting PWM properties
const int pwmFreq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int ledBTChannel = 2;
const int pwmResolution = 8;

int pwmFadeAmount = 10;
int btFadeLevel = 0;

int battLevel = 0; //SOC measured from fuel gauge, in %. Used in multiple places (display, serial debug, log)
float battVoltage = 0.0;
float battChangeRate = 0.0;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

uint32_t lastBattUpdate = 0;

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Battery example");

  Wire.begin();

  beginLEDs(); //LED and PWM setup

  beginFuelGauge(); //Configure battery fuel guage monitor
  checkBatteryLevels(); //Force check so you see battery level immediately at power on
}

void loop()
{
  updateBattLEDs();
  Serial.println(".");
  delay(1000);
}
