// Setup the u-blox module for any setup (base or rover)
// In general we check if the setting is incorrect before writing it. Otherwise, the set commands have, on rare
// occasion, become corrupt. The worst is when the I2C port gets turned off or the I2C address gets borked.
bool configureUbloxModule()
{
    if (online.gnss == false)
        return (false);

    bool response = true;

    // Turn on/off debug messages
    if (settings.enableI2Cdebug)
    {
#if defined(REF_STN_GNSS_DEBUG)
        if (ENABLE_DEVELOPER && productVariant == REFERENCE_STATION)
            theGNSS.enableDebugging(serialGNSS); // Output all debug messages over serialGNSS
        else
#endif                                             // REF_STN_GNSS_DEBUG
            theGNSS.enableDebugging(Serial, true); // Enable only the critical debug messages over Serial
    }
    else
        theGNSS.disableDebugging();

    // Wait for initial report from module
    int maxWait = 2000;
    startTime = millis();
    while (pvtUpdated == false)
    {
        theGNSS.checkUblox();     // Regularly poll to get latest data and any RTCM
        theGNSS.checkCallbacks(); // Process any callbacks: ie, eventTriggerReceived
        delay(10);
        if ((millis() - startTime) > maxWait)
        {
            log_d("PVT Update failed");
            break;
        }
    }

    // The first thing we do is go to 1Hz to lighten any I2C traffic from a previous configuration
    response &= theGNSS.newCfgValset();
    response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, 1000);
    response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, 1);

    if (commandSupported(UBLOX_CFG_TMODE_MODE) == true)
        response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0); // Disable survey-in mode

    // UART1 will primarily be used to pass NMEA and UBX from ZED to ESP32 (eventually to cell phone)
    // but the phone can also provide RTCM data and a user may want to configure the ZED over Bluetooth.
    // So let's be sure to enable UBX+NMEA+RTCM on the input
    if (USE_I2C_GNSS)
    {
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 1);
        if (commandSupported(UBLOX_CFG_UART1OUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 1);
        if (commandSupported(UBLOX_CFG_UART1INPROT_SPARTN) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);

        response &= theGNSS.addCfgValset(
            UBLOX_CFG_UART1_BAUDRATE, settings.dataPortBaud); // Defaults to 230400 to maximize message output support
        response &= theGNSS.addCfgValset(
            UBLOX_CFG_UART2_BAUDRATE,
            settings.radioPortBaud); // Defaults to 57600 to match SiK telemetry radio firmware default

        // Disable SPI port - This is just to remove some overhead by ZED
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_NMEA, 0);
        if (commandSupported(UBLOX_CFG_SPIOUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_RTCM3X, 0);

        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_NMEA, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_RTCM3X, 0);
        if (commandSupported(UBLOX_CFG_SPIINPROT_SPARTN) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_SPARTN, 0);
    }
    else // SPI GNSS
    {
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_NMEA, 1);
        if (commandSupported(UBLOX_CFG_SPIOUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_SPIOUTPROT_RTCM3X, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_NMEA, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_RTCM3X, 1);
        if (commandSupported(UBLOX_CFG_SPIINPROT_SPARTN) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_SPIINPROT_SPARTN, 0);

        // Disable I2C and UART1 ports - This is just to remove some overhead by ZED
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_NMEA, 0);
        if (commandSupported(UBLOX_CFG_I2COUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_RTCM3X, 0);

        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_NMEA, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_RTCM3X, 0);
        if (commandSupported(UBLOX_CFG_I2CINPROT_SPARTN) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_SPARTN, 0);

        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 0);
        if (commandSupported(UBLOX_CFG_UART1OUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 0);

        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 0);
        if (commandSupported(UBLOX_CFG_UART1INPROT_SPARTN) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);
    }

    // Set the UART2 to only do RTCM (in case this device goes into base mode)
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_NMEA, 0);
    if (commandSupported(UBLOX_CFG_UART2OUTPROT_RTCM3X) == true)
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_RTCM3X, 1);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_UBX, settings.enableUART2UBXIn);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_NMEA, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_RTCM3X, 1);
    if (commandSupported(UBLOX_CFG_UART2INPROT_SPARTN) == true)
        response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_SPARTN, 0);

    // We don't want NMEA over I2C, but we will want to deliver RTCM, and UBX+RTCM is not an option
    if (USE_I2C_GNSS)
    {
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_NMEA, 1);
        if (commandSupported(UBLOX_CFG_I2COUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_I2COUTPROT_RTCM3X, 1);

        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_NMEA, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_RTCM3X, 1);

        if (commandSupported(UBLOX_CFG_I2CINPROT_SPARTN) == true)
        {
            // We push NEO-D9S correction data over the I2C interface via the PMP message. This uses the UBX protocol.
            // SPARTN is not needed on I2C
            response &= theGNSS.addCfgValset(UBLOX_CFG_I2CINPROT_SPARTN, 0);
        }
    }

    if (settings.enableZedUsb == true)
    {
        // The USB port on the ZED may be used for RTCM to/from the computer (as an NTRIP caster or client)
        // So let's be sure all protocols are on for the USB port
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_NMEA, 1);
        if (commandSupported(UBLOX_CFG_USBOUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_RTCM3X, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_UBX, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_NMEA, 1);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_RTCM3X, 1);
        if (commandSupported(UBLOX_CFG_USBINPROT_SPARTN) == true)
        {
            // See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/713
            response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_SPARTN, 1);
        }
    }
    else
    {
        //Disable all protocols over USB
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_NMEA, 0);
        if (commandSupported(UBLOX_CFG_USBOUTPROT_RTCM3X) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_USBOUTPROT_RTCM3X, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_UBX, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_NMEA, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_RTCM3X, 0);
        if (commandSupported(UBLOX_CFG_USBINPROT_SPARTN) == true)
        {
            // See issue: https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/713
            response &= theGNSS.addCfgValset(UBLOX_CFG_USBINPROT_SPARTN, 0);
        }
    }

    if (commandSupported(UBLOX_CFG_NAVSPG_INFIL_MINCNO) == true)
    {
        if (zedModuleType == PLATFORM_F9R)
            response &= theGNSS.addCfgValset(
                UBLOX_CFG_NAVSPG_INFIL_MINCNO,
                settings.minCNO_F9R); // Set minimum satellite signal level for navigation - default 20
        else
            response &= theGNSS.addCfgValset(
                UBLOX_CFG_NAVSPG_INFIL_MINCNO,
                settings.minCNO_F9P); // Set minimum satellite signal level for navigation - default 6
    }

    if (commandSupported(UBLOX_CFG_NAV2_OUT_ENABLED) == true)
    {
        // Count NAV2 messages and enable NAV2 as needed.
        if (getNAV2MessageCount() > 0)
        {
            response &= theGNSS.addCfgValset(
                UBLOX_CFG_NAV2_OUT_ENABLED,
                1); // Enable NAV2 messages. This has the side effect of causing RTCM to generate twice as fast.
        }
        else
            response &= theGNSS.addCfgValset(UBLOX_CFG_NAV2_OUT_ENABLED, 0); // Disable NAV2 messages
    }

    response &= theGNSS.sendCfgValset();

    if (response == false)
        systemPrintln("Module failed config block 0");
    response = true; // Reset

    // Enable the constellations the user has set
    response &= setConstellations(true); // 19 messages. Send newCfg or sendCfg with value set
    if (response == false)
        systemPrintln("Module failed config block 1");
    response = true; // Reset

    // Make sure the appropriate messages are enabled
    response &= setMessages(MAX_SET_MESSAGES_RETRIES); // Does a complete open/closed val set
    if (response == false)
        systemPrintln("Module failed config block 2");
    response = true; // Reset

    // Disable NMEA messages on all but UART1
    response &= theGNSS.newCfgValset();

    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_I2C, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_I2C, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_I2C, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_I2C, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_I2C, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_I2C, 0);

    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART2, 0);
    response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART2, 0);

    if (USE_I2C_GNSS) // Don't disable NMEA on SPI if the GNSS is SPI!
    {
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_SPI, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_SPI, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_SPI, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_SPI, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_SPI, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI, 0);
    }

    if (USE_SPI_GNSS) // If the GNSS is SPI, _do_ disable NMEA on UART1
    {
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART1, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1, 0);
        response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1, 0);
    }

    response &= theGNSS.sendCfgValset();

    if (response == false)
        systemPrintln("Module failed config block 3");

    if (zedModuleType == PLATFORM_F9R)
    {
        response &= theGNSS.setAutoESFSTATUS(
            true, false); // Tell the GPS to "send" each ESF Status, but do not update stale data when accessed
    }

    return (response);
}

