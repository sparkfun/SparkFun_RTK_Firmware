/*
 Starting and restarting BT is a problem
 https://github.com/espressif/arduino-esp32/issues/2718

 To work around the bug without modifying the core we create our own btStop() function with
 the patch from github
 */
#include <WiFi.h>
WiFiClient caster;

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

#define AP_SSID  "esp32"

#include "esp_bt.h"

void setup() {
  Serial.begin(115200);
  delay(100);

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
      Serial.println("Stopping Wifi");
      WiFi.mode(WIFI_OFF);

      Serial.println("Starting BT");
      btStart();

      if (SerialBT.begin("Surveyor Name") == false)
        Serial.println("Failed BT start");
      else
      {
        Serial.print("Bluetooth broadcasting as: ");
        Serial.println("Surveyor Name");
      }
    }
    else if (incoming == '2')
    {
      Serial.println("Stopping BT");
      customBTstop();

      Serial.printf("Connecting to local WiFi: %s\n", "sparkfun-guest");
      WiFi.begin("sparkfun-guest", "sparkfun6333");
    }

  }

  delay(100);
}

//Starting and restarting BT is a problem. See issue: https://github.com/espressif/arduino-esp32/issues/2718
//To work around the bug without modifying the core we create our own btStop() function with
//the patch from github
bool customBTstop(){
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
        return true;
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        if (esp_bt_controller_disable()) {
            log_e("BT Disable failed");
            return false;
        }
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
    }
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
    {
        log_i("inited");
        if (esp_bt_controller_deinit())
        {
            log_e("BT deint failed");
            return false;
        }
        while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
            ;
        return true;
    }
    log_e("BT Stop failed");
    return false;
}
