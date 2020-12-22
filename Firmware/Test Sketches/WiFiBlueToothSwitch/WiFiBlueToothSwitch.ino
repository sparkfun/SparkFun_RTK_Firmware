// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Sketch shows how to switch between WiFi and BlueTooth or use both
// Button is attached between GPIO 0 and GND and modes are switched with each press

//This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
struct struct_settings {
  bool enableNtripServer = false;
  char casterHost[50] = "rtk2go.com";
  uint16_t casterPort = 2101;
  char mountPoint[50] = "bldr_dwntwn2";
  char mountPointPW[50] = "WR5wRo4H";
  char wifiSSID[50] = "sparkfun-guest";
  char wifiPW[50] = "sparkfun6333";
} settings;

//Connection settings to NTRIP Caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <WiFi.h>
WiFiClient caster;
const char * ntrip_server_name = "SparkFun_RTK_Surveyor";

unsigned long lastServerSent_ms = 0; //Time of last data pushed to caster
unsigned long lastServerReport_ms = 0; //Time of last report of caster bytes sent
int maxTimeBeforeHangup_ms = 10000; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

uint32_t serverBytesSent = 0; //Just a running total
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

HardwareSerial GPS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 16384 //Using a large buffer. This might be much bigger than needed but the ESP32 has enough RAM
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
TaskHandle_t F9PSerialWriteTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <Wire.h> //Needed for I2C to GNSS

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

#include "SparkFun_Ublox_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


char deviceName[20]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'

#define AP_SSID  "esp32"

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_AP_START:
      Serial.println("AP Started");
      WiFi.softAPsetHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("AP Stopped");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi STA Started");
      WiFi.setHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi STA Connected");
      WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      Serial.print("WiFi STA IPv6: ");
      Serial.println(WiFi.localIPv6());
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("WiFi STA IPv4: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi STA Disconnected");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi STA Stopped");
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin();

  beginGNSS();

  WiFi.onEvent(WiFiEvent);

  Serial.print("ESP32 SDK: ");
  Serial.println(ESP.getSdkVersion());
  Serial.println("Press the button to select the next mode");
}

void loop() {

  if (Serial.available())
  {
    byte incoming = Serial.read();

    Serial.println();
    Serial.println("1) Start Bluetooth");
    Serial.println("2) Start Wifi");

    if (incoming == '1')
    {
      Serial.println("Starting BT");
      startBluetooth();
    }
    else if (incoming == '2')
    {
      Serial.println("Stopping BT");
      btStop();

      updateNtripServer();
    }

  }

  delay(100);

  if (caster.connected() == true)
  {
    myGPS.checkUblox();

    //Close socket if we don't have new data for 10s
    //RTK2Go will ban your IP address if you abuse it. See http://www.rtk2go.com/how-to-get-your-ip-banned/
    //So let's not leave the socket open/hanging without data
    if (millis() - lastServerSent_ms > maxTimeBeforeHangup_ms)
    {
      Serial.println(F("RTCM timeout. Disconnecting from caster."));
      caster.stop();
    }

    //Report some statistics every 250
    if (millis() - lastServerReport_ms > 250)
    {
      lastServerReport_ms += 250;
      Serial.printf("Total bytes sent to caster: %d\n", serverBytesSent);
    }
  }
}

//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool startBluetooth()
{
  if (caster.connected()) caster.stop();

  //Make sure WiFi is off
  //  if (WiFi.getMode() != WIFI_MODE_NULL)
  //  {
  WiFi.mode(WIFI_OFF);
  delay(200);
  //  }


  btStart();

  if (SerialBT.begin("Surveyor Name") == false)
    return (false);
  Serial.print("Bluetooth broadcasting as: ");
  Serial.println("Surveyor Name");

  //Start the tasks for handling incoming and outgoing BT bytes to/from ZED-F9P
  //  xTaskCreate(F9PSerialReadTask, "F9Read", 10000, NULL, 0, &F9PSerialReadTaskHandle);
  //  xTaskCreate(F9PSerialWriteTask, "F9Write", 10000, NULL, 0, &F9PSerialWriteTaskHandle);
  //
  //  SerialBT.setTimeout(1);

  return (true);
}

