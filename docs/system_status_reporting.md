# System Reporting

Surveyor: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express Plus: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Facet: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png)

Sending the `~` character to the device over the serial port will trigger a system status report. This is a custom NMEA style sentence, complete with CRC.

![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/SparkFun RTK System Status Trigger.png)

*Terminal showing System Status*

> $GNTXT,01,01,05,202447.00,270522,0.380,29,40.090355193,-105.184764700,1560.56,3,0,86*71
* $GNTXT : Start of custom NMEA sentence
* 01 : Number of sentences
* 01 : Sentence number
* 05 : Sentence type ID (5 is for System Status messages)
* 202447.00 : Current hour, minute, second, milliseconds
* 270522 : Current day, month, year
* 0.380 : Current horizontal positional accuracy (m)
* 29 : Satellites in view
* 40.090355193 : Latitude
* -105.184764700 : Longitude
* 1560.56 : Altitude
* 3 : Fix type (0 = no fix, 2 = 2D fix, 3 = 3D fix, 4 = 3D + Dead Reackoning, 5 = Time)
* 0 : Carrier solution (0 = No RTK, 1 = RTK Float, 2 = RTK Fix)
* 86 : Battery level (% remaining)