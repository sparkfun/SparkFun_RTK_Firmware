
void beginIdleTasks()
{
  if (idleTaskHandle0 == nullptr)
    xTaskCreatePinnedToCore(
      idleTask0, // Function to call
      "IdleTask0", // Just for humans
      2000,     // Stack Size
      nullptr,  // Task input parameter
      0,        // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
      &idleTaskHandle0, // Task handle
      0);                 // Core where task should run, 0=core, 1=Arduino

  if (idleTaskHandle1 == nullptr)
    xTaskCreatePinnedToCore(
      idleTask1, // Function to call
      "IdleTask1", // Just for humans
      2000,     // Stack Size
      nullptr,  // Task input parameter
      0,        // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
      &idleTaskHandle1, // Task handle
      1);                 // Core where task should run, 0=core, 1=Arduino
}

void idleTask0(void *e)
{
  uint32_t lastDisplayIdleTime0 = 0;

  while (1)
  {
    idleCount0++; // Increment a count during the idle time

    // Report idle time periodically
    if (millis() - lastDisplayIdleTime0 > 1000)
    {
      lastDisplayIdleTime0 = millis();

      if (idleCount0 > maxIdleCount0)
        maxIdleCount0 = idleCount0;

      Serial.printf("CPU 0 idle time: %d%% (idleCount0: %d/ maxIdleCount0: %d)\r\n", idleCount0 * 100 / maxIdleCount0, idleCount0,
                    maxIdleCount0);

      //        Serial.printf("%d Tasks\r\n", uxTaskGetNumberOfTasks());

      idleCount0 = 0; // Restart the idle count
    }
    //The idle task should NOT delay or yield
  }
}

void idleTask1(void *e)
{
  uint32_t lastDisplayIdleTime1 = 0;

  while (1)
  {
    idleCount1++; // Increment a count during the idle time

    // Report idle time periodically
    if (millis() - lastDisplayIdleTime1 > 1000)
    {
      lastDisplayIdleTime1 = millis();

      if (idleCount1 > maxIdleCount1)
        maxIdleCount1 = idleCount1;

      Serial.printf("CPU 1 idle time: %d%% (idleCount1: %d/ maxIdleCount1: %d)\r\n", idleCount1 * 100 / maxIdleCount1, idleCount1,
                    maxIdleCount1);

      //        Serial.printf("%d Tasks\r\n", uxTaskGetNumberOfTasks());

      idleCount1 = 0; // Restart the idle count
    }
    //The idle task should NOT delay or yield
  }
}

// Normally a delay(1) will feed the WDT but if we don't want to wait that long,
// this allows lower priority tasks to run including feeding the WDT with minimum delay
void lowerTaskYield()
{
  vTaskDelay(1); //How long is vTaskDelay(1)? 1ms. Similar to delay(1)

  //delayMicroseconds(100); //By itself, does not allow lower priority tasks to run
}
