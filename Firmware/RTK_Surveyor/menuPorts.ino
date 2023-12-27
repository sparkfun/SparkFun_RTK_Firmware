void menuPorts()
{
    if (productVariant == RTK_SURVEYOR || productVariant == REFERENCE_STATION)
        menuPortsSurveyor();
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET ||
             productVariant == RTK_FACET_LBAND || productVariant == RTK_FACET_LBAND_DIRECT)
        menuPortsMultiplexed();
}

// Set the baud rates for the radio and data ports
void menuPortsSurveyor()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Ports");

        systemPrint("1) Set serial baud rate for Radio Port: ");
        if (settings.radioPortBaud == 0)
        {
            systemPrintln("Disabled");
        }
        else
        {
            systemPrint(theGNSS.getVal32(UBLOX_CFG_UART2_BAUDRATE));
            systemPrintln(" bps");
        }

        systemPrint("2) Set serial baud rate for Data Port: ");
        if (settings.dataPortBaud == 0)
        {
            systemPrintln("Disabled");
        }
        else
        {
            systemPrint(theGNSS.getVal32(UBLOX_CFG_UART1_BAUDRATE));
            systemPrintln(" bps");
        }

        systemPrint("3) GNSS UART2 UBX Protocol In: ");
        if (settings.enableUART2UBXIn == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrintln("x) Exit");

        int incoming = getNumber(); // Returns EXIT, TIMEOUT, or long

        if (incoming == 1)
        {
            systemPrint("Enter baud rate (4800 to 921600, 0 = disable) for Radio Port: ");
            int newBaud = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (newBaud == 0 || newBaud == 4800 || newBaud == 9600 || newBaud == 19200 || newBaud == 38400 ||
                    newBaud == 57600 || newBaud == 115200 || newBaud == 230400 || newBaud == 460800 ||
                    newBaud == 921600)
                {
                    settings.radioPortBaud = newBaud;
                    if (online.gnss == true)
                    {
                        if (newBaud == 0)
                        {
                            // Disable all protocols in/out of UART2
                            bool response = true;

                            response &= theGNSS.newCfgValset();
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_NMEA, 0);
                            if (commandSupported(UBLOX_CFG_UART2OUTPROT_RTCM3X) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_RTCM3X, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_NMEA, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_RTCM3X, 0);
                            if (commandSupported(UBLOX_CFG_UART2INPROT_SPARTN) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_SPARTN, 0);
                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART2 settings");
                        }
                        else
                        {
                            bool response = true;

                            response &= theGNSS.newCfgValset();

                            // Set the baud rate
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, settings.radioPortBaud);

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

                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART2 settings");
                        }
                    }
                }
                else
                {
                    systemPrintln("Error: Baud rate out of range");
                }
            }
        }
        else if (incoming == 2)
        {
            systemPrint("Enter baud rate (4800 to 921600, 0 = disable) for Data Port: ");
            int newBaud = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (newBaud == 0 || newBaud == 4800 || newBaud == 9600 || newBaud == 19200 || newBaud == 38400 ||
                    newBaud == 57600 || newBaud == 115200 || newBaud == 230400 || newBaud == 460800 ||
                    newBaud == 921600)
                {
                    settings.dataPortBaud = newBaud;
                    if (online.gnss == true)
                    {
                        if (newBaud == 0)
                        {
                            // Disable all protocols in/out of UART1
                            bool response = true;

                            response &= theGNSS.newCfgValset();
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 0);
                            if (commandSupported(UBLOX_CFG_UART1OUTPROT_RTCM3X) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 0);
                            if (commandSupported(UBLOX_CFG_UART1INPROT_SPARTN) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);
                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART1 settings");
                        }
                        else
                        {
                            bool response = true;

                            response &= theGNSS.newCfgValset();

                            // Set the baud rate
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1_BAUDRATE, settings.dataPortBaud);

                            // Turn on all protocols except SPARTN
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 1);
                            if (commandSupported(UBLOX_CFG_UART1OUTPROT_RTCM3X) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 1);
                            if (commandSupported(UBLOX_CFG_UART1INPROT_SPARTN) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);
                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART1 settings");
                        }
                    }
                }
                else
                {
                    systemPrintln("Error: Baud rate out of range");
                }
            }
        }
        else if (incoming == 3)
        {
            settings.enableUART2UBXIn ^= 1;
            systemPrintln("UART2 Protocol In updated. Changes will be applied at next restart");
        }

        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    clearBuffer(); // Empty buffer of any newline chars
}

