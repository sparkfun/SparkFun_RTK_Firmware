volatile static uint16_t dataHead = 0; //Head advances as data comes in from GNSS's UART
volatile static uint16_t sdTail = 0; //SD Tail advances as it is recorded to SD

int bufferOverruns = 0;

//Read bytes from UART2 into circular buffer
void F9PSerialReadTask(void *e)
{
  bool newDataLost = false;

  while (true)
  {
    //Determine the amount of microSD card logging data in the buffer
    if (online.logging)
    {
      if (serialGNSS.available())
      {
        int usedBytes = dataHead - sdTail; //Distance between head and tail
        if (usedBytes < 0)
          usedBytes += gnssHandlerBufferSize;

        int availableHandlerSpace = gnssHandlerBufferSize - usedBytes;

        //Don't fill the last byte to prevent buffer overflow
        if (availableHandlerSpace)
          availableHandlerSpace -= 1;

        //Check for buffer overruns
        //int availableUARTSpace = settings.uartReceiveBufferSize - serialGNSS.available();
        int availableUARTSpace = uartReceiveBufferSize - serialGNSS.available();
        int combinedSpaceRemaining = availableHandlerSpace + availableUARTSpace;

        //log_d("availableHandlerSpace = %d availableUARTSpace = %d bufferOverruns: %d", availableHandlerSpace, availableUARTSpace, bufferOverruns);

        if (combinedSpaceRemaining == 0)
        {
          if (newDataLost == false)
          {
            newDataLost = true;
            bufferOverruns++;
            //if (settings.enablePrintBufferOverrun)
            //Serial.sprintf("Data loss likely! availableHandlerSpace = %d availableUARTSpace = %d bufferOverruns: %d\n\r", availableHandlerSpace, availableUARTSpace, bufferOverruns);
            log_d("Data loss likely! availableHandlerSpace = %d availableUARTSpace = %d bufferOverruns: %d", availableHandlerSpace, availableUARTSpace, bufferOverruns);
          }
        }
        //else if (combinedSpaceRemaining < ( (gnssHandlerBufferSize + settings.uartReceiveBufferSize) / 16))
        else if (combinedSpaceRemaining < ( (gnssHandlerBufferSize + uartReceiveBufferSize) / 16))
        {
          //if (settings.enablePrintBufferOverrun)
          log_d("Low space: availableHandlerSpace = %d availableUARTSpace = %d bufferOverruns: %d", availableHandlerSpace, availableUARTSpace, bufferOverruns);
        }
        else
          newDataLost = false; //Reset

        //While there is free buffer space and UART2 has at least one RX byte
        while (availableHandlerSpace && serialGNSS.available())
        {
          //Fill the buffer to the end and then start at the beginning
          int availableSlice = availableHandlerSpace;
          if ((dataHead + availableHandlerSpace) >= gnssHandlerBufferSize)
            availableSlice = gnssHandlerBufferSize - dataHead;

          //Add new data into circular buffer in front of the head
          //availableHandlerSpace is already reduced to avoid buffer overflow
          int newBytesToRecord = serialGNSS.read(&rBuffer[dataHead], availableSlice);

          //Check for negative (error)
          if (newBytesToRecord <= 0)
            break;

          //Account for the bytes read
          availableHandlerSpace -= newBytesToRecord;

          dataHead += newBytesToRecord;
          if (dataHead >= gnssHandlerBufferSize)
            dataHead -= gnssHandlerBufferSize;

          delay(1);
          taskYIELD();
        } //End Serial.available()
      }
    }

    delay(1);
    taskYIELD();
  }
}

//Log data to the SD card
void SDWriteTask(void *e)
{
  while (true)
  {
    int lastDataHead = dataHead; //dataHead may change during this task by the harvesting task. Use a snapshot.

    //If user wants to log, record to SD
    if (!online.logging)
      //Discard the data
      sdTail = lastDataHead;
    else
    {
      int sdBytesToRecord = lastDataHead - sdTail; //Amount of buffered microSD card logging data
      if (sdBytesToRecord < 0)
        sdBytesToRecord += gnssHandlerBufferSize;

      if (sdBytesToRecord > 0)
      {
        //Reduce bytes to record to SD if we have more to send then the end of the buffer
        //We'll wrap next loop
        if ((sdTail + sdBytesToRecord) > gnssHandlerBufferSize)
          sdBytesToRecord = gnssHandlerBufferSize - sdTail;

        long startTime = millis();
        int recordedBytes = ubxFile.write(&rBuffer[sdTail], sdBytesToRecord);
        long endTime = millis();

        if (endTime - startTime > 150) log_d("Long Write! Delta time: %d / Recorded %d bytes", endTime - startTime, recordedBytes);

        fileSize = ubxFile.fileSize(); //Get updated filed size

        //Account for the sent data or dropped
        if (recordedBytes > 0)
        {
          sdTail += recordedBytes;
          if (sdTail >= gnssHandlerBufferSize)
            sdTail -= gnssHandlerBufferSize;
        }

        //Force file sync every 60s
        if (millis() - lastUBXLogSyncTime > 60000)
        {
          ubxFile.sync();
          lastUBXLogSyncTime = millis();
        } //End sdCardSemaphore

      } //End logging
    }

    delay(1);
    taskYIELD();
  }
}

//Stop UART and SD tasks - useful when running firmware update or WiFi AP is running
void stopTasks()
{
  //Delete tasks if running
  if (F9PSerialReadTaskHandle != NULL)
  {
    vTaskDelete(F9PSerialReadTaskHandle);
    F9PSerialReadTaskHandle = NULL;
  }
  if (SDWriteTaskHandle != NULL)
  {
    vTaskDelete(SDWriteTaskHandle);
    SDWriteTaskHandle = NULL;
  }

  //Give the other CPU time to finish
  //Eliminates CPU bus hang condition
  delay(100);
}
