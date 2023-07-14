# Quick Start

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

This quick start guide will get you started in 10 minutes or less. For the full product manual, please proceed to the [**Introduction**](index.md).

Are you using [Android](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#android) or [iOS](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#ios)?

## Android

1. Download [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps). This may not be the GIS software you intend to do your data collection, but SW Maps is free and makes sure everything is working correctly out of the box.

    ![Download SW Maps](<img/SWMaps/SparkFun RTK SW Maps for Android QR Code.png>)

    *Download SW Maps for Android*

2. Mount the hardware:

    * For RTK Surveyor/Express/Express Plus: Attach the included [antenna](https://www.sparkfun.com/products/17751) to a [monopole](https://www.amazon.com/AmazonBasics-WT1003-67-Inch-Monopod/dp/B00FAYL1YU) using the included [thread adapter](https://www.sparkfun.com/products/17546). [Clamp](https://www.amazon.com/gp/product/B072DSRF3J) the RTK device to the monopole. Use the included [cable](https://www.sparkfun.com/products/21739) to connect the antenna to the RTK Surveyor/Express/Express Plus (Figure 1).
    * For RTK Facet/Facet L-Band: Attach the Facet to a [monopole](https://www.amazon.com/AmazonBasics-WT1003-67-Inch-Monopod/dp/B00FAYL1YU) using the included [thread adapter](https://www.sparkfun.com/products/17546) (Figure 1).

    ![RTK devices attached to a monopole](<img/SparkFun RTK Device Attached to Monopole.png>)

    *Figure 1*

3. Turn on your RTK device by pressing the POWER button until the display shows ‘SparkFun RTK’ then you can release it (Figure 2). 

    ![RTK Boot Display](<img/Displays/SparkFun RTK Facet Boot Display.png>)

    *Figure 2*


4. Please note the four-digit code in the top left corner of the display (**B022** in the picture below). This is the MAC address you will want to pair with (Figure 3).

    ![Display showing MAC address](<img/Displays/SparkFun RTK Rover Display.png>)

    *Figure 3*

5. From your cell phone, open Bluetooth settings and pair it with a new device. You will see a list of available Bluetooth devices. Select the ‘Facet Rover-3AF1’ where 'Facet' is the type of device you have (Surveyor, Express, Facet, etc) and 3AF1 is the MAC address you see on the device’s display (Figure 4).

    ![List of Bluetooth devices on Android](<img/Bluetooth/SparkFun RTK Bluetooth List Connect.png>)

    *Figure 4*

6. Once paired, open SW Maps. Select ‘New Project’ and give your project a name like ‘RTK Project’. 

7. Press the SW Maps icon in the top left corner of the home screen and select Bluetooth GNSS. You should see the ‘Facet Rover-3AF1’ in the list. Select it then press the ‘Connect’ button in the bottom left corner (Figure 5). SW Maps will show a warning that the instrument height is 0m. That’s ok. 

    ![SW Map list of Bluetooth devices](<img/SWMaps/SparkFun RTK SWMaps Bluetooth Connect.png>)

    *Figure 5*

8. Once connected, have a look at the display on the RTK device. You should see the MAC address disappear and be replaced by the Bluetooth icon (Figure 6). You’re connected!

    ![Display showing Bluetooth connection](<img/Displays/SparkFun RTK Display - Bluetooth.png>)

    *Figure 6*

9. Now put the device outside with a clear view of the sky. GNSS doesn’t work indoors or near windows. Within about 30 seconds you should see 10 or more satellites in view (SIV) (Figure 7). More SIV is better. We regularly see 30 or more SIV. The horizontal positional accuracy (HPA) will start at around 10 meters and begin to decrease. The lower the HPA the more accurate your position. If you wait a few moments, this will drop dramatically to around 0.3 meters (300mm = 1ft) or better. 

    ![RTK Display Explanation](img/Displays/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)

    *Figure 7*

You can now use your RTK device to measure points with very good (sub-meter) accuracy. If you need extreme accuracy (down to 10mm) continue reading the [RTK Crash Course](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#rtk-crash-course).

## iOS

The software options for Apple iOS are much more limited because Apple products do not support Bluetooth SPP. That's ok! The SparkFun RTK products support Bluetooth Low Energy (BLE) which *does* work with iOS.

1. Download [SW Maps for iOS](https://apps.apple.com/us/app/sw-maps/id6444248083). This may not be the GIS software you intend to do your data collection, but SW Maps is free and makes sure everything is working correctly out of the box.

    ![SW Maps for Apple](<img/SWMaps/SparkFun RTK SW Maps for Apple QR Code.png>)

    *Download SW Maps for iOS*

2. Mount the hardware:

    * For RTK Surveyor/Express/Express Plus: Attach the included [antenna](https://www.sparkfun.com/products/17751) to a [monopole](https://www.amazon.com/AmazonBasics-WT1003-67-Inch-Monopod/dp/B00FAYL1YU) using the included [thread adapter](https://www.sparkfun.com/products/17546). [Clamp](https://www.amazon.com/gp/product/B072DSRF3J) the RTK device to the monopole. Use the included [cable](https://www.sparkfun.com/products/21739) to connect the antenna to the RTK Surveyor/Express/Express Plus (Figure 1).
    * For RTK Facet/Facet L-Band: Attach the Facet to a [monopole](https://www.amazon.com/AmazonBasics-WT1003-67-Inch-Monopod/dp/B00FAYL1YU) using the included [thread adapter](https://www.sparkfun.com/products/17546) (Figure 1).

    ![RTK devices attached to a monopole](<img/SparkFun RTK Device Attached to Monopole.png>)

    *Figure 1*

3. Turn on your RTK device by pressing the POWER button until the display shows ‘SparkFun RTK' then you can release it (Figure 2). 

    ![RTK Facet Boot Display](<img/Displays/SparkFun RTK Facet Boot Display.png>)

    *Figure 2*

4. Put the RTK device into configuration mode by tapping the POWER or SETUP button multiple times until the Config menu is highlighted (Figure 3).

    ![Config menu highlighted on the display](<img/Displays/SparkFun RTK Config Display.png>)

    *Figure 3*

5. From your phone, connect to the WiFi network *RTK Config*.

6. Open a browser (Chrome is preferred) and type **rtk.local** into the address bar. Note: Devices with older firmware may still need to enter **192.168.4.1**.

7. Under the *System Configuration* menu, change the **Bluetooth Protocol** to **BLE** (Figure 4). Then click **Save Configuration** and then **Exit and Reset**. The unit will now reboot.

    ![Configure Bluetooth Protocol in WiFi Config](<img/WiFi Config/SparkFun RTK Change Bluetooth to BLE.png>)

    *Figure 4*

8. You should now be disconnected from the *RTK Config* WiFi network. Make sure Bluetooth is enabled on your iOS device Settings (Figure 5). The RTK device will not appear in the OTHER DEVICES list. That is OK.

    ![iOS Bluetooth settings](<img/iOS/SparkFun RTK iOS Bluetooth Devices.png>)

    *Figure 5*

9. Open SW Maps. Select ‘New Project’ and give your project a name like ‘RTK Project’. 

10. Press the SW Maps icon in the top left corner of the home screen and select Bluetooth GNSS. You will need to agree to allow a Bluetooth connection. Set the *Instrument Model* to **Generic NMEA (Bluetooth LE)**. Press 'Scan' and your RTK device should appear. Select it then press the ‘Connect’ button in the bottom left corner.

11. Once connected, have a look at the display on the RTK device. You should see the MAC address disappear and be replaced by the Bluetooth icon (Figure 6). You’re connected!

    ![Display showing Bluetooth connection](<img/Displays/SparkFun RTK Display - Bluetooth.png>)

    *Figure 6*

12. Now put the device outside with a clear view of the sky. GNSS doesn’t work indoors or near windows. Within about 30 seconds you should see 10 or more satellites in view (SIV) (Figure 7). More SIV is better. We regularly see 30 or more SIV. The horizontal positional accuracy (HPA) will start at around 10 meters and begin to decrease. The lower the HPA the more accurate your position. If you wait a few moments, this will drop dramatically to around 0.3 meters (300mm = 1ft) or better. 

    ![RTK Display Explanation](img/Displays/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)

    *Figure 7*

You can now use your RTK device to measure points with very good (sub-meter) accuracy. If you need extreme accuracy (down to 10mm) continue reading the [RTK Crash Course](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#rtk-crash-course).

## RTK Crash Course

To get millimeter accuracy we need to provide the RTK unit with correction values. Corrections, often called RTCM, help the RTK unit refine its position calculations. RTCM (Radio Technical Commission for Maritime Services) can be obtained from a variety of sources but they fall into three buckets: Commercial, Public, and Civilian Reference Stations.

**Commercial Reference Networks**

These companies set up a large number of reference stations that cover entire regions and countries, but charge a monthly fee. They are often easy to use but can be expensive.

* [PointOneNav](https://app.pointonenav.com/trial?src=sparkfun) ($50/month) - US, EU
* [Skylark](https://www.swiftnav.com/skylark) ($29 to $69/month) - US, EU, Japan, Australia
* [Vector RTK](https://vectorrtk.com/) ($115/month) - UK
* [SensorCloud RTK](https://rtk.sensorcloud.com/pricing/) ($100/month) partial US, EU
* [KeyNetGPS](https://www.keypre.com/KeynetGPS) ($375/month) North Eastern US
* [Hexagon/Leica](https://hxgnsmartnet.com/en-US) ($500/month) - partial US, EU

**Public Reference Stations**

![Wisconsin network of CORS](<img/Corrections/SparkFun NTRIP 7 - Wisconsin Map.png>) 

*State Wide Network of Continuously Operating Reference Stations (CORS)*

Be sure to check if your state or country provides corrections for free. Many do! Currently, there are 21 states in the USA that provide this for free as a department of transportation service. Search ‘Wisconsin CORS’ as an example. Similarly, in France, check out [CentipedeRTK](https://docs.centipede.fr/). There are several public networks across the globe, be sure to google around!

**Civilian Reference Stations**

![SparkFun Base Station Enclosure](img/Corrections/Roof_Enclosure.jpg)

*The base station at SparkFun*

You can set up your own correction source. This is done with a 2nd GNSS receiver that is stationary, often called a Base Station. There is just the one-time upfront cost of the Base Station hardware. See the [Creating a Permanent Base](https://docs.sparkfun.com/SparkFun_RTK_Firmware/permanent_base/) document for more information.

## NTRIP Example

Once you have decided on a correction source we need to feed that data into your SparkFun RTK device. In this example, we will use PointOneNav and SW Maps.

1. Create an account on [PointOneNav](https://app.pointonenav.com/trial?src=sparkfun). **Note:** This service costs $50 per month at the time of writing.

2. Open SW Maps and connect to the RTK device over Bluetooth.

3. Once connected, open the SW Maps menu again (top left corner) and you will see a new option; click on ‘NTRIP Client'.

4. Enter the credentials provided by PointOneNav and click Connect (Figure 1). Verify that *Send NMEA GGA* is checked.

    ![NTRIP credentials in SW Maps](<img/SWMaps/SparkFun RTK SW Maps - NTRIP Credentials.png>)

    *Figure 1*

5. Corrections will be downloaded every second from PointOneNav using your phone’s cellular connection and then sent down to the RTK device over Bluetooth. You don't need a very fast internet connection or a lot of data; it's only about 530 bytes per second.

Assuming you are outside, as soon as corrections are sent to the device, the Crosshair icon will become double and begin flashing. Once RTK Fix is achieved (usually under 30 seconds) the double crosshairs will become solid and the HPA will be below 20mm (Figure 2). You can now take positional readings with millimeter accuracy!

![Double crosshair indicating RTK Fix](<img/Displays/SparkFun RTK Display - Double Crosshair.png>)

*Figure 2*

In SW Maps, the position bubble will turn from Blue (regular GNSS fix), then to Orange (RTK Float), then to Green (RTK Fix) (Figure 3).

![Green bubble indicating RTK Fix](<img/SWMaps/SparkFun RTK SW Maps - Green Bubble-1.png>)

*Figure 3*

RTK Fix will be maintained as long as there is a clear view of the sky and corrections are delivered to the device every few seconds.

## Common Gotchas

* High-precision GNSS only works with dual frequency L1/L2 antennas. This means that GPS antenna you got in the early 2000s with your TomTom is not going to work. Please use the L1/L2 antennas provided by SparkFun.

* High-precision GNSS works best with a clear view of the sky; it does not work indoors or near a window. GNSS performance is generally *not* affected by clouds or storms. Trees and buildings *can* degrade performance but usually only in very thick canopies or very near tall building walls. GNSS reception is very possible in dense urban centers with skyscrapers but high-precision RTK may be impossible.

* The location reported by the RTK device is the location of the antenna element; it's *not* the location of the pointy end of the stick. Lat and Long are fairly easy to obtain but if you're capturing altitude be sure to do additional reading on ARPs (antenna reference points) and how to account for the antenna height in your data collection software.

* An internet connection is required for most types of RTK. RTCM corrections can be transmitted over other types of connections (such as serial telemetry radios). See [Correction Transport](correction_transport.md) for more details.

## RTK Facet L-Band Keys

The RTK Facet L-Band is unique in that it must obtain keys to decrypt the signal from a geosynchronous satellite. Here are the steps to do so:

1. Turn on your RTK Facet L-Band by pressing the POWER button until the display shows ‘SparkFun RTK' then you can release it (Figure 1). 

    ![RTK Boot Display](<img/Displays/SparkFun RTK Facet Boot Display.png>)

    *Figure 1*

2. Put the RTK device into configuration mode by tapping the POWER button multiple times until the Config menu is highlighted (Figure 2).

    ![Config menu highlighted on the display](<img/Displays/SparkFun RTK Config Display.png>)
    
    *Figure 2*

3. From your phone or laptop, connect to the WiFi network *RTK Config*.

4. Open a browser (Chrome is preferred) and type **rtk.local** into the address bar. Note: Devices with older firmware may still need to enter **192.168.4.1**.

5. Under the *WiFi Configuration* menu, enter the SSID and password for your local WiFi network (Figure 3). You can enter up to four. This can be a home,   office, cellular hotspot, or any other WiFi network. The unit will attempt to connect to the internet periodically to obtain new keys, including this first day. Then click **Save Configuration** and then **Exit and Reset**. The unit will now reboot.

    ![WiFi settings](<img/WiFi%20Config/SparkFun%20RTK%20AP%20WiFi%20Menu.png>)

    *Figure 3*

6. After reboot, the device will connect to WiFi and obtain keys. You should see a series of displays indicating the automatic process (Figure 4).

    ![Days until L-Band keys expire](<img/Displays/SparkFun_RTK_LBand_DayToExpire.jpg>)

    *Figure 4* 

    Keys are valid for a minimum of 29 days and a maximum of 60. The device will automatically attempt to connect to WiFi to obtain new keys. If WiFi is not available during that period the keys will expire. The device will continue to operate with expired keys, with ~0.3m accuracy but not be able to obtain RTK Fix mode.

7. Now put the device outside with a clear view of the sky. GNSS doesn’t work indoors or near windows. Within about 30 seconds you should see 10 or more satellites in view (SIV). More SIV is better. We regularly see 30 or more SIV. The horizontal positional accuracy (HPA) will start at around 10 meters and begin to decrease. The lower the HPA the more accurate your position. 

    ![Days until L-Band keys expire](<img/Displays/SparkFun_RTK_LBand_Indicator.jpg>)

    *Figure 5*

    Upon successful reception and decryption of L-Band corrections, the satellite dish icon will increase to a three-pronged icon (Figure 5). As the unit's accuracy increases a normal cross-hair will turn to a double blinking cross-hair indicating a floating RTK solution, and a solid double cross-hair will indicate a fixed RTK solution. The HPA will be below 0.030 (30mm) or better once RTK Fix is achieved.

You can now use your RTK device to measure points with millimeter accuracy. Please see [Android](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#android) or [iOS](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/#ios) for guidance on getting the RTK device connected to GIS software over Bluetooth.

## RTK Reference Station

![The SparkFun RTK Reference Station](img/SparkFun_RTK_Reference_Station.jpg)

While most of this Quick Start guide can be used with the [RTK Reference Station](https://www.sparkfun.com/products/22429), the [Reference Station Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-reference-station-hookup-guide) is the best place to get started.

