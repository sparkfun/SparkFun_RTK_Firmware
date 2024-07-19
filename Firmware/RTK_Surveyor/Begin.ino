/*------------------------------------------------------------------------------
Begin.ino

  This module implements the initial startup functions for GNSS, SD, display,
  radio, etc.
------------------------------------------------------------------------------*/

//----------------------------------------
// Constants
//----------------------------------------

#define MAX_ADC_VOLTAGE 3300 // Millivolts

// Testing shows the combined ADC+resistors is under a 1% window
#define TOLERANCE 5.20 // Percent:  94.8% - 105.2%

//----------------------------------------
// Hardware initialization functions
//----------------------------------------
// Determine if the measured value matches the product ID value
// idWithAdc applies resistor tolerance using worst-case tolerances:
// Upper threshold: R1 down by TOLERANCE, R2 up by TOLERANCE
// Lower threshold: R1 up by TOLERANCE, R2 down by TOLERANCE
bool idWithAdc(uint16_t mvMeasured, float r1, float r2)
{
    float lowerThreshold;
    float upperThreshold;

    //                                ADC input
    //                       r1 KOhms     |     r2 KOhms
    //  MAX_ADC_VOLTAGE -----/\/\/\/\-----+-----/\/\/\/\----- Ground

    // Return true if the mvMeasured value is within the tolerance range
    // of the mvProduct value
    upperThreshold = ceil(MAX_ADC_VOLTAGE * (r2 * (1.0 + (TOLERANCE / 100.0))) /
                          ((r1 * (1.0 - (TOLERANCE / 100.0))) + (r2 * (1.0 + (TOLERANCE / 100.0)))));
    lowerThreshold = floor(MAX_ADC_VOLTAGE * (r2 * (1.0 - (TOLERANCE / 100.0))) /
                           ((r1 * (1.0 + (TOLERANCE / 100.0))) + (r2 * (1.0 - (TOLERANCE / 100.0)))));

    // systemPrintf("r1: %0.2f r2: %0.2f lowerThreshold: %0.0f mvMeasured: %d upperThreshold: %0.0f\r\n", r1, r2,
    // lowerThreshold, mvMeasured, upperThreshold);

    return (upperThreshold > mvMeasured) && (mvMeasured > lowerThreshold);
}

// Use a pair of resistors on pin 35 to ID the board type
// If the ID resistors are not available then use a variety of other methods
// (I2C, GPIO test, etc) to ID the board.
// Assume no hardware interfaces have been started so we need to start/stop any hardware
// used in tests accordingly.
void identifyBoard()
{
    // Use ADC to check the resistor divider
    int pin_deviceID = 35;
    uint16_t idValue = analogReadMilliVolts(pin_deviceID);
    log_d("Board ADC ID (mV): %d", idValue);

    // Order the following ID checks, by millivolt values high to low

    // Facet L-Band Direct: 4.7/1  -->  534mV < 579mV < 626mV
    if (idWithAdc(idValue, 4.7, 1))
        productVariant = RTK_FACET_LBAND_DIRECT;

    // Express: 10/3.3  -->  761mV < 819mV < 879mV
    else if (idWithAdc(idValue, 10, 3.3))
        productVariant = RTK_EXPRESS;

    // Reference Station: 20/10  -->  1031mV < 1100mV < 1171mV
    else if (idWithAdc(idValue, 20, 10))
    {
        productVariant = REFERENCE_STATION;
        // We can't auto-detect the ZED version if the firmware is in configViaEthernet mode,
        // so fake it here - otherwise messageSupported always returns false
        zedFirmwareVersionInt = 112;
    }
    // Facet: 10/10  -->  1571mV < 1650mV < 1729mV
    else if (idWithAdc(idValue, 10, 10))
        productVariant = RTK_FACET;

    // Facet L-Band: 10/20  -->  2129mV < 2200mV < 2269mV
    else if (idWithAdc(idValue, 10, 20))
        productVariant = RTK_FACET_LBAND;

    // Express+: 3.3/10  -->  2421mV < 2481mV < 2539mV
    else if (idWithAdc(idValue, 3.3, 10))
        productVariant = RTK_EXPRESS_PLUS;

    // ID resistors do not exist for the following:
    //      Surveyor
    //      Unknown
    else
    {
        log_d("Out of band or nonexistent resistor IDs");
        productVariant = RTK_UNKNOWN; // Need to wait until the GNSS and Accel have been initialized
    }
}

// Setup any essential power pins
// E.g. turn on power for the display before beginDisplay
void initializePowerPins()
{
    if (productVariant == REFERENCE_STATION)
    {
        // v10
        // Pin Allocations:
        // D0  : Boot + Boot Button
        // D1  : Serial TX (CH340 RX)
        // D2  : SDIO DAT0 - via 74HC4066 switch
        // D3  : Serial RX (CH340 TX)
        // D4  : SDIO DAT1
        // D5  : GNSS Chip Select
        // D12 : SDIO DAT2 - via 74HC4066 switch
        // D13 : SDIO DAT3
        // D14 : SDIO CLK
        // D15 : SDIO CMD - via 74HC4066 switch
        // D16 : Serial1 RXD : Note: connected to the I/O connector only - not to the ZED-F9P
        // D17 : Serial1 TXD : Note: connected to the I/O connector only - not to the ZED-F9P
        // D18 : SPI SCK
        // D19 : SPI POCI
        // D21 : I2C SDA
        // D22 : I2C SCL
        // D23 : SPI PICO
        // D25 : GNSS Time Pulse
        // D26 : STAT LED
        // D27 : Ethernet Chip Select
        // D32 : PWREN
        // D33 : Ethernet Interrupt
        // A34 : GNSS TX RDY
        // A35 : Board Detect (1.1V)
        // A36 : microSD card detect
        // A39 : Unused analog pin - used to generate random values for SSL

        pin_baseStatusLED = 26;
        pin_peripheralPowerControl = 32;
        pin_Ethernet_CS = 27;
        pin_GNSS_CS = 5;
        pin_GNSS_TimePulse = 25;
        pin_adc39 = 39;
        pin_zed_tx_ready = 34;
        pin_microSD_CardDetect = 36;
        pin_Ethernet_Interrupt = 33;
        pin_setupButton = 0;

        pin_radio_rx = 17; // Radio RX In = ESP TX Out
        pin_radio_tx = 16; // Radio TX Out = ESP RX In

        pinMode(pin_Ethernet_CS, OUTPUT);
        digitalWrite(pin_Ethernet_CS, HIGH);
        pinMode(pin_GNSS_CS, OUTPUT);
        digitalWrite(pin_GNSS_CS, HIGH);

        pinMode(pin_peripheralPowerControl, OUTPUT);
        digitalWrite(pin_peripheralPowerControl, HIGH); // Turn on SD, W5500, etc
        delay(100);
    }
}

