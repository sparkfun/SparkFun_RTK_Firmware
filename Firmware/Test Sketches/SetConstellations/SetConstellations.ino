/*
  Set correct bits to enable/disable constallations on the ZED-F9P

  See -CFG-GNSS in ZED's protocol doc

  21:20:31  0000  B5 62 06 3E (len)34 00(len)
  00(ver) 3C(tracking chan) 3C(chan) 06(config blocks)
  00(gnssid) 08 10 00 01(enable) 00 11(sigCfgMask) 11 GPS
  01 03 03 00 01 00 01 01 SBAS
  02 0A 12 00 01 00 21 21 Gal
  03 02 05 00 01 00 11 11 Bei
  05 00 04 00 01 00 11 15 QZSS <-- Note 04 IMES is missing
  06 08 0C 00 01 00 11 11 GLO
  Above is poll of default config from u-center

  21:20:31  0000  B5 62 06 3E 34 00
  00 00 3C 06
  00 08 10 00 00 00 11 01 GPS
  01 03 03 00 01 00 01 01 SBAS
  02 0A 12 00 01 00 21 01 Gal
  03 02 05 00 01 00 11 01 Bei
  05 00 04 00 00 00 11 01 QZSS
  06 08 0C 00 01 00 11 01 GLO
  Above is command for GPS and QZSS turned off

  00 3C 3C 05
  00 08 10 00 01 00 11 11
  02 0A 12 00 00 00 21 21
  03 04 05 00 00 00 11 11
  05 00 04 00 01 00 11 11
  06 08 0C 00 00 00 11 11
  00 00 00 00 00 00 00 00
  Above, on ZED-F9P v1.12, BeiDou is disabled. Why is SBAS not being reported?
  Ah, it's a v1.12 bug. Works fine in v1.13 and on ZED-F9R v1.0

  Ugh. The issue is that the doc says IMES is gnssid 4 but really QZSS is in 4th position but with ID 5.

      Works:
      GPS
      GLONASS
      SBAS
      Galileo
      BeiDou
      QZSS

      Not working:
      IMES - Does not seem to be supported

*/

#include <Wire.h> //Needed for I2C to GPS

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS i2cGNSS;

#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

void setup()
{
  Serial.begin(115200);
  delay(500); //Wait for ESP32
  Serial.println("SparkFun u-blox Example");

  Wire.begin();
  Wire.setClock(400000);

  //i2cGNSS.enableDebugging(); // Uncomment this line to enable debug messages

  if (i2cGNSS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("u-blox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'G')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_GPS, true) == false)
        Serial.println("Enable GPS Fail");
      else
        Serial.println("Enable GPS Success");
    }
    else if (incoming == 'g')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_GPS, false) == false)
        Serial.println("Disable GPS Fail");
      else
        Serial.println("Disable GPS Success");
    }
    else if (incoming == 'R')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_GLONASS, true) == false)
        Serial.println("Enable GLONASS Fail");
      else
        Serial.println("Enable GLONASS Success");
    }
    else if (incoming == 'r')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_GLONASS, false) == false)
        Serial.println("Disable GLONASS Fail");
      else
        Serial.println("Disable GLONASS Success");
    }
    else if (incoming == 'S')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_SBAS, true) == false)
        Serial.println("Enable SBAS Fail");
      else
        Serial.println("Enable SBAS Success");
    }
    else if (incoming == 's')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_SBAS, false) == false)
        Serial.println("Disable SBAS Fail");
      else
        Serial.println("Disable SBAS Success");
    }
    else if (incoming == 'A')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_GALILEO, true) == false)
        Serial.println("Enable Galileo Fail");
      else
        Serial.println("Enable Galileo Success");
    }
    else if (incoming == 'a')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_GALILEO, false) == false)
        Serial.println("Disable Galileo Fail");
      else
        Serial.println("Disable Galileo Success");
    }
    else if (incoming == 'B')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_BEIDOU, true) == false)
        Serial.println("Enable BeiDou Fail");
      else
        Serial.println("Enable BeiDou Success");
    }
    else if (incoming == 'b')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_BEIDOU, false) == false)
        Serial.println("Disable BeiDou Fail");
      else
        Serial.println("Disable BeiDou Success");
    }
    else if (incoming == 'Q')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_QZSS, true) == false)
        Serial.println("Enable QZSS Fail");
      else
        Serial.println("Enable QZSS Success");
    }
    else if (incoming == 'q')
    {
      if (setConstellation(SFE_UBLOX_GNSS_ID_QZSS, false) == false)
        Serial.println("Disable QZSS Fail");
      else
        Serial.println("Disable QZSS Success");
    }
    else if(incoming == '!')
    {
    for(int x = 0 ; x < 6 ; x++)
    {
      bool isEnabled = false;
      if(x == 0) isEnabled = getConstellation(SFE_UBLOX_GNSS_ID_GPS);
      else if(x == 1) isEnabled = getConstellation(SFE_UBLOX_GNSS_ID_SBAS);
      else if(x == 2) isEnabled = getConstellation(SFE_UBLOX_GNSS_ID_GALILEO);
      else if(x == 3) isEnabled = getConstellation(SFE_UBLOX_GNSS_ID_BEIDOU);
      else if(x == 4) isEnabled = getConstellation(SFE_UBLOX_GNSS_ID_QZSS);
      else if(x == 5) isEnabled = getConstellation(SFE_UBLOX_GNSS_ID_GLONASS);

      Serial.print("Module reports ");
      if(x == 0) Serial.print("GPS");
      else if(x == 1) Serial.print("SBAS");
      else if(x == 2) Serial.print("GALILEO");
      else if(x == 3) Serial.print("BeiDou");
      else if(x == 4) Serial.print("QZSS");
      else if(x == 5) Serial.print("GLONASS");
      Serial.print(": ");
      if (isEnabled == true)
        Serial.println("Enabled");
      else
        Serial.println("Disabled");
    }      
    }
    else if(incoming == '\n' || incoming == '\r')
    {
      //Do nothing
    }
    else
    {
      //Serial.println("Unknown");
    }
   
  }

}

