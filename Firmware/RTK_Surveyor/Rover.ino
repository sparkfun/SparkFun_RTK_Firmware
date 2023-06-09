// Configure specific aspects of the receiver for rover mode
bool configureUbloxModuleRover()
{
    if (online.gnss == false)
    {
        log_d("GNSS not online");
        return (false);
    }

    // If our settings haven't changed, and this is first config since power on, trust ZED's settings
    if (settings.updateZEDSettings == false && firstPowerOn == true)
    {
        firstPowerOn = false; // Next time user switches modes, new settings will be applied
        log_d("Skipping ZED Rover configuration");
        return (true);
    }

    firstPowerOn = false; // If we switch between rover/base in the future, force config of module.

    theGNSS.checkUblox();     // Regularly poll to get latest data and any RTCM
    theGNSS.checkCallbacks(); // Process any callbacks: ie, storePVTdata

    bool success = false;
    int tryNo = -1;

    // Try up to MAX_SET_MESSAGES_RETRIES times to configure the GNSS
    // This corrects occasional failures seen on the Reference Station where the GNSS is connected via SPI
    // instead of I2C and UART1. I believe the SETVAL ACK is occasionally missed due to the level of messages being
    // processed.
    while ((++tryNo < MAX_SET_MESSAGES_RETRIES) && !success)
    {
        bool response = true;

        // Set output rate
        response &= theGNSS.newCfgValset();
        response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, settings.measurementRate);
        response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, settings.navigationRate);

        // Survey mode is only available on ZED-F9P modules
        if (commandSupported(UBLOX_CFG_TMODE_MODE) == true)
            response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0); // Disable survey-in mode

        response &=
            theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_DYNMODEL, (dynModel)settings.dynamicModel); // Set dynamic model

        // RTCM is only available on ZED-F9P modules
        //
        // For most RTK products, the GNSS is interfaced via both I2C and UART1. Configuration and PVT/HPPOS messages
        // are configured over I2C. Any messages that need to be logged are output on UART1, and received by this code
        // using serialGNSS. So in Rover mode, we want to disable any RTCM messages on I2C (and USB and UART2).
        //
        // But, on the Reference Station, the GNSS is interfaced via SPI. It has no access to I2C and UART1. So for that
        // product - in Rover mode - we want to leave any RTCM messages enabled on SPI so they can be logged if desired.

        // Find first RTCM record in ubxMessage array
        int firstRTCMRecord = getMessageNumberByName("UBX_RTCM_1005");

        if (zedModuleType == PLATFORM_F9P)
        {
            if (USE_I2C_GNSS)
            {
                // Set RTCM messages to user's settings
                for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
                    response &= theGNSS.addCfgValset(
                        ubxMessages[firstRTCMRecord + x].msgConfigKey - 1,
                        settings.ubxMessageRates[firstRTCMRecord + x]); // UBLOX_CFG UART1 - 1 = I2C
            }
            else
            {
                for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
                    response &= theGNSS.addCfgValset(
                        ubxMessages[firstRTCMRecord + x].msgConfigKey + 3,
                        settings.ubxMessageRates[firstRTCMRecord + x]); // UBLOX_CFG UART1 + 3 = SPI
            }

            // Set RTCM messages to user's settings
            for (int x = 0; x < MAX_UBX_MSG_RTCM; x++)
            {
                response &=
                    theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey + 1,
                                         settings.ubxMessageRates[firstRTCMRecord + x]); // UBLOX_CFG UART1 + 1 = UART2
                response &=
                    theGNSS.addCfgValset(ubxMessages[firstRTCMRecord + x].msgConfigKey + 2,
                                         settings.ubxMessageRates[firstRTCMRecord + x]); // UBLOX_CFG UART1 + 2 = USB
            }
        }

        response &= theGNSS.addCfgValset(UBLOX_CFG_NMEA_MAINTALKERID,
                                         3); // Return talker ID to GNGGA after NTRIP Client set to GPGGA

        response &= theGNSS.addCfgValset(UBLOX_CFG_NMEA_HIGHPREC, 1);    // Enable high precision NMEA
        response &= theGNSS.addCfgValset(UBLOX_CFG_NMEA_SVNUMBERING, 1); // Enable extended satellite numbering

        response &= theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_INFIL_MINELEV, settings.minElev); // Set minimum elevation

        response &= theGNSS.sendCfgValset(); // Closing

        if (response)
            success = true;
    }

    if (!success)
        log_d("Rover config failed 1");

    if (zedModuleType == PLATFORM_F9R)
    {
        bool response = true;

        response &= theGNSS.newCfgValset();

        response &=
            theGNSS.addCfgValset(UBLOX_CFG_SFCORE_USE_SF, settings.enableSensorFusion); // Enable/disable sensor fusion
        response &=
            theGNSS.addCfgValset(UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA,
                                 settings.autoIMUmountAlignment); // Enable/disable Automatic IMU-mount Alignment

        if (zedFirmwareVersionInt >= 121)
        {
            response &= theGNSS.addCfgValset(UBLOX_CFG_SFIMU_IMU_MNTALG_YAW, settings.imuYaw);
            response &= theGNSS.addCfgValset(UBLOX_CFG_SFIMU_IMU_MNTALG_PITCH, settings.imuPitch);
            response &= theGNSS.addCfgValset(UBLOX_CFG_SFIMU_IMU_MNTALG_ROLL, settings.imuRoll);
            response &= theGNSS.addCfgValset(UBLOX_CFG_SFODO_DIS_AUTODIRPINPOL, settings.sfDisableWheelDirection);
            response &= theGNSS.addCfgValset(UBLOX_CFG_SFODO_COMBINE_TICKS, settings.sfCombineWheelTicks);
            response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV_PRIO, settings.rateNavPrio);
            response &= theGNSS.addCfgValset(UBLOX_CFG_SFODO_USE_SPEED, settings.sfUseSpeed);
        }

        response &= theGNSS.sendCfgValset(); // Closing - 28 keys

        if (response == false)
        {
            log_d("Rover config failed 2");
            success = false;
        }
    }

    if (!success)
        systemPrintln("Rover config fail");

    return (success);
}