// Based on hardware features, determine if this is RTK Surveyor or RTK Express hardware
// Must be called after beginI2C (Wire.begin) so that we can do I2C tests
// Must be called after beginGNSS so the GNSS type is known
void beginBoard()
{
    if (productVariant == RTK_UNKNOWN)
    {
        if (isConnected(0x19) == true) // Check for accelerometer
        {
            if (zedModuleType == PLATFORM_F9P)
                productVariant = RTK_EXPRESS;
            else if (zedModuleType == PLATFORM_F9R)
                productVariant = RTK_EXPRESS_PLUS;
        }
        else
        {
            // Detect RTK Expresses (v1.3 and below) that do not have an accel or device ID resistors

            // On a Surveyor, pin 34 is not connected. On Express, 34 is connected to ZED_TX_READY
            const int pin_ZedTxReady = 34;
            uint16_t pinValue = analogReadMilliVolts(pin_ZedTxReady);
            log_d("Alternate ID pinValue (mV): %d\r\n", pinValue); // Surveyor = 142 to 152, //Express = 3129
            if (pinValue > 3000)
            {
                if (zedModuleType == PLATFORM_F9P)
                    productVariant = RTK_EXPRESS;
                else if (zedModuleType == PLATFORM_F9R)
                    productVariant = RTK_EXPRESS_PLUS;
            }
            else
                productVariant = RTK_SURVEYOR;
        }
    }

    // Setup hardware pins
    if (productVariant == RTK_SURVEYOR)
    {
        pin_batteryLevelLED_Red = 32;
        pin_batteryLevelLED_Green = 33;
        pin_positionAccuracyLED_1cm = 2;
        pin_positionAccuracyLED_10cm = 15;
        pin_positionAccuracyLED_100cm = 13;
        pin_baseStatusLED = 4;
        pin_bluetoothStatusLED = 12;
        pin_setupButton = 5;
        pin_microSD_CS = 25;
        pin_zed_tx_ready = 26;
        pin_zed_reset = 27;
        pin_batteryLevel_alert = 36;

        // Bug in ZED-F9P v1.13 firmware causes RTK LED to not light when RTK Floating with SBAS on.
        // The following changes the POR default but will be overwritten by settings in NVM or settings file
        settings.ubxConstellations[1].enabled = false;
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
    {
        pin_muxA = 2;
        pin_muxB = 4;
        pin_powerSenseAndControl = 13;
        pin_setupButton = 14;
        pin_microSD_CS = 25;
        pin_dac26 = 26;
        pin_powerFastOff = 27;
        pin_adc39 = 39;

        pinMode(pin_powerSenseAndControl, INPUT_PULLUP);
        pinMode(pin_powerFastOff, INPUT);

        if (esp_reset_reason() == ESP_RST_POWERON)
        {
            powerOnCheck(); // Only do check if we POR start
        }

        pinMode(pin_setupButton, INPUT_PULLUP);

        setMuxport(settings.dataPortChannel); // Set mux to user's choice: NMEA, I2C, PPS, or DAC
    }
    else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND ||
             productVariant == RTK_FACET_LBAND_DIRECT)
    {
        // v11
        pin_muxA = 2;
        pin_muxB = 0;
        pin_powerSenseAndControl = 13;
        pin_peripheralPowerControl = 14;
        pin_microSD_CS = 25;
        pin_dac26 = 26;
        pin_powerFastOff = 27;
        pin_adc39 = 39;

        pin_radio_rx = 33;
        pin_radio_tx = 32;
        pin_radio_rst = 15;
        pin_radio_pwr = 4;
        pin_radio_cts = 5;
        // pin_radio_rts = 255; //Not implemented

        pinMode(pin_powerSenseAndControl, INPUT_PULLUP);
        pinMode(pin_powerFastOff, INPUT);

        if (esp_reset_reason() == ESP_RST_POWERON)
        {
            powerOnCheck(); // Only do check if we POR start
        }

        pinMode(pin_peripheralPowerControl, OUTPUT);
        digitalWrite(pin_peripheralPowerControl, HIGH); // Turn on SD, ZED, etc

        setMuxport(settings.dataPortChannel); // Set mux to user's choice: NMEA, I2C, PPS, or DAC

        // CTS is active low. ESP32 pin 5 has pullup at POR. We must drive it low.
        pinMode(pin_radio_cts, OUTPUT);
        digitalWrite(pin_radio_cts, LOW);

        if (productVariant == RTK_FACET_LBAND_DIRECT)
        {
            // Override the default setting if a user has not explicitly configured the setting
            if (settings.useI2cForLbandCorrectionsConfigured == false)
                settings.useI2cForLbandCorrections = false;
        }
    }
    else if (productVariant == REFERENCE_STATION)
    {
        // No powerOnCheck

        settings.enablePrintBatteryMessages = false; // No pesky battery messages
    }

    displaySfeFlame();

    char versionString[21];
    getFirmwareVersion(versionString, sizeof(versionString), true);
    systemPrintf("SparkFun RTK %s %s\r\n", platformPrefix, versionString);

    // Get unit MAC address
    esp_read_mac(wifiMACAddress, ESP_MAC_WIFI_STA);
    memcpy(btMACAddress, wifiMACAddress, sizeof(wifiMACAddress));
    btMACAddress[5] +=
        2; // Convert MAC address to Bluetooth MAC (add 2):
           // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address
    memcpy(ethernetMACAddress, wifiMACAddress, sizeof(wifiMACAddress));
    ethernetMACAddress[5] += 3; // Convert MAC address to Ethernet MAC (add 3)

    // For all boards, check reset reason. If reset was due to wdt or panic, append last log
    loadSettingsPartial(); // Loads settings from LFS
    if ((esp_reset_reason() == ESP_RST_POWERON) || (esp_reset_reason() == ESP_RST_SW))
    {
        reuseLastLog = false; // Start new log

        if (settings.enableResetDisplay == true)
        {
            settings.resetCount = 0;
            recordSystemSettingsToFileLFS(settingsFileName); // Avoid overwriting LittleFS settings onto SD
        }
        settings.resetCount = 0;
    }
    else
    {
        reuseLastLog = true; // Attempt to reuse previous log

        if (settings.enableResetDisplay == true)
        {
            settings.resetCount++;
            systemPrintf("resetCount: %d\r\n", settings.resetCount);
            recordSystemSettingsToFileLFS(settingsFileName); // Avoid overwriting LittleFS settings onto SD
        }

        systemPrint("Reset reason: ");
        switch (esp_reset_reason())
        {
        case ESP_RST_UNKNOWN:
            systemPrintln("ESP_RST_UNKNOWN");
            break;
        case ESP_RST_POWERON:
            systemPrintln("ESP_RST_POWERON");
            break;
        case ESP_RST_SW:
            systemPrintln("ESP_RST_SW");
            break;
        case ESP_RST_PANIC:
            systemPrintln("ESP_RST_PANIC");
            break;
        case ESP_RST_INT_WDT:
            systemPrintln("ESP_RST_INT_WDT");
            break;
        case ESP_RST_TASK_WDT:
            systemPrintln("ESP_RST_TASK_WDT");
            break;
        case ESP_RST_WDT:
            systemPrintln("ESP_RST_WDT");
            break;
        case ESP_RST_DEEPSLEEP:
            systemPrintln("ESP_RST_DEEPSLEEP");
            break;
        case ESP_RST_BROWNOUT:
            systemPrintln("ESP_RST_BROWNOUT");
            break;
        case ESP_RST_SDIO:
            systemPrintln("ESP_RST_SDIO");
            break;
        default:
            systemPrintln("Unknown");
        }
    }
}