//The u-blox library doesn't directly support constellation control so let's do it manually
//Also allows the enable/disable of any constellation (BeiDou, Galileo, etc)
bool setConstellation(uint8_t constellation, bool enable)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Set Constellation failed"));
    return (false);
  }

  if (enable)
  {
    if (constellation == SFE_UBLOX_GNSS_ID_GPS || constellation == SFE_UBLOX_GNSS_ID_QZSS)
    {
      //QZSS must follow GPS
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_GPS) + 4] |= (1 << 0); //Set the enable bit
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 4] |= (1 << 0); //Set the enable bit
    }
    else
    {
      customPayload[locateGNSSID(customPayload, constellation) + 4] |= (1 << 0); //Set the enable bit
    }

    //Set sigCfgMask as well
    if (constellation == SFE_UBLOX_GNSS_ID_GPS || constellation == SFE_UBLOX_GNSS_ID_QZSS)
    {
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_GPS) + 6] |= 0x11; //Enable GPS L1C/A, and L2C

      //QZSS must follow GPS
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 6] = 0x11; //Enable QZSS L1C/A, and L2C - Follow u-center
      //customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 6] = 0x15; //Enable QZSS L1C/A, L1S, and L2C
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_SBAS)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x01; //Enable SBAS L1C/A
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_GALILEO)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x21; //Enable Galileo E1/E5b
    }
    else if (constellation == SFE_UBLOX_GNSS_ID_BEIDOU)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x11; //Enable BeiDou B1I/B2I
    }
    //    else if (constellation == SFE_UBLOX_GNSS_ID_IMES) //Does not exist in u-center v21.02
    //    {
    //      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x01; //Enable IMES L1
    //    }
    else if (constellation == SFE_UBLOX_GNSS_ID_GLONASS)
    {
      customPayload[locateGNSSID(customPayload, constellation) + 6] |= 0x11; //Enable GLONASS L1 and L2
    }
  }
  else //Disable
  {
    //QZSS must follow GPS
    if (constellation == SFE_UBLOX_GNSS_ID_GPS || constellation == SFE_UBLOX_GNSS_ID_QZSS)
    {
      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_GPS) + 4] &= ~(1 << 0); //Clear the enable bit

      customPayload[locateGNSSID(customPayload, SFE_UBLOX_GNSS_ID_QZSS) + 4] &= ~(1 << 0); //Clear the enable bit
    }
    else
    {
      customPayload[locateGNSSID(customPayload, constellation) + 4] &= ~(1 << 0); //Clear the enable bit
    }

  }

  Serial.println("Custom payload:");
  for (int x = 0 ; x < 4 + 8 * 6 ; x++)
  {
    if (x == 4 + 8 * 0) Serial.println(); //Hdr
    if (x == 4 + 8 * 1) Serial.println(); //GPS
    if (x == 4 + 8 * 2) Serial.println();
    if (x == 4 + 8 * 3) Serial.println();
    if (x == 4 + 8 * 4) Serial.println();
    if (x == 4 + 8 * 5) Serial.println();
    if (x == 4 + 8 * 6) Serial.println();
    if (x == 4 + 8 * 7) Serial.println();
    Serial.print(" ");
    if (customPayload[x] < 0x10) Serial.print("0");
    Serial.print(customPayload[x], HEX);
  }
  Serial.println();

  // Now we write the custom packet back again to change the setting
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("Constellation setting failed"));
    return (false);
  }

  return (true);
}

//Given a payload, return the location of a given constellation
//This is needed because IMES is not currently returned in the query packet
//so QZSS and GLONAS are offset by -8 bytes.
uint8_t locateGNSSID(uint8_t *customPayload, uint8_t constellation)
{
  for (int x = 0 ; x < 7 ; x++) //Assume max of 7 constellations
  {
    if (customPayload[4 + 8 * x] == constellation) //Test gnssid
      return (4 + x * 8);
  }

  Serial.println(F("locateGNSSID failed"));
  return (0);
}

//Returns true if constellation is enabled
bool getConstellation(uint8_t constellationID)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_GNSS; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero lets us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Get Constellation failed"));
    return (false);
  }

  if (customPayload[locateGNSSID(customPayload, constellationID) + 4] & (1 << 0)) return true; //Check if bit 0 is set
  return false;
}
