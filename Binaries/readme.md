This folder contains the various firmware versions for RTK Surveyor. Each binary is created by exporting a sketch binary from Arduino then combining (using the ESP32 tool) with boot_app0.bin, bootloader_qio_80m.bin, and RTK_Surveyor.ino.partitions.bin. You can update the firmware on a device by using the following CLI:

esptool.exe --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x00 RTK_Surveyor_Firmware_v10.bin

Where *COM4* is replaced with the COM port that RTK Surveyor enumerated at.