/*
  RTK Surveyor Firmware - Test Sketch using u-blox GNSS Library v3 (I2C and SPI)
*/

#include <Wire.h> // Needed for I2C to GNSS
#include <SPI.h> // Needed for SPI to GNSS

#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3

// In this test, the GNSS object is instantiated inside the loop - to check for memory leaks

#define gnssSerial Serial1

// Define which hardware interface the module will use. This depends on the RTK hardware.
// Most are I2C. Reference Station is SPI. Future hardware _could_ be Serial...
typedef enum {
  RTK_GNSS_IS_I2C,
  RTK_GNSS_IS_SPI,
  RTK_GNSS_IS_SERIAL
} RTK_GNSS_INTERFACE;
RTK_GNSS_INTERFACE theGNSSinterface;

const int GNSS_SPI_CS = 4; // Chip select for the SPI interface - the "free" pin on Thing Plus C

void setup()
{
  delay(1000);

  Serial.begin(115200);
  Serial.println("RTK Surveyor Firmware - u-blox GNSS Test Sketch");

  theGNSSinterface = RTK_GNSS_IS_I2C; // Select I2C for this test

  // Prepare the correct hardware interface for GNSS comms
  
  if (theGNSSinterface == RTK_GNSS_IS_SERIAL)
  {
    gnssSerial.begin(38400);
  }
  else if (theGNSSinterface == RTK_GNSS_IS_SPI)
  {
    SPI.begin(); // Redundant - just to show what could be done here
  }
  else // if (theGNSSinterface == RTK_GNSS_IS_I2C) // Catch-all. Default to I2C
  {
    Wire.begin();
    //Wire.setClock(400000);
  }

  while (Serial.available()) // Make sure the Serial buffer is empty
    Serial.read();
}

void loop()
{
  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // Create an object of the GNSS super-class inside the loop - just as a test to check for memory leaks

  SFE_UBLOX_GNSS_SUPER theGNSS;

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  
  //theGNSS.enableDebugging(Serial); // Uncomment this line to enable debug messages

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // Now begin the GNSS
  
  bool beginSuccess = false;

  if (theGNSSinterface == RTK_GNSS_IS_SERIAL)
  {
    beginSuccess = theGNSS.begin(gnssSerial);
  }
  
  else if (theGNSSinterface == RTK_GNSS_IS_SPI)
  {
    beginSuccess = theGNSS.begin(SPI, GNSS_SPI_CS); // SPI, default to 4MHz
    
    //beginSuccess = theGNSS.begin(SPI, GNSS_SPI_CS, 4000000); // Custom
    
    //SPISettings customSPIsettings = SPISettings(4000000, MSBFIRST, SPI_MODE0);
    //beginSuccess = theGNSS.begin(SPI, GNSS_SPI_CS, customSPIsettings); // Custom
  }
  
  else // if (theGNSSinterface == RTK_GNSS_IS_I2C) // Catch-all. Default to I2C
  {
    beginSuccess = theGNSS.begin(); // Wire, 0x42
    
    //beginSuccess = theGNSS.begin(Wire, 0x42); // Custom
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // Check begin was successful
 
  if (!beginSuccess)
  {
    Serial.println(F("u-blox GNSS not detected. Please check wiring. Freezing."));
    while (1)
    {
      if (Serial.available())
        ESP.restart();
      delay(10);
    }
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // Disable the NMEA, just to show how to do it:
  
  if (theGNSSinterface == RTK_GNSS_IS_SERIAL)
  {
    // Assume we're connected to UART1. Could be UART2
    theGNSS.setUART1Output(COM_TYPE_UBX); //Set the UART1 port to output UBX only (turn off NMEA noise)
  }
  else if (theGNSSinterface == RTK_GNSS_IS_SPI)
  {
    theGNSS.setSPIOutput(COM_TYPE_UBX); //Set the SPI port to output UBX only (turn off NMEA noise)
  }
  else // if (theGNSSinterface == RTK_GNSS_IS_I2C) // Catch-all. Default to I2C
  {
    theGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //Query module info

  if (theGNSS.getModuleInfo())
  {
      Serial.print(F("FWVER: "));
      Serial.print(theGNSS.getFirmwareVersionHigh());
      Serial.print(F("."));
      Serial.println(theGNSS.getFirmwareVersionLow());
      
      Serial.print(F("Firmware: "));
      Serial.println(theGNSS.getFirmwareType());    

      Serial.print(F("PROTVER: "));
      Serial.print(theGNSS.getProtocolVersionHigh());
      Serial.print(F("."));
      Serial.println(theGNSS.getProtocolVersionLow());
      
      Serial.print(F("MOD: "));
      Serial.println(theGNSS.getModuleName());    
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //Query module for HPPOSLLH data

  if (theGNSS.getHPPOSLLH()) // Returns true when fresh data is available
  {
    int32_t latitude = theGNSS.getHighResLatitude();
    int8_t latitudeHp = theGNSS.getHighResLatitudeHp();
    int32_t longitude = theGNSS.getHighResLongitude();
    int8_t longitudeHp = theGNSS.getHighResLongitudeHp();
    int32_t ellipsoid = theGNSS.getElipsoid();
    int8_t ellipsoidHp = theGNSS.getElipsoidHp();
    int32_t msl = theGNSS.getMeanSeaLevel();
    int8_t mslHp = theGNSS.getMeanSeaLevelHp();
    uint32_t accuracy = theGNSS.getHorizontalAccuracy();

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

    Serial.print(", Accuracy (m): ");
    Serial.print(f_accuracy, 4); // Print the accuracy with 4 decimal places

    Serial.print(", Altitude (m): ");
    Serial.print(f_ellipsoid, 4); // Print the ellipsoid with 4 decimal places

    Serial.println();
  }

  if (Serial.available())
    ESP.restart();
}
