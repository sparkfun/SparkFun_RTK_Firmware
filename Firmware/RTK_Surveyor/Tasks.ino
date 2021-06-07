//High frequency tasks made by createTask()
//And any low frequency tasks that are called by Ticker

//If the phone has any new data (NTRIP RTCM, etc), read it in over Bluetooth and pass along to ZED
//Task for writing to the GNSS receiver
void F9PSerialWriteTask(void *e)
{
  while (true)
  {
    //Receive RTCM corrections or UBX config messages over bluetooth and pass along to ZED
    if (SerialBT.available())
    {
      while (SerialBT.available())
      {
        if (inTestMode == false)
        {
          //Pass bytes to GNSS receiver
          auto s = SerialBT.readBytes(wBuffer, SERIAL_SIZE_RX);
          serialGNSS.write(wBuffer, s);
        }
        else
        {
          char incoming = SerialBT.read();
          Serial.printf("I heard: %c\n", incoming);
          incomingBTTest = incoming; //Displayed during system test
        }
      }
    }

    taskYIELD();
  }
}

//If the ZED has any new NMEA data, pass it out over Bluetooth
//Task for reading data from the GNSS receiver.
void F9PSerialReadTask(void *e)
{
  while (true)
  {
    if (serialGNSS.available())
    {
      auto s = serialGNSS.readBytes(rBuffer, SERIAL_SIZE_RX);

      //If we are actively survey-in then do not pass NMEA data from ZED to phone
      if (systemState == STATE_BASE_TEMP_SETTLE || systemState == STATE_BASE_TEMP_SURVEY_STARTED)
      {
        //Do nothing
      }
      else if (SerialBT.connected())
      {
        SerialBT.write(rBuffer, s);
      }

      if (settings.enableHeapReport == true)
      {
        if (millis() - lastTaskHeapReport > 1000)
        {
          lastTaskHeapReport = millis();
          Serial.printf("Task freeHeap: %d\n\r", ESP.getFreeHeap());
        }
      }

      //If user wants to log, record to SD
      if (online.logging == true)
      {
        //Check if we are inside the max time window for logging
        if ((systemTime_minutes - startLogTime_minutes) < settings.maxLogTime_minutes)
        {
          //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile()
          if (xSemaphoreTake(xFATSemaphore, fatSemaphore_shortWait_ms) == pdPASS)
          {
            ubxFile.write(rBuffer, s);

            //Force file sync every 5000ms
            if (millis() - lastUBXLogSyncTime > 5000)
            {
              if (productVariant == RTK_SURVEYOR)
                digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Blink LED to indicate logging activity

              long startWriteTime = micros();
              ubxFile.sync();
              long stopWriteTime = micros();
              totalWriteTime += stopWriteTime - startWriteTime; //Used to calculate overall write speed

              if (productVariant == RTK_SURVEYOR)
                digitalWrite(pin_baseStatusLED, !digitalRead(pin_baseStatusLED)); //Return LED to previous state

              updateDataFileAccess(&ubxFile); // Update the file access time & date

              lastUBXLogSyncTime = millis();
            }

            xSemaphoreGive(xFATSemaphore);
          } //End xFATSemaphore
          else
          {
            log_d("F9SerialRead: Semaphore failed to yield");
          }
        } //End maxLogTime
      } //End logging
    } //End serial available from GNSS

    taskYIELD();
  }
}

//Assign UART2 interrupts to the current core. See: https://github.com/espressif/arduino-esp32/issues/3386
void startUART2Task( void *pvParameters )
{
  serialGNSS.begin(settings.dataPortBaud); //UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to output NMEA over its UART1 at the same rate.
  serialGNSS.setRxBufferSize(SERIAL_SIZE_RX);
  serialGNSS.setTimeout(50);

  uart2Started = true;

  vTaskDelete( NULL ); //Delete task once it has run once
}

//Control BT status LED according to bluetoothState
void updateBTled()
{
  if (productVariant == RTK_SURVEYOR)
  {
    if (radioState == BT_ON_NOCONNECTION)
      digitalWrite(pin_bluetoothStatusLED, !digitalRead(pin_bluetoothStatusLED));
    else if (radioState == BT_CONNECTED)
      digitalWrite(pin_bluetoothStatusLED, HIGH);
    else
      digitalWrite(pin_bluetoothStatusLED, LOW);
  }
}
