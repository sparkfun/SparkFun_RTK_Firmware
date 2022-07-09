void menuBluetooth()
{
    while (1)
    {
        Serial.println();
        Serial.println(F("Menu: Blueooth Menu"));

        Serial.println();
        Serial.print(F("Current Bluetooth Mode: "));
        if (settings.enableBLE == true)
            Serial.println(F("BLE"));
        else
            Serial.println(F("Classic"));

        Serial.println();
        Serial.println(F("1) Set Bluetooth Mode to Classic"));
        Serial.println(F("2) Set Bluetooth Mode to BLE"));
        Serial.println(F("x) Exit"));

        byte incoming = getByteChoice(menuTimeout);
        if (incoming == '1')
        {
            // Restart Bluetooth
            stopBluetooth();
            settings.enableBLE = false;
            startBluetooth();
        }
        else if (incoming == '2')
        {
            // restart Bluetooth
            stopBluetooth();
            settings.enableBLE = true;
            startBluetooth();
        }
        else if (incoming == 'x')
            break;
        else if (incoming == STATUS_GETBYTE_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    while (Serial.available()) Serial.read();
}

