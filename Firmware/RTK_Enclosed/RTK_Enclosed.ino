/*
  September 1st, 2020
  SparkFun Electronics
  Nathan Seidle

  This firmware runs the core of the SparkFun RTK Surveyor product. It runs on an ESP32
  and communicates with the ZED-F9P.
*/

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

//Hardware connections v10
const int statLED = 13;
const int positionAccuracyLED_20mm = 13; //POSACC1
const int positionAccuracyLED_100mm = 15; //POSACC2
const int positionAccuracyLED_1000mm = 2; //POSACC3
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 13;

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
  BASE_SURVEYING_IN,
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
  Serial1.begin(115200); //UART1 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at 115200bps.
  Serial.println("SparkFun RTK Surveyor v1.0");

  pinMode(statLED, OUTPUT);
  pinMode(positionAccuracyLED_20mm, OUTPUT);
  pinMode(positionAccuracyLED_100mm, OUTPUT);
  pinMode(positionAccuracyLED_1000mm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP);

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
    SerialBT.write(Serial1.read());
    yield();

    if (digitalRead(bluetoothStatusLED) == LOW)
      digitalWrite(bluetoothStatusLED, HIGH);
    else
      digitalWrite(bluetoothStatusLED, LOW);
  }

  //  }
  //  else
  //  {
  //    Serial.println("No BT");
  //    for (int x = 0 ; x < 1000 ; x++)
  //    {
  //      delay(1);
  //      yield();
  //    }
  //    //delay(1000);
  //  }
  delay(1);

}
