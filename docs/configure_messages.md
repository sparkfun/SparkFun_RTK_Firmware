# Messages Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

[![Message rate configuration boxes](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_GNSS_Config_Messages.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_GNSS_Config_Messages.jpg)

*Message rate configuration from WiFi AP Config*

[![The Messages configuration menu](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu.jpg)

*The messages configuration menu*

From this menu, a user can control the output of various NMEA, RTCM, RXM, and other messages. Any enabled message will be broadcast over Bluetooth *and* recorded to SD (if available).

Because of the large number of configurations possible, we provide a few common settings:

* Reset to Surveying Defaults (NMEAx5)
* Reset to PPP Logging Defaults (NMEAx5 + RXMx2)
* Turn off all messages (serial command only)
* Turn on all messages (serial command only)

## Reset to Surveying Defaults (NMEAx5)

This will turn off all messages and enable the following messages:

* NMEA-GGA, NMEA-SGA, NMEA-GST, NMEA-GSV, NMEA-RMC

These five NMEA sentences are commonly used with SW Maps for general surveying.

## Reset to PPP Logging Defaults (NMEAx5 + RXMx2)

This will turn off all messages and enable the following messages:

* NMEA-GGA, NMEA-SGA, NMEA-GST, NMEA-GSV, NMEA-RMC, RXM-RAWX, RXM-SFRBX

These seven sentences are commonly used when logging and doing Precise Point Positioning (PPP) or Post Processed Kinematics (PPK). You can read more about PPP [here](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station).

## Individual Messages

[![Configuring the NMEA messages](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu_-_NMEA.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Messages_Menu_-_NMEA.jpg)

*Configuring the NMEA messages*

There are a large number of messages supported (listed below). Each message sub-menu will present the user with the ability to set the message report rate.

Each message rate input controls which messages are disabled (0) and how often the message is reported (1 = one message reported per 1 fix, 5 = one report every 5 fixes). The message rate range is 0 to 20.

**Note:** The message report rate is the *number of fixes* between message reports. In the image above, with GSV set to 4, the NMEA GSV message will be produced once every 4 fixes. Because the device defaults to a 4Hz fix rate, the GSV message will appear once per second.

The following 67 messages are supported for Bluetooth output and logging:

<table class="table">
 <table>
  <COLGROUP><COL WIDTH=200><COL WIDTH=200><COL WIDTH=200></COLGROUP>
  <tr>
	<td>&#8226; NMEA-GSA</td>
	<td>&#8226; NMEA-GST</td>
	<td>&#8226; NMEA-GSV</td>
  </tr>
  <tr>
	<td>&#8226; NMEA-RMC</td>
	<td>&#8226; NMEA-VLW</td>
	<td>&#8226; NMEA-VTG</td>
  </tr>
  <tr>
    <td>&#8226; NMEA-ZDA</td>
    <td>&#8226; NAV-CLOCK</td>
    <td>&#8226; NAV-DOP</td>
  </tr>
  <tr>
    <td>&#8226; NAV-EOE</td>
    <td>&#8226; NAV-GEOFENCE</td>
    <td>&#8226; NAV-HPPOSECEF</td>
  </tr>
  <tr>
    <td>&#8226; NAV-HPPOSLLH</td>
    <td>&#8226; NAV-ODO</td>
    <td>&#8226; NAV-ORB</td>
  </tr>
  <tr>
    <td>&#8226; NAV-POSECEF</td>
    <td>&#8226; NAV-POSLLH</td>
    <td>&#8226; NAV-PVT</td>
  </tr>
  <tr>
    <td>&#8226; NAV-RELPOSNED</td>
    <td>&#8226; NAV-SAT</td>
    <td>&#8226; NAV-SIG</td>
  </tr>
  <tr>
    <td>&#8226; NAV-STATUS</td>
    <td>&#8226; NAV-SVIN</td>
    <td>&#8226; NAV-TIMEBDS</td>
  </tr>
  <tr>
    <td>&#8226; NAV-TIMEGAL</td>
    <td>&#8226; NAV-TIMEGLO</td>
    <td>&#8226; NAV-TIMEGPS</td>
  </tr>
  <tr>
    <td>&#8226; NAV-TIMELS</td>
    <td>&#8226; NAV-TIMEUTC</td>
    <td>&#8226; NAV-VELECEF</td>
  </tr>
  <tr>
    <td>&#8226; NAV-VELNED</td>
    <td>&#8226; RXM-MEASX</td>
    <td>&#8226; RXM-RAWX</td>
  </tr>
  <tr>
    <td>&#8226; RXM-RLM</td>
    <td>&#8226; RXM-RTCM</td>
    <td>&#8226; RXM-SFRBX</td>
  </tr>
  <tr>
    <td>&#8226; MON-COMMS</td>
    <td>&#8226; MON-HW2</td>
    <td>&#8226; MON-HW3</td>
  </tr>
  <tr>
    <td>&#8226; MON-HW</td>
    <td>&#8226; MON-IO</td>
    <td>&#8226; MON-MSGPP</td>
  </tr>
  <tr>
    <td>&#8226; MON-RF</td>
    <td>&#8226; MON-RXBUF</td>
    <td>&#8226; MON-RXR</td>
  </tr>
  <tr>
    <td>&#8226; MON-TXBUF</td>
    <td>&#8226; TIM-TM2</td>
    <td>&#8226; TIM-TP</td>
  </tr>
  <tr>
    <td>&#8226; TIM-VRFY</td>
    <td>&#8226; RTCM3x-1005</td>
    <td>&#8226; RTCM3x-1074</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-1077</td>
    <td>&#8226; RTCM3x-1084</td>
    <td>&#8226; RTCM3x-1087</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-1094</td>
    <td>&#8226; RTCM3x-1097</td>
    <td>&#8226; RTCM3x-1124</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-1127</td>
    <td>&#8226; RTCM3x-1230</td>
    <td>&#8226; RTCM3x-4072-0</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-4072-1</td>
    <td></td>
    <td></td>
  </tr>
</table></table>

## Turn off all messages

This will turn off all messages. This is handy for advanced users who need to start from a blank slate. This setting is only available over serial configuration.

## Turn on all messages

This will turn on all messages. This is a setting used for firmware testing and should not be needed in normal use. This setting is only available over serial configuration.

## ESF Messages

The ZED-F9R module, found only on the Express Plus, supports additional External Sensor Fusion messages. These messages show the raw accelerometer and gyroscope values of the internal IMU. These messages can consume up to 34,000 bytes of bandwidth. Please see [here](https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/81#issuecomment-1059338377) for more information.