//Call regularly to get data from u-blox module and send out over local WiFi
//We make sure we are connected to WiFi, then
bool updateNtripServer()
{
  //Is WiFi setup?
  if (WiFi.isConnected() == false)
  {
    //Turn off Bluetooth and turn on WiFi
    endBluetooth();

    Serial.printf("Connecting to local WiFi: %s\n", settings.wifiSSID);
    WiFi.begin(settings.wifiSSID, settings.wifiPW);

    int maxTime = 10000;
    long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(F("."));

      if (millis() - startTime > maxTime)
      {
        Serial.println(F("Failed to connect to WiFi. Are you sure your WiFi credentials are correct?"));
        return (false);
      }
    }
  } //End WiFi connect check

  //Are we connected to caster?
  if (caster.connected() == false)
  {
    Serial.printf("Opening socket to %s\n", settings.casterHost);

    if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
    {
      Serial.printf("Connected to %s:%d\n", settings.casterHost, settings.casterPort);

      const int SERVER_BUFFER_SIZE  = 512;
      char serverBuffer[SERVER_BUFFER_SIZE];

      snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/%s\r\n\r\n",
               settings.mountPointPW, settings.mountPoint, ntrip_server_name, "App Version 1.0");

      Serial.printf("Sending credentials:\n%s\n", serverBuffer);
      caster.write(serverBuffer, strlen(serverBuffer));

      //Wait for response
      unsigned long timeout = millis();
      while (caster.available() == 0)
      {
        if (millis() - timeout > 5000)
        {
          Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
          caster.stop();
          return (false);
        }
        delay(10);
      }

      delay(10); //Yield to RTOS

      //Check reply
      bool connectionSuccess = false;
      char response[512];
      int responseSpot = 0;
      while (caster.available())
      {
        response[responseSpot++] = caster.read();
        if (strstr(response, "200") > 0) //Look for 'ICY 200 OK'
          connectionSuccess = true;
        if (responseSpot == 512 - 1) break;
      }
      response[responseSpot] = '\0';

      Serial.printf("Caster responded with: %s\n", response);

      if (connectionSuccess == false)
      {
        Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?\n", response);
      }
      else
      {
        //We're connected!
        Serial.println(F("Connected to caster"));

        //Reset flags
        lastServerReport_ms = millis();
        lastServerSent_ms = millis();
        serverBytesSent = 0;
      }
    } //End attempt to connect
    else
    {
      Serial.println(F("Failed to connect to caster. Is your caster host and port correct?"));
      delay(10); //Give RTOS some time
      return (false);
    }
  } //End connected == false

  Serial.println("Begin pushing");

  return (true);
}

//This function gets called from the SparkFun u-blox Arduino Library.
//As each RTCM byte comes in you can specify what to do with it
//Useful for passing the RTCM correction data to a radio, Ntrip broadcaster, etc.
void SFE_UBLOX_GPS::processRTCM(uint8_t incoming)
{
  if (caster.connected() == true)
  {
    caster.write(incoming); //Send this byte to socket
    serverBytesSent++;
    lastServerSent_ms = millis();
  }
}

bool endBluetooth()
{
  //Kill tasks if running
  if (F9PSerialReadTaskHandle != NULL)
    vTaskDelete(F9PSerialReadTaskHandle);
  if (F9PSerialWriteTaskHandle != NULL)
    vTaskDelete(F9PSerialWriteTaskHandle);

  SerialBT.end();
  btStop();

  Serial.println(F("Bluetooth turned off"));
}

//If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
//Task for writing to the GNSS receiver
void F9PSerialWriteTask(void *e)
{
  while (true)
  {

    if (SerialBT.available())
    {
      while (SerialBT.available())
      {
        //Pass bytes to GNSS receiver
        auto s = SerialBT.readBytes(wBuffer, SERIAL_SIZE_RX);
        GPS.write(wBuffer, s);
      }
    }

    taskYIELD();
  }
}

//If the ZED has any new NMEA data, pass it out over Bluetooth
//Task for reading data from the GNSS receiver.
void F9PSerialReadTask(void *e)
{
  while (true)
  {
    if (GPS.available())
    {
      auto s = GPS.readBytes(rBuffer, SERIAL_SIZE_RX);

      if (SerialBT.connected())
      {
        SerialBT.write(rBuffer, s);
      }
    }
    taskYIELD();
  }
}

//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (myGPS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGPS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      while (1);
    }
  }

  Serial.println(F("GNSS configuration complete"));
}
