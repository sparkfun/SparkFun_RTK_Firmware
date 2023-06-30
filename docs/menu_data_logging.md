# Data Logging Menu

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

## WiFi Interface

Please see the System Menu [WiFi Interface](menu_system.md#wifi-interface).

## Serial Interface

![RTK Data Logging Configuration Menu](img/Terminal/SparkFun%20RTK%20Logging%20Menu.png)

*RTK Data Logging Configuration Menu*

From the Main Menu, pressing 5 will enter the Logging Menu. This menu will report the status of the microSD card. While you can enable logging, you cannot begin logging until a microSD card is inserted. Any FAT16 or FAT32 formatted microSD card up to 128GB will work. We regularly use the [SparkX brand 1GB cards](https://www.sparkfun.com/products/15107) but note that these log files can get very large (>500MB) so plan accordingly.

* Option 1 will enable/disable logging. If logging is enabled, all messages from the ZED-F9P will be recorded to microSD. A log file is created at power on with the format *SFE_[DeviceName]_YYMMDD_HHMMSS.txt* based on current GPS data/time. The `[DeviceName]` is Surveyor, Express, etc.
* Option 2 allows a user to set the max logging time. This is convenient to determine the location of a fixed antenna or a receiver on a repeatable landmark. Set the RTK Facet to log RAWX data for 10 hours, convert to RINEX, run through an observation processing station and you’ll get the corrected position with <10mm accuracy. Please see the [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station) tutorial for more information.
* Option 3 allows a user to set the max logging length in minutes. Every 'max long length' amount of time the current log will be closed and a new log will be started. This is known as cyclic logging and is convenient on *very* long surveys (ie, months or years) to prevent logs from getting too unwieldy and helps limit the risk of log corruption. This will continue until the unit is powered down or the *max logging time* is reached.
* Option 4 will close the current log and start a new log.

* Option 5 will record the coordinates of the base antenna to a custom NMEA message within the log if the RTCM1005 or RTCM1006 message is received. This can be helpful when doing field work and the location of the base is needed; the log on the roving device will contain the location of the base preventing the user from needing to record the base location separately. The ARP is logged in a custom GNTXT,01,01,10 message as ECEF-X, ECEF-Y, ECEF-Z, Antenna Height. The Antenna Height will be zero if the data was extracted from RTCM1005.

* Option 7 will enable/disable creating a comma separated file (Marks_date.csv) that is written each time the mark state is selected with the setup button on the RTK Surveyor, RTK Express or RTK Express Plus, or the power button on the RTK Facet.

* Option 8 will enable/disable the resetting of the system if an SD card is detected but fails to initialize. This can be helpful to harden a system that may be deployed for long periods of time. Without intervention, if an SD card is detected but fails to respond, the system will reset in an attempt to re-mount the faulty SD card interface.

**Note:** If you are wanting to log RAWX sentences to create RINEX files useful for post-processing the position of the receiver please see the GNSS Configuration Menu. For more information on how to use a RAWX GNSS log to get a higher accuracy base location please see the [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data) tutorial.
