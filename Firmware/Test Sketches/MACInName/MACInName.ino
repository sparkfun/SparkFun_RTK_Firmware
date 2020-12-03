#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

const int baseSwitch = 5;

void setup() {
  Serial.begin(115200);
  Serial.println("MAC test");

  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  startBluetooth();

}

void loop() {


}

//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
void startBluetooth()
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  mac[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  char deviceName[20];
  if (digitalRead(baseSwitch) == HIGH) 
  {
    //Rover mode
    sprintf(deviceName, "Surveyor Rover-%02X%02X", mac[4], mac[5]);
  }
  else
  {
    //Base mode
    sprintf(deviceName, "Surveyor Base-%02X%02X", mac[4], mac[5]);
  }

  Serial.print("Bluetooth broadcast as: ");
  Serial.println(deviceName);

  SerialBT.begin(deviceName);
}