// Turn on indicator LEDs to verify LED function and indicate setup sucess
void danceLEDs()
{
    if (productVariant == RTK_SURVEYOR)
    {
        for (int x = 0; x < 2; x++)
        {
            digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
            digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
            digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
            digitalWrite(pin_baseStatusLED, HIGH);
            digitalWrite(pin_bluetoothStatusLED, HIGH);
            delay(100);
            digitalWrite(pin_positionAccuracyLED_1cm, LOW);
            digitalWrite(pin_positionAccuracyLED_10cm, LOW);
            digitalWrite(pin_positionAccuracyLED_100cm, LOW);
            digitalWrite(pin_baseStatusLED, LOW);
            digitalWrite(pin_bluetoothStatusLED, LOW);
            delay(100);
        }

        digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
        digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
        digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
        digitalWrite(pin_baseStatusLED, HIGH);
        digitalWrite(pin_bluetoothStatusLED, HIGH);

        delay(250);
        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
        delay(250);
        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
        delay(250);
        digitalWrite(pin_positionAccuracyLED_100cm, LOW);

        delay(250);
        digitalWrite(pin_baseStatusLED, LOW);
        delay(250);
        digitalWrite(pin_bluetoothStatusLED, LOW);
    }
    else
    {
        // Units can boot under 1s. Keep splash screen up for at least 2s.
        while ((millis() - splashStart) < 2000)
            delay(1);
    }
}

