# Messages Menu

[![The Messages configuration menu](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu.jpg)

*The messages configuration menu*

From this menu a user can control the output of various NMEA, RTCM, RXM, and other messages. Any enabled message will be broadcast over Bluetooth *and* recorded to SD (if available).

Because of the large number of configurations possible, we provide a few common settings:

* Reset to Surveying Defaults (NMEAx5)
* Reset to PPP Logging Defaults (NMEAx5 + RXMx2)
* Turn off all messages
* Turn on all messages

**Reset to Surveying Defaults (NMEAx5)** will turn off all messages and enable the following messages:

* NMEA-GGA, NMEA-SGA, NMEA-GST, NMEA-GSV, NMEA-RMC

These five NMEA sentences are commonly used with SW Maps for general surveying.

**Reset to PPP Logging Defaults (NMEAx5 + RXMx2)** will turn off all messages and enable the following messages:

* NMEA-GGA, NMEA-SGA, NMEA-GST, NMEA-GSV, NMEA-RMC, RXM-RAWX, RXM-SFRBX

These seven sentences are commonly used when logging and doing Precise Point Positioning (PPP) or Post Processed Kinematics (PPK). You can read more about PPP [here](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station).

**Turn off all messages** will turn off all messages. This is handy for advanced users who need to start from a blank slate.

**Turn on all messages** will turn on all messages. This is a setting used for firmware testing and should not be needed in normal use.

[![Configuring the NMEA messages](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu_-_NMEA.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu_-_NMEA.jpg)

*Configuring the NMEA messages*

As mentioned is the microSD section of the [Hardware Overview](https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-hookup-guide/all#hardware-overview) there are a large number of messages supported. Each message sub menu will present the user with the ability to set the message report rate.

Note: The message report rate is the *number of fixes* between message reports. In the image above, with GSV set to 4, the NMEA GSV message will be produced once every 4 fixes. Because the device defaults to 4Hz fix rate, the GSV message will appear once per second.