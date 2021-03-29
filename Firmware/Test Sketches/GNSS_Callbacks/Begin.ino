//Get MAC, start radio
//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool beginBluetooth()
{
  //Shutdown any previous WiFi
  //caster.stop();
  //WiFi.mode(WIFI_OFF);
  radioState = RADIO_OFF;

  //Due to a known issue, you cannot call esp_bt_controller_enable() a second time
  //to change the controller mode dynamically. To change controller mode, call
  //esp_bt_controller_disable() and then call esp_bt_controller_enable() with the new mode.

  //btStart(); //In v1.1 we do not turn off or on the bt radio. See WiFiBluetooSwitch sketch for more info:

  //Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  if (digitalRead(baseSwitch) == HIGH)
    sprintf(deviceName, "Surveyor Rover-%02X%02X", unitMACAddress[4], unitMACAddress[5]); //Rover mode
  else
    sprintf(deviceName, "Surveyor Base-%02X%02X", unitMACAddress[4], unitMACAddress[5]); //Base mode

  if (SerialBT.begin(deviceName) == false)
  {
    Serial.println(F("An error occurred initializing Bluetooth"));
    radioState = RADIO_OFF;
    digitalWrite(bluetoothStatusLED, LOW);
    return (false);
  }

  //Set PIN to 1234 so we can connect to older BT devices, but not require a PIN for modern device pairing
  //See issue: https://github.com/sparkfun/SparkFun_RTK_Surveyor/issues/5
  //https://github.com/espressif/esp-idf/issues/1541
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;

  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE; //Requires pin 1234 on old BT dongle, No prompt on new BT dongle
  //esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_OUT; //Works but prompts for either pin (old) or 'Does this 6 pin appear on the device?' (new)

  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
  esp_bt_pin_code_t pin_code;
  pin_code[0] = '1';
  pin_code[1] = '2';
  pin_code[2] = '3';
  pin_code[3] = '4';
  esp_bt_gap_set_pin(pin_type, 4, pin_code);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  SerialBT.register_callback(btCallback);
  SerialBT.setTimeout(1);

  Serial.print(F("Bluetooth broadcasting as: "));
  Serial.println(deviceName);

  radioState = BT_ON_NOCONNECTION;
  digitalWrite(bluetoothStatusLED, HIGH);

  //Start the tasks for handling incoming and outgoing BT bytes to/from ZED-F9P
  //Reduced stack size from 10,000 to 1,000 to make room for WiFi/NTRIP server capabilities
  if (F9PSerialReadTaskHandle == NULL) xTaskCreate(F9PSerialReadTask, "F9Read", 1000, NULL, 0, &F9PSerialReadTaskHandle);
  if (F9PSerialWriteTaskHandle == NULL) xTaskCreate(F9PSerialWriteTask, "F9Write", 1000, NULL, 0, &F9PSerialWriteTaskHandle);

  return (true);
}

void beginSD()
{
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if (sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24)) == false) //Standard SdFat
    {
      //printDebug("SD init failed (first attempt). Trying again...\r\n");
      //Give SD more time to power up, then try again
      delay(250);
      if (sd.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(24)) == false) //Standard SdFat
      {
        Serial.println(F("SD init failed (second attempt). Is card present? Formatted?"));
        digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      Serial.println(F("SD change directory failed"));
      online.microSD = false;
      return;
    }

    online.microSD = true;
  }
  else
  {
    online.microSD = false;
  }
}

//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (settings.logUBX == true)
  {
    i2cGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C

    // RAWX messages can be over 2KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
    // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
    // to hold the backlog of data if/when this happens.
    // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
    i2cGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin
  }

  if (i2cGNSS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (i2cGNSS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      while (1); //blinkError(ERROR_NO_I2C);
    }
  }

  //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
  //  if (i2cGNSS.getModuleInfo(1100) == true) // Try to get the module info
  //  {
  //    if (strcmp(i2cGNSS.minfo.extension[1], latestZEDFirmware) != 0)
  //    {
  //      Serial.print(F("The ZED-F9P appears to have outdated firmware. Found: "));
  //      Serial.println(i2cGNSS.minfo.extension[1]);
  //      Serial.print(F("The Surveyor works best with "));
  //      Serial.println(latestZEDFirmware);
  //      Serial.print(F("Please upgrade using u-center."));
  //      Serial.println();
  //    }
  //    else
  //    {
  //      Serial.println(F("ZED-F9P firmware is current"));
  //    }
  //  }

  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure module. Trying again."));
    delay(1000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure module. Hard stop."));
      while (1); //blinkError(ERROR_GPS_CONFIG_FAIL);
    }
  }
  Serial.println(F("GNSS configuration complete"));
}
