This folder contains the various firmware versions for RTK Products (Surveyor, Express, Express+, and Facet). Each binary is created by exporting a sketch binary from Arduino. The sketch binary is then uploaded to the ESP32 along with boot_app0.bin, RTK_Surveyor.ino.bootloader.bin, and RTK_Surveyor.ino.partitions.bin. You can update the firmware on a device by loading a binary onto the SD card and inserting it into the RTK Surveyor (see more information [here](https://learn.sparkfun.com/tutorials/sparkfun-rtk-surveyor-hookup-guide/firmware-updates-and-customization)) or by using the following CLI:

esptool.exe --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 ./bin/RTK_Surveyor.ino.bootloader.bin 0x8000 ./bin/RTK_Surveyor.ino.partitions.bin 0xe000 ./bin/boot_app0.bin 0x10000 ./RTK_Surveyor_Firmware_vxx.bin

Where *COM4* is replaced with the COM port that the RTK product enumerated at and *RTK_Surveyor_Firmware_vxx.bin* is the firmware you would like to load.

---------------------

### Force Firmware Loading

In the rare event a unit is not staying on long enough for new firmware to be loaded into a COM port, the RTK Firmware (as of version 1.2) has an override function. If a file named *RTK_Surveyor_Firmware_Force.bin* is detected on the SD card at boot that file will be used to overwrite the current firmware, and then be deleted. This update path is generally not recommend. Use the [WiFi OTA method](https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-hookup-guide/all#firmware-updates-and-customization) or via the serial menu as first resort.