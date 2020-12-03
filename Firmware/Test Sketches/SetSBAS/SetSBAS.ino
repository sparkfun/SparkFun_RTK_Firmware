/*

*/

#include <Wire.h> //Needed for I2C to GPS

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

void setup()
{
  Serial.begin(115200); // You may need to increase this for high navigation rates!
  while (!Serial) ;
  Serial.println("SparkFun Ublox Example");

  Wire.begin();

  //myGPS.enableDebugging(); // Uncomment this line to enable debug messages

  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  setSBAS(true);

  Serial.println("Done");
  while (1);
}

void loop()
{
  //Query the module as fast as possible
  int32_t latitude = myGPS.getLatitude();
  Serial.print(F("Lat: "));
  Serial.print(latitude);

  int32_t longitude = myGPS.getLongitude();
  Serial.print(F(" Lon: "));
  Serial.print(longitude);
  Serial.print(F(" (degrees * 10^-7)"));

  int32_t altitude = myGPS.getAltitude();
  Serial.print(F(" Alt: "));
  Serial.print(altitude);
  Serial.println(F(" (mm)"));
}

//The Ublox library doesn't directly support SBAS control so let's do it manually
bool setSBAS(bool enableSBAS)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Set SBAS failed!"));
    return (false);
  }

  if (enableSBAS)
    customPayload[8 + 1 * 8] |= (1 << 0); //Set the enable bit
  else
    customPayload[8 + 1 * 8] &= ~(1 << 0); //Clear the enable bit

  // Now we write the custom packet back again to change the setting
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("SBAS setting failed!"));
    return (false);
  }

  return (true);
}
