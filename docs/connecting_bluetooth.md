# Connecting Bluetooth

Surveyor: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express Plus: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Facet: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png)

The RTK products transmit full NMEA sentences over Bluetooth serial port profile (SPP) at 4Hz and 115200bps. This means that nearly any GIS application that can receive NMEA data over serial port (almost all do) can be used with the RTK Express. As long as your device can open a serial port over Bluetooth (also known as SPP) your device can retrieve industry standard NMEA positional data. The following steps show how to use SW Maps but the same steps can be followed to connect any serial port based GIS application.

## SW Maps

The best mobile app that we’ve found is the powerful, free, and easy to use *[SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps)* by Softwel. You’ll need an Android phone or tablet with Bluetooth. What makes SW Maps truly powerful is its built-in NTRIP client. This is a fancy way of saying that we’ll be showing you how to get RTCM correction data over the cellular network. 

![SW Maps with RTK Fix](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/SparkFun RTK SWMaps GNSS Status.png)

*SW Maps with RTK Fix*

When powered on, the RTK product will broadcast itself as either '[Platform] Rover-5556' or '[Platform] Base-5556' depending on which state it is in. [Platform] is Facet, Express, Surveyor, etc. Discover and pair with this device from your phone or tablet. Once paired, open SW Maps. 

[![alt text](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_Fixed.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_Fixed.jpg)

*MAC address 5522 is shown in the upper left corner*

**Note:** *5522* is the last four digits of your unit's MAC address and will be unique to the device in front of you. This is helpful in case there are multiple RTK devices within Bluetooth range.

[![Pairing with the RTK Express over Bluetooth](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/8/5/7/RTK_Express_-_Bluetooth_Connect.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/RTK_Express_-_Bluetooth_Connect.jpg)

*Pairing with the 'Express Rover-5556' over Bluetooth*

Open Android's system settings and find the 'Bluetooth' or 'Connected devices' options. Scan for devices and pair with the device in the list that matches the Bluetooth MAC address on your RTK device.

![List of BT Devices in SW Maps](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/SparkFun RTK SWMaps Bluetooth Connect.png)

*List of available Bluetooth devices*

From SW Map's main menu, select *Bluetooth GNSS*. This will display a list of available Bluetooth devices. Select the Rover or Base you just paired with. If your are taking height measurements (altitude) in addition to position (lat/long) be sure to enter the height of your antenna off the ground including any [ARP offsets](https://geodesy.noaa.gov/ANTCAL/FAQ.xhtml#faq4) of your antenna (should be printed on the side).

Click on 'CONNECT' to open a Bluetooth connection. Assuming this process takes a few seconds, you should immediately have a location fix.

## NTRIP Client

If you’re using a serial radio for your correction data, you can skip this part.

Next we need to send RTCM correction data from the phone back to the RTK device so that it can improve its fix accuracy. This is the amazing power of the SparkFun RTK products and SW Maps. Your phone can be the radio link! From the main SW Maps menu select NTRIP Client. Not there? Be sure the 'SparkFun RTK' instrument was automatically selected connecting. Disconnect and change the instrument to 'SparkFun RTK' to enable the NTRIP Connection option.

[![SW Maps NTRIP Connection menu](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_SW_Maps_NTRIP_Connection.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_SW_Maps_NTRIP_Connection.jpg)

*NTRIP Connection - Not there? Be sure to select 'SparkFun RTK' was selected as the instrument*

Enter your NTRIP caster credentials and click connect. You will see bytes begin to transfer from your phone to the RTK Express. Within a few seconds the RTK Express will go from ~300mm accuracy to 14mm. Pretty nifty, no?

[![SW Maps NTRIP client](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SW_Maps_-_NTRIP_Client.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SW_Maps_-_NTRIP_Client.jpg)

*Connecting to an NTRIP Caster*

What's an NTRIP caster? In a nut shell it's a server that is sending out correction data every second. There are thousands of sites around the globe that calculate the perturbations in the ionosphere and troposphere that decrease the accuracy of GNSS accuracy. Once the inaccuracies are known, correction values are encoded into data packets in the RTCM format. You, the user, don't need to know how to decode or deal with RTCM, you simply need to get RTCM from a source within 10km of your location into the RTK Express. The NTRIP client logs into the server (also known as the NTRIP caster) and grabs that data, every second, and sends it over Bluetooth to the RTK Express.

Don't have access to an NTRIP Caster? You can use a 2nd RTK product in operating in Base mode to provide the correction data. Checkout [Creating a Permanent Base](https://sparkfun.github.io/SparkFun_RTK_Firmware/permanent_base/). If you're the DIY sort, you can create your own low cost base station using an ESP32 and a ZED-F9P breakout board. Checkout [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station). If you'd just like a service, [Syklark](https://www.swiftnav.com/skylark) provides RTCM coverage for $49 a month (as of writing) and is extremely easy to setup and use. Remember, you can always use a 2nd RTK device in *Base* mode to provide RTCM correction data but it will less accurate than a fixed position caster.

Once you have a full RTK fix you'll notice the location bubble in SW Maps turns to green. Just for fun, rock your rover monopole back and forth on a fixed point. You'll see your location accurately reflected in SW Maps. Millimeter location precision is a truly staggering thing.