void beginSD()
{
    if(sdCardForcedOffline == true)
        return;

    bool gotSemaphore;

    online.microSD = false;
    gotSemaphore = false;

    while (settings.enableSD == true)
    {
        // Setup SD card access semaphore
        if (sdCardSemaphore == nullptr)
            sdCardSemaphore = xSemaphoreCreateMutex();
        else if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_shortWait_ms) != pdPASS)
        {
            // This is OK since a retry will occur next loop
            log_d("sdCardSemaphore failed to yield, Begin.ino line %d", __LINE__);
            break;
        }
        gotSemaphore = true;
        markSemaphore(FUNCTION_BEGINSD);

        if (USE_SPI_MICROSD)
        {
            log_d("Initializing microSD - using SPI, SdFat and SdFile");

            pinMode(pin_microSD_CS, OUTPUT);
            digitalWrite(pin_microSD_CS, HIGH); // Be sure SD is deselected
            resetSPI();                         // Re-initialize the SPI/SD interface

            // Do a quick test to see if a card is present
            int tries = 0;
            int maxTries = 5;
            while (tries < maxTries)
            {
                if (sdPresent() == true)
                    break;
                // log_d("SD present failed. Trying again %d out of %d", tries + 1, maxTries);

                // Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
                // Max current is 200mA average across 1s, peak 300mA
                delay(10);
                tries++;
            }
            if (tries == maxTries)
                break; // Give up loop

            // If an SD card is present, allow SdFat to take over
            log_d("SD card detected - using SPI and SdFat");

            // Allocate the data structure that manages the microSD card
            if (!sd)
            {
                sd = new SdFat();
                if (!sd)
                {
                    log_d("Failed to allocate the SdFat structure!");
                    break;
                }
            }

            if (settings.spiFrequency > 16)
            {
                systemPrintln("Error: SPI Frequency out of range. Default to 16MHz");
                settings.spiFrequency = 16;
            }

            resetSPI(); // Re-initialize the SPI/SD interface

            if (sd->begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == false)
            {
                tries = 0;
                maxTries = 1;
                for (; tries < maxTries; tries++)
                {
                    log_d("SD init failed - using SPI and SdFat. Trying again %d out of %d", tries + 1, maxTries);

                    delay(250); // Give SD more time to power up, then try again
                    if (sd->begin(SdSpiConfig(pin_microSD_CS, SHARED_SPI, SD_SCK_MHZ(settings.spiFrequency))) == true)
                        break;
                }

                if (tries == maxTries)
                {
                    systemPrintln("SD init failed - using SPI and SdFat. Is card formatted?");
                    digitalWrite(pin_microSD_CS, HIGH); // Be sure SD is deselected

                    sdCardForcedOffline = true; //Prevent future scans for SD cards

                    // Check reset count and prevent rolling reboot
                    if (settings.resetCount < 5)
                    {
                        if (settings.forceResetOnSDFail == true)
                            ESP.restart();
                    }
                    break;
                }
            }

            // Change to root directory. All new file creation will be in root.
            if (sd->chdir() == false)
            {
                systemPrintln("SD change directory failed");
                break;
            }
        }
#ifdef COMPILE_SD_MMC
        else
        {
            // Check to see if a card is present
            if (sdPresent() == false)
                break; // Give up on loop

            systemPrintln("Initializing microSD - using SDIO, SD_MMC and File");

            // SDIO MMC
            if (SD_MMC.begin() == false)
            {
                int tries = 0;
                int maxTries = 1;
                for (; tries < maxTries; tries++)
                {
                    log_d("SD init failed - using SD_MMC. Trying again %d out of %d", tries + 1, maxTries);

                    delay(250); // Give SD more time to power up, then try again
                    if (SD_MMC.begin() == true)
                        break;
                }

                if (tries == maxTries)
                {
                    systemPrintln("SD init failed - using SD_MMC. Is card formatted?");

                    // Check reset count and prevent rolling reboot
                    if (settings.resetCount < 5)
                    {
                        if (settings.forceResetOnSDFail == true)
                            ESP.restart();
                    }
                    break;
                }
            }
        }
