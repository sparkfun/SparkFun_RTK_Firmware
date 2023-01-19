# System Status Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

![System menu](img/SparkFun%20RTK%20System%20Menu.png)

*Menu showing various attributes of the system*

The System Status menu will show a large number of system parameters including a full system check to verify what is and what is not online. For example, if an SD card is detected it will be shown as online. Not all systems have all hardware. For example, the RTK Surveyor does not have an accelerometer so it will always be shown offline.

This menu is helpful when reporting technical issues or requesting support as it displays helpful information about the current ZED-F9x firmware version, and which parts of the unit are online.

* **z** - A local timezone in hours, minutes and seconds may be set by pressing 'z'. The timezone values change the RTC clock setting and the file system's timestamps for new files.

* **d** - Enters the [debug menu](https://sparkfun.github.io/SparkFun_RTK_Firmware/menu_debug/) that is for advanced users.

* **f** - Show any files on the microSD card (if present).

* **b** - Change the Bluetooth protocol. By default, Serial Port Profile (SPP) for Bluetooth v2.0 is used. This can be changed to BLE if desired at which time serial is sent over BLESerial. Additionally, Bluetooth can be turned off. This state is normally used for debugging.

* **r** - Reset all settings to default including a factory reset of the ZED-F9x receiver. This can be helpful if the unit has been configured into an unknown or problematic state.

* **B, R, W, or S** - Change the mode the device is in without needing to press the external SETUP or POWER buttons.

![System Config over WiFi](img/SparkFun%20RTK%20WiFi%20Config%20System.png)

*System Config over WiFi Config*

The WiFi Config page also allows various aspects of the system to be configured but it is limited by design.

