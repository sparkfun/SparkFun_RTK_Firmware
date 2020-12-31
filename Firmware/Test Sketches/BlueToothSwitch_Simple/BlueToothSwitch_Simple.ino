/*
  Starting and restarting BT is a problem
  https://github.com/espressif/arduino-esp32/issues/2718

  To work around the bug without modifying the core we create our own btStop() function with
  the patch from github. This prevent core panic.

  Unfortunately while BT reports starting, it does not advertise correctly the 2nd time.

  Maybe 2 configs needed:
  https://github.com/espressif/arduino-esp32/issues/1150#issuecomment-368484759
  
*/
//#include <WiFi.h>

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

#include "esp_bt.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define BT_CONTROLLER_INIT_CONFIG_NOTDEFAULT() {                              \
    .controller_task_stack_size = ESP_TASK_BT_CONTROLLER_STACK,            \
    .controller_task_prio = ESP_TASK_BT_CONTROLLER_PRIO,                   \
    .hci_uart_no = BT_HCI_UART_NO_DEFAULT,                                 \
    .hci_uart_baudrate = BT_HCI_UART_BAUDRATE_DEFAULT,                     \
    .scan_duplicate_mode = SCAN_DUPLICATE_MODE,                            \
    .scan_duplicate_type = SCAN_DUPLICATE_TYPE_VALUE,                      \
    .normal_adv_size = NORMAL_SCAN_DUPLICATE_CACHE_SIZE,                   \
    .mesh_adv_size = MESH_DUPLICATE_SCAN_CACHE_SIZE,                       \
    .send_adv_reserved_size = SCAN_SEND_ADV_RESERVED_SIZE,                 \
    .controller_debug_flag = CONTROLLER_ADV_LOST_DEBUG_BIT,                \
    .mode = ESP_BT_MODE_BLE,                                      \
    .ble_max_conn = 3,                     \
    .bt_max_acl_conn = 0,           \
    .bt_sco_datapath = 0,          \
    .bt_max_sync_conn = 0,         \
    .magic = ESP_BT_CONTROLLER_CONFIG_MAGIC_VAL,                           \
};


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
      //WiFi.disconnect(true, true);
      //WiFi.mode(WIFI_OFF);

      //Due to a known issue, you cannot call esp_bt_controller_enable() a second time
      //to change the controller mode dynamically. To change controller mode, call
      //esp_bt_controller_disable() and then call esp_bt_controller_enable() with the new mode.

//      ble.begin("Quickie");
      //delay(2000);

      //customBTstop();
      //customBTstartBLE();
      customBTstop();
    esp_bt_controller_disable();
    if (esp_spp_init(ESP_SPP_MODE_CB) != ESP_OK){
        log_e("spp init failed");
    }
//      esp_bt_controller_enable(ESP_BT_MODE_BLE);
      //customBTstartBLE();
      esp_bt_controller_disable();

      Serial.println("Starting BT");
      //btStart();
      customBTstartBTDM();

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
      //btStop(); //Causes crash when we restart BT

      //Serial.printf("Connecting to local WiFi: %s", "sparkfun-guest");
      //WiFi.begin("sparkfun-guest", "sparkfun6333");
      //      WiFi.begin("Sparky", "sparkfun");
      //
      //      while (WiFi.status() != WL_CONNECTED) {
      //        delay(500);
      //        Serial.print(F("."));
      //      }
      //      Serial.println();
    }

  }

  delay(100);
}

//Starting and restarting BT is a problem. See issue: https://github.com/espressif/arduino-esp32/issues/2718
//To work around the bug without modifying the core we create our own btStop() function with
//the patch from github
bool customBTstop() {
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
    Serial.println("Already in idle");
    return true;
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    if (esp_bt_controller_disable()) {
      Serial.println("BT Disable failed");
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
  {
    Serial.println("inited");
    if (esp_bt_controller_deinit())
    {
      Serial.println("BT deint failed");
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED);
    return true;
  }
  Serial.println("BT Stop failed");
  return false;
}

//#ifdef CONFIG_CLASSIC_BT_ENABLED
#define BT_MODE ESP_BT_MODE_BTDM
//#else
//#define BT_MODE ESP_BT_MODE_BLE
//#endif

bool customBTstartBTDM() {
  esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  //  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
  //    return true;
  //  }

  Serial.println("BTDM Start");
  //if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
  Serial.println("BTDM Init");
  esp_bt_controller_init(&cfg);
  while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {}
  //}

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
    Serial.println("BTDM Init'd");

    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM)) {
      Serial.println("BT BTDM Enable failed");
      return false;
    }
    Serial.println("BTDM Enabled");
  }

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    return true;
  }

  Serial.println("BT Start failed");
  return false;
}

bool customBTstartBLE() {
  esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_NOTDEFAULT();

  //  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
  //    return true;
  //  }

  Serial.println("BLE Start");

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
  {
    Serial.println("inited");
    if (esp_bt_controller_deinit())
    {
      Serial.println("BT deint failed");
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED);
  }

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
    Serial.println("BLE Init");
    esp_bt_controller_init(&cfg);
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {}
  }

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
    Serial.println("BLE Init'd");
    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM)) {
      Serial.println("BT BLE Enable failed");
      return false;
    }
    Serial.println("BLE Enabled");
  }

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    return true;
  }

  Serial.println("BT Start failed");
  return false;
}
