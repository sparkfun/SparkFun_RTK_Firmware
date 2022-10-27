/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  Bluetooth State Values:
  BT_OFF = 0,
  BT_NOTCONNECTED,
  BT_CONNECTED,

  Bluetooth States:

                                  BT_OFF (Using WiFi)
                                    |   ^
                      Use Bluetooth |   | Use WiFi
                     bluetoothStart |   | bluetoothStop
                                    v   |
                               BT_NOTCONNECTED
                                    |   ^
                   Client connected |   | Client disconnected
                                    v   |
                                BT_CONNECTED

  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Constants
//----------------------------------------

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#ifdef COMPILE_BT
BTSerialInterface *bluetoothSerial;
static volatile byte bluetoothState = BT_OFF;

//----------------------------------------
// Bluetooth Routines - compiled out
//----------------------------------------

//Call back for when BT connection event happens (connected/disconnect)
//Used for updating the bluetoothState state machine
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("BT client Connected");
    bluetoothState = BT_CONNECTED;
    if (productVariant == RTK_SURVEYOR)
      digitalWrite(pin_bluetoothStatusLED, HIGH);
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("BT client disconnected");
    bluetoothState = BT_NOTCONNECTED;
    if (productVariant == RTK_SURVEYOR)
      digitalWrite(pin_bluetoothStatusLED, LOW);
  }
}

#endif  //COMPILE_BT

//----------------------------------------
// Global Bluetooth Routines
//----------------------------------------

//Return the Bluetooth state
byte bluetoothGetState()
{
#ifdef COMPILE_BT
  return bluetoothState;
#else   //COMPILE_BT
  return BT_OFF;
#endif  //COMPILE_BT
}

//Read data from the Bluetooth device
int bluetoothReadBytes(uint8_t * buffer, int length)
{
#ifdef COMPILE_BT
  return bluetoothSerial->readBytes(buffer, length);
#else   //COMPILE_BT
  return 0;
#endif  //COMPILE_BT
}

//Determine if data is available
bool bluetoothRxDataAvailable()
{
#ifdef COMPILE_BT
  return bluetoothSerial->available();
#else   //COMPILE_BT
  return false;
#endif  //COMPILE_BT
}

//Get MAC, start radio
//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
void bluetoothStart()
{
#ifdef COMPILE_BT
  if (bluetoothState == BT_OFF)
  {
    char stateName[10];
    if (systemState >= STATE_ROVER_NOT_STARTED && systemState <= STATE_ROVER_RTK_FIX)
      strcpy(stateName, "Rover-");
    else if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_FIXED_TRANSMITTING)
      strcpy(stateName, "Base-");

    sprintf(deviceName, "%s %s%02X%02X", platformPrefix, stateName, btMACAddress[4], btMACAddress[5]);

    // Select Bluetooth setup
    if (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF)
      return;
    else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
      bluetoothSerial = new BTClassicSerial();
    else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
      bluetoothSerial = new BTLESerial();

    if (bluetoothSerial->begin(deviceName) == false)
    {
      Serial.println("An error occurred initializing Bluetooth");

      if (productVariant == RTK_SURVEYOR)
        digitalWrite(pin_bluetoothStatusLED, LOW);
      return;
    }

    //Set PIN to 1234 so we can connect to older BT devices, but not require a PIN for modern device pairing
    //See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/5
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

    bluetoothSerial->register_callback(bluetoothCallback); //Controls BT Status LED on Surveyor
    bluetoothSerial->setTimeout(250);

    Serial.print("Bluetooth broadcasting as: ");
    Serial.println(deviceName);

    //Start task for controlling Bluetooth pair LED
    if (productVariant == RTK_SURVEYOR)
    {
      ledcWrite(ledBTChannel, 255); //Turn on BT LED
      btLEDTask.detach(); //Slow down the BT LED blinker task
      btLEDTask.attach(btLEDTaskPace2Hz, updateBTled); //Rate in seconds, callback
    }

    bluetoothState = BT_NOTCONNECTED;
    reportHeapNow();
  }
#endif  //COMPILE_BT
}

//This function stops BT so that it can be restarted later
//It also releases as much system resources as possible so that WiFi/caster is more stable
void bluetoothStop()
{
#ifdef COMPILE_BT
  if (bluetoothState == BT_NOTCONNECTED || bluetoothState == BT_CONNECTED)
  {
    bluetoothSerial->register_callback(NULL);
    bluetoothSerial->flush(); //Complete any transfers
    bluetoothSerial->disconnect(); //Drop any clients
    bluetoothSerial->end(); //bluetoothSerial->end() will release significant RAM (~100k!) but a bluetoothSerial->start will crash.

    log_d("Bluetooth turned off");

    bluetoothState = BT_OFF;
    reportHeapNow();
  }
#endif  //COMPILE_BT
  bluetoothIncomingRTCM = false;
}

//Test the bidirectional communication through UART2
void bluetoothTest(bool runTest)
{
  //Verify the ESP UART2 can communicate TX/RX to ZED UART1
  const char * bluetoothStatusText;
  if (online.gnss == true)
  {
    if (runTest && (zedUartPassed == false))
    {
      tasksStopUART2(); //Stop absoring ZED serial via task

      i2cGNSS.setVal32(UBLOX_CFG_UART1_BAUDRATE, (115200 * 2)); //Defaults to 230400 to maximize message output support
      serialGNSS.begin((115200 * 2)); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

      SFE_UBLOX_GNSS myGNSS;
      if (myGNSS.begin(serialGNSS) == true) //begin() attempts 3 connections
      {
        zedUartPassed = true;
        bluetoothStatusText = (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF) ? "Off" : "Online";
      }
      else
        bluetoothStatusText = "Offline";

      i2cGNSS.setVal32(UBLOX_CFG_UART1_BAUDRATE, settings.dataPortBaud); //Defaults to 230400 to maximize message output support
      serialGNSS.begin(settings.dataPortBaud); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.

      tasksStartUART2(); //Return to normal operation
    }
    else
      bluetoothStatusText = (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF) ? "Off" : "Online";
  }
  else
    bluetoothStatusText = "GNSS Offline";

  //Display Bluetooth MAC address and test results
  char macAddress[5];
  sprintf(macAddress, "%02X%02X", btMACAddress[4], btMACAddress[5]);
  Serial.print("Bluetooth ");
  if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
    Serial.print("Low Energy ");
  Serial.print("(");
  Serial.print(macAddress);
  Serial.print("): ");
  Serial.println(bluetoothStatusText);
}

//Write data to the Bluetooth device
int bluetoothWriteBytes(const uint8_t * buffer, int length)
{
#ifdef COMPILE_BT
  //Push new data to BT SPP
  return bluetoothSerial->write(buffer, length);
#else   //COMPILE_BT
  return 0;
#endif  //COMPILE_BT
}