#else  // COMPILE_SD_MMC
        else
        {
            log_d("SD_MMC not compiled");
            break; // No SD available.
        }
#endif // COMPILE_SD_MMC

        if (createTestFile() == false)
        {
            systemPrintln("Failed to create test file. Format SD card with 'SD Card Formatter'.");
            displaySDFail(5000);
            break;
        }

        // Load firmware file from the microSD card if it is present
        scanForFirmware();

        // Mark card not yet usable for logging
        sdCardSize = 0;
        outOfSDSpace = true;

        systemPrintln("microSD: Online");
        online.microSD = true;
        break;
    }

    // Free the semaphore
    if (sdCardSemaphore && gotSemaphore)
        xSemaphoreGive(sdCardSemaphore); // Make the file system available for use
}

void endSD(bool alreadyHaveSemaphore, bool releaseSemaphore)
{
    // Disable logging
    endLogging(alreadyHaveSemaphore, false);

    // Done with the SD card
    if (online.microSD)
    {
        if (USE_SPI_MICROSD)
            sd->end();
#ifdef COMPILE_SD_MMC
        else
            SD_MMC.end();
#endif // COMPILE_SD_MMC

        online.microSD = false;
        systemPrintln("microSD: Offline");
    }

    // Free the caches for the microSD card
    if (USE_SPI_MICROSD)
    {
        if (sd)
        {
            delete sd;
            sd = nullptr;
        }
    }

    // Release the semaphore
    if (releaseSemaphore)
        xSemaphoreGive(sdCardSemaphore);
}

// Attempt to de-init the SD card - SPI only
// https://github.com/greiman/SdFat/issues/351
void resetSPI()
{
    if (USE_SPI_MICROSD)
    {
        pinMode(pin_microSD_CS, OUTPUT);
        digitalWrite(pin_microSD_CS, HIGH); // De-select SD card

        // Flush SPI interface
        SPI.begin();
        SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
        for (int x = 0; x < 10; x++)
            SPI.transfer(0XFF);
        SPI.endTransaction();
        SPI.end();

        digitalWrite(pin_microSD_CS, LOW); // Select SD card

        // Flush SD interface
        SPI.begin();
        SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
        for (int x = 0; x < 10; x++)
            SPI.transfer(0XFF);
        SPI.endTransaction();
        SPI.end();

        digitalWrite(pin_microSD_CS, HIGH); // Deselet SD card
    }
}

// We want the UART2 interrupts to be pinned to core 0 to avoid competing with I2C interrupts
// We do not start the UART2 for GNSS->BT reception here because the interrupts would be pinned to core 1
// We instead start a task that runs on core 0, that then begins serial
// See issue: https://github.com/espressif/arduino-esp32/issues/3386
void beginUART2()
{
    size_t length;

    // Determine the length of data to be retained in the ring buffer
    // after discarding the oldest data
    length = settings.gnssHandlerBufferSize;
    rbOffsetEntries = (length >> 1) / AVERAGE_SENTENCE_LENGTH_IN_BYTES;
    length = settings.gnssHandlerBufferSize + (rbOffsetEntries * sizeof(RING_BUFFER_OFFSET));
    ringBuffer = nullptr;
    rbOffsetArray = (RING_BUFFER_OFFSET *)malloc(length);
    if (!rbOffsetArray)
    {
        rbOffsetEntries = 0;
        systemPrintln("ERROR: Failed to allocate the ring buffer!");
    }
    else
    {
        ringBuffer = (uint8_t *)&rbOffsetArray[rbOffsetEntries];
        rbOffsetArray[0] = 0;
        if (pinUART2TaskHandle == nullptr)
            xTaskCreatePinnedToCore(
                pinUART2Task,
                "UARTStart", // Just for humans
                2000,        // Stack Size
                nullptr,     // Task input parameter
                0,           // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
                &pinUART2TaskHandle,              // Task handle
                settings.gnssUartInterruptsCore); // Core where task should run, 0=core, 1=Arduino

        while (uart2pinned == false) // Wait for task to run once
            delay(1);
    }
}

// Assign UART2 interrupts to the core that started the task. See:
// https://github.com/espressif/arduino-esp32/issues/3386
void pinUART2Task(void *pvParameters)
{
    // Note: ESP32 2.0.6 does some strange auto-bauding thing here which takes 20s to complete if there is no data for
    // it to auto-baud.
    //       That's fine for most RTK products, but causes the Ref Stn to stall for 20s. However, it doesn't stall with
    //       ESP32 2.0.2... Uncomment these lines to prevent the stall if/when we upgrade to ESP32 ~2.0.6.
    // #if defined(REF_STN_GNSS_DEBUG)
    //   if (ENABLE_DEVELOPER && productVariant == REFERENCE_STATION)
    // #else   // REF_STN_GNSS_DEBUG
    //   if (USE_I2C_GNSS)
    // #endif  // REF_STN_GNSS_DEBUG
    {
        serialGNSS.setRxBufferSize(
            settings.uartReceiveBufferSize); // TODO: work out if we can reduce or skip this when using SPI GNSS
        serialGNSS.setTimeout(settings.serialTimeoutGNSS); // Requires serial traffic on the UART pins for detection
        serialGNSS.begin(settings.dataPortBaud); // UART2 on pins 16/17 for SPP. The ZED-F9P will be configured to
                                                 // output NMEA over its UART1 at the same rate.

        // Reduce threshold value above which RX FIFO full interrupt is generated
        // Allows more time between when the UART interrupt occurs and when the FIFO buffer overruns
        // serialGNSS.setRxFIFOFull(50); //Available in >v2.0.5
        uart_set_rx_full_threshold(2, settings.serialGNSSRxFullThreshold); // uart_num, threshold
    }

    uart2pinned = true;

    vTaskDelete(nullptr); // Delete task once it has run once
}

void beginFS()
{
    if (online.fs == false)
    {
        if (LittleFS.begin(true) == false) // Format LittleFS if begin fails
        {
            systemPrintln("Error: LittleFS not online");
        }
        else
        {
            systemPrintln("LittleFS Started");
            online.fs = true;
        }
    }
}

