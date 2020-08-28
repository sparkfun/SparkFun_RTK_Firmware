#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

volatile bool btconnected = false;

const int statLED = 13;

uint32_t lastTime = 0;

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected");
    btconnected = true;
    SerialBT.flush(); //Clear buffer
    if(Serial1.available()) SerialBT.write(Serial1.read());
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("Client disconnected");
    btconnected = false;
    digitalWrite(statLED, LOW);
    SerialBT.flush(); //Clear buffer
  }

  if (event == ESP_SPP_WRITE_EVT ) {
    Serial.println("W");
    if(Serial1.available()) SerialBT.write(Serial1.read());
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200); //Default UART1?

  pinMode(statLED, OUTPUT);

  SerialBT.register_callback(callback);

  if (!SerialBT.begin("ESP32test")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }

}

void loop() {

//  //if (btconnected == true)
//  //{
//  if (Serial1.available())
//  {
//    //yield();
//    SerialBT.write(Serial1.read());
//
//    //Blink LED to indicate traffic
//    if (millis() - lastTime > 1000)
//    {
//      lastTime += 1000;
//
//      if (digitalRead(statLED) == LOW)
//        digitalWrite(statLED, HIGH);
//      else
//        digitalWrite(statLED, LOW);
//    }
//  }
//
//  //  }
//  //  else
//  //  {
//  //    Serial.println("No BT");
//  //    for (int x = 0 ; x < 1000 ; x++)
//  //    {
//  //      delay(1);
//  //      yield();
//  //    }
//  //    //delay(1000);
//  //  }
//  delay(1);

}
