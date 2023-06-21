/*
  Lee Leahy
  12 June 2023

  Determine the performance of the PSRAM.

  Based upon: https://thingpulse.com/esp32-how-to-use-psram/
         and: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/external-ram.html
*/

#include <Arduino.h>

#define MAX_LOOP_COUNT      (1 * 1000 * 1000)
#define PSRAM_BYTES         (2 * 1024 * 1024)

volatile uint8_t * buffer;
volatile uint8_t internalRam;
int psramBytes;
uint32_t psramPageSize;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println();
    Serial.print("PSRAM Size: ");
    Serial.print(ESP.getPsramSize());
    Serial.println();
    Serial.println("PSRAM Access Times");
}

void loop()
{
    int32_t index;
    uint8_t junk;
    uint32_t loopOverhead;
    uint32_t microsEnd;
    uint32_t microsStart;
    uint32_t offset;

    // Read data from the data cache
    buffer = (volatile uint8_t *)ps_malloc(PSRAM_BYTES);
    Serial.print("PSRAM buffer: ");
    Serial.printf("%p\r\n", buffer);
    Serial.flush();

    // Compute the loop overhead
    microsStart = micros();
    for (index = 0; index < MAX_LOOP_COUNT; index++)
    {
    }
    microsEnd = micros();
    displayTime(microsStart, microsEnd, 0, "Loop overhead");
    loopOverhead = microsEnd - microsStart;

    // Prime the data
    junk = internalRam;

    // Read data from the internal RAM
    microsStart = micros();
    for (index = 0; index < MAX_LOOP_COUNT; index++)
        junk = internalRam;
    microsEnd = micros();
    displayTime(microsStart, microsEnd, loopOverhead, "Internal RAM access");

    if (buffer)
    {
        // Measure cached PSRAM access
        microsStart = micros();
        for (index = 0; index < MAX_LOOP_COUNT; index++)
            junk = buffer[0];
        microsEnd = micros();
        displayTime(microsStart, microsEnd, loopOverhead, "Cached PSRAM access");

        // Separate the two sets of measurements
        Serial.println();

        // Compute the loop overhead
        microsStart = micros();
        for (index = 0; index < MAX_LOOP_COUNT; index++)
        {
            junk = buffer[offset];
            offset = (offset + psramPageSize) & (PSRAM_BYTES - 1);
        }
        microsEnd = micros();
        displayTime(microsStart, microsEnd, 0, "Loop overhead");
        loopOverhead = microsEnd - microsStart;

        // Measure sequential accesses to PSRAM
        for (psramPageSize = 1; psramPageSize <= 4096; psramPageSize <<= 1)
        {
            microsStart = micros();
            for (index = 0; index < MAX_LOOP_COUNT; index++)
            {
                junk = buffer[offset];
                offset = (offset + psramPageSize) & (PSRAM_BYTES - 1);
            }
            microsEnd = micros();
            sprintf((char *)buffer, "Uncached PSRAM access, %4d byte page size", psramPageSize);
            displayTime(microsStart, microsEnd, loopOverhead, (const char *)buffer);
        }
    }

    // Done
    while (1);
}

void displayTime(uint32_t microsStart, uint32_t microsEnd, uint32_t loopOverhead, const char * string)
{
    uint32_t delta;
    float nanoSeconds;
    char text[128];

    // Display the cache read time
    delta = microsEnd - microsStart - loopOverhead;
    nanoSeconds = (double)delta / 1000.;
    sprintf(text, "%s: %8d - %8d - %5d = %7d: %7.3f nSec",
            string, microsEnd, microsStart, loopOverhead, delta, nanoSeconds);
    Serial.println(text);
    Serial.flush();
}