// Check if configureViaEthernet.txt exists
// Used to indicate if SparkFun_WebServer_ESP32_W5500 needs _exclusive_ access to SPI and interrupts
bool checkConfigureViaEthernet()
{
    if (online.fs == false)
        return false;

    if (LittleFS.exists("/configureViaEthernet.txt"))
    {
        log_d("LittleFS configureViaEthernet.txt exists");
        LittleFS.remove("/configureViaEthernet.txt");
        return true;
    }

    return false;
}

// Force configure-via-ethernet mode by creating configureViaEthernet.txt in LittleFS
// Used to indicate if SparkFun_WebServer_ESP32_W5500 needs _exclusive_ access to SPI and interrupts
bool forceConfigureViaEthernet()
{
    if (online.fs == false)
        return false;

    if (LittleFS.exists("/configureViaEthernet.txt"))
    {
        log_d("LittleFS configureViaEthernet.txt already exists");
        return true;
    }

    File cveFile = LittleFS.open("/configureViaEthernet.txt", FILE_WRITE);
    cveFile.close();

    if (LittleFS.exists("/configureViaEthernet.txt"))
        return true;

    log_d("Unable to create configureViaEthernet.txt on LittleFS");
    return false;
}

// Connect to ZED module and identify particulars
void beginGNSS()
{
    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping beginGNSS");
        return;
    }

    // If we're using SPI, then increase the logging buffer
    if (USE_SPI_GNSS)
    {
        SPI.begin(); // Begin SPI here - beginSD has not yet been called

        // setFileBufferSize must be called _before_ .begin
        // Use gnssHandlerBufferSize for now. TODO: work out if the SPI GNSS needs its own buffer size setting
        // Also used by Tasks.ino
        theGNSS.setFileBufferSize(settings.gnssHandlerBufferSize);
        theGNSS.setRTCMBufferSize(settings.gnssHandlerBufferSize);
    }

    if (USE_I2C_GNSS)
    {
        if (theGNSS.begin() == false)
        {
            log_d("GNSS Failed to begin. Trying again.");

            // Try again with power on delay
            delay(1000); // Wait for ZED-F9P to power up before it can respond to ACK
            if (theGNSS.begin() == false)
            {
                log_d("GNSS offline");
                displayGNSSFail(1000);
                return;
            }
        }
    }
    else
    {
        if (theGNSS.begin(SPI, pin_GNSS_CS) == false)
        {
            log_d("GNSS Failed to begin. Trying again.");

            // Try again with power on delay
            delay(1000); // Wait for ZED-F9P to power up before it can respond to ACK
            if (theGNSS.begin(SPI, pin_GNSS_CS) == false)
            {
                log_d("GNSS offline");
                displayGNSSFail(1000);
                return;
            }
        }

        if (theGNSS.getFileBufferSize() != settings.gnssHandlerBufferSize) // Need to call getFileBufferSize after begin
        {
            log_d("GNSS offline - no RAM for file buffer");
            displayGNSSFail(1000);
            return;
        }
        if (theGNSS.getRTCMBufferSize() != settings.gnssHandlerBufferSize) // Need to call getRTCMBufferSize after begin
        {
            log_d("GNSS offline - no RAM for RTCM buffer");
            displayGNSSFail(1000);
            return;
        }
    }

    // Increase transactions to reduce transfer time
    if (USE_I2C_GNSS)
        theGNSS.i2cTransactionSize = 128;

    // Auto-send Valset messages before the buffer is completely full
    theGNSS.autoSendCfgValsetAtSpaceRemaining(16);

    // Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
    if (theGNSS.getModuleInfo(1100) == true) // Try to get the module info
    {
        // Reconstruct the firmware version
        snprintf(zedFirmwareVersion, sizeof(zedFirmwareVersion), "%s %d.%02d", theGNSS.getFirmwareType(),
                 theGNSS.getFirmwareVersionHigh(), theGNSS.getFirmwareVersionLow());

        // Construct the firmware version as uint8_t. Note: will fail above 2.55!
        zedFirmwareVersionInt = (theGNSS.getFirmwareVersionHigh() * 100) + theGNSS.getFirmwareVersionLow();

        // Check if this is known firmware
        //"1.20" - Mostly for F9R HPS 1.20, but also F9P HPG v1.20
        //"1.21" - F9R HPS v1.21
        //"1.30" - ZED-F9P (HPG) released Dec, 2021. Also ZED-F9R (HPS) released Sept, 2022
        //"1.32" - ZED-F9P released May, 2022
        //"1.50" - ZED-F9P released July, 2024

        const uint8_t knownFirmwareVersions[] = {100, 112, 113, 120, 121, 130, 132, 150};
        bool knownFirmware = false;
        for (uint8_t i = 0; i < (sizeof(knownFirmwareVersions) / sizeof(uint8_t)); i++)
        {
            if (zedFirmwareVersionInt == knownFirmwareVersions[i])
                knownFirmware = true;
        }

        if (!knownFirmware)
        {
            systemPrintf("Unknown firmware version: %s\r\n", zedFirmwareVersion);
            zedFirmwareVersionInt = 99; // 0.99 invalid firmware version
        }

        // Determine if we have a ZED-F9P (Express/Facet) or an ZED-F9R (Express Plus/Facet Plus)
        if (strstr(theGNSS.getModuleName(), "ZED-F9P") != nullptr)
            zedModuleType = PLATFORM_F9P;
        else if (strstr(theGNSS.getModuleName(), "ZED-F9R") != nullptr)
            zedModuleType = PLATFORM_F9R;
        else
        {
            systemPrintf("Unknown ZED module: %s\r\n", theGNSS.getModuleName());
            zedModuleType = PLATFORM_F9P;
        }

        printZEDInfo(); // Print module type and firmware version
    }

    UBX_SEC_UNIQID_data_t chipID;
    if (theGNSS.getUniqueChipId(&chipID))
    {
        snprintf(zedUniqueId, sizeof(zedUniqueId), "%s", theGNSS.getUniqueChipIdStr(&chipID));
    }

    online.gnss = true;
}

