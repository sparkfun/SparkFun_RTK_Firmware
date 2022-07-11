/*
  Idle loop
  By: Lee Leahy
  SparkFun Electronics
  Date: July 9th, 2022
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example determines the count for the idle loop
*/

#define testTimeSecs              (3 * 60)

uint32_t startTime;
uint32_t idleCount;

void setup()
{
  Serial.begin(115200);
  idleCount = 0;
  startTime = millis();
}

void loop()
{
  //Query module only every second.
  //The module only responds when a new position is available.
  while ((millis() - startTime) < (testTimeSecs * 1000))
  {
    idleCount++;
    yield();
  }

  //Display the idle count
  Serial.printf("Count / Second = %d\r\n", idleCount / testTimeSecs);

  //Done
  while(1)
    delay(1000);
}
