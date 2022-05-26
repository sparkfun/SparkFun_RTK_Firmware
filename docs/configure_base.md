# Base Menu

Surveyor: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express Plus: ![Feature Not Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/RedDot.png) / Facet: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png)

In addition to providing accurate local location fixes, the SparkFun RTK devices can also serve as a correction source, also called a *Base*. The Base doesn't move and 'knows' where it is so it can calculate the discrepancies between the signals it is receiving and what it should be receiving. These differences are the correction values passed to the Rover so that the Rover can have millimeter level accuracy.

There are two types of bases: *Surveyed* and *Fixed*. A surveyed base is often a temporary base setup in the field. Called a 'Survey-In', this is less accurate but requires only 60 seconds to complete. The 'Fixed' base is much more accurate but the precise location at which the antenna is located must be known. A fixed base is often a structure with an antenna bolted to the side. Raw satellite signals are gathered for a few hours then processed using Precision Point Position. We have a variety of tutorials that go into depth on these subjects but all you need to know is that the RTK Facet supports both Survey-In and Fixed Base techniques.

Please see the following tutorials for more information:

<!-- tutorials_by_id(813, 815, 1362, 1363) -->

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://learn.sparkfun.com/tutorials/what-is-gps-rtk"><img src="https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/8/1/3/Location-Wandering-GPS-combined.jpg"></a></td>
   <td><a href="https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox"><img src="https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/8/1/5/u-center.jpg"></a></td>
   <td><a href="https://learn.sparkfun.com/tutorials/setting-up-a-rover-base-rtk-system"><img src="https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/1/3/6/2/GNSS_RTK_DIY_Surveying_Tutorial.jpg"></a></td>
   <td><a href="https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station"><img src="https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/1/3/6/3/Roof_Enclosure.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://learn.sparkfun.com/tutorials/what-is-gps-rtk">What is GPS RTK?</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox">Getting Started with u-center for u-blox</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/setting-up-a-rover-base-rtk-system">Setting up a Rover Base RTK System</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station">SparkFun RTK Facet (GPS-19029)</a></td>
  </tr>
</table>


The Base Menu allows the user to select between Survey-In or Fixed Base setups.

[![Dos CMD window showing Base menu optionns](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Base_Menu.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Base_Menu.jpg)

## Mode

In **Survey-In** mode, the minimum observation time and Mean 3D Standard Deviation can be set. The defaults are 60s and 5m as directed by u-blox. The device will wait for the position accuracy to be better than 1 meter before a Survey-In is started. Don't be fooled; setting the observation time to 4 hours or an initial positional accuracy of 0.3m is not going to significantly improve the accuracy of the survey - use [PPP](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data) instead.

In **Fixed** mode, the coordinates of the antenna need to be sent. These can be entered in ECEF or Geographic coordinates. Whenever a user enters Base mode by pressing the SETUP button the GNSS receiver will immediately go into base mode with these coordinates and immediately begin outputting RTCM correction data.

## NTRIP Server

**NTRIP** is where the real fun begins. The Base needs a method for getting the correction data to the Rover. This can be done using radios but that's limited to a few kilometers at best. If you've got WiFi reception, use the internet! 

Enabling NTRIP will present a handful of new options seen below:

[![SparkFun RTK Facet NTRIP Settings](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Base_Menu_-_Fixed_NTRIP.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Base_Menu_-_Fixed_NTRIP.jpg)

*Settings for the NTRIP Server*

This is a powerful feature of the RTK line of products. The RTK device can be configured to transmit its RTCM directly over WiFi to the user's mountpoint. This eliminates the need for a radio link.

Once the NTRIP server is enabled you will need a handful of credentials:

* Local WiFi SSID and password
* A casting service such as [RTK2Go](http://www.rtk2go.com) or [Emlid](http://caster.emlid.com) (the port is almost always 2101)
* A mount point and password

[![NTRIP Server Connected](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Device_Configuration_-_NTRIP_Server_Broadcasting_v11.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Device_Configuration_-_NTRIP_Server_Broadcasting_v11.jpg)

*NTRIP Server Connected!*

With these credentials set, RTK device will attempt to connect to WiFi, your caster of choice, and begin transmitting the RTCM data over WiFi. We tried to make it as easy as possible.

[![Transmitting to mount point](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Device_Configuration_-_NTRIP_Server_Broadcasting_Bytes_v11.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Device_Configuration_-_NTRIP_Server_Broadcasting_Bytes_v11.jpg)

Every second a few hundred bytes, up to ~2k, will be transmitted to your mount point.
