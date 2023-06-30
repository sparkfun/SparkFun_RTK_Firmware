# Quick Start

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

This quick start guide will get you started in 10 minutes or less. For the full product manual, please proceed to the [**Introduction**](index.md).

Are you using [Android](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#android) or [iOS](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#ios)?

## Android

![Download SW Maps](<img/SWMaps/SparkFun RTK SW Maps for Android QR Code.png>)

*Download SW Maps for Android*

1. Download [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps). This may not be the GIS software you intend to do your data collection, but SW Maps is free and makes sure everything is working correctly out of the box.

    ![RTK Boot Display](<img/Displays/SparkFun RTK Facet Boot Display.png>)

2. Turn on your RTK device by pressing the POWER button until the display shows ‘SparkFun RTK’ then you can release it. 

    ![Display showing MAC address](<img/Displays/SparkFun RTK Rover Display.png>)

3. Please note the four-digit code in the top left corner of the display (**B022** in the above example). This is the MAC address you will want to pair with.

    ![List of Bluetooth devices on Android](<img/Bluetooth/SparkFun RTK Bluetooth List Connect.png>)

4. From your cell phone, open Bluetooth settings and pair with a new device. You will see a list of available Bluetooth devices. Select the ‘Facet Rover-3AF1’ where 'Facet' is the type of device you have (Surveyor, Express, Facet, etc) and 3AF1 is the MAC address you see on the device’s display.

5. Once paired, open SW Maps. Select ‘New Project’ and give your project a name like ‘RTK Project’. 

    ![SW Map list of Bluetooth devices](<img/SWMaps/SparkFun RTK SWMaps Bluetooth Connect.png>)

6. Press the SW Maps icon in the top left corner and select Bluetooth GNSS. You should see the ‘Facet Rover-3AF1’ in the list. Select it then press the ‘Connect’ button in the bottom left corner. SW Maps will show a warning that the instrument height is 0m. That’s ok. 

    ![Display showing Bluetooth connection](<img/Displays/SparkFun RTK Display - Bluetooth.png>)

7. Once connected, have a look at the display on the RTK device. You should see the MAC address disappear and be replaced by the Bluetooth icon. You’re connected!

    ![RTK Display Explanation](img/Displays/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)

8. Now put the device outside with a clear view of the sky. GNSS doesn’t work indoors or near windows. Within about 30 seconds you should see 10 or more satellites in view (SIV). More SIV is better. We regularly see 30 or more SIV. The horizontal positional accuracy (HPA) will start at around 10 meters and begin to decrease. The lower the HPA the more accurate your position. If you wait a few moments, this will drop dramatically to around 0.3 meters (300mm = 1ft) or better. 

Now you use your RTK device to begin measuring points. If you need extreme accuracy (down to 10mm) continue reading the [RTK Crash Course](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#rtk-crash-course).

## iOS

The software options for Apple iOS are much more limited because Apple products do not support Bluetooth SPP. That's ok! The SparkFun RTK products support Bluetooth Low Energy (BLE) which *does* work with iOS.

![SW Maps for Apple](<img/SWMaps/SparkFun RTK SW Maps for Apple QR Code.png>)

*Download SW Maps for iOS*

1. Download [SW Maps for iOS](https://apps.apple.com/us/app/sw-maps/id6444248083). This may not be the GIS software you intend to do your data collection, but SW Maps is free and makes sure everything is working correctly out of the box.

    ![RTK Facet Boot Display](<img/Displays/SparkFun RTK Facet Boot Display.png>)

2. Turn on your RTK device by pressing the POWER button until the display shows ‘SparkFun RTK' then you can release it. 

    ![Config menu highlighted on display](<img/Displays/SparkFun RTK Config Display.png>)

3. Put the RTK device into configuration mode by pressing the POWER or SETUP button until the Config menu is highlighted.

4. From your phone, connect to the WiFi network *RTK Config*.

5. Open a browser (Chrome is preferred) and type **192.168.4.1** into the address bar.

    ![Configure Bluetooth Protocol in WiFi Config](<img/WiFi Config/SparkFun RTK Change Bluetooth to BLE.png>)

6. Under the *System Configuration* menu, change the **Bluetooth Protocol** to **BLE**. Then click *Save* and then *Exit*. The unit will now reboot.

    ![iOS Bluetooth settings](<img/iOS/SparkFun RTK iOS Bluetooth Devices.png>)

7. From your cell phone, disconnect from the *RTK Config* WiFi network and make sure Bluetooth is enabled on your iOS device Settings. The RTK device will not appear in the OTHER DEVICES list. That is OK.

8. Open SW Maps. Select ‘New Project’ and give your project a name like ‘RTK Project’. 

9. Press the SW Maps icon in the top left corner and select Bluetooth GNSS. Set the *Instrument Model* to **Generic NMEA (Bluetooth LE)**. Press 'Scan' and your RTK device should appear. Select it then press the ‘Connect’ button in the bottom left corner. SW Maps will show a warning that the instrument height is 0m. That’s ok.

    ![Display showing Bluetooth connection](<img/Displays/SparkFun RTK Display - Bluetooth.png>)

10. Once connected, have a look at the display on the RTK device. You should see the MAC address disappear and be replaced by the Bluetooth icon. You’re connected!

    ![RTK Display Explanation](img/Displays/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)

11. Now put the device outside with a clear view of the sky. GNSS doesn’t work indoors or near windows. Within about 30 seconds you should see 10 or more satellites in view (SIV). More SIV is better. We regularly see 30 or more SIV. The horizontal positional accuracy (HPA) will start at around 10 meters and begin to decrease. The lower the HPA the more accurate your position. If you wait a few moments, this will drop dramatically to around 0.3 meters (300mm = 1ft) or better. 

Now you use your RTK device to begin measuring points. If you need extreme accuracy (down to 10mm) continue reading the [RTK Crash Course](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#rtk-crash-course).

## RTK Crash Course

To get millimeter accuracy we need to provide the RTK unit with correction values. Corrections, often called RTCM, help the RTK unit refine its position calculations. Corrections can be obtained from a variety of sources but they fall into three buckets: Paid, Free by the Government, and Free by Me.

**Paid Services**

These services cover entire countries and regions but charge a monthly fee. Easy to use, but the most expensive.

* [PointOneNav](https://app.pointonenav.com/trial?src=sparkfun) ($50/month) - US, EU
* [Skylark](https://www.swiftnav.com/skylark) ($29 to $69/month) - US, EU, Japan, Australia
* [Vector RTK](https://vectorrtk.com/) ($115/month) - UK
* [SensorCloud RTK](https://rtk.sensorcloud.com/pricing/) ($100/month) partial US, EU
* [Hexagon/Leica](https://hxgnsmartnet.com/en-US) ($500/month) - partial US, EU

**Government Provided Corrections**

![Wisconsin network of CORS](<img/Corrections/SparkFun NTRIP 7 - Wisconsin Map.png>) 

*State Wide Network of Continuously Operating Reference Stations (CORS)*

Be sure to check if your state or country provides corrections for free. Many do! Currently, there are 21 states in the USA that provide this for free as a department of transportation service. Search ‘Wisconsin CORS’ as an example. Similarly, in France, check out [CentipedeRTK](https://docs.centipede.fr/). There are several public networks across the globe, be sure to google around!

**Civilian Base Station**

![SparkFun Base Station Enclosure](img/Corrections/Roof_Enclosure.jpg)

*The base station at SparkFun*

You can set up your own correction source. This is done with a 2nd GNSS receiver that is stationary, often called a Base Station. There is just the one-time upfront cost of the Base Station hardware. See the [Creating a Permanent Base](https://docs.sparkfun.com/SparkFun_RTK_Firmware/permanent_base/) document for more information.

## NTRIP Example

Once you have decided on a correction source we need to feed that data into your SparkFun RTK device. In this example, we will use PointOneNav and SW Maps.

1. Create an account on [PointOneNav](https://app.pointonenav.com/trial?src=sparkfun).

2. Open SW Maps and connect to the RTK device over Bluetooth.

3. Once connected, open the SW Maps menu again (top left corner) and you will see a new option; click on ‘NTRIP Connection’.

    ![NTRIP credentials in SW Maps](<img/SWMaps/SparkFun RTK SW Maps - NTRIP Credentials.png>)

4. Enter the credentials provided by PointOneNav and click Connect.

5. Corrections will be downloaded every second from PointOneNav using your phone’s cellular connection and then sent down to the RTK device over Bluetooth. You don't need a very fast internet connection or a lot of data; it's only about 530 bytes per second.

![Double crosshair indicating RTK Fix](<img/Displays/SparkFun RTK Display - Double Crosshair.png>)

As soon as corrections are sent to the device, the Crosshair icon will become double and begin flashing. Once RTK Fix is achieved (usually under 30 seconds) the double crosshairs will become solid and the HPA will be below 20mm. You can now take positional readings with millimeter accuracy!

![Green bubble indicating RTK Fix](<img/SWMaps/SparkFun RTK SW Maps - Green Bubble-1.png>)

In SW Maps, the position bubble will turn from Blue (regular GNSS fix), to Orange (RTK Float), to Green (RTK Fix).

RTK Fix will be maintained as long as there is a clear view of the sky and corrections are delivered to the device every few seconds.

## Common Gotchas

* High-precision GNSS only works with dual frequency L1/L2 antennas. This means that GPS antenna you got in the early 2000s with your TomTom is not going to work. Please use the L1/L2 antennas provided by SparkFun.

* High-precision GNSS works best with a clear view of the sky; it does not work indoors or near a window. GNSS performance is generally *not* affected by clouds or storms. Trees and buildings *can* degrade performance but usually only in very thick canopies or very near tall building walls. GNSS reception is very possible in dense urban centers with skyscrapers but high-precision RTK may be impossible.

* The location reported by the RTK device is the location of the antenna element; it's *not* the location of the pointy end of the stick. Lat and Long are fairly easy to obtain but if you're capturing altitude be sure to do additional reading on ARPs (antenna reference points) and how to account for the antenna height.

* An internet connection is required for most types of RTK. RTCM corrections can be transmitted over other types of connections (such as serial telemetry radios). See [Correction Transport](correction_transport.md) for more details.

## RTK Reference Station

While most of this Quick Start guide applies to the [RTK Reference Station](https://www.sparkfun.com/products/22429), the [Reference Station Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-reference-station-hookup-guide) is the best place to get started.