// Turn on the three accuracy LEDs depending on our current HPA (horizontal positional accuracy)
void updateAccuracyLEDs()
{
    // Update the horizontal accuracy LEDs only every second or so
    if (millis() - lastAccuracyLEDUpdate > 2000)
    {
        lastAccuracyLEDUpdate = millis();

        if (online.gnss == true)
        {
            if (horizontalAccuracy > 0)
            {
                if (settings.enablePrintRoverAccuracy)
                {
                    systemPrint("Rover Accuracy (m): ");
                    systemPrint(horizontalAccuracy, 4); // Print the accuracy with 4 decimal places
                    systemPrint(", SIV: ");
                    systemPrint(numSV);
                    systemPrintln();
                }

                if (productVariant == RTK_SURVEYOR)
                {
                    if (horizontalAccuracy <= 0.02)
                    {
                        digitalWrite(pin_positionAccuracyLED_1cm, HIGH);
                        digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
                        digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
                    }
                    else if (horizontalAccuracy <= 0.100)
                    {
                        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                        digitalWrite(pin_positionAccuracyLED_10cm, HIGH);
                        digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
                    }
                    else if (horizontalAccuracy <= 1.0000)
                    {
                        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                        digitalWrite(pin_positionAccuracyLED_100cm, HIGH);
                    }
                    else if (horizontalAccuracy > 1.0)
                    {
                        digitalWrite(pin_positionAccuracyLED_1cm, LOW);
                        digitalWrite(pin_positionAccuracyLED_10cm, LOW);
                        digitalWrite(pin_positionAccuracyLED_100cm, LOW);
                    }
                }
            }
            else if (settings.enablePrintRoverAccuracy)
            {
                systemPrint("Rover Accuracy: ");
                systemPrint(horizontalAccuracy);
                systemPrint(" ");
                systemPrint("No lock. SIV: ");
                systemPrint(numSV);
                systemPrintln();
            }
        } // End GNSS online checking
    }     // Check every 2000ms
}

// These are the callbacks that get regularly called, globals are updated
void storePVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
    altitude = ubxDataStruct->height / 1000.0;

    gnssDay = ubxDataStruct->day;
    gnssMonth = ubxDataStruct->month;
    gnssYear = ubxDataStruct->year;

    gnssHour = ubxDataStruct->hour;
    gnssMinute = ubxDataStruct->min;
    gnssSecond = ubxDataStruct->sec;
    gnssNano = ubxDataStruct->nano;
    mseconds = ceil((ubxDataStruct->iTOW % 1000) / 10.0); // Limit to first two digits

    numSV = ubxDataStruct->numSV;
    fixType = ubxDataStruct->fixType;
    carrSoln = ubxDataStruct->flags.bits.carrSoln;

    validDate = ubxDataStruct->valid.bits.validDate;
    validTime = ubxDataStruct->valid.bits.validTime;
    fullyResolved = ubxDataStruct->valid.bits.fullyResolved;
    tAcc = ubxDataStruct->tAcc;
    confirmedDate = ubxDataStruct->flags2.bits.confirmedDate;
    confirmedTime = ubxDataStruct->flags2.bits.confirmedTime;

    pvtArrivalMillis = millis();
    pvtUpdated = true;
}

