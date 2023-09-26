// Enable data output from the NEO
bool zedEnableLBandCommunication()
{
    bool response = true;

#ifdef COMPILE_L_BAND

    response &= theGNSS.setRXMCORcallbackPtr(&checkRXMCOR); // Enable callback to check if the PMP data is being decrypted successfully

    if (productVariant == RTK_FACET_LBAND_DIRECT)
    {
        // Setup for ZED to NEO serial communication
        response &= theGNSS.setVal32(UBLOX_CFG_UART2INPROT_UBX, true); // Configure ZED for UBX input on UART2

        // Disable PMP callback over I2C. Enable UARTs.
        response &= i2cLBand.setRXMPMPmessageCallbackPtr(nullptr); // Disable PMP callback to push raw PMP over I2C

        response &= i2cLBand.newCfgValset();
        response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C, 0); // Disable UBX-RXM-PMP on NEO's I2C port

        response &= i2cLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 1);         // Enable UBX output on NEO's UART2
        response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2, 1); // Output UBX-RXM-PMP on NEO's UART2
        response &= i2cLBand.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, settings.radioPortBaud); // Match baudrate with ZED
    }
    else if (productVariant == RTK_FACET_LBAND)
    {
        // Older versions of the Facet L-Band had solder jumpers that could be closed to directly connect the NEO
        // to the ZED. If the user has explicitly disabled I2C corrections, enable a UART connection.
        if (settings.useI2cForLbandCorrections == true)
        {
            response &= theGNSS.setVal32(UBLOX_CFG_UART2INPROT_UBX, settings.enableUART2UBXIn);

            i2cLBand.setRXMPMPmessageCallbackPtr(&pushRXMPMP); // Enable PMP callback to push raw PMP over I2C

            response &= i2cLBand.newCfgValset();
            response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C, 1); // Enable UBX-RXM-PMP on NEO's I2C port

            response &= i2cLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0);         // Disable UBX output on NEO's UART2
            response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2, 0); // Disable UBX-RXM-PMP on NEO's UART2
        }
        else // Setup ZED to NEO serial communication
        {
            response &= theGNSS.setVal32(UBLOX_CFG_UART2INPROT_UBX, true); // Configure ZED for UBX input on UART2

            i2cLBand.setRXMPMPmessageCallbackPtr(nullptr); // Disable PMP callback to push raw PMP over I2C

            response &= i2cLBand.newCfgValset();
            response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C, 0); // Disable UBX-RXM-PMP on NEO's I2C port

            response &= i2cLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 1);         // Enable UBX output on UART2
            response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2, 1); // Output UBX-RXM-PMP on UART2
            response &=
                i2cLBand.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, settings.radioPortBaud); // Match baudrate with ZED
        }
    }
    else
    {
        systemPrintln("zedEnableLBandCorrections: Unknown platform");
        return (false);
    }

    response &= i2cLBand.sendCfgValset();

#endif

    return (response);
}

// Disable data output from the NEO
bool zedDisableLBandCommunication()
{
    bool response = true;

#ifdef COMPILE_L_BAND
    response &= i2cLBand.setRXMPMPmessageCallbackPtr(nullptr); // Disable PMP callback no matter the platform
    response &= theGNSS.setRXMCORcallbackPtr(nullptr); // Disable callback to check if the PMP data is being decrypted successfully

    if (productVariant == RTK_FACET_LBAND_DIRECT)
    {
        response &= i2cLBand.newCfgValset();
        response &= i2cLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0); // Disable UBX output from NEO's UART2
    }
    else if (productVariant == RTK_FACET_LBAND)
    {
        // Older versions of the Facet L-Band had solder jumpers that could be closed to directly connect the NEO
        // to the ZED. Check if the user has explicitly set I2C corrections.
        if (settings.useI2cForLbandCorrections == true)
        {
            response &= i2cLBand.newCfgValset();
            response &= i2cLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C, 0); // Disable UBX-RXM-PMP from NEO's I2C port
        }
        else // Setup ZED to NEO serial communication
        {
            response &= i2cLBand.newCfgValset();
            response &= i2cLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, 0); // Disable UBX output from NEO's UART2
        }
    }
    else
    {
        systemPrintln("zedEnableLBandCorrections: Unknown platform");
        return (false);
    }

    response &= i2cLBand.sendCfgValset();

#endif

    return (response);
}