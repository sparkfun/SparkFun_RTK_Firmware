This folder contains a 'StayOn' firmware that forces the ESP32 into standby mode regardless of the rest of the system. This is useful on units that require troubleshooting or alternate firmware to be loaded. A USB cable must be connected between the RTK unit and the 'CONFIG ESP32' port. Run 'batch_program.bat COM4' where *COM4* is replaced with the COM port that RTK unit enumerated at. Once the batch file is running, press return to attempt to load the firmware. You will need to press and hold the power button on the unit then press enter to send new firmware while the ESP32 is awake. Once the firmware is loaded, you will no longer need to hold the power button.

Once the firmware is loaded, the unit will not have a display, or respond to Bluetooth or do anything useful, other than stay available for a new firmware to be loaded.

For those who want to run the CLI directly, use the following command:

esptool.exe --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x00 StayOn.bin

