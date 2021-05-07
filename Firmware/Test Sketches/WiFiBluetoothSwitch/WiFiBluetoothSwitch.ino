/*
  Starting and restarting BT is a problem
  https://github.com/espressif/arduino-esp32/issues/2718

  To work around the bug without modifying the core we create our own btStop() function with
  the patch from github

  Heap readings:
  238148 - Sketch start
  140932 - BT start
  238148 - BT end
  186916 - WiFi start
  223224 - WiFi end - There is a ~15k RAM leak here
  126016 - BT re-start
*/

#include <WiFi.h>
WiFiClient caster;

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

#include "esp_bt.h"
#include "esp_wifi.h"

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.print("ESP32 SDK: ");
  Serial.println(ESP.getSdkVersion());
  Serial.println("Press the button to select the next mode");

  Serial.printf("freeHeap: %d\n\r", ESP.getFreeHeap());
}

void loop() {

  if (Serial.available())
  {
    byte incoming = Serial.read();

    Serial.println();
    Serial.println("1) Start Bluetooth");
    Serial.println("2) End Bluetooth");
    Serial.println("3) Start Wifi");
    Serial.println("4) End Wifi");

    if (incoming == '1')
    {
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
      Serial.println(F("Stopping Bluetooth"));
      customBTstop(); //Gracefully turn off Bluetooth so we can turn it back on if needed
    }
    else if (incoming == '3')
    {
      wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
      esp_wifi_init(&wifi_init_config); //Restart WiFi resources

      Serial.printf("Connecting to local WiFi: %s\n", "sparkfun-guest");
      WiFi.begin("TRex", "parachutes");
      //WiFi.begin("sparkfun-guest", "sparkfun6333");
    }
    else if (incoming == '4')
    {
      Serial.println("Stopping Wifi");
      caster.stop();
      WiFi.mode(WIFI_OFF);
      esp_wifi_deinit(); //Free all resources
    }

    Serial.printf("freeHeap: %d\n\r", ESP.getFreeHeap());
  }

  delay(250);
}

//Starting and restarting BT is a problem. See issue: https://github.com/espressif/arduino-esp32/issues/2718
//To work around the bug without modifying the core we create our own btStop() function with
//the patch from github
bool customBTstop() {

  SerialBT.flush(); //Complete any transfers
  SerialBT.disconnect(); //Drop any clients
  SerialBT.end(); //SerialBT.end() will release significant RAM (~100k!) but a SerialBT.start will crash.
  //The follow code releases the BT hardware so that it can be restarted with a SerialBT.begin

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
    return true;
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    if (esp_bt_controller_disable()) {
      log_e("BT Disable failed");
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
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
