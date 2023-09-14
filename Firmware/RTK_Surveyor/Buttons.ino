// User has pressed the power button to turn on the system
// Was it an accidental bump or do they really want to turn on?
// Let's make sure they continue to press for 500ms
void powerOnCheck()
{
    powerPressedStartTime = millis();
    if (pin_powerSenseAndControl >= 0)
        if (digitalRead(pin_powerSenseAndControl) == LOW)
            delay(500);

    if (FIRMWARE_VERSION_MAJOR == 99)
    {
        // Do not check button if this is a locally compiled developer firmware
    }
    else
    {
        if (pin_powerSenseAndControl >= 0)
            if (digitalRead(pin_powerSenseAndControl) != LOW)
                powerDown(false); // Power button tap. Returning to off state.
    }

    powerPressedStartTime = 0; // Reset var to return to normal 'on' state
}

// If we have a power button tap, or if the display is not yet started (no I2C!)
// then don't display a shutdown screen
void powerDown(bool displayInfo)
{
    // Disable SD card use
    endSD(false, false);

    // Prevent other tasks from logging, even if access to the microSD card was denied
    online.logging = false;

    // If we are in configureViaEthernet mode, we need to shut down the async web server
    // otherwise it causes a core panic and badness at the restart
    if (configureViaEthernet)
        ethernetWebServerStopESP32W5500();

    if (displayInfo == true)
    {
        displayShutdown();
        delay(2000);
    }

    beginLEDs(); // Turn LEDs off

    if (pin_powerSenseAndControl >= 0)
    {
        pinMode(pin_powerSenseAndControl, OUTPUT);
        digitalWrite(pin_powerSenseAndControl, LOW);
    }

    if (pin_powerFastOff >= 0)
    {
        pinMode(pin_powerFastOff, OUTPUT);
        digitalWrite(pin_powerFastOff, LOW);
    }

    if ((productVariant == RTK_FACET) || (productVariant == RTK_FACET_LBAND) ||
        (productVariant == RTK_FACET_LBAND_DIRECT) || (productVariant == REFERENCE_STATION))
        digitalWrite(pin_peripheralPowerControl, LOW);

    while (1)
        delay(1);
}
