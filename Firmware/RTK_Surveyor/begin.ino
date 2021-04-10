//Initial startup functions for GNSS, SD, display, radio, etc

void beginSD()
{
  int spiFreq = 40000000;

  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if (SD.begin(PIN_MICROSD_CHIP_SELECT, spi, spiFreq) == false)
    {
      int tries = 0;
      int maxTries = 4;
      for ( ; tries < maxTries ; tries++)
      {
        Serial.printf("SD init failed. Trying again %d out of %d\n", tries + 1, maxTries);

        delay(250); //Give SD more time to power up, then try again
        if (SD.begin(PIN_MICROSD_CHIP_SELECT, spi, spiFreq) == true) break;
      }

      if (tries == maxTries)
      {
        Serial.println(F("SD init failed. Is card present? Formatted?"));
        digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    online.microSD = true;
  }
  else
  {
    online.microSD = false;
  }
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    online.display = true;

    //Init and display splash
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory

    oled.setCursor(10, 2); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("SparkFun"));

    oled.setCursor(21, 13);
    oled.setFontType(1);
    oled.print(F("RTK"));

    int surveyorTextY = 25;
    int surveyorTextX = 2;
    int surveyorTextKerning = 8;
    oled.setFontType(1);

    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("S");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("u");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("v");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("e");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("y");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("o");

    surveyorTextX += surveyorTextKerning;
    oled.setCursor(surveyorTextX, surveyorTextY);
    oled.print("r");

    oled.setCursor(20, 41);
    oled.setFontType(0); //Set font to smallest
    oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
    oled.display();
  }
}

//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (settings.logUBX == true)
  {
    i2cGNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C
  }

  // RAWX messages can be over 2KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
  // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
  // to hold the backlog of data if/when this happens.
  // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
  i2cGNSS.setFileBufferSize(gnssFileBufferSize); // setFileBufferSize must be called _before_ .begin

  if (i2cGNSS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (i2cGNSS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      blinkError(ERROR_NO_I2C);
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
      blinkError(ERROR_GPS_CONFIG_FAIL);
    }
  }

  online.gnss = true;

  Serial.println(F("GNSS configuration complete"));
}

//Get MAC, start radio
//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool beginBluetooth()
{
  //Shutdown any previous WiFi
  caster.stop();
  WiFi.mode(WIFI_OFF);
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
  if (F9PSerialReadTaskHandle == NULL) xTaskCreate(F9PSerialReadTask, "F9Read", readTaskStackSize, NULL, 0, &F9PSerialReadTaskHandle);
  if (F9PSerialWriteTaskHandle == NULL) xTaskCreate(F9PSerialWriteTask, "F9Write", writeTaskStackSize, NULL, 0, &F9PSerialWriteTaskHandle);

  return (true);
}

//Turn off BT so we can go into WiFi mode
bool endBluetooth()
{
  //Delete tasks if running
  //  if (F9PSerialReadTaskHandle != NULL)
  //  {
  //    vTaskDelete(F9PSerialReadTaskHandle);
  //    F9PSerialReadTaskHandle = NULL;
  //  }
  //  if (F9PSerialWriteTaskHandle != NULL)
  //  {
  //    vTaskDelete(F9PSerialWriteTaskHandle);
  //    F9PSerialWriteTaskHandle = NULL;
  //  }

  SerialBT.flush();
  SerialBT.disconnect();
  //SerialBT.end(); //Do not call both end and custom stop
  //customBTstop(); //Gracefully turn off Bluetooth so we can turn it back on if needed

  Serial.println(F("Bluetooth turned off"));
}

//Set LEDs for output and configure PWM
void beginLEDs()
{
  pinMode(positionAccuracyLED_1cm, OUTPUT);
  pinMode(positionAccuracyLED_10cm, OUTPUT);
  pinMode(positionAccuracyLED_100cm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);
  pinMode(baseSwitch, INPUT_PULLUP); //HIGH = rover, LOW = base

  digitalWrite(positionAccuracyLED_1cm, LOW);
  digitalWrite(positionAccuracyLED_10cm, LOW);
  digitalWrite(positionAccuracyLED_100cm, LOW);
  digitalWrite(baseStatusLED, LOW);
  digitalWrite(bluetoothStatusLED, LOW);

  ledcSetup(ledRedChannel, freq, resolution);
  ledcSetup(ledGreenChannel, freq, resolution);

  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);

  ledcWrite(ledRedChannel, 0);
  ledcWrite(ledGreenChannel, 0);
}

//Configure the on board MAX17048 fuel gauge
void beginFuelGauge()
{
  // Set up the MAX17048 LiPo fuel gauge
  if (lipo.begin() == false)
  {
    Serial.println(F("MAX17048 not detected. Continuing."));
    return;
  }

  //Always use hibernate mode
  if (lipo.getHIBRTActThr() < 0xFF) lipo.setHIBRTActThr((uint8_t)0xFF);
  if (lipo.getHIBRTHibThr() < 0xFF) lipo.setHIBRTHibThr((uint8_t)0xFF);

  Serial.println(F("MAX17048 configuration complete"));
}
