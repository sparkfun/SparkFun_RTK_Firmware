/*
  September 1st, 2020
  SparkFun Electronics
  Nathan Seidle

  This firmware runs the core of the SparkFun RTK Surveyor product. It runs on an ESP32
  and communicates with the ZED-F9P.

  Select the ESP32 Dev Module from the boards list. This maps the same pins to the ESP32-WROOM module.

  -Bluetooth NMEA works but switch label is reverse
  Fix BT reconnect failure
  -Implement survey in and out based on switch
  (Done) Try better survey in setup: https://portal.u-blox.com/s/question/0D52p00009IsVoMCAV/restarting-surveyin-on-an-f9p
  Startup sequence LEDs like a car (all on, then count down quickly)
  Make the Base LED blink faster/slower (for real) depending on base accuracy
  (Done) Enable the Avinab suggested NMEA sentences:
  - Enable GST
  - Enable the high accuracy mode and NMEA 4.0+ in the NMEA configuration
  - Set the satellite numbering to Extended (3 Digits) for full skyplot support
  Add ESP32 ID to end of Bluetooth broadcast ID

  Can we add NTRIP reception over Wifi to the ESP32 to aid in survey in time?

*/

#define RXD2 16
#define TXD2 17

#include <Wire.h> //Needed for I2C to GPS

#include "SparkFun_Ublox_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

//Hardware connections v10
const int positionAccuracyLED_20mm = 13; //POSACC1
const int positionAccuracyLED_100mm = 15; //POSACC2
const int positionAccuracyLED_1000mm = 2; //POSACC3
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 12;

//Bluetooth status LED goes from off (LED off), no connection (blinking), to connected (solid)
enum BluetoothState
{
  BT_OFF = 0,
  BT_ON_NOCONNECTION,
  BT_CONNECTED,
};
volatile byte bluetoothState = BT_OFF;

//Base status goes from Rover-Mode (LED off), surveying in (blinking), to survey is complete/trasmitting RTCM (solid)
enum BaseState
{
  BASE_OFF = 0,
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
};
volatile byte baseState = BASE_OFF;

uint32_t lastBluetoothLEDBlink = 0;
uint32_t lastTime = 0;

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected");
    bluetoothState = BT_CONNECTED;
    digitalWrite(bluetoothStatusLED, HIGH);
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("Client disconnected");
    bluetoothState = BT_ON_NOCONNECTION;
    digitalWrite(bluetoothStatusLED, LOW);
    lastBluetoothLEDBlink = millis();
  }

  //  if (event == ESP_SPP_WRITE_EVT ) {
  //    Serial.println("W");
  //    if(Serial1.available()) SerialBT.write(Serial1.read());
  //  }
}

void setup()
{
  Serial.begin(115200); //UART0 for programming and debugging
  //Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(115200); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at 115200bps.
  Serial.println("SparkFun RTK Surveyor v1.0");

  pinMode(positionAccuracyLED_20mm, OUTPUT);
  pinMode(positionAccuracyLED_100mm, OUTPUT);
  pinMode(positionAccuracyLED_1000mm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  digitalWrite(positionAccuracyLED_20mm, LOW);
  digitalWrite(positionAccuracyLED_100mm, LOW);
  digitalWrite(positionAccuracyLED_1000mm, LOW);
  digitalWrite(baseStatusLED, LOW);
  digitalWrite(bluetoothStatusLED, LOW);

  Wire.begin();
  if (myGPS.begin() == false)
  {
    //Try again with power on delay
    delay(1500); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGPS.begin() == false)
      Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring."));
    else
      Serial.println(F("Ublox GPS detected"));
  }
  else
    Serial.println(F("Ublox GPS detected"));

  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure module. Trying again."));
    delay(2000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure module. Power cycle? Freezing..."));
      while (1);
    }
  }
  Serial.println(F("GPS configuration complete"));

  SerialBT.register_callback(callback);
  if (!SerialBT.begin("SparkFun RTK Surveyor"))
  {
    Serial.println("An error occurred initializing Bluetooth");
    bluetoothState = BT_OFF;
    digitalWrite(bluetoothStatusLED, LOW);
  }
  else
  {
    Serial.println("Bluetooth initialized");
    bluetoothState = BT_ON_NOCONNECTION;
    digitalWrite(bluetoothStatusLED, HIGH);
    lastBluetoothLEDBlink = millis();
  }
}

