# Updating Firmware From CLI

The command line interface is also available for more advanced users or users who want to avoid the hassle of swapping out SD cards. You’ll need to download esptool.exe and RTK_Surveyor_Firmware_vXXX_Combined.bin from [the repo](https://github.com/sparkfun/SparkFun_RTK_Firmware/tree/main/Binaries).

Connect a USB A to C cable from your computer to the ESP32 port on the RTK Facet. Now identify the com port the RTK Enumerated at. The easiest way to do this is to open the device manager:

[![CH340 is on COM6 as shown in Device Manager](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_COM_Port.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_COM_Port.jpg)

*CH340 is on COM6 as shown in Device Manager*

If the COM port is not showing be sure the unit is turned **On**. If an unknown device is appearing, you’ll need to [install drivers for the CH340](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all). Once you know the COM port, open a command prompt (Windows button + r then type ‘cmd’).

Navigate to the directory that contains the firmware file and esptool.exe. Run the following command:

    language:c
    esptool.exe --chip esp32 --port COM6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0 RTK_Surveyor_Firmware_v19_combined.bin

Note: You will need to modify **COM6** to match the serial port that RTK Facet enumerates at.

[![Programming via the esptool CLI](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_CLI.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_CLI.jpg)

*Programming via the esptool CLI*

Upon completion, your RTK Facet will have the latest and greatest features!