void storeHPdata(UBX_NAV_HPPOSLLH_data_t *ubxDataStruct)
{
    horizontalAccuracy = ((float)ubxDataStruct->hAcc) / 10000.0; // Convert hAcc from mm*0.1 to m

    latitude = ((double)ubxDataStruct->lat) / 10000000.0;
    latitude += ((double)ubxDataStruct->latHp) / 1000000000.0;
    longitude = ((double)ubxDataStruct->lon) / 10000000.0;
    longitude += ((double)ubxDataStruct->lonHp) / 1000000000.0;
}

void storeTIMTPdata(UBX_TIM_TP_data_t *ubxDataStruct)
{
    uint32_t tow = ubxDataStruct->week - SFE_UBLOX_JAN_1ST_2020_WEEK; // Calculate the number of weeks since Jan 1st
                                                                      // 2020
    tow *= SFE_UBLOX_SECS_PER_WEEK;     // Convert weeks to seconds
    tow += SFE_UBLOX_EPOCH_WEEK_2086;   // Add the TOW for Jan 1st 2020
    tow += ubxDataStruct->towMS / 1000; // Add the TOW for the next TP

    uint32_t us = ubxDataStruct->towMS % 1000; // Extract the milliseconds
    us *= 1000;                                // Convert to microseconds

    double subMS = ubxDataStruct->towSubMS; // Get towSubMS (ms * 2^-32)
    subMS *= pow(2.0, -32.0);               // Convert to milliseconds
    subMS *= 1000;                          // Convert to microseconds

    us += (uint32_t)subMS; // Add subMS

    timTpEpoch = tow;
    timTpMicros = us;
    timTpArrivalMillis = millis();
    timTpUpdated = true;
}

void storeMONHWdata(UBX_MON_HW_data_t *ubxDataStruct)
{
    aStatus = ubxDataStruct->aStatus;
}

void storeRTCM1005data(RTCM_1005_data_t *rtcmData1005)
{
    ARPECEFX = rtcmData1005->AntennaReferencePointECEFX;
    ARPECEFY = rtcmData1005->AntennaReferencePointECEFY;
    ARPECEFZ = rtcmData1005->AntennaReferencePointECEFZ;
    ARPECEFH = 0;
    newARPAvailable = true;
}

void storeRTCM1006data(RTCM_1006_data_t *rtcmData1006)
{
    ARPECEFX = rtcmData1006->AntennaReferencePointECEFX;
    ARPECEFY = rtcmData1006->AntennaReferencePointECEFY;
    ARPECEFZ = rtcmData1006->AntennaReferencePointECEFZ;
    ARPECEFH = rtcmData1006->AntennaHeight;
    newARPAvailable = true;
}

// Helper method to convert GNSS time and date into Unix Epoch
void convertGnssTimeToEpoch(uint32_t *epochSecs, uint32_t *epochMicros)
{
    uint32_t t = SFE_UBLOX_DAYS_FROM_1970_TO_2020;             // Jan 1st 2020 as days from Jan 1st 1970
    t += (uint32_t)SFE_UBLOX_DAYS_SINCE_2020[gnssYear - 2020]; // Add on the number of days since 2020
    t += (uint32_t)
        SFE_UBLOX_DAYS_SINCE_MONTH[gnssYear % 4 == 0 ? 0 : 1][gnssMonth - 1]; // Add on the number of days since Jan 1st
    t += (uint32_t)gnssDay - 1; // Add on the number of days since the 1st of the month
    t *= 24;                    // Convert to hours
    t += (uint32_t)gnssHour;    // Add on the hour
    t *= 60;                    // Convert to minutes
    t += (uint32_t)gnssMinute;  // Add on the minute
    t *= 60;                    // Convert to seconds
    t += (uint32_t)gnssSecond;  // Add on the second

    int32_t us = gnssNano / 1000; // Convert nanos to micros
    uint32_t micro;
    // Adjust t if nano is negative
    if (us < 0)
    {
        micro = (uint32_t)(us + 1000000); // Make nano +ve
        t--;                              // Decrement t by 1 second
    }
    else
    {
        micro = us;
    }

    *epochSecs = t;
    *epochMicros = micro;
}
