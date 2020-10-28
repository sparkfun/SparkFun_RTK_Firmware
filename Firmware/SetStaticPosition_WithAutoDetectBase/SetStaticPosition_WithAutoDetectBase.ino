/*
  Module Info - extracts and prints the full module information from UBX_MON_VER
  using a custom command.
  By: @mayopan
  Date: May 9th, 2020

  Based on:
  Send Custom Command
  By: Paul Clark (PaulZC)
  Date: April 20th, 2020

  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  Previously it was possible to create and send a custom packet
  through the library but it would always appear to timeout as
  some of the internal functions referred to the internal private
  struct packetCfg.
  The most recent version of the library allows sendCommand to
  use a custom packet as if it were packetCfg and so:
  - sendCommand will return a sfe_ublox_status_e enum as if
    it had been called from within the library
  - the custom packet will be updated with data returned by the module
    (previously this was not possible from outside the library)

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/15136
  NEO-M8P RTK: https://www.sparkfun.com/products/15005
  SAM-M8Q: https://www.sparkfun.com/products/15106

  Hardware Connections:
  Plug a Qwiic cable into the GPS and a BlackBoard
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

#include <Wire.h> //Needed for I2C to GPS

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS

// Extend the class for getModuleInfo
class SFE_UBLOX_GPS_ADD : public SFE_UBLOX_GPS
{
  public:
    bool setStaticPosition(int32_t, int32_t, int32_t, uint16_t maxWait = 1100);
    bool setStaticPosition(int32_t, int8_t, int32_t, int8_t, int32_t, int8_t, uint16_t maxWait = 1100);
};

SFE_UBLOX_GPS_ADD myGPS;

void setup()
{
  Serial.begin(115200); // You may need to increase this for high navigation rates!
  while (!Serial)
    ; //Wait for user to open terminal
  Serial.println(F("SparkFun Ublox Example"));

  Wire.begin();

  //myGPS.enableDebugging(); // Uncomment this line to enable debug messages

  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

  //TODO Insert SparkFun coords

  //Units are cm so 1234 = 12.34m
  //myGPS.setStaticPosition(-128960825, -471964628, 408056543);

  //Units are cm with a high precision extension so -1234.5678 should be called: (-123456, -78) 
  myGPS.setStaticPosition(-128960825, -34, -471964628, -91, 408056543, 13); //With high precision 0.1mm parts

  Serial.println(F("Done!"));
}

void loop()
{
}

bool SFE_UBLOX_GPS_ADD::setStaticPosition(int32_t ecefX, int8_t ecefXHP, int32_t ecefY, int8_t ecefYHP, int32_t ecefZ, int8_t ecefZHP, uint16_t maxWait)
{
  // Let's create our custom packet
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

  // The next line creates and initialises the packet information which wraps around the payload
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_TMODE3;    // This is the message ID
  customCfg.len = 0;             // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0;    // Always set the startingSpot to zero (unless you really know what you are doing)

  //Ask module for the current TimeMode3 settings. Loads into payloadCfg.
  if (sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
    return (false); //If command send fails then bail

  customCfg.len = 40;             // Setting the len (length) to zero let's us poll the current settings

  //Clear packet payload
  for (uint8_t x = 0; x < customCfg.len; x++)
   customPayload[x] = 0;

  //customCfg should be loaded with poll response. Now modify only the bits we care about
  customPayload[2] = 2; //Set mode to fixed. Use ECEF (not LAT/LON/ALT).
  //customPayload[2] = 2 | (1<<8); //Set mode to fixed. Use LAT/LON/ALT.

  //Set ECEF X
  customPayload[4] = (ecefX >> 8 * 0) & 0xFF; //LSB
  customPayload[5] = (ecefX >> 8 * 1) & 0xFF;
  customPayload[6] = (ecefX >> 8 * 2) & 0xFF;
  customPayload[7] = (ecefX >> 8 * 3) & 0xFF; //MSB

  //Set ECEF Y
  customPayload[8] = (ecefY >> 8 * 0) & 0xFF; //LSB
  customPayload[9] = (ecefY >> 8 * 1) & 0xFF;
  customPayload[10] = (ecefY >> 8 * 2) & 0xFF;
  customPayload[11] = (ecefY >> 8 * 3) & 0xFF; //MSB

  //Set ECEF Z
  customPayload[12] = (ecefZ >> 8 * 0) & 0xFF; //LSB
  customPayload[13] = (ecefZ >> 8 * 1) & 0xFF;
  customPayload[14] = (ecefZ >> 8 * 2) & 0xFF;
  customPayload[15] = (ecefZ >> 8 * 3) & 0xFF; //MSB

  //Set ECEF HP
  customPayload[16] = ecefXHP;
  customPayload[17] = ecefYHP;
  customPayload[18] = ecefZHP;
  

  return ((sendCommand(&customCfg, maxWait)) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

bool SFE_UBLOX_GPS_ADD::setStaticPosition(int32_t ecefX, int32_t ecefY, int32_t ecefZ, uint16_t maxWait)
{
  return(setStaticPosition(ecefX, 0, ecefY, 0, ecefZ, 0, maxWait));
}