// Update Battery level LEDs every 5s
void updateBattery()
{
    if (millis() - lastBattUpdate > 5000)
    {
        lastBattUpdate = millis();

        checkBatteryLevels();
    }
}

// When called, checks level of battery and updates the LED brightnesses
// And outputs a serial message to USB
void checkBatteryLevels()
{
    if (online.battery == true)
    {
        battLevel = lipo.getSOC();
        battVoltage = lipo.getVoltage();
        battChangeRate = lipo.getChangeRate();
    }
    else
    {
        // False numbers but above system cut-off level
        battLevel = 10;
        battVoltage = 3.7;
        battChangeRate = 0;
    }

    if (battChangeRate >= -0.01)
        externalPowerConnected = true;
    else
        externalPowerConnected = false;

    if (settings.enablePrintBatteryMessages)
    {
        char tempStr[25];
        if (externalPowerConnected)
            snprintf(tempStr, sizeof(tempStr), "C");
        else
            snprintf(tempStr, sizeof(tempStr), "Disc");

        systemPrintf("Batt (%d%%): Voltage: %0.02fV", battLevel, battVoltage);

        systemPrintf(" %sharging: %0.02f%%/hr ", tempStr, battChangeRate);

        if (battLevel < 10)
            snprintf(tempStr, sizeof(tempStr), "Red");
        else if (battLevel < 50)
            snprintf(tempStr, sizeof(tempStr), "Yellow");
        else if (battLevel <= 110)
            snprintf(tempStr, sizeof(tempStr), "Green");
        else
            snprintf(tempStr, sizeof(tempStr), "No batt");

        systemPrintf("%s\r\n", tempStr);
    }

    // Check if we need to shutdown due to no charging
    if (settings.shutdownNoChargeTimeout_s > 0)
    {
        if (externalPowerConnected == false)
        {
            int secondsSinceLastCharger = (millis() - shutdownNoChargeTimer) / 1000;
            if (secondsSinceLastCharger > settings.shutdownNoChargeTimeout_s)
                powerDown(true);
        }
        else
        {
            shutdownNoChargeTimer = millis(); // Reset timer because power is attached
        }
    }

    if (productVariant == RTK_SURVEYOR)
    {
        if (battLevel < 10)
        {
            ledcWrite(ledRedChannel, 255);
            ledcWrite(ledGreenChannel, 0);
        }
        else if (battLevel < 50)
        {
            ledcWrite(ledRedChannel, 128);
            ledcWrite(ledGreenChannel, 128);
        }
        else if (battLevel <= 110)
        {
            ledcWrite(ledRedChannel, 0);
            ledcWrite(ledGreenChannel, 255);
        }
        else
        {
            ledcWrite(ledRedChannel, 10);
            ledcWrite(ledGreenChannel, 0);
        }
    }
}

// Ping an I2C device and see if it responds
bool isConnected(uint8_t deviceAddress)
{
    Wire.beginTransmission(deviceAddress);
    if (Wire.endTransmission() == 0)
        return true;
    return false;
}

// Create a test file in file structure to make sure we can
bool createTestFile()
{
    FileSdFatMMC testFile;

    // TODO: double-check that SdFat tollerates preceding slashes
    char testFileName[40] = "/testfile.txt";

    // Attempt to write to the file system
    if ((!testFile) || (testFile.open(testFileName, O_CREAT | O_APPEND | O_WRITE) != true))
    {
        systemPrintln("createTestFile: failed to create (open) test file");
        return (false);
    }

    testFile.println("Testing...");

    // File successfully created
    testFile.close();

    if (USE_SPI_MICROSD)
    {
        if (sd->exists(testFileName))
            sd->remove(testFileName);
        return (!sd->exists(testFileName));
    }
#ifdef COMPILE_SD_MMC
    else
    {
        if (SD_MMC.exists(testFileName))
            SD_MMC.remove(testFileName);
        return (!SD_MMC.exists(testFileName));
    }
#endif // COMPILE_SD_MMC

    return (false);
}

