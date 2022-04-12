/*
  Get the high precision geodetic solution for latitude and longitude using double
  By: Nathan Seidle
  Modified by: Paul Clark (PaulZC)
  SparkFun Electronics
  Date: April 17th, 2020
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example shows how to inspect the accuracy of the high-precision
  positional solution. Please see below for information about the units.
*/

#include <Wire.h> // Needed for I2C to GNSS

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS i2cGNSS;

long lastTime = 0; //Simple local timer. Limits amount of I2C traffic to u-blox module.

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("u-blox high precision example");

  Wire.begin();
  //Wire.setClock(400000);

  //i2cGNSS.enableDebugging(Serial); // Uncomment this line to enable debug messages

  if (i2cGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
    {
      if(Serial.available()) ESP.restart();
      delay(10);
    }
  }

  i2cGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
}

void loop()
{
  //Query module only every second.
  //The module only responds when a new position is available.
  if (millis() - lastTime > 1000)
  {
    lastTime = millis(); //Update the timer

    int32_t latitude = i2cGNSS.getHighResLatitude();
    int8_t latitudeHp = i2cGNSS.getHighResLatitudeHp();
    int32_t longitude = i2cGNSS.getHighResLongitude();
    int8_t longitudeHp = i2cGNSS.getHighResLongitudeHp();
    int32_t ellipsoid = i2cGNSS.getElipsoid();
    int8_t ellipsoidHp = i2cGNSS.getElipsoidHp();
    int32_t msl = i2cGNSS.getMeanSeaLevel();
    int8_t mslHp = i2cGNSS.getMeanSeaLevelHp();
    uint32_t accuracy = i2cGNSS.getHorizontalAccuracy();
    uint8_t siv = i2cGNSS.getSIV();

    // Defines storage for the lat and lon as double
    double d_lat; // latitude
    double d_lon; // longitude

    // Assemble the high precision latitude and longitude
    d_lat = ((double)latitude) / 10000000.0; // Convert latitude from degrees * 10^-7 to degrees
    d_lat += ((double)latitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )
    d_lon = ((double)longitude) / 10000000.0; // Convert longitude from degrees * 10^-7 to degrees
    d_lon += ((double)longitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )

    float f_ellipsoid;
    float f_accuracy;

    // Calculate the height above ellipsoid in mm * 10^-1
    f_ellipsoid = (ellipsoid * 10) + ellipsoidHp;
    f_ellipsoid = f_ellipsoid / 10000.0; // Convert from mm * 10^-1 to m

    f_accuracy = accuracy / 10000.0; // Convert from mm * 10^-1 to m

    // Finally, do the printing
    Serial.print("Lat (deg): ");
    Serial.print(d_lat, 9);
    Serial.print(", Lon (deg): ");
    Serial.print(d_lon, 9);

    Serial.print(", SIV: ");
    Serial.print(siv);
    
    Serial.print(", Accuracy (m): ");
    Serial.print(f_accuracy, 4); // Print the accuracy with 4 decimal places

    Serial.print(", Altitude (m): ");
    Serial.print(f_ellipsoid, 4); // Print the ellipsoid with 4 decimal places

    Serial.println();
  }

  if(Serial.available()) ESP.restart();
}
