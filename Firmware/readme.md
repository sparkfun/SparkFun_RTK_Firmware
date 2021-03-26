### Firmware

There are two firmwares that operate on the device:

* Firmware on the ZED-F9P Receiver
* Firmware on the ESP32 microcontroller

This folder contains the firmware for the ESP32

* **RTK_Surveyor** - The main firmware for the RTK Surveyor
* **Test Sketches** - Various sketches used in the making of the main firmware. Used internally to verify different features. Reader beware.

----
### A note about ZED-F9P firmware

The firmware loaded onto the ZED-F9P receiver is currently one of two versions: v1.12 or v1.13. All field testing and device specific performance parameters were obtained with v1.12.

v1.12 has the benefit of working with SBAS and an operational RTK status signal (the LED illuminates correctly).

v1.13 has a few RTK and receiver performance improvements but introduces a bug that causes the RTK Status LED to fail when SBAS is enabled.

A tutorial with step-by-step instructions for locating the firmware version as well as changing the firmware can be found [here](https://learn.sparkfun.com/tutorials/how-to-upgrade-firmware-of-a-u-blox-gnss-receiver).

More information about the differences can be found [here](https://www.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG113_RN_%28UBX-20019211%29.pdf).