// Configuration can take >1s so configure during splash
void configureGNSS()
{
    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping configureGNSS");
        return;
    }

    if (online.gnss == false)
        return;

    // Check if the ubxMessageRates or ubxMessageRatesBase need to be defaulted
    checkMessageRates();

    theGNSS.setAutoPVTcallbackPtr(&storePVTdata); // Enable automatic NAV PVT messages with callback to storePVTdata
    theGNSS.setAutoHPPOSLLHcallbackPtr(
        &storeHPdata); // Enable automatic NAV HPPOSLLH messages with callback to storeHPdata
    theGNSS.setRTCM1005InputcallbackPtr(
        &storeRTCM1005data); // Configure a callback for RTCM 1005 - parsed from pushRawData
    theGNSS.setRTCM1006InputcallbackPtr(
        &storeRTCM1006data); // Configure a callback for RTCM 1006 - parsed from pushRawData

    if (HAS_GNSS_TP_INT)
        theGNSS.setAutoTIMTPcallbackPtr(
            &storeTIMTPdata); // Enable automatic TIM TP messages with callback to storeTIMTPdata

    if (HAS_ANTENNA_SHORT_OPEN)
    {
        theGNSS.newCfgValset();

        theGNSS.addCfgValset(UBLOX_CFG_HW_ANT_CFG_SHORTDET, 1); // Enable antenna short detection
        theGNSS.addCfgValset(UBLOX_CFG_HW_ANT_CFG_OPENDET, 1);  // Enable antenna open detection

        if (theGNSS.sendCfgValset())
        {
            theGNSS.setAutoMONHWcallbackPtr(
                &storeMONHWdata); // Enable automatic MON HW messages with callback to storeMONHWdata
        }
        else
        {
            systemPrintln("Failed to configure GNSS antenna detection");
        }
    }

    // Configuring the ZED can take more than 2000ms. We save configuration to
    // ZED so there is no need to update settings unless user has modified
    // the settings file or internal settings.
    if (settings.updateZEDSettings == false)
    {
        log_d("Skipping ZED configuration");
        return;
    }

    bool response = configureUbloxModule();
    if (response == false)
    {
        // Try once more
        systemPrintln("Failed to configure GNSS module. Trying again.");
        delay(1000);
        response = configureUbloxModule();

        if (response == false)
        {
            systemPrintln("Failed to configure GNSS module.");
            displayGNSSFail(1000);
            online.gnss = false;
            return;
        }
    }

    systemPrintln("GNSS configuration complete");
}

// Begin interrupts
void beginInterrupts()
{
    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping beginInterrupts");
        return;
    }

    if (HAS_GNSS_TP_INT) // If the GNSS Time Pulse is connected, use it as an interrupt to set the clock accurately
    {
        pinMode(pin_GNSS_TimePulse, INPUT);
        attachInterrupt(pin_GNSS_TimePulse, tpISR, RISING);
    }

#ifdef COMPILE_ETHERNET
    if (HAS_ETHERNET)
    {
        pinMode(pin_Ethernet_Interrupt, INPUT_PULLUP);                 // Prepare the interrupt pin
        attachInterrupt(pin_Ethernet_Interrupt, ethernetISR, FALLING); // Attach the interrupt
    }
#endif // COMPILE_ETHERNET
}

// Set LEDs for output and configure PWM
void beginLEDs()
{
    if (productVariant == RTK_SURVEYOR)
    {
        pinMode(pin_positionAccuracyLED_1cm, OUTPUT);
        pinMode(pin_positionAccuracyLED_10cm, OUTPUT);
        pinMode(pin_positionAccuracyLED_100cm, OUTPUT);
        pinMode(pin_baseStatusLED, OUTPUT);
        pinMode(pin_bluetoothStatusLED, OUTPUT);
        pinMode(pin_setupButton, INPUT_PULLUP); // HIGH = rover, LOW = base

        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
        digitalWrite(pin_positionAccuracyLED_100cm, LOW);
        digitalWrite(pin_baseStatusLED, LOW);
        digitalWrite(pin_bluetoothStatusLED, LOW);

        ledcSetup(ledRedChannel, pwmFreq, pwmResolution);
        ledcSetup(ledGreenChannel, pwmFreq, pwmResolution);
        ledcSetup(ledBTChannel, pwmFreq, pwmResolution);

        ledcAttachPin(pin_batteryLevelLED_Red, ledRedChannel);
        ledcAttachPin(pin_batteryLevelLED_Green, ledGreenChannel);
        ledcAttachPin(pin_bluetoothStatusLED, ledBTChannel);

        ledcWrite(ledRedChannel, 0);
        ledcWrite(ledGreenChannel, 0);
        ledcWrite(ledBTChannel, 0);
    }
    else if (productVariant == REFERENCE_STATION)
    {
        pinMode(pin_baseStatusLED, OUTPUT);
        digitalWrite(pin_baseStatusLED, LOW);
    }
}

// Configure the on board MAX17048 fuel gauge
void beginFuelGauge()
{
    if (HAS_NO_BATTERY)
        return; // Reference station does not have a battery

    // Set up the MAX17048 LiPo fuel gauge
    if (lipo.begin() == false)
    {
        systemPrintln("Fuel gauge not detected.");
        return;
    }

    online.battery = true;

    // Always use hibernate mode
    if (lipo.getHIBRTActThr() < 0xFF)
        lipo.setHIBRTActThr((uint8_t)0xFF);
    if (lipo.getHIBRTHibThr() < 0xFF)
        lipo.setHIBRTHibThr((uint8_t)0xFF);

    systemPrintln("Fuel gauge configuration complete");

    checkBatteryLevels(); // Force check so you see battery level immediately at power on

    // Check to see if we are dangerously low
    if (battLevel < 5 && battChangeRate < 0.5) // 5% and not charging
    {
        systemPrintln("Battery too low. Please charge. Shutting down...");

        if (online.display == true)
            displayMessage("Charge Battery", 0);

        delay(2000);

        powerDown(false); // Don't display 'Shutting Down'
    }
}

// Begin accelerometer if available
void beginAccelerometer()
{
    if (accel.begin() == false)
    {
        online.accelerometer = false;

        return;
    }

    // The larger the avgAmount the faster we should read the sensor
    // accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
    accel.setDataRate(LIS2DH12_ODR_400Hz); // 25 measurements a second

    systemPrintln("Accelerometer configuration complete");

    online.accelerometer = true;
}