// If debug option is on, print available heap
void reportHeapNow(bool alwaysPrint)
{
    if (alwaysPrint || (settings.enableHeapReport == true))
    {
        lastHeapReport = millis();
        systemPrintf("FreeHeap: %d / HeapLowestPoint: %d / LargestBlock: %d\r\n", ESP.getFreeHeap(),
                     xPortGetMinimumEverFreeHeapSize(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    }
}

// If debug option is on, print available heap
void reportHeap()
{
    if (settings.enableHeapReport == true)
    {
        if (millis() - lastHeapReport > 1000)
        {
            reportHeapNow(false);
        }
    }
}

// Based on current LED state, blink upwards fashion
// Used to indicate casting
void cyclePositionLEDs()
{
    if (productVariant == RTK_SURVEYOR)
    {
        // Cycle position LEDs to indicate casting
        if (millis() - lastCasterLEDupdate > 500)
        {
            lastCasterLEDupdate = millis();
            if (digitalRead(pin_positionAccuracyLED_100cm) == HIGH)
            {
                digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
                digitalWrite(pin_positionAccuracyLED_100cm, LOW);
            }
            else if (digitalRead(pin_positionAccuracyLED_10cm) == HIGH)
            {
                digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
                digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                digitalWrite(pin_positionAccuracyLED_100cm, LOW);
            }
            else // Catch all
            {
                digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
            }
        }
    }
}

// Determine MUX pins for this platform and set MUX to ADC/DAC to avoid I2C bus failure
void beginMux()
{
    if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
    {
        pin_muxA = 2;
        pin_muxB = 4;
    }
    else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
    {
        pin_muxA = 2;
        pin_muxB = 0;
    }

    setMuxport(MUX_ADC_DAC); // Set mux to user's choice: NMEA, I2C, PPS, or DAC
}

// Set the port of the 1:4 dual channel analog mux
// This allows NMEA, I2C, PPS/Event, and ADC/DAC to be routed through data port via software select
void setMuxport(int channelNumber)
{
    if (pin_muxA >= 0 && pin_muxB >= 0)
    {
        pinMode(pin_muxA, OUTPUT);
        pinMode(pin_muxB, OUTPUT);

        if (channelNumber > 3)
            return; // Error check

        switch (channelNumber)
        {
        case 0:
            digitalWrite(pin_muxA, LOW);
            digitalWrite(pin_muxB, LOW);
            break;
        case 1:
            digitalWrite(pin_muxA, HIGH);
            digitalWrite(pin_muxB, LOW);
            break;
        case 2:
            digitalWrite(pin_muxA, LOW);
            digitalWrite(pin_muxB, HIGH);
            break;
        case 3:
            digitalWrite(pin_muxA, HIGH);
            digitalWrite(pin_muxB, HIGH);
            break;
        }
    }
}

// Create $GNTXT, type message complete with CRC
// https://www.nmea.org/Assets/20160520%20txt%20amendment.pdf
// Used for recording system events (boot reason, event triggers, etc) inside the log
void createNMEASentence(customNmeaType_e textID, char *nmeaMessage, size_t sizeOfNmeaMessage, char *textMessage)
{
    // Currently we don't have messages longer than 82 char max so we hardcode the sentence numbers
    const uint8_t totalNumberOfSentences = 1;
    const uint8_t sentenceNumber = 1;

    char nmeaTxt[200]; // Max NMEA sentence length is 82
    snprintf(nmeaTxt, sizeof(nmeaTxt), "$GNTXT,%02d,%02d,%02d,%s*", totalNumberOfSentences, sentenceNumber, textID,
             textMessage);

    // From: http://engineeringnotes.blogspot.com/2015/02/generate-crc-for-nmea-strings-arduino.html
    byte CRC = 0; // XOR chars between '$' and '*'
    for (byte x = 1; x < strlen(nmeaTxt) - 1; x++)
        CRC = CRC ^ nmeaTxt[x];

    snprintf(nmeaMessage, sizeOfNmeaMessage, "%s%02X", nmeaTxt, CRC);
}

// Reset settings struct to default initializers
void settingsToDefaults()
{
    static const Settings defaultSettings;
    memcpy(&settings, &defaultSettings, sizeof(defaultSettings));
}

// Given a spot in the ubxMsg array, return true if this message is supported on this platform and firmware version
bool messageSupported(int messageNumber)
{
    bool messageSupported = false;

    if ((zedModuleType == PLATFORM_F9P) &&
        (zedFirmwareVersionInt >= ubxMessages[messageNumber].f9pFirmwareVersionSupported))
        messageSupported = true;
    else if ((zedModuleType == PLATFORM_F9R) &&
             (zedFirmwareVersionInt >= ubxMessages[messageNumber].f9rFirmwareVersionSupported))
        messageSupported = true;

    return (messageSupported);
}
// Given a command key, return true if that key is supported on this platform and firmware version
bool commandSupported(const uint32_t key)
{
    bool commandSupported = false;

    // Locate this key in the known key array
    int commandNumber = 0;
    for (; commandNumber < MAX_UBX_CMD; commandNumber++)
    {
        if (ubxCommands[commandNumber].cmdKey == key)
            break;
    }
    if (commandNumber == MAX_UBX_CMD)
    {
        systemPrintf("commandSupported: Unknown command key 0x%02X\r\n", key);
        commandSupported = false;
    }
    else
    {
        if ((zedModuleType == PLATFORM_F9P) &&
            (zedFirmwareVersionInt >= ubxCommands[commandNumber].f9pFirmwareVersionSupported))
            commandSupported = true;
        else if ((zedModuleType == PLATFORM_F9R) &&
                 (zedFirmwareVersionInt >= ubxCommands[commandNumber].f9rFirmwareVersionSupported))
            commandSupported = true;
    }
    return (commandSupported);
}

// Enable all the valid messages for this platform
// There are many messages so split into batches. VALSET is limited to 64 max per batch
// Uses dummy newCfg and sendCfg values to be sure we open/close a complete set
bool setMessages(int maxRetries)
{
    uint32_t spiOffset =
        0; // Set to 3 if using SPI to convert UART1 keys to SPI. This is brittle and non-perfect, but works.
    if (USE_SPI_GNSS)
        spiOffset = 3;

    bool success = false;
    int tryNo = -1;

    // Try up to maxRetries times to configure the messages
    // This corrects occasional failures seen on the Reference Station where the GNSS is connected via SPI
    // instead of I2C and UART1. I believe the SETVAL ACK is occasionally missed due to the level of messages being
    // processed.
    while ((++tryNo < maxRetries) && !success)
    {
        bool response = true;
        int messageNumber = 0;

        while (messageNumber < MAX_UBX_MSG)
        {
            response &= theGNSS.newCfgValset();

            do
            {
                if (messageSupported(messageNumber) == true)
                {
                    uint8_t rate = settings.ubxMessageRates[messageNumber];

                    // If the GNSS is SPI, we need to make sure that NAV_PVT, NAV_HPPOSLLH and ESF_STATUS remained
                    // enabled (but not enabled for logging)
                    if (USE_SPI_GNSS)
                    {
                        if (ubxMessages[messageNumber].msgClass == UBX_CLASS_NAV)
                            if ((ubxMessages[messageNumber].msgID == UBX_NAV_PVT) ||
                                (ubxMessages[messageNumber].msgID == UBX_NAV_HPPOSLLH))
                                if (rate == 0)
                                    rate = 1;
                        if (ubxMessages[messageNumber].msgClass == UBX_CLASS_ESF)
                            if (ubxMessages[messageNumber].msgID == UBX_ESF_STATUS)
                                if (zedModuleType == PLATFORM_F9R)
                                    if (rate == 0)
                                        rate = 1;
                        if (ubxMessages[messageNumber].msgClass == UBX_CLASS_TIM)
                        {
                            if (ubxMessages[messageNumber].msgID == UBX_TIM_TM2)
                                if (rate == 0)
                                    rate = 1;
                            if (ubxMessages[messageNumber].msgID == UBX_TIM_TP)
                                if (HAS_GNSS_TP_INT)
                                    if (rate == 0)
                                        rate = 1;
                        }
                        if (ubxMessages[messageNumber].msgClass == UBX_CLASS_RXM)
                            if (ubxMessages[messageNumber].msgID == UBX_RXM_COR)
                                if (rate == 0)
                                    rate = 1;
                        if (ubxMessages[messageNumber].msgClass == UBX_CLASS_NMEA)
                            if (ubxMessages[messageNumber].msgID == UBX_NMEA_GGA)
                                if (rate == 0)
                                    rate = 1;
                        if (ubxMessages[messageNumber].msgClass == UBX_CLASS_MON)
                            if (ubxMessages[messageNumber].msgID == UBX_MON_HW)
                                if (rate == 0)
                                    rate = 1;
                    }

                    response &= theGNSS.addCfgValset(ubxMessages[messageNumber].msgConfigKey + spiOffset, rate);
                }
                messageNumber++;
            } while (((messageNumber % 43) < 42) &&
                     (messageNumber < MAX_UBX_MSG)); // Limit 1st batch to 42. Batches after that will be (up to) 43 in
                                                     // size. It's a HHGTTG thing.

            if (theGNSS.sendCfgValset() == false)
            {
                log_d("sendCfg failed at messageNumber %d %s. Try %d of %d.", messageNumber - 1,
                      (messageNumber - 1) < MAX_UBX_MSG ? ubxMessages[messageNumber - 1].msgTextName : "", tryNo + 1,
                      maxRetries);
                response &= false; // If any one of the Valset fails, report failure overall
            }
        }

        // For SPI GNSS products, we need to add each message to the GNSS Library logging buffer
        // to mimic UART1
        if (USE_SPI_GNSS)
        {
            uint32_t logRTCMMessages = 0;
            uint32_t logNMEAMessages = 0;

            for (messageNumber = 0; messageNumber < MAX_UBX_MSG; messageNumber++)
            {
                if (ubxMessages[messageNumber].msgClass == UBX_RTCM_MSB) // RTCM messages
                {
                    if (messageSupported(messageNumber) == true)
                        logRTCMMessages |= ubxMessages[messageNumber].filterMask;
                }
                else if (ubxMessages[messageNumber].msgClass == UBX_CLASS_NMEA) // NMEA messages
                {
                    if (messageSupported(messageNumber) == true)
                        logNMEAMessages |= ubxMessages[messageNumber].filterMask;
                }
                else // UBX messages
                {
                    if (messageSupported(messageNumber) == true)
                        theGNSS.enableUBXlogging(ubxMessages[messageNumber].msgClass, ubxMessages[messageNumber].msgID,
                                                 settings.ubxMessageRates[messageNumber] > 0);
                }
            }

            theGNSS.setRTCMLoggingMask(logRTCMMessages);
            theGNSS.setNMEALoggingMask(logNMEAMessages);
        }

        if (response)
            success = true;
    }

    return (success);
}

// Enable all the valid messages for this platform over the USB port
// Add 2 to every UART1 key. This is brittle and non-perfect, but works.
bool setMessagesUSB(int maxRetries)
{
    bool success = false;
    int tryNo = -1;

    // Try up to maxRetries times to configure the messages
    // This corrects occasional failures seen on the Reference Station where the GNSS is connected via SPI
    // instead of I2C and UART1. I believe the SETVAL ACK is occasionally missed due to the level of messages being
    // processed.
    while ((++tryNo < maxRetries) && !success)
    {
        bool response = true;
        int messageNumber = 0;

        while (messageNumber < MAX_UBX_MSG)
        {
            response &= theGNSS.newCfgValset();

            do
            {
                if (messageSupported(messageNumber) == true)
                    response &= theGNSS.addCfgValset(ubxMessages[messageNumber].msgConfigKey + 2,
                                                     settings.ubxMessageRates[messageNumber]);
                messageNumber++;
            } while (((messageNumber % 43) < 42) &&
                     (messageNumber < MAX_UBX_MSG)); // Limit 1st batch to 42. Batches after that will be (up to) 43 in
                                                     // size. It's a HHGTTG thing.

            response &= theGNSS.sendCfgValset();
        }

        if (response)
            success = true;
    }

    return (success);
}

// Enable all the valid constellations and bands for this platform
// Band support varies between platforms and firmware versions
// We open/close a complete set if sendCompleteBatch = true
// 19 messages
bool setConstellations(bool sendCompleteBatch)
{
    bool response = true;

    if (sendCompleteBatch)
        response &= theGNSS.newCfgValset();

    bool enableMe = settings.ubxConstellations[0].enabled;
    response &= theGNSS.addCfgValset(settings.ubxConstellations[0].configKey, enableMe); // GPS

    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GPS_L1CA_ENA, settings.ubxConstellations[0].enabled);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GPS_L2C_ENA, settings.ubxConstellations[0].enabled);

    // v1.12 ZED-F9P firmware does not allow for SBAS control
    // Also, if we can't identify the version (99), skip SBAS enable
    if ((zedModuleType == PLATFORM_F9P) && ((zedFirmwareVersionInt == 112) || (zedFirmwareVersionInt == 99)))
    {
        // Skip
    }
    else
    {
        response &= theGNSS.addCfgValset(settings.ubxConstellations[1].configKey,
                                         settings.ubxConstellations[1].enabled); // SBAS
        response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_SBAS_L1CA_ENA, settings.ubxConstellations[1].enabled);
    }

    response &=
        theGNSS.addCfgValset(settings.ubxConstellations[2].configKey, settings.ubxConstellations[2].enabled); // GAL
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GAL_E1_ENA, settings.ubxConstellations[2].enabled);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GAL_E5B_ENA, settings.ubxConstellations[2].enabled);

    response &=
        theGNSS.addCfgValset(settings.ubxConstellations[3].configKey, settings.ubxConstellations[3].enabled); // BDS
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_BDS_B1_ENA, settings.ubxConstellations[3].enabled);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_BDS_B2_ENA, settings.ubxConstellations[3].enabled);

    response &=
        theGNSS.addCfgValset(settings.ubxConstellations[4].configKey, settings.ubxConstellations[4].enabled); // QZSS
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L1CA_ENA, settings.ubxConstellations[4].enabled);

    // UBLOX_CFG_SIGNAL_QZSS_L1S_ENA not supported on F9R in v1.21 and below
    if (zedModuleType == PLATFORM_F9P)
        response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L1S_ENA, settings.ubxConstellations[4].enabled);
    else if ((zedModuleType == PLATFORM_F9R) && (zedFirmwareVersionInt > 121))
        response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L1S_ENA, settings.ubxConstellations[4].enabled);

    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_QZSS_L2C_ENA, settings.ubxConstellations[4].enabled);

    response &=
        theGNSS.addCfgValset(settings.ubxConstellations[5].configKey, settings.ubxConstellations[5].enabled); // GLO
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GLO_L1_ENA, settings.ubxConstellations[5].enabled);
    response &= theGNSS.addCfgValset(UBLOX_CFG_SIGNAL_GLO_L2_ENA, settings.ubxConstellations[5].enabled);

    if (sendCompleteBatch)
        response &= theGNSS.sendCfgValset();

    return (response);
}

