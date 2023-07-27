# System Menu

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

## WiFi Interface

Because of the nature of these controls, the AP config page is different than the terminal menu.

![System Config Menu on WiFi Config Page](img/WiFi Config/SparkFun%20RTK%20WiFi%20Config%20System.png)

*System Config Menu on WiFi Config Page*

### Check for New Firmware

This feature allows over-the-air updates of the RTK device's firmware. Please see [Updating RTK Firmware](firmware_update.md) for more information.

### System Initial State

At power on, the device will enter either Rover or Base state.

### Log to SD

If a microSD card is detected, all messages will be logged. 

### Max Log Time

Once the max log time is achieved, logging will cease. This is useful for limiting long-term, overnight, static surveys to a certain length of time. Default: 1440 minutes (24 hours). Limit: 1 to 2880 minutes.

### Max Log Length

Every 'max long length' amount of time the current log will be closed and a new log will be started. This is known as cyclic logging and is convenient on *very* long surveys (ie, months or years) to prevent logs from getting too unwieldy and helps limit the risk of log corruption. This will continue until the unit is powered down or the *max logging time* is reached.

### Start New Log

Pressing the 'Start New Log' button will close the current log. A new log will be opened immediately and the file name will be shown. This can be helpful in the field when a certain set of coordinates or feature marks need to be recorded in close proximity to one another. By dividing up the logs, the work can be more easily identified.

### Bluetooth Protocol

By default, the RTK products use Bluetooth v2.0 SPP (Serial Port Profile) to connect to data collectors. Nearly all data collectors support this protocol. The RTK product line also supports BLE (Bluetooth Low Energy). The BLE protocol has a variety of improvements but very few data collectors support it.

**Note:** Bluetooth SPP cannot operate concurrently with ESP-Now radio transmissions. Therefore, if you plan to use the ESP-Now radio system to connect RTK products, the BLE protocol must be used to communicate over Bluetooth to data collectors. Alternatively, ESP-Now works concurrently with WiFi so connecting to a data collector over WiFi can be used.

### Enable Factory Defaults

See [Factory Reset](menu_system.md#factory-reset).

### SD Card

Various stats for the SD card are shown. 

### Update Firmware

New firmware may be uploaded via WiFi to the unit. See [Updating Firmware from WiFi](firmware_update.md#updating-firmware-from-wifi) for more information.

### Reset Counter

A counter is displayed indicating the number of non-power-on-resets since the last power-on.

## Serial Interface

![System menu](img/Terminal/SparkFun%20RTK%20System%20Menu.png)

*Menu showing various attributes of the system*

The System Status menu will show a large number of system parameters including a full system check to verify what is and what is not online. For example, if an SD card is detected it will be shown as online. Not all systems have all hardware. For example, the RTK Surveyor does not have an accelerometer so it will always be shown offline.

This menu is helpful when reporting technical issues or requesting support as it displays helpful information about the current ZED-F9x firmware version, and which parts of the unit are online.

* **z** - A local timezone in hours, minutes and seconds may be set by pressing 'z'. The timezone values change the RTC clock setting and the file system's timestamps for new files.

* **d** - Enters the [debug menu](menu_debug.md) that is for advanced users.

* **f** - Show any files on the microSD card (if present).

* **b** - Change the Bluetooth protocol. By default, Serial Port Profile (SPP) for Bluetooth v2.0 is used. This can be changed to BLE if desired at which time serial is sent over BLESerial. Additionally, Bluetooth can be turned off. This state is normally used for debugging.

* **r** - Reset all settings to default including a factory reset of the ZED-F9x receiver. This can be helpful if the unit has been configured into an unknown or problematic state.

* **B, R, W, or S** - Change the mode the device is in without needing to press the external SETUP or POWER buttons.

![System Config over WiFi](img/WiFi Config/SparkFun%20RTK%20WiFi%20Config%20System.png)

*System Config over WiFi Config*

The WiFi Config page also allows various aspects of the system to be configured but it is limited by design.

## Factory Reset

If a device gets into an unknown state it can be returned to default settings using the WiFi or Serial interfaces. 

![Factory Default button](img/WiFi Config/SparkFun%20RTK%20WiFi%20Factory%20Defaults.png)

*Enabling and Starting a Factory Reset*

Factory Defaults will erase any user settings and reset the internal receiver to stock settings. To prevent accidental reset the checkbox must first be checked before the button is pressed. Any logs on SD are maintained. Any settings file and commonly used coordinate files on the SD card associated with the current profile will be removed.

![Issuing a factory reset](img/Terminal/SparkFun%20RTK%20System%20Menu%20-%20Factory%20Reset.png)

*Issuing and confirming a Factory Reset*

If a device gets into an unknown state it can be returned to default settings. Press 'r' then 'y' to confirm. Factory Default will erase any user settings and reset the internal receiver to stock settings. Any settings file and commonly used coordinate files on the SD card associated with the current profile will be removed.

**Note:** Log files and any other files on the SD card are *not* removed or modified.

Note: A factory reset can also be accomplished by editing the settings files. See Force a [Factory Reset](configure_with_settings_file.md#forcing-a-factory-reset) for more information. 