// Depending on platform and previous power down state, set system state
void beginSystemState()
{
    if (systemState > STATE_NOT_SET)
    {
        systemPrintln("Unknown state - factory reset");
        factoryReset(false); // We do not have the SD semaphore
    }

    if (productVariant == RTK_SURVEYOR)
    {
        if (settings.lastState == STATE_NOT_SET) // Default
        {
            systemState = STATE_ROVER_NOT_STARTED;
            settings.lastState = systemState;
        }

        // If the rocker switch was moved while off, force module settings
        // When switch is set to '1' = BASE, pin will be shorted to ground
        if (settings.lastState == STATE_ROVER_NOT_STARTED && digitalRead(pin_setupButton) == LOW)
            settings.updateZEDSettings = true;
        else if (settings.lastState == STATE_BASE_NOT_STARTED && digitalRead(pin_setupButton) == HIGH)
            settings.updateZEDSettings = true;

        systemState = STATE_ROVER_NOT_STARTED; // Assume Rover. ButtonCheckTask() will correct as needed.

        setupBtn = new Button(pin_setupButton); // Create the button in memory
        // Allocation failure handled in ButtonCheckTask
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
    {
        if (settings.lastState == STATE_NOT_SET) // Default
        {
            systemState = STATE_ROVER_NOT_STARTED;
            settings.lastState = systemState;
        }

        if (online.lband == false)
            systemState =
                settings
                    .lastState; // Return to either Rover or Base Not Started. The last state previous to power down.
        else
            systemState = STATE_KEYS_STARTED; // Begin process for getting new keys

        setupBtn = new Button(pin_setupButton);          // Create the button in memory
        powerBtn = new Button(pin_powerSenseAndControl); // Create the button in memory
        // Allocation failures handled in ButtonCheckTask
    }
    else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND ||
             productVariant == RTK_FACET_LBAND_DIRECT)
    {
        if (settings.lastState == STATE_NOT_SET) // Default
        {
            systemState = STATE_ROVER_NOT_STARTED;
            settings.lastState = systemState;
        }

        if (online.lband == false)
            systemState =
                settings
                    .lastState; // Return to either Rover or Base Not Started. The last state previous to power down.
        else
            systemState = STATE_KEYS_STARTED; // Begin process for getting new keys

        firstRoverStart = true; // Allow user to enter test screen during first rover start
        if (systemState == STATE_BASE_NOT_STARTED)
            firstRoverStart = false;

        powerBtn = new Button(pin_powerSenseAndControl); // Create the button in memory
        // Allocation failure handled in ButtonCheckTask
    }
    else if (productVariant == REFERENCE_STATION)
    {
        if (settings.lastState == STATE_NOT_SET) // Default
        {
            systemState = STATE_BASE_NOT_STARTED;
            settings.lastState = systemState;
        }

        systemState =
            settings
                .lastState; // Return to either NTP, Base or Rover Not Started. The last state previous to power down.

        setupBtn = new Button(pin_setupButton); // Create the button in memory
        // Allocation failure handled in ButtonCheckTask
    }

    // Starts task for monitoring button presses
    if (ButtonCheckTaskHandle == nullptr)
        xTaskCreate(ButtonCheckTask,
                    "BtnCheck",          // Just for humans
                    buttonTaskStackSize, // Stack Size
                    nullptr,             // Task input parameter
                    ButtonCheckTaskPriority,
                    &ButtonCheckTaskHandle); // Task handle
}

// Setup the timepulse output on the PPS pin for external triggering
// Setup TM2 time stamp input as need
bool beginExternalTriggers()
{
    // Skip if going into configure-via-ethernet mode
    if (configureViaEthernet)
    {
        log_d("configureViaEthernet: skipping beginExternalTriggers");
        return (false);
    }

    if (online.gnss == false)
        return (false);

    // If our settings haven't changed, trust ZED's settings
    if (settings.updateZEDSettings == false)
    {
        log_d("Skipping ZED Trigger configuration");
        return (true);
    }

    if (settings.dataPortChannel != MUX_PPS_EVENTTRIGGER)
        return (true); // No need to configure PPS if port is not selected

    bool response = true;

    response &= theGNSS.newCfgValset();
    response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_DEF, 0);        // Time pulse definition is a period (in us)
    response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_LENGTH_DEF, 1); // Define timepulse by length (not ratio)
    response &=
        theGNSS.addCfgValset(UBLOX_CFG_TP_USE_LOCKED_TP1,
                             1); // Use CFG-TP-PERIOD_LOCK_TP1 and CFG-TP-LEN_LOCK_TP1 as soon as GNSS time is valid
    response &= theGNSS.addCfgValset(UBLOX_CFG_TP_TP1_ENA, settings.enableExternalPulse); // Enable/disable timepulse
    response &=
        theGNSS.addCfgValset(UBLOX_CFG_TP_POL_TP1, settings.externalPulsePolarity); // 0 = falling, 1 = rising edge

    // While the module is _locking_ to GNSS time, turn off pulse
    response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PERIOD_TP1, 1000000); // Set the period between pulses in us
    response &= theGNSS.addCfgValset(UBLOX_CFG_TP_LEN_TP1, 0);          // Set the pulse length in us

    // When the module is _locked_ to GNSS time, make it generate 1Hz (Default is 100ms high, 900ms low)
    response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PERIOD_LOCK_TP1,
                                     settings.externalPulseTimeBetweenPulse_us); // Set the period between pulses is us
    response &=
        theGNSS.addCfgValset(UBLOX_CFG_TP_LEN_LOCK_TP1, settings.externalPulseLength_us); // Set the pulse length in us
    response &= theGNSS.sendCfgValset();

    if (response == false)
        systemPrintln("beginExternalTriggers config failed");

    if (settings.enableExternalHardwareEventLogging == true)
    {
        theGNSS.setAutoTIMTM2callbackPtr(
            &eventTriggerReceived); // Enable automatic TIM TM2 messages with callback to eventTriggerReceived
    }
    else
        theGNSS.setAutoTIMTM2callbackPtr(nullptr);

    return (response);
}