// Periodically print position if enabled
void printPosition()
{
    // Periodically print the position
    if (settings.enablePrintPosition && ((millis() - lastPrintPosition) > 15000))
    {
        printCurrentConditions();
        lastPrintPosition = millis();
    }
}

// Periodically print RTK state if enabled
void printRTKState()
{
    // Periodically print the RTK state
    if (settings.enablePrintState && ((millis() - lastPrintState) > 15000))
    {
        printCurrentRTKState();
        lastPrintState = millis();
    }
}

// Given a user's string, try to identify the type and return the coordinate in DD.ddddddddd format
CoordinateInputType coordinateIdentifyInputType(const char *userEntryOriginal, double *coordinate)
{
    char userEntry[50];
    strncpy(userEntry, userEntryOriginal,
            sizeof(userEntry) - 1); // strtok modifies the message so make copy into userEntry

    *coordinate = 0.0; // Clear what is given to us

    CoordinateInputType coordinateInputType = COORDINATE_INPUT_TYPE_INVALID_UNKNOWN;

    int dashCount = 0;
    int spaceCount = 0;
    int decimalCount = 0;
    int lengthOfLeadingNumber = 0;

    // Scan entry for invalid chars
    // A valid entry has only numbers, -, ' ', and .
    for (int x = 0; x < strlen(userEntry); x++)
    {
        if (isdigit(userEntry[x])) // All good
        {
            if (decimalCount == 0)
                lengthOfLeadingNumber++;
        }
        else if (userEntry[x] == '-')
            dashCount++; // All good
        else if (userEntry[x] == ' ')
            spaceCount++; // All good
        else if (userEntry[x] == '.')
            decimalCount++; // All good
        else
            return (COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); // String contains invalid character
    }

    // Seven possible entry types
    // DD.ddddddddd
    // DDMM.mmmmmmm
    // DD MM.mmmmmmm
    // DD-MM.mmmmmmm
    // DDMMSS.ssssss
    // DD MM SS.ssssss
    // DD-MM-SS.ssssss
    // DDMMSS
    // DD MM SS
    // DD-MM-SS

    if (decimalCount > 1)
        return (COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); // 40.09.033 is not valid.
    if (spaceCount > 2)
        return (COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); // Only 0, 1, or 2 allowed. 40 05 25.2049 is valid.
    if (dashCount > 3)
        return (COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); // Only 0, 1, 2, or 3 allowed. -105-11-05.1629 is valid.
    if (lengthOfLeadingNumber > 7)
        return (COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); // Only 7 or fewer. -1051105.188992 (DDDMMSS or DDMMSS) is valid

    bool negativeSign = false;
    if (userEntry[0] == '-')
    {
        userEntry[0] = ' ';
        negativeSign = true;
        dashCount--; // Use dashCount as the internal dashes only, not the leading negative sign
    }

    if (spaceCount == 0 && dashCount == 0 &&
        (lengthOfLeadingNumber == 7 || lengthOfLeadingNumber == 6)) // DDMMSS.ssssss or DDMMSS
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DDMMSS;

        long intPortion = atoi(userEntry);  // Get DDDMMSS
        long decimal = intPortion / 10000L; // Get DDD
        intPortion -= (decimal * 10000L);
        long minutes = intPortion / 100L; // Get MM

        // Find '.'
        char *decimalPtr = strchr(userEntry, '.');
        if (decimalPtr == nullptr)
            coordinateInputType = COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL;

        double seconds = atof(userEntry); // Get DDDMMSS.ssssss
        seconds -= (decimal * 10000);     // Remove DDD
        seconds -= (minutes * 100);       // Remove MM
        *coordinate = decimal + (minutes / (double)60) + (seconds / (double)3600);

        if (negativeSign)
            *coordinate *= -1;
    }
    else if (spaceCount == 0 && dashCount == 0 &&
             (lengthOfLeadingNumber == 5 || lengthOfLeadingNumber == 4)) // DDMM.mmmmmmm
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DDMM;

        long intPortion = atoi(userEntry); // Get DDDMM
        long decimal = intPortion / 100L;  // Get DDD
        intPortion -= (decimal * 100L);
        double minutes = atof(userEntry); // Get DDDMM.mmmmmmm
        minutes -= (decimal * 100L);      // Remove DDD
        *coordinate = decimal + (minutes / (double)60);
        if (negativeSign)
            *coordinate *= -1;
    }
    else if (dashCount == 1) // DD-MM.mmmmmmm
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DD_MM_DASH;

        char *token = strtok(userEntry, "-"); // Modifies the given array
        // We trust that token points at something because the dashCount is > 0
        int decimal = atoi(token); // Get DD
        token = strtok(nullptr, "-");
        double minutes = atof(token); // Get MM.mmmmmmm
        *coordinate = decimal + (minutes / 60.0);
        if (negativeSign)
            *coordinate *= -1;
    }
    else if (dashCount == 2) // DD-MM-SS.ssss or DD-MM-SS
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DD_MM_SS_DASH;

        char *token = strtok(userEntry, "-"); // Modifies the given array
        // We trust that token points at something because the spaceCount is > 0
        int decimal = atoi(token); // Get DD
        token = strtok(nullptr, "-");
        int minutes = atoi(token); // Get MM
        token = strtok(nullptr, "-");

        // Find '.'
        char *decimalPtr = strchr(token, '.'); // Use token, not userEntry, as the dashes are now NULL
        if (decimalPtr == nullptr)
            coordinateInputType = COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL;

        double seconds = atof(token); // Get SS.ssssss
        *coordinate = decimal + (minutes / (double)60) + (seconds / (double)3600);
        if (negativeSign)
            *coordinate *= -1;
    }
    else if (spaceCount == 0) // DD.dddddd
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DD;
        sscanf(userEntry, "%lf", coordinate); // Load float from userEntry into coordinate
        if (negativeSign)
            *coordinate *= -1;
    }
    else if (spaceCount == 1) // DD MM.mmmmmmm
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DD_MM;

        char *token = strtok(userEntry, " "); // Modifies the given array
        // We trust that token points at something because the spaceCount is > 0
        int decimal = atoi(token); // Get DD
        token = strtok(nullptr, " ");
        double minutes = atof(token); // Get MM.mmmmmmm
        *coordinate = decimal + (minutes / 60.0);
        if (negativeSign)
            *coordinate *= -1;
    }
    else if (spaceCount == 2) // DD MM SS.ssssss or DD MM SS
    {
        coordinateInputType = COORDINATE_INPUT_TYPE_DD_MM_SS;

        char *token = strtok(userEntry, " "); // Modifies the given array
        // We trust that token points at something because the spaceCount is > 0
        int decimal = atoi(token); // Get DD
        token = strtok(nullptr, " ");
        int minutes = atoi(token); // Get MM
        token = strtok(nullptr, " ");

        // Find '.'
        char *decimalPtr = strchr(token, '.');
        if (decimalPtr == nullptr)
            coordinateInputType = COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL;

        double seconds = atof(token); // Get SS.ssssss

        *coordinate = decimal + (minutes / (double)60) + (seconds / (double)3600);
        if (negativeSign)
            *coordinate *= -1;
    }

    return (coordinateInputType);
}

