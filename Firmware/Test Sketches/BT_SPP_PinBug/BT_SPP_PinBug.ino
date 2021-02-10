/*

 Older BT dongles and devices (user is using Windows CE) require a pin for BT SPP pairing. On
 newer device, it pairs without issue. We need to accept a pin if required, but not ask for one.
 
*/

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

#include "esp_gap_bt_api.h"

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32test"); //Bluetooth device name

  /* Set default parameters for Secure Simple Pairing */
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  
  //https://github.com/espressif/esp-idf/issues/1541
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE; //Requires pin 1234 on old BT dongle, No prompt on new BT dongle
  //esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_OUT; //Works but prompts for either pin (old) or 'Does this 6 pin appear on the device?' (new)
  
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

  /*
     Set default parameters for Legacy Pairing
     Use fixed pin code
  */
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
  esp_bt_pin_code_t pin_code;
  pin_code[0] = '1';
  pin_code[1] = '2';
  pin_code[2] = '3';
  pin_code[3] = '4';
  esp_bt_gap_set_pin(pin_type, 4, pin_code);

  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20);
}
