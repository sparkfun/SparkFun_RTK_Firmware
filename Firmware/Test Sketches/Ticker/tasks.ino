//These are all the low frequency tasks that are called by Ticker

//Display battery level and various datums based on system state (is BT connected? are we in base mode? etc)
//This task is only activated if a display is detected at POR
void updateDisplay()
{
  long startTime = millis();

  oled.clear(PAGE); // Clear the display's internal buffer

  //Current battery charge level
  if (battLevel < 25)
    oled.drawIcon(45, 0, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
  else if (battLevel < 50)
    oled.drawIcon(45, 0, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
  else if (battLevel < 75)
    oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);
  else //batt level > 75
    oled.drawIcon(45, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);

  //Bluetooth Address or RSSI
  if (bluetoothState == BT_CONNECTED)
  {
    oled.drawIcon(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol, sizeof(BT_Symbol), true);
  }
  else
  {
    char macAddress[5] = "BC4D";
    //    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
    oled.setFontType(0); //Set font to smallest
    oled.setCursor(0, 4);
    oled.print(macAddress);
  }

  if (digitalRead(baseSwitch) == LOW)
    oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true); //true - blend with other pixels
  else
    oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);

  //Horz positional accuracy
  oled.setFontType(1); //Set font to type 1: 8x16
  oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
  oled.setCursor(16, 20); //x, y
  oled.print(":");
  float hpa = myGPS.getHorizontalAccuracy() / 10000.0;
  //  float hpa = 10000.0;
  if (hpa > 30.0)
  {
    oled.print(">30");
  }
  else if (hpa > 9.9)
  {
    oled.print(hpa, 1); //Print down to decimeter
  }
  else if (hpa > 1.0)
  {
    oled.print(hpa, 2); //Print down to centimeter
  }
  else
  {
    oled.print("."); //Remove leading zero
    oled.printf("%03d", (int)(hpa * 1000)); //Print down to millimeter
  }

  //SIV
  oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
  oled.setCursor(16, 36); //x, y
  oled.print(":");

  if (myGPS.getFixType() == 0) //0 = No Fix
  {
    oled.print("0");
  }
  else
  {
    oled.print(myGPS.getSIV());
  }

  //Check I2C semaphore
  if (xSemaphoreTake(xI2CSemaphore, i2cSemaphore_maxWait) == pdPASS)
  {
    oled.display();
    xSemaphoreGive(xI2CSemaphore);
  }

  //Serial.printf("Display time to update: %d\n", millis() - startTime);
}

//AutoPVT without implicit updates is used
//This means we regularly (every 100ms) call checkUblox and all the global datums are updated as they are reported (4Hz default)
//If we ask for something like myGPS.getSIV(), there is no blocking wait, we simply get the last reported value
void checkUblox()
{
  long startTime = millis();
  //Check I2C semaphore
  if (xSemaphoreTake(xI2CSemaphore, i2cSemaphore_maxWait) == pdPASS)
  {
    myGPS.checkUblox();
    xSemaphoreGive(xI2CSemaphore);
  }
  //Serial.printf("GPS time to update: %d\n", millis() - startTime);
}

//Control BT status LED according to bluetoothState
void updateBTled()
{
  if (bluetoothState == BT_ON_NOCONNECTION)
    digitalWrite(bluetoothStatusLED, !digitalRead(bluetoothStatusLED));
  else if (bluetoothState == BT_CONNECTED)
    digitalWrite(bluetoothStatusLED, HIGH);
  else
    digitalWrite(bluetoothStatusLED, LOW);
}

//When called, checks level of battery and updates the LED brightnesses
//And outputs a serial message to USB and BT
void checkBatteryLevels()
{
  String battMsg = "";

  long startTime = millis();
  
  //Check I2C semaphore
  if (xSemaphoreTake(xI2CSemaphore, i2cSemaphore_maxWait) == pdPASS)
  {
    battLevel = lipo.getSOC();
    battVoltage = lipo.getVoltage();
    battChangeRate = lipo.getChangeRate();
    xSemaphoreGive(xI2CSemaphore);
  }
  //Serial.printf("Batt time to update: %d\n", millis() - startTime);

  battMsg += "Batt (";
  battMsg += battLevel;
  battMsg += "%): ";

  battMsg += "Voltage: ";
  battMsg += battVoltage;
  battMsg += "V";

  if (battChangeRate > 0)
    battMsg += " Charging: ";
  else
    battMsg += " Discharging: ";
  battMsg += battChangeRate;
  battMsg += "%/hr ";

  if (battLevel < 10)
  {
    battMsg += "RED uh oh!";
    ledcWrite(ledRedChannel, 255);
    ledcWrite(ledGreenChannel, 0);
  }
  else if (battLevel < 50)
  {
    battMsg += "Yellow ok";
    ledcWrite(ledRedChannel, 128);
    ledcWrite(ledGreenChannel, 128);
  }
  else if (battLevel >= 50)
  {
    battMsg += "Green all good";
    ledcWrite(ledRedChannel, 0);
    ledcWrite(ledGreenChannel, 255);
  }
  else
  {
    battMsg += "No batt";
    ledcWrite(ledRedChannel, 10);
    ledcWrite(ledGreenChannel, 0);
  }
  battMsg += "\n\r";
  //SerialBT.print(battMsg);
  Serial.print(battMsg);

}