// Set the baud rates for the radio and data ports
void menuPortsMultiplexed()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Ports");

        systemPrint("1) Set Radio port serial baud rate: ");
        if (settings.radioPortBaud == 0)
        {
            systemPrintln("Disabled");
        }
        else
        {
            systemPrint(theGNSS.getVal32(UBLOX_CFG_UART2_BAUDRATE));
            systemPrintln(" bps");
        }

        systemPrint("2) Set Data port connections: ");
        if (settings.dataPortChannel == MUX_UBLOX_NMEA)
            systemPrintln("NMEA TX Out/RX In");
        else if (settings.dataPortChannel == MUX_PPS_EVENTTRIGGER)
            systemPrintln("PPS OUT/Event Trigger In");
        else if (settings.dataPortChannel == MUX_I2C_WT)
        {
            if (zedModuleType == PLATFORM_F9P)
                systemPrintln("I2C SDA/SCL");
            else if (zedModuleType == PLATFORM_F9R)
                systemPrintln("Wheel Tick/Direction");
        }
        else if (settings.dataPortChannel == MUX_ADC_DAC)
            systemPrintln("ESP32 DAC Out/ADC In");

        if (settings.dataPortChannel == MUX_UBLOX_NMEA)
        {
            systemPrint("3) Set Data port serial baud rate: ");
            if (settings.dataPortBaud == 0)
            {
                systemPrintln("Disabled");
            }
            else
            {
                systemPrint(theGNSS.getVal32(UBLOX_CFG_UART1_BAUDRATE));
                systemPrintln(" bps");
            }
        }
        else if (settings.dataPortChannel == MUX_PPS_EVENTTRIGGER)
        {
            systemPrintln("3) Configure External Triggers");
        }

        systemPrint("4) GNSS UART2 UBX Protocol In: ");
        if (settings.enableUART2UBXIn == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        // The Facet L-Band Direct has ZED tied to NEO over serial. Attaching an external radio
        // will cause the NEO to compete with the radio. If user wants to use external radio, we
        // switch off the NEO and send PMP over I2C. By default, Facet L-Band v14 does not
        // useI2cForLbandCorrections.
        if (productVariant == RTK_FACET_LBAND_DIRECT)
        {
            systemPrint("5) Enable external radio: ");
            if (settings.useI2cForLbandCorrections == true)
                systemPrintln("Enabled");
            else
                systemPrintln("Disabled");
        }

        systemPrintln("x) Exit");

        int incoming = getNumber(); // Returns EXIT, TIMEOUT, or long

        if (incoming == 1)
        {
            systemPrint("Enter baud rate (4800 to 921600, 0 = disable) for Radio Port: ");
            int newBaud = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (newBaud == 0 || newBaud == 4800 || newBaud == 9600 || newBaud == 19200 || newBaud == 38400 ||
                    newBaud == 57600 || newBaud == 115200 || newBaud == 230400 || newBaud == 460800 ||
                    newBaud == 921600)
                {
                    settings.radioPortBaud = newBaud;
                    if (online.gnss == true)
                    {
                        if (newBaud == 0)
                        {
                            // Disable all protocols in/out of UART2
                            bool response = true;

                            response &= theGNSS.newCfgValset();
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_NMEA, 0);
                            if (commandSupported(UBLOX_CFG_UART2OUTPROT_RTCM3X) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_RTCM3X, 0);

                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_NMEA, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_RTCM3X, 0);
                            if (commandSupported(UBLOX_CFG_UART2INPROT_SPARTN) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART2INPROT_SPARTN, 0);
                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART2 settings");
                        }
                        else
                        {
                            bool response = true;

                            response &= theGNSS.newCfgValset();

                            // Set the baud rate
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, settings.radioPortBaud);

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
                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART2 settings");
                        }
                    }
                }
                else
                {
                    systemPrintln("Error: Baud rate out of range");
                }
            }
        }
        else if (incoming == 2)
        {
            systemPrintln("\r\nEnter the pin connection to use (1 to 4) for Data Port: ");
            systemPrintln("1) NMEA TX Out/RX In");
            systemPrintln("2) PPS OUT/Event Trigger In");
            if (zedModuleType == PLATFORM_F9P)
                systemPrintln("3) I2C SDA/SCL");
            else if (zedModuleType == PLATFORM_F9R)
                systemPrintln("3) Wheel Tick/Direction");
            systemPrintln("4) ESP32 DAC Out/ADC In");

            int muxPort = getNumber(); // Returns EXIT, TIMEOUT, or long
            if (muxPort < 1 || muxPort > 4)
            {
                systemPrintln("Error: Pin connection out of range");
            }
            else
            {
                settings.dataPortChannel = (muxConnectionType_e)(muxPort - 1); // Adjust user input from 1-4 to 0-3
                setMuxport(settings.dataPortChannel);
            }
        }
        else if (incoming == 3 && settings.dataPortChannel == MUX_UBLOX_NMEA)
        {
            systemPrint("Enter baud rate (4800 to 921600, 0 = disable) for Data Port: ");
            int newBaud = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (newBaud == 0 || newBaud == 4800 || newBaud == 9600 || newBaud == 19200 || newBaud == 38400 ||
                    newBaud == 57600 || newBaud == 115200 || newBaud == 230400 || newBaud == 460800 ||
                    newBaud == 921600)
                {
                    settings.dataPortBaud = newBaud;
                    if (online.gnss == true)
                    {
                        if (newBaud == 0)
                        {
                            // Disable all protocols in/out of UART1
                            bool response = true;

                            response &= theGNSS.newCfgValset();
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 0);
                            if (commandSupported(UBLOX_CFG_UART1OUTPROT_RTCM3X) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 0);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 0);
                            if (commandSupported(UBLOX_CFG_UART1INPROT_SPARTN) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);
                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART1 settings");
                        }
                        else
                        {
                            bool response = true;

                            response &= theGNSS.newCfgValset();

                            // Set the baud rate
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1_BAUDRATE, settings.dataPortBaud);

                            // Turn on all protocols except SPARTN
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, 1);
                            if (commandSupported(UBLOX_CFG_UART1OUTPROT_RTCM3X) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1OUTPROT_RTCM3X, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_UBX, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, 1);
                            response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_RTCM3X, 1);
                            if (commandSupported(UBLOX_CFG_UART1INPROT_SPARTN) == true)
                                response &= theGNSS.addCfgValset(UBLOX_CFG_UART1INPROT_SPARTN, 0);

                            response &= theGNSS.sendCfgValset();

                            if (response == false)
                                systemPrintln("Failed to set UART1 settings");
                        }
                    }
                }
                else
                {
                    systemPrintln("Error: Baud rate out of range");
                }
            }
        }
        else if (incoming == 3 && settings.dataPortChannel == MUX_PPS_EVENTTRIGGER)
        {
            menuPortHardwareTriggers();
        }
        else if (incoming == 4)
        {
            settings.enableUART2UBXIn ^= 1;
            systemPrintln("UART2 Protocol In updated. Changes will be applied at next restart.");
        }
        else if (productVariant == RTK_FACET_LBAND_DIRECT && incoming == 5)
        {
            settings.useI2cForLbandCorrectionsConfigured =
                true; // Record that the user has manually modified the settings.
            settings.useI2cForLbandCorrections ^= 1;
            systemPrintln("External radio port updated. Changes will be applied at next restart.");
        }
        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    clearBuffer(); // Empty buffer of any newline chars
}

