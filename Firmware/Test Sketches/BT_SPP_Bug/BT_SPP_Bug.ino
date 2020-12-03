/*
Demo code for reading solution and writing RTCM 
to the Sparkfun ZED-F9P receivers using Bluetooth and
USB Serial

Avinab Malla <avinabmalla@yahoo.com>
8 October 2020
*/

#include <Arduino.h>
#include <BluetoothSerial.h>


//Set the TX and RX pins for F9P UART1 here
#define F9RX 23
#define F9TX 22

//Using a large buffer. This might be much bigger than needed
//but the ESP32 has enough RAM
#define SERIAL_SIZE_RX 16384

//Set baudrate here. Configure the u-blox UART1 baudrate using u-center
#define SERIAL_BAUD 115200

HardwareSerial GPS(2);
BluetoothSerial SerialBT;

//Buffer for reading F9P
uint8_t rBuffer[SERIAL_SIZE_RX];

//Buffer for writing to F9P
uint8_t wBuffer[SERIAL_SIZE_RX];


//Task for writing to the GPS receiver
void F9PSerialWriteTask(void *e)
{
  while (true)
  {

    //Receive corrections from either the ESP32 USB or bluetooth
    //and write to the GPS
    if (Serial.available())
    {
      auto s = Serial.readBytes(wBuffer, SERIAL_SIZE_RX);
      GPS.write(wBuffer, s);
    }
    else if (SerialBT.connected() && SerialBT.available())
    {
      auto s = SerialBT.readBytes(wBuffer, SERIAL_SIZE_RX);
      GPS.write(wBuffer, s);
    }

    taskYIELD();
  }
}


//Task for reading data from the GPS receiver.
void F9PSerialReadTask(void *e)
{
  while (true)
  {
    //Read the GPS UART, and write to USB and bluetooth
    if (GPS.available())
    {
      auto s = GPS.readBytes(rBuffer, SERIAL_SIZE_RX);
      Serial.write(rBuffer, s);
      if (SerialBT.connected())
      {
        SerialBT.write(rBuffer, s);
      }
    }
    taskYIELD();
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  Serial.setRxBufferSize(SERIAL_SIZE_RX);
  Serial.setTimeout(1);

//  GPS.begin(SERIAL_BAUD, SERIAL_8N1, F9TX, F9RX);
  GPS.begin(115200);
  GPS.setRxBufferSize(SERIAL_SIZE_RX);
  GPS.setTimeout(1);

  Serial.println("Begin SPP testing");

  //Start Bluetooth Serial
  SerialBT.begin("GNSSTEST");

  //Start the tasks.
  //Can also use xTaskCreatePinnedToCore to pin a task to one of the two cores
  //on the ESP32
  xTaskCreate(F9PSerialReadTask, "F9Read", 10000, NULL, 0, NULL);
  xTaskCreate(F9PSerialWriteTask, "F9Write", 10000, NULL, 0, NULL);
}

void loop()
{
  //This task is not needed, delete it.
  vTaskDelete(NULL);
}
