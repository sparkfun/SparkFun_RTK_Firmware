/*
  This example demonstrates how to connect to the ZED-F9x over UART1 on the ZED to UART2 on the ESP32.
  We use this interface for passing NMEA from the ZED to the ESP32 that is then broadcast over BT SPP.

  By default, the ZED in the RTK product is configured at 460800bps for maximum logging
*/

const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 11;

#include <Wire.h>

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

SFE_UBLOX_GNSS i2cGNSS;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

HardwareSerial serialGNSS(2); //TX on 17, RX on 16

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("GNSS UART2 Connection Test");

  Wire.begin();
  //Wire.setClock(400000);

  if (i2cGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Trying again"));

    //On the RTK Facet the ESP32 controls the power to the ZED. During power cycles, the ZED can take up to ~1000ms to respond to I2C pings.
    //delay(400); //Bad
    delay(500); //Good - may be a combination of startup delay of 200ms

    if (i2cGNSS.begin() == false) //Connect to the u-blox module using Wire port
      Serial.println(F("u-blox GNSS not detected at default I2C address."));
    else
      Serial.println(F("ZED online over I2C"));
  }
  else
    Serial.println(F("ZED online over I2C"));

  Serial.println();
  Serial.println("s) Single test");
  Serial.println("r) Reset ESP32");
  Serial.println("x) Reset u-blox module to factory defaults");
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 's')
    {
      Serial.println("Start ZED over serial connection");

      i2cGNSS.setSerialRate(115200 * 4, COM_PORT_UART1); //Set u-blox UART1 to 460800 using I2C

      serialGNSS.begin(115200 * 4);  //Open ESP32 UART1 at 460800

      //See if ZED responds to serial
      SFE_UBLOX_GNSS myGNSS;

      if (myGNSS.begin(serialGNSS) == false)
        Serial.println(F("u-blox GNSS not detected over serial UART2."));
      else
        Serial.println("u-blox detected over ESP32 UART2 / u-blox UART1. BT connection is good.");
    }
    else if (incoming == 'r')
    {
      Serial.println("ESP32 Reset");
      ESP.restart();
    }
    else if (incoming == 'x')
    {
      Serial.println("Reset u-blox module to factory defaults. Please wait.");
      i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

      delay(5000); // Wait while the module restarts
    }
    else
    {
      Serial.println();
      Serial.println("s) Single test");
      Serial.println("r) Reset ESP32");
      Serial.println("x) Reset u-blox module to factory defaults");
    }
  }
}
