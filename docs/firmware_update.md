# Updating RTK Firmware

There are two firmwares that operate on the device:

* Firmware on the ESP32 microcontroller. Keep reading.
* Firmware on the ZED-F9P Receiver. [See below](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/#zed-f9x-firmware).

![[Main Menu showing RTK Firmware v1.8-Oct 7 2021](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Serial_Config_-_Main.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Serial_Config_-_Main.jpg)

*Main Menu showing RTK Firmware v1.8-Oct 7 2021*

You can check your firmware by opening the main menu by pressing a key at any time.

From time to time SparkFun will release new firmware for the RTK product line to add and improve functionality. For most users, firmware can be upgraded by loading the appropriate binary file located on the [releases page](https://github.com/sparkfun/SparkFun_RTK_Firmware/releases) or from the [binaries folder](https://github.com/sparkfun/SparkFun_RTK_Firmware/tree/main/Binaries). Once the firmware is downloaded, loading the firmware onto an RTK product can be achieved by using one of the following methods:

* [SD Method](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-the-sd-card): Load the firmware on an SD card, then use a serial terminal with *Firmware Upgrade* menu
* [WiFi Method](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-wifi): Load the firmware over WiFi when the device is in WiFi AP Config Mode
* [GUI Method](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-gui): Use the [Windows GUI](https://github.com/sparkfun/SparkFun_RTK_Firmware/Uploader_GUI) and a USB cable. (This method is python based which can also be used on Linux, Mac OS, and Raspberry Pi)
* [CLI Method](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-cli): Use the command line *batch_program.bat* (see below)

The SD method is generally recommended. For more information see [here](https://learn.sparkfun.com/tutorials/sparkfun-rtk-surveyor-hookup-guide/firmware-updates-and-customization).

Remember, the RTK Facet is open source hardware meaning you have total access to the [firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware) and [hardware](https://github.com/sparkfun/SparkFun_RTK_Facet). Be sure to checkout each repo for the latest firmware and hardware information.

## Updating Firmware From the SD Card

[![Firmware update menu](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Firmware_Update.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Firmware_Update.jpg)

*Firmware update taking place*

From time to time SparkFun will release new firmware for the RTK Facet to add and improve functionality. For most users, firmware can be upgraded by loading the appropriate firmware file from the [binaries repo folder](https://github.com/sparkfun/SparkFun_RTK_Firmware/tree/main/Binaries/For_SD_Loading) onto the SD card and bringing up the firmware menu as shown above.

The firmware upgrade menu will only display files that have the "RTK_Surveyor_Firmware*.bin" file name format so don't change the file names once loaded onto the SD card. Select the firmware you'd like to load and the system will proceed to load the new firmware, then reboot.

Note: The firmware is called `RTK_Surveyor_Firmware_vXX.bin` even though this product is called the *RTK Facet*. We united the different platforms into one. The [RTK Firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware) runs on all our RTK products.

## Updating Firmware From WiFi

**Note:** Firmware versions 1.1 to 1.9 have an issue that severely limit firmware upload over WiFi and is not recommended; use the 'Updating Firmware From the SD Card' method instead. Firmware versions v1.10 and beyond support direct firmware update via WiFi and is the preferred method for updating the firmware on a unit.

[![Advanced system settings](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Firmware_Update_Button.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Firmware_Update_Button.jpg)

*Advanced system settings*

Alternatively, firmware may be uploaded via the WiFi AP interface. 

**Versions 1.1 to 1.9:** For firmware versions 1.1 to 1.9, the upload process is limited in speed resulting in upload times of nearly 2 minutes. Because of this, we recommend using the 'Updating Firmware From the SD Card' method instead. If you do upload firmware over WiFi, once it has been uploaded it will be viewable on the 'Available Firmware' on the page. To prevent accidental loading the *Enable Firmware Update* checkbox must first be checked before the button is enabled.

**Versions 1.10 and Greater:** Firmware may be uploaded to the unit by clicking on 'Choose File', selecting the binary such as 'RTK_Surveyor_Firmware_v1_xx.bin' and pressing upload. The unit will automatically reset once firmware upload is complete.

## Updating Firmware From GUI

[![RTK Firmware GUI](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/release_candidate/Uploader_GUI/SparkFun%20RTK%20Firmware%20Uploader.jpg?raw=true)](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/release_candidate/Uploader_GUI/SparkFun%20RTK%20Firmware%20Uploader.jpg?raw=true)

*RTK Firmware GUI*

In general, the SD firmware update method is recommended, but for some firmware updates (for example from version v1.x to v2.x) a serial connection via USB is required. This GUI makes it easy to point and click your way through a firmware update.

### To Use

* Attach the RTK device to your computer using a USB cable. 
* Turn the RTK device on.
* Open Device Manager to confirm which COM port the device is operating on.

![Device Manager showing USB Serial port on COM27](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/release_candidate/Uploader_GUI/SparkFun RTK Firmware Uploader COM Port.jpg)

*Device Manager showing USB Serial port on COM27*

* Get the latest binary firmware file from the *[Binaries](https://github.com/sparkfun/SparkFun_RTK_Firmware/tree/main/Binaries)* folder.
* Run *RTK_Firmware_Uploader_GUI.exe* (it takes a few seconds to start)
* Click *Browse* and select the binary file to upload
* Select the COM port previously seen in the Device Manager.
* Click *Upload Firmware*

Once complete, the device will reset and power down.

## Updating Firmware From CLI

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

## Compiling from Source

The SparkFun RTK firmware is compiled using Arduino (currently v1.8.15). To compile:

1. Install [Arduino](https://www.arduino.cc/en/software). 
2. Install ESP32 for Arduino. [Here](https://learn.sparkfun.com/tutorials/esp32-thing-hookup-guide#installing-via-arduino-ide-boards-manager) are some good instructions for installing it via the Arduino Boards manager. **Note**: Use v2.0.2 of the core. **Note:** We use the 'ESP32 Dev Module' for pin numbering. Select the correct board under Tools->Board->ESP32 Arduino->ESP32 Dev Module.
3. Change the Parition table. Replace 'C:\Users\\[user name]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.2\tools\partitions\min_spiffs.csv' with the min_spiff.csv file found in this folder. This will increase the program partition from a maximum of 1.9MB to 3MB.
4. Set the core settings: The 'Partition Scheme' must be set to 'Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS). This will use the 'min_spiffs.csv' updated partition table.
5. Set the 'Flash Size' to 16MB (128mbit)
6. Obtain all the required libraries. **Note:** You should click on the link next to each of the #includes at the top of RTK_Surveyor.ino within the Arduino IDE to open the library manager and download them. Getting them directly from github also works but may not be 'official' releases:
    * [ESP32Time](https://github.com/fbiego/ESP32Time)
    * [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) (not available via library manager)
    * [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) (not available via library manager)
    * [JC_Button](https://github.com/JChristensen/JC_Button)
    * [SdFat](https://github.com/greiman/SdFat)
    * [SparkFun u-blox GNSS Arduino Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library)
    * [SparkFun MAX1704x Fuel Gauge Arduino Library](https://github.com/sparkfun/SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library)
    * [SparkFun Micro OLED Arduino Library](https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library) -  Note the Arduino Library manager lists this as 'SparkFun Micro OLED Breakout'
    * [SparkFun LIS2DH12 Accelerometer Arduino Library](https://github.com/sparkfun/SparkFun_LIS2DH12_Arduino_Library)

Once compiled, firmware can be uploaded directly to a unit when the RTK unit is on and the correct COM port is selected under the Arduino IDE Tools->Port menu.

Note: The COMPILE_WIFI and COMPILE_BT defines at the top of RTK_Surveyor.ino can be commented out to remove them from compilation. This will greatly reduce the firmware size and allow for faster development of functions that do not rely on WiFi or Bluetooth (serial menus, system configuration, logging, etc).

## ZED-F9x firmware

The firmware loaded onto the ZED-F9P or ZED-F9R receiver can vary depending on manufacture date. The RTK Firmware (that runs on the ESP32) is designed to work with any ZED-F9x firmware. Upgrading the ZED-F9x is a good thing to consider but is not crucial to the use of the RTK products.

A tutorial with step-by-step instructions for locating the firmware version as well as changing the firmware can be found in [How to Upgrade Firmware of a u-blox Receiver](https://learn.sparkfun.com/tutorials/how-to-upgrade-firmware-of-a-u-blox-gnss-receiver/all).

### ZED-F9P

This module is used in the Surveyor, Express, and Facet. It is capable of both Rover *and* base modes.

All field testing and device specific performance parameters were obtained with ZED-F9P v1.30.

v1.12 has the benefit of working with SBAS and an operational RTK status signal (the LED illuminates correctly). See [release notes](https://content.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG112_RN_%28UBX-19026698%29.pdf).

v1.13 has a few RTK and receiver performance improvements but introduces a bug that causes the RTK Status LED to fail when SBAS is enabled. See [release notes](https://content.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG113_RN_%28UBX-20019211%29.pdf).

v1.30 has a few RTK and receiver performance improvements, I<sup>2</sup>C communication improvements, and most importantly support for SPARTN PMP packets. See [release notes](https://www.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG130_RN_UBX-21047459.pdf).

v1.32 has a few SPARTN protocol specific improvements. See [release notes](https://www.u-blox.com/sites/default/files/documents/ZED-F9P-FW100-HPG132_RN_UBX-22004887.pdf).

### ZED-F9R

This module is used in the Express Plus. It contains an internal IMU and additional algorithms to support high precision location fixes using dead reckoning. The ZED-F9R is not capable of operating in base mode.

v1.00 Initial release.

v1.21 SPARTN support as well as adding E-scooter and robotic lawnmower dynamic models. See [release notes](https://www.u-blox.com/sites/default/files/ZED-F9R-02B_FW1.00HPS1.21_RN_UBX-21035491_1.3.pdf).
