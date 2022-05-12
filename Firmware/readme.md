### Firmware

There are two firmwares that operate on the device:

* Firmware on the ZED-F9P Receiver
* Firmware on the ESP32 microcontroller

This folder contains the firmware for the ESP32

* **RTK_Surveyor** - The main firmware for the RTK Surveyor
* **Test Sketches** - Various sketches used in the making of the main firmware. Used internally to verify different features. Reader beware.

----
### Compilation Instructions

The SparkFun RTK firmware is compiled using Arduino (currently v1.8.15). To compile:

1. Install [Arduino](https://www.arduino.cc/en/software). 
2. Install ESP32 for Arduino. [Here](https://learn.sparkfun.com/tutorials/esp32-thing-hookup-guide#installing-via-arduino-ide-boards-manager) are some good instructions for installing it via the Arduino Boards manager. **Note**: Use v2.0.2 of the core. **Note:** We use the 'ESP32 Dev Module' for pin numbering. Select the correct board under Tools->Board->ESP32 Arduino->ESP32 Dev Module.
3. Change the Parition table. Replace 'C:\Users\\[user name]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.2\tools\partitions\app3M_fat9M_16MB.csv' with the app3M_fat9M_16MB.csv file found in this folder. This will increase the program partition from a maximum of 1.9MB to 3MB.
4. Set the core settings: The 'Partition Scheme' must be set to '16M Flash (3MB APP/9MB FATFS)'. This will use the 'app3M_fat9M_16MB.csv' updated partition table.
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

----
### A note about ZED-F9P firmware

The firmware loaded onto the ZED-F9P receiver can vary depending on manufacture date. Currently one of three versions: v1.12, v1.13, and v1.30. All field testing and device specific performance parameters were obtained with v1.30.

v1.12 has the benefit of working with SBAS and an operational RTK status signal (the LED illuminates correctly).

v1.13 has a few RTK and receiver performance improvements but introduces a bug that causes the RTK Status LED to fail when SBAS is enabled.

v1.30 has a few RTK and receiver performance improvements, I<sup>2</sup>C communication improvements, and most importantly support for Spartan PMP packets.

The RTK Firmware is designed to work with any ZED-F9x firmware. Upgrading the ZED-F9x is a good thing to consider but is not crucial to the use of the RTK products.

A tutorial with step-by-step instructions for locating the firmware version as well as changing the firmware can be found [here](https://learn.sparkfun.com/tutorials/how-to-upgrade-firmware-of-a-u-blox-gnss-receiver).

More information about the differences can be found [here](https://content.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG130_RN_UBX-21047459.pdf).