// Configure the behavior of the PPS and INT pins on the ZED-F9P
// Most often used for logging events (inputs) and when external triggers (outputs) occur
void menuPortHardwareTriggers()
{
    bool updateSettings = false;
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Port Hardware Trigger");

        systemPrint("1) Enable External Pulse: ");
        if (settings.enableExternalPulse == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        if (settings.enableExternalPulse == true)
        {
            systemPrint("2) Set time between pulses: ");
            systemPrint(settings.externalPulseTimeBetweenPulse_us / 1000.0, 0);
            systemPrintln("ms");

            systemPrint("3) Set pulse length: ");
            systemPrint(settings.externalPulseLength_us / 1000.0, 0);
            systemPrintln("ms");

            systemPrint("4) Set pulse polarity: ");
            if (settings.externalPulsePolarity == PULSE_RISING_EDGE)
                systemPrintln("Rising");
            else
                systemPrintln("Falling");
        }

        systemPrint("5) Log External Events: ");
        if (settings.enableExternalHardwareEventLogging == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrintln("x) Exit");

        int incoming = getNumber(); // Returns EXIT, TIMEOUT, or long

        if (incoming == 1)
        {
            settings.enableExternalPulse ^= 1;
            updateSettings = true;
        }
        else if (incoming == 2 && settings.enableExternalPulse == true)
        {
            systemPrint("Time between pulses in milliseconds: ");
            long pulseTime = getNumber(); // Returns EXIT, TIMEOUT, or long

            if (pulseTime != INPUT_RESPONSE_GETNUMBER_TIMEOUT && pulseTime != INPUT_RESPONSE_GETNUMBER_EXIT)
            {
                if (pulseTime < 1 || pulseTime > 60000) // 60s max
                    systemPrintln("Error: Time between pulses out of range");
                else
                {
                    settings.externalPulseTimeBetweenPulse_us = pulseTime * 1000;

                    if (pulseTime <
                        (settings.externalPulseLength_us / 1000)) // pulseTime must be longer than pulseLength
                        settings.externalPulseLength_us = settings.externalPulseTimeBetweenPulse_us /
                                                          2; // Force pulse length to be 1/2 time between pulses

                    updateSettings = true;
                }
            }
        }
        else if (incoming == 3 && settings.enableExternalPulse == true)
        {
            systemPrint("Pulse length in milliseconds: ");
            long pulseLength = getNumber(); // Returns EXIT, TIMEOUT, or long

            if (pulseLength != INPUT_RESPONSE_GETNUMBER_TIMEOUT && pulseLength != INPUT_RESPONSE_GETNUMBER_EXIT)
            {
                if (pulseLength >
                    (settings.externalPulseTimeBetweenPulse_us / 1000)) // pulseLength must be shorter than pulseTime
                    systemPrintln("Error: Pulse length must be shorter than time between pulses");
                else
                {
                    settings.externalPulseLength_us = pulseLength * 1000;
                    updateSettings = true;
                }
            }
        }
        else if (incoming == 4 && settings.enableExternalPulse == true)
        {
            if (settings.externalPulsePolarity == PULSE_RISING_EDGE)
                settings.externalPulsePolarity = PULSE_FALLING_EDGE;
            else
                settings.externalPulsePolarity = PULSE_RISING_EDGE;
            updateSettings = true;
        }
        else if (incoming == 5)
        {
            settings.enableExternalHardwareEventLogging ^= 1;
            updateSettings = true;
        }
        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    clearBuffer(); // Empty buffer of any newline chars

    if (updateSettings)
    {
        settings.updateZEDSettings = true; // Force update
        beginExternalTriggers();           // Update with new settings
    }
}

void eventTriggerReceived(UBX_TIM_TM2_data_t *ubxDataStruct)
{
    // It is the rising edge of the sound event (TRIG) which is important
    // The falling edge is less useful, as it will be "debounced" by the loop code
    if (ubxDataStruct->flags.bits.newRisingEdge) // 1 if a new rising edge was detected
    {
        systemPrintln("Rising Edge Event");

        triggerCount = ubxDataStruct->count;
        triggerTowMsR = ubxDataStruct->towMsR; // Time Of Week of rising edge (ms)
        triggerTowSubMsR =
            ubxDataStruct->towSubMsR;          // Millisecond fraction of Time Of Week of rising edge in nanoseconds
        triggerAccEst = ubxDataStruct->accEst; // Nanosecond accuracy estimate

        newEventToRecord = true;
    }
}