void loop()
{
  //Update Bluetooth LED status
  if (bluetoothState == BT_ON_NOCONNECTION)
  {
    if (millis() - lastBluetoothLEDBlink > 500)
    {
      if (digitalRead(bluetoothStatusLED) == LOW)
        digitalWrite(bluetoothStatusLED, HIGH);
      else
        digitalWrite(bluetoothStatusLED, LOW);
      lastBluetoothLEDBlink = millis();
    }
  }

  if (bluetoothState == BT_CONNECTED)
  {
    yield();

    //If the ZED has any new NMEA data, pass it out over Bluetooth
    if (Serial2.available())
      SerialBT.write(Serial2.read());

    yield();

    //If the phone has any new data (NTRIP RTCM, etc), pass it out over Bluetooth
    if (SerialBT.available())
      Serial2.write(SerialBT.read());

    if (digitalRead(bluetoothStatusLED) == LOW)
      digitalWrite(bluetoothStatusLED, HIGH);
    else
      digitalWrite(bluetoothStatusLED, LOW);
  }

  //delay(1);
}

//Setup the Ublox module
bool configureUbloxModule()
{
  boolean response = true;

  myGPS.setSerialRate(115200, 1); //Set UART1 to 115200

  //UART1 will primarily be used to pass NMEA from ZED to ESP32/Cell phone but the phone
  //can also provide RTCM data. So let's be sure to enable RTCM on UART1 input.
  response &= myGPS.setPortOutput(COM_PORT_UART1, COM_TYPE_NMEA); //Set the UART1 to output NMEA
  response &= myGPS.setPortInput(COM_PORT_UART1, COM_TYPE_RTCM3); //Set the UART1 to input RTCM

  //Disable SPI port - This is just to remove some overhead by ZED
  response &= myGPS.setPortOutput(COM_PORT_SPI, 0); //Disable all protocols
  response &= myGPS.setPortInput(COM_PORT_SPI, 0); //Disable all protocols

  myGPS.setSerialRate(57600, 2); //Set UART2 to 57600 to match SiK firmware default
  response &= myGPS.setPortOutput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to output RTCM (in case this device goes into base mode)
  response &= myGPS.setPortInput(COM_PORT_UART2, COM_TYPE_RTCM3); //Set the UART2 to input RTCM

  response &= myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

  //myGPS.setNavigationFrequency(10); //Set output to 10 times a second
  response &= myGPS.setNavigationFrequency(4); //Set output in Hz

  //Make sure the appropriate sentences are enabled
  response &= myGPS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1);
  response &= myGPS.enableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1);
  response &= myGPS.enableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1);
  response &= myGPS.enableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1);
  response &= myGPS.enableNMEAMessage(UBX_NMEA_GST, COM_PORT_UART1); //Enable extended NMEA sentences to better support SW Maps

  response &= myGPS.setAutoPVT(true); //Tell the GPS to "send" each solution

  if (response == false)
  {
    Serial.println(F("Module failed initial config."));
    return (false);
  }

  //Check rover switch and configure module accordingly
  //When switch is set to '1' = BASE, pin will be shorted to ground
  if (digitalRead(baseSwitch) == HIGH)
  {
    //Configure for rover mode
    if (configureUbloxModuleRover() == false)
    {
      Serial.println("Rover config failed!");
      return (false);
    }
  }
  else
  {
    //Configure for base mode
    if (configureUbloxModuleBase() == false)
    {
      Serial.println("Base config failed!");
      return (false);
    }
  }

  response &= myGPS.saveConfiguration(); //Save the current settings to flash and BBR

  if (response == false)
  {
    Serial.println(F("Module failed to save."));
    return (false);
  }

  return (true);
}

//Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
  return (setNMEASettings());
}

//Configure specific aspects of the receiver for base mode
bool configureUbloxModuleBase()
{
  bool response;

  response &= myGPS.setDynamicModel(DYN_MODEL_STATIONARY); // Set the dynamic model to STATIONARY
  if (response == false) // Set the dynamic model to STATIONARY
  {
    Serial.println(F("Warning: setDynamicModel failed!"));
    return (false);
  }

  // Let's read the new dynamic model to see if it worked
  uint8_t newDynamicModel = myGPS.getDynamicModel();
  if (newDynamicModel != 2)
  {
    Serial.println(F("setDynamicModel failed!"));
    return (false);
  }

  response &= myGPS.enableRTCMmessage(UBX_RTCM_1005, COM_PORT_UART2, 1); //Enable message 1005 to output through UART2, message every second
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1074, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1084, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1094, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1124, COM_PORT_UART2, 1);
  response &= myGPS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 10); //Enable message every 10 seconds

  if (response == false)
  {
    Serial.println(F("RTCM failed to enable."));
    return (false);
  }

  return (response);
}

//The Ublox library doesn't directly support NMEA configuration so let's do it manually
bool setNMEASettings()
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_NMEA; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  // Read the current setting. The results will be loaded into customCfg.
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("NMEA setting failed!"));
    return (false);
  }

  customPayload[3] |= (1 << 3); //Set the highPrec flag

  customPayload[8] = 1; //Enable extended satellite numbering

  // Now we write the custom packet back again to change the setting
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // This time we are only expecting an ACK
  {
    Serial.println(F("NMEA setting failed!"));
    return (false);
  }
  return (true);
}
