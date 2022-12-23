# Data Logging Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

## WiFi Interface

Because of the nature of these controls, the AP config page is different than the terminal menu.

![System Config Menu on WiFi AP Config Page](img/SparkFun%20RTK%20System%20and%20Data%20Logging%20Configuration.png)

*System Config Menu on WiFi AP Config Page*

### System Initial State

At power on, the device will enter either Rover or Base state.

### Log to SD

If a microSD card is detected, all messages will be logged. 

### Max Log Time

Once the max log time is achieved, logging will cease. This is useful for limiting long-term, overnight, static surveys to a certain length of time. Default: 1440 minutes (24 hours). Limit: 1 to 2880 minutes.

### Max Log Length

Every 'max long length' amount of time the current log will be closed and a new log will be started. This is known as cyclic logging and is convenient on *very* long surveys (ie, months or years) to prevent logs from getting too unwieldy and helps limit the risk of log corruption. This will continue until the unit is powered down or the *max logging time* is reached.

### Start New Log

Pressing the 'Start New Log' button will close the current log. A new log will be opened when the AP Config page is closed and the unit restarts. This can be helpful in the field when a certain set of coordinates or feature marks need to be recorded in close proximity to one another. By dividing up the logs, the work can be more easily identified.

### Bluetooth Protocol

By default, the RTK products use Bluetooth v2.0 SPP (Serial Port Profile) to connect to data collectors. Nearly all data collectors support this protocol. The RTK product line also supports BLE (Bluetooth Low Energy). The BLE protocol has a variety of improvements but very few data collectors support it.

**Note:** Bluetooth SPP cannot operate concurrently with ESP-Now radio transmissions. Therefore, if you plan to use the ESP-Now radio system to connect RTK products, the BLE protocol must be used to communicate over Bluetooth to data collectors. Alternatively, ESP-Now works concurrently with WiFi so connecting to a data collector over WiFi can be used.

### Enable Factory Defaults

Factory Defaults will erase any user settings and reset the internal receiver to stock settings. Any logs on SD are maintained. To prevent accidental reset the checkbox must first be checked before the button is pressed.

### SD Card

Various stats for the SD card are shown. 

### Update Firmware

New firmware may be uploaded via WiFi to the unit. See [Updating Firmware from WiFi](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-wifi) for more information.

### Reset Counter

A counter is displayed indicating the number of non-power-on-resets since the last power-on.

## Serial Interface

[![RTK Data Logging Configuration Menu](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_ExpressPlus_Logging_Cyclic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_ExpressPlus_Logging_Cyclic.jpg)

*RTK Data Logging Configuration Menu*

From the Main Menu, pressing 5 will enter the Logging Menu. This menu will report the status of the microSD card. While you can enable logging, you cannot begin logging until a microSD card is inserted. Any FAT16 or FAT32 formatted microSD card up to 32GB will work. We regularly use the [SparkX brand 1GB cards](https://www.sparkfun.com/products/15107) but note that these log files can get very large (>500MB) so plan accordingly.

* Option 1 will enable/disable logging. If logging is enabled, all messages from the ZED-F9P will be recorded to microSD. A log file is created at power on with the format *SFE_[DeviceName]_YYMMDD_HHMMSS.txt* based on current GPS data/time. The `[DeviceName]` is Surveyor, Express, etc.
* Option 2 allows a user to set the max logging time. This is convenient to determine the location of a fixed antenna or a receiver on a repeatable landmark. Set the RTK Facet to log RAWX data for 10 hours, convert to RINEX, run through an observation processing station and you’ll get the corrected position with <10mm accuracy. Please see the [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station) tutorial for more information.
* Option 3 allows a user to set the max logging length in minutes. Every 'max long length' amount of time the current log will be closed and a new log will be started. This is known as cyclic logging and is convenient on *very* long surveys (ie, months or years) to prevent logs from getting too unwieldy and helps limit the risk of log corruption. This will continue until the unit is powered down or the *max logging time* is reached.
* Option 4 will enable/disable creating a comma separated file (Marks_date.csv) that is written each time the mark state is selected with the setup button on the RTK Surveyor, RTK Express or RTK Express Plus, or the power button on the RTK Facet.

**Note:** If you are wanting to log RAWX sentences to create RINEX files useful for post-processing the position of the receiver please see the GNSS Configuration Menu. For more information on how to use a RAWX GNSS log to get a higher accuracy base location please see the [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data) tutorial.