void beginIdleTasks()
{
    if (settings.enablePrintIdleTime == true)
    {
        char taskName[32];

        for (int index = 0; index < MAX_CPU_CORES; index++)
        {
            snprintf(taskName, sizeof(taskName), "IdleTask%d", index);
            if (idleTaskHandle[index] == nullptr)
                xTaskCreatePinnedToCore(
                    idleTask,
                    taskName, // Just for humans
                    2000,     // Stack Size
                    nullptr,  // Task input parameter
                    0,        // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
                    &idleTaskHandle[index], // Task handle
                    index);                 // Core where task should run, 0=core, 1=Arduino
        }
    }
}

void beginI2C()
{
    if (pinI2CTaskHandle == nullptr)
        xTaskCreatePinnedToCore(
            pinI2CTask,
            "I2CStart",        // Just for humans
            2000,              // Stack Size
            nullptr,           // Task input parameter
            0,                 // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
            &pinI2CTaskHandle, // Task handle
            settings.i2cInterruptsCore); // Core where task should run, 0=core, 1=Arduino

    while (i2cPinned == false) // Wait for task to run once
        delay(1);
}

// Assign I2C interrupts to the core that started the task. See: https://github.com/espressif/arduino-esp32/issues/3386
void pinI2CTask(void *pvParameters)
{
    bool i2cBusAvailable;
    uint32_t timer;

    Wire.begin(); // Start I2C on core the core that was chosen when the task was started
    // Wire.setClock(400000);

    // Display the device addresses
    i2cBusAvailable = false;
    for (uint8_t addr = 0; addr < 127; addr++)
    {
        // begin/end wire transmission to see if the bus is responding correctly
        // All good: 0ms, response 2
        // SDA/SCL shorted: 1000ms timeout, response 5
        // SCL/VCC shorted: 14ms, response 5
        // SCL/GND shorted: 1000ms, response 5
        // SDA/VCC shorted: 1000ms, response 5
        // SDA/GND shorted: 14ms, response 5
        timer = millis();
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            i2cBusAvailable = true;
            switch (addr)
            {
            default: {
                systemPrintf("0x%02x\r\n", addr);
                break;
            }

            case 0x19: {
                systemPrintf("0x%02x - LIS2DH12 Accelerometer\r\n", addr);
                break;
            }

            case 0x36: {
                systemPrintf("0x%02x - MAX17048 Fuel Gauge\r\n", addr);
                break;
            }

            case 0x3d: {
                systemPrintf("0x%02x - SSD1306 (64x48) OLED Driver\r\n", addr);
                break;
            }

            case 0x42: {
                systemPrintf("0x%02x - u-blox ZED-F9P GNSS Receiver\r\n", addr);
                break;
            }

            case 0x43: {
                systemPrintf("0x%02x - u-blox NEO-D9S-00B Correction Data Receiver\r\n", addr);
                break;
            }

            case 0x60: {
                systemPrintf("0x%02x - Crypto Coprocessor\r\n", addr);
                break;
            }
            }
        }
        else if ((millis() - timer) > 3)
        {
            systemPrintln("Error: I2C Bus Not Responding");
            i2cBusAvailable = false;
            break;
        }
    }

    // Update the I2C status
    online.i2c = i2cBusAvailable;
    i2cPinned = true;
    vTaskDelete(nullptr); // Delete task once it has run once
}

// Depending on radio selection, begin hardware
void radioStart()
{
    if (settings.radioType == RADIO_EXTERNAL)
    {
        espnowStop();

        // Nothing to start. UART2 of ZED is connected to external Radio port and is configured at
        // configureUbloxModule()
    }
    else if (settings.radioType == RADIO_ESPNOW)
        espnowStart();
}

// Start task to determine SD card size
void beginSDSizeCheckTask()
{
    if (sdSizeCheckTaskHandle == nullptr)
    {
        xTaskCreate(sdSizeCheckTask,         // Function to call
                    "SDSizeCheck",           // Just for humans
                    sdSizeCheckStackSize,    // Stack Size
                    nullptr,                 // Task input parameter
                    sdSizeCheckTaskPriority, // Priority
                    &sdSizeCheckTaskHandle); // Task handle

        log_d("sdSizeCheck Task started");
    }
}

void deleteSDSizeCheckTask()
{
    // Delete task once it's complete
    if (sdSizeCheckTaskHandle != nullptr)
    {
        vTaskDelete(sdSizeCheckTaskHandle);
        sdSizeCheckTaskHandle = nullptr;
        sdSizeCheckTaskComplete = false;
        log_d("sdSizeCheck Task deleted");
    }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Time Pulse ISR
// Triggered by the rising edge of the time pulse signal, indicates the top-of-second.
// Set the ESP32 RTC to UTC

void tpISR()
{
    unsigned long millisNow = millis();
    if (!inMainMenu) // Skip this if the menu is open
    {
        if (online.rtc) // Only sync if the RTC has been set via PVT first
        {
            if (timTpUpdated) // Only sync if timTpUpdated is true
            {
                if (millisNow - lastRTCSync >
                    syncRTCInterval) // Only sync if it is more than syncRTCInterval since the last sync
                {
                    if (millisNow < (timTpArrivalMillis + 999)) // Only sync if the GNSS time is not stale
                    {
                        if (fullyResolved) // Only sync if GNSS time is fully resolved
                        {
                            if (tAcc < 5000) // Only sync if the tAcc is better than 5000ns
                            {
                                // To perform the time zone adjustment correctly, it's easiest if we convert the GNSS
                                // time and date into Unix epoch first and then apply the timeZone offset
                                uint32_t epochSecs = timTpEpoch;
                                uint32_t epochMicros = timTpMicros;
                                epochSecs += settings.timeZoneSeconds;
                                epochSecs += settings.timeZoneMinutes * 60;
                                epochSecs += settings.timeZoneHours * 60 * 60;

                                // Set the internal system time
                                rtc.setTime(epochSecs, epochMicros);

                                lastRTCSync = millis();
                                rtcSyncd = true;

                                gnssSyncTv.tv_sec = epochSecs; // Store the timeval of the sync
                                gnssSyncTv.tv_usec = epochMicros;

                                if (syncRTCInterval < 59000) // From now on, sync every minute
                                    syncRTCInterval = 59000;
                            }
                        }
                    }
                }
            }
        }
    }
}
