# System Status Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

[![Showing various attributes of the system](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_SystemStatus.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_SystemStatus.jpg)

*Showing various attributes of the system*

The System Status menu will show a large number of system parameters including a full system check to verify what is and what is not online. For example, if an SD card is detected it will be shown as online. Not all systems have all hardware. For example, the RTK Surveyor does not have an accelerometer so it will always be shown offline.

This menu is helpful when reporting technical issues or requesting support as it displays helpful information about the current ZED-F9x firmware version, and which parts of the unit are online.

A local timezone in hours, minutes and seconds may be set by pressing 'z'. The timezone values change the RTC clock setting and the file system's timestamps for new files.

Pressing 'f' will show any files on the microSD card (if present).

Pressing 'b' will change the Bluetooth protocol. By default, Serial Port Profile (SPP) for Bluetooth v2.0 is used. This can be changed to BLE if desired at which time serial is sent over BLESerial. Additionally, Bluetooth can be turned off. This state is normally used for debugging.

Pressing 'r' will allow a user to reset all settings to default including a factory reset of the ZED-F9x receiver. This can be helpful if the unit has been configured into an unknown or problematic state.

Pressing 'd' will enter a [debug menu](https://sparkfun.github.io/SparkFun_RTK_Firmware/menu_debug/) that is for advanced users.