// Given a coordinate and input type, output a string
// So DD.ddddddddd can become 'DD MM SS.ssssss', etc
void coordinateConvertInput(double coordinate, CoordinateInputType coordinateInputType, char *coordinateString,
                            int sizeOfCoordinateString)
{
    if (coordinateInputType == COORDINATE_INPUT_TYPE_DD)
    {
        snprintf(coordinateString, sizeOfCoordinateString, "%0.9f", coordinate);
    }
    else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM || coordinateInputType == COORDINATE_INPUT_TYPE_DDMM ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_DASH ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SYMBOL)
    {
        int longitudeDegrees = (int)coordinate;
        coordinate -= longitudeDegrees;
        coordinate *= 60;
        if (coordinate < 1)
            coordinate *= -1;

        if (coordinateInputType == COORDINATE_INPUT_TYPE_DDMM)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d%010.7f", longitudeDegrees, coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_DASH)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d-%010.7f", longitudeDegrees, coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SYMBOL)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d°%010.7f'", longitudeDegrees, coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d %010.7f", longitudeDegrees, coordinate);
    }
    else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DDMMSS ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_DASH ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL ||
             coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL)
    {
        int longitudeDegrees = (int)coordinate;
        coordinate -= longitudeDegrees;
        coordinate *= 60;
        if (coordinate < 1)
            coordinate *= -1;

        int longitudeMinutes = (int)coordinate;
        coordinate -= longitudeMinutes;
        coordinate *= 60;
        if (coordinateInputType == COORDINATE_INPUT_TYPE_DDMMSS)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d%02d%09.6f", longitudeDegrees, longitudeMinutes,
                     coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_DASH)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d-%02d-%09.6f", longitudeDegrees, longitudeMinutes,
                     coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d°%02d'%09.6f\"", longitudeDegrees, longitudeMinutes,
                     coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d %02d %09.6f", longitudeDegrees, longitudeMinutes,
                     coordinate);
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d%02d%02d", longitudeDegrees, longitudeMinutes,
                     (int)round(coordinate));
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d %02d %02d", longitudeDegrees, longitudeMinutes,
                     (int)round(coordinate));
        else if (coordinateInputType == COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL)
            snprintf(coordinateString, sizeOfCoordinateString, "%02d-%02d-%02d", longitudeDegrees, longitudeMinutes,
                     (int)round(coordinate));
    }
    else
    {
        log_d("Unknown coordinate input type");
    }
}
// Given an input type, return a printable string
const char *coordinatePrintableInputType(CoordinateInputType coordinateInputType)
{
    switch (coordinateInputType)
    {
    default:
        return ("Unknown");
        break;
    case (COORDINATE_INPUT_TYPE_DD):
        return ("DD.ddddddddd");
        break;
    case (COORDINATE_INPUT_TYPE_DDMM):
        return ("DDMM.mmmmmmm");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM):
        return ("DD MM.mmmmmmm");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_DASH):
        return ("DD-MM.mmmmmmm");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_SYMBOL):
        return ("DD°MM.mmmmmmm'");
        break;
    case (COORDINATE_INPUT_TYPE_DDMMSS):
        return ("DDMMSS.ssssss");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_SS):
        return ("DD MM SS.ssssss");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_SS_DASH):
        return ("DD-MM-SS.ssssss");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL):
        return ("DD°MM'SS.ssssss\"");
        break;
    case (COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL):
        return ("DDMMSS");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL):
        return ("DD MM SS");
        break;
    case (COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL):
        return ("DD-MM-SS");
        break;
    }
    return ("Unknown");
}

// Print the error message every 15 seconds
void reportFatalError(const char *errorMsg)
{
    while (1)
    {
        systemPrint("HALTED: ");
        systemPrint(errorMsg);
        systemPrintln();
        sleep(15);
    }
}
