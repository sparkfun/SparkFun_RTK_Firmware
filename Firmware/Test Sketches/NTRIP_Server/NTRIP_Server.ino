/*
   Push RTCM data to RTK2Go

   RTCM will come in over UART1 from ZED into ESP32
   ESP32 will read UART1 and then push over WiFi to RTK2Go

   For testing, the ZED is manually configured for:
     Permanent base location (immediate RTCM output)
     UART1 is enabled for RTCM output
     Sentences for RTCM are enabled for UART1
     Reduce fixes to 1Hz

  The I2C library won't see the UART1 RTCM bytes

*/
const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 1;

#define RTK_IDENTIFIER (FIRMWARE_VERSION_MAJOR * 0x10 + FIRMWARE_VERSION_MINOR)

#include <WiFi.h>

#include "secrets.h"

//Hardware serial buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HardwareSerial GPS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 16384 //Using a large buffer. This might be much bigger than needed but the ESP32 has enough RAM
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
//uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

#include "SparkFun_Ublox_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Connection settings to RTK2Go NTRIP Caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const uint16_t casterPort = 2101;
const char * casterHost = "rtk2go.com";
const char * ntrip_server_name = "SparkFun_RTK_Surveyor";

#define TOCAST_BUFFER_SIZE 512 * 4 //RTCM updates are ~300 bytes but can vary in size
char toCastBuffer[TOCAST_BUFFER_SIZE];
int castSpot = 0; //Tracks next available spot in cast buffer
bool toCastFrameComplete = false;
bool toCastFrameStale = false;

long lastReceivedRTCM_ms = 0; //5 RTCM messages take approximately ~300ms to arrive at 115200bps
long lastReceivedCastFrame_ms = 0; //Time of last complete frame
int maxTimeBeforeHangup_ms = 10000; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

uint32_t serverBytesSent = 0; //Just a running total
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  Serial.begin(115200);
  Serial.println("NTRIP testing");

  GPS.begin(115200); //The ZED-F9P will be configured to output RTCM over its UART1 at 115200bps into the ESP32.
  GPS.setRxBufferSize(SERIAL_SIZE_RX);
  GPS.setTimeout(1);

  Wire.begin(); //Start I2C

  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("u-blox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }
  Serial.println(F("u-blox module connected"));

  Serial.print("Connecting to local WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nWiFi connected with IP: ");
  Serial.println(WiFi.localIP());

  //Start the tasks for handling incoming and outgoing BT bytes to/from ZED-F9P
  xTaskCreate(F9PSerialReadTask, "F9Read", 10000, NULL, 0, NULL);
  //xTaskCreate(F9PSerialWriteTask, "F9Write", 10000, NULL, 0, NULL);

}

void loop()
{
  if (Serial.available()) menuMain(); //Present user menu

  myGPS.checkUblox(); //See if new data is available. Process bytes as they come in.

  delay(10);
}
