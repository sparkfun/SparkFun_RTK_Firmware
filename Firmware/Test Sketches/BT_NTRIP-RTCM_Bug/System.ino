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
        auto s = SerialBT.readBytes(wBuffer, SERIAL_SIZE_RX);
        GPS.write(wBuffer, s);
      }
    }

    taskYIELD();
  }
}

//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool startBluetooth()
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

  if (SerialBT.begin(deviceName) == false)
    return (false);
  Serial.print("Bluetooth broadcasting as: ");
  Serial.println(deviceName);

  //Start the tasks.
  //Can also use xTaskCreatePinnedToCore to pin a task to one of the two cores
  //on the ESP32
  //xTaskCreate(F9PSerialReadTask, "F9Read", 10000, NULL, 0, NULL);
  xTaskCreate(F9PSerialWriteTask, "F9Write", 10000, NULL, 0, NULL);

  SerialBT.setTimeout(1);

  return (true);
}
