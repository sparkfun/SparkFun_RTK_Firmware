/*
  https://esp32.com/viewtopic.php?t=9820
  https://esp32.com/viewtopic.php?t=7086
  https://github.com/pglen/esp32_cpu_load/blob/master/main/cpu_load.c
  https://esp32.com/viewtopic.php?t=11371
  https://forums.freertos.org/t/how-can-a-task-with-lower-priority-to-recover-from-starvation/11705/2

  The lowest priority should not block.
*/

#define MAX_CPU_CORES 2

TaskHandle_t idleTaskHandle0 = nullptr;
uint32_t maxIdleCount0 = 0;
uint32_t idleCount0 = 0;

TaskHandle_t idleTaskHandle1 = nullptr;
uint32_t maxIdleCount1 = 0;
uint32_t idleCount1 = 0;

unsigned long lastLoadTest = 0;

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("ESP32 Core Loads");

  beginIdleTasks();

  //Calculate the 100% idle amount
  idleCount0 = 0;
  idleCount1 = 0;
  delay(10); //Allow the idle tasks to freely run up counter
  maxIdleCount0 = idleCount0 * 100;
  maxIdleCount1 = idleCount1 * 100;

  Serial.printf("idleCount0: %d\r\n", idleCount0);
  Serial.printf("maxIdleCount0: %d\r\n", maxIdleCount0);
  Serial.printf("idleCount1: %d\r\n", idleCount1);
  Serial.printf("maxIdleCount1: %d\r\n", maxIdleCount1);
}

void loop()
{
  // Every 5 seconds, we put the processor to work.
  if (millis() - lastLoadTest > 5000)
  {
    lastLoadTest = millis();
    Serial.print("Loading core - ");

    //delayMicroseconds(100000); //Blocks the 0 priority task for 10%

    //Do a million floating point calcs to tie up the processor for ~20%
    double counter = 1.0;
    for (uint32_t x = 0 ; x < 1000000; x++)
      counter *= 3.14159;
    //      counter *= 1.000001;

    //We have to actually use/print the variable otherwise compiler will dispose of unused chars/code
    //and there will be no load on the processor
    Serial.printf("counter: %0.2f\r\n", counter);
  }

  lowerTaskYield(); //Allows lower priority (0) tasks to run - block the active task so the idle task can run and feed the watchdog
  taskYIELD(); //Yields to tasks with the same priority or higher
}
