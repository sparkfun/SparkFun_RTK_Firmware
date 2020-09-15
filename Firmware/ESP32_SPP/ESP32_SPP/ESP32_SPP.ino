#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

//Bluetooth status LED goes from off (LED off), no connection (blinking), to connected (solid)
enum BluetoothState
{
  BT_OFF = 0,
  BT_ON_NOCONNECTION,
  BT_CONNECTED,
};
volatile byte bluetoothState = BT_OFF;

const int bluetoothStatusLED = 12;
uint32_t lastBluetoothLEDBlink = 0;

//uint32_t lastTime = 0;

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected");
    bluetoothState = BT_CONNECTED;
    digitalWrite(bluetoothStatusLED, HIGH);

  Serial2.begin(115200); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at 115200bps.
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("Client disconnected");
    bluetoothState = BT_ON_NOCONNECTION;
    digitalWrite(bluetoothStatusLED, LOW);
    lastBluetoothLEDBlink = millis();
    Serial2.end();
  }

  //    if (event == ESP_SPP_WRITE_EVT ) {
  //      Serial.println("W");
  //      if(Serial2.available()) SerialBT.write(Serial2.read());
  //    }
}

void setup() {
  Serial.begin(115200);
  Serial.println("SparkFun RTK Surveyor v1.0");

  pinMode(bluetoothStatusLED, OUTPUT);

  SerialBT.register_callback(callback);

  if (!SerialBT.begin("SparkFun Surveyor"))
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

void loop() {

  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'r')
    {
      Serial.println("Reset bluetooth...");
      //SerialBT.end();
      //Reset bluetooth
      SerialBT.register_callback(callback);

      if (!SerialBT.begin("SparkFun Surveyor"))
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
    else
      Serial.println("Unknown command");
  }

  //Update Bluetooth LED status
    if (bluetoothState == BT_ON_NOCONNECTION)
    {
//      Serial2.end();
      
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
    //yield();
    //SerialBT.print("Test a lot of text to try to break it");
    //delay(20);
    //yield();

    //If the ZED has any new NMEA data, pass it out over Bluetooth
    if (Serial2.available())
      SerialBT.write(Serial2.read());
    //
    //    //  yield();
    //
        //If the phone has any new data (NTRIP RTCM, etc), pass it out over Bluetooth
//    if (SerialBT.available())
//      Serial2.write(SerialBT.read());

    //    if (digitalRead(bluetoothStatusLED) == LOW)
    //      digitalWrite(bluetoothStatusLED, HIGH);
    //    else
    //      digitalWrite(bluetoothStatusLED, LOW);
  }

  //delay(20);
}
