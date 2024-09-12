# iOS

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

There are a variety of 3rd party apps available for GIS and surveying for [Android](gis_software_android.md), [iOS](gis_software_ios.md), and [Windows](gis_software_windows.md). We will cover a few examples below that should give you an idea of how to get the incoming NMEA data into the software of your choice.

The software options for Apple iOS are much more limited because Apple products do not support Bluetooth SPP. That's ok! The SparkFun RTK products support additional connection options including TCP and Bluetooth Low Energy (BLE).

## ArcGIS Field Maps

For reasons unknown, Esri removed TCP support from Field Maps for iOS and is therefore not usable by SparkFun RTK devices at this time. 

If you must use iOS, checkout [SW Maps](gis_software_ios.md/#sw-maps), [ArcGIS QuickCapture](gis_software_ios.md/#arcgis-quickcapture), or [ArcGIS Survey123](gis_software_ios.md/#arcgis-survey123). 

[Field Maps for Android](gis_software_android.md/#arcgis-field-maps) is supported.
## ArcGIS QuickCapture

![ArcGIS QuickCapture splash screen](<img/QuickCapture/SparkFun RTK QuickCapture - Main Window.png>)

[ArcGIS QuickCapture](https://apps.apple.com/us/app/arcgis-quickcapture/id1451433781) is a popular offering from Esri that works well with SparkFun RTK products.

ArcGIS QuickCapture connects to the RTK device over TCP. In other words, the RTK device needs to be connected to the same WiFi network as the device running QuickCapture. Generally, this is an iPhone or iPad operating as a hotspot.

**Note:** The iOS hotspot defaults to 5.5GHz. This must be changed to 2.4GHz. Please see [Hotspot Settings](gis_software_ios.md/#hotspot-settings).

![WiFi network setup via Web Config](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi Hotspot Web Config.png>)

![Adding WiFi network to settings](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi Hotspot.png>)

The RTK device must use WiFi to connect to the iPhone or iPad. In the above image, the device will attempt to connect to *iPhone* (a cell phone hotspot) when WiFi is needed.

![PVT Server Enabled on port 2948](<img/QuickCapture/iOS/SparkFun RTK QuickCapture iOS - Enable PVT Server.png>)


Next, the RTK device must be configured as a *PVT Server*. The default port of 2948 works well. See [TCP/UDP Menu](menu_tcp_udp.md) for more information.

![RTK device showing IP address](<img/QuickCapture/iOS/SparkFun RTK QuickCapture iOS - IP Address.png>)

Once the RTK device connects to the WiFi hotspot, its IP address can be found in the [System Menu](menu_system.md). This is the number that needs to be entered into QuickCapture. You can now proceed to the QuickCapture app to set up the software connection.

![Main screen](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - Main Screen.png>)

From the main screen, press the hamburger icon in the top left corner. 

![Settings button](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - Settings Menu.png>)

Press the **Settings** button.

![Location Provider button](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - Settings Menu Location Provider.png>)

Select the **Location Provider** option.

Select **Via Network**.

![TCP Network Information](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - TCP Settings.png>)

Enter the IP address and port previously obtained from the RTK device and click **ADD**.

![List of providers showing TCP connection](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - TCP Added.png>)

That provider should now be shown connected.

![Browse project button](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - Main Add Project.png>)

From the main screen, click on the plus in the lower left corner and then **BROWSE PROJECTS**.

![List of projects](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - Add Project.png>)

For this example, add the BioBlitz project.

![GPS Accuracy and map in BioBlitz project](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - BioBlitz Project.png>)

Above, we can see the GPS accuracy is better than 1ft. Click on the map icon in the top right corner.

![QuickCapture map](<img/QuickCapture/iOS/SparkFun RTK QuickCapture - Map.png>)

From the map view, we can see our location with very high accuracy. We can now begin gathering point information with millimeter accuracy.

## ArcGIS Survey123

![ArcGIS Survey123 Home Screen](<img/ArcGIS/SparkFun RTK - ArcGIS Main Screen.png>)

*ArcGIS Survey123 Home Screen*

[ArcGIS Survey123](https://apps.apple.com/us/app/arcgis-survey123/id993015031) is a popular offering from Esri that works well with SparkFun RTK products.

ArcGIS Survey123 connects to the RTK device over TCP. In other words, the RTK device needs to be connected to the same WiFi network as the device running ArcGIS. Generally, this is an iPhone or iPad.

![WiFi network setup via Web Config](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi Hotspot Web Config.png>)


![Adding WiFi network to settings](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi Hotspot.png>)

*Adding WiFi network to settings*

The RTK device must use WiFi to connect to the data collector. Using a cellular hotspot or cellphone is recommended. In the above image, the device will attempt to connect to *iPhone* (a cell phone hotspot) when WiFi is needed.

![TCP Server Enabled on port 2948](<img/ArcGIS/SparkFun RTK - ArcGIS TCP Config.png>)

*TCP Server Enabled on port 2948*

Next, the RTK device must be configured as a *TCP Server*. The default port of 2948 works well. See [TCP/UDP Menu](menu_tcp_udp.md) for more information.

![RTK device showing IP address](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi IP Address.png>)

*RTK device showing IP address*

Once the RTK device connects to the WiFi hotspot, its IP address can be found in the [System Menu](menu_system.md). This is the number that needs to be entered into ArcGIS Survey123. You can now proceed to the ArcGIS Survey123 app to set up the software connection.

![ArcGIS Survey123 Home Screen](<img/ArcGIS/SparkFun RTK - ArcGIS Main Screen.png>)

*ArcGIS Survey123 Home Screen*

From the home screen, click on the 'hamburger' icon in the upper right corner.

![ArcGIS Survey123 Settings Menu](<img/ArcGIS/SparkFun RTK - ArcGIS Settings.png>)

*ArcGIS Survey123 Settings Menu*

From the settings menu, click on the *Settings* gear.

![ArcGIS Survey123 Settings List](<img/ArcGIS/SparkFun RTK - ArcGIS Settings List.png>)

*ArcGIS Survey123 Settings List*

From the settings list, click on *Location*.

![ArcGIS Survey123 List of Location Providers](<img/ArcGIS/SparkFun RTK - ArcGIS Location Providers.png>)

*ArcGIS Survey123 List of Location Providers*

Click on the *Add location provider*.

![ArcGIS Survey123 Network Connection Type](<img/ArcGIS/SparkFun RTK - ArcGIS Location Network.png>)

*ArcGIS Survey123 Network Connection Type*

Select *Network*.

![ArcGIS Survey123 TCP Connection Information](<img/ArcGIS/SparkFun RTK - ArcGIS Network Information.png>)

*ArcGIS Survey123 TCP Connection Information*

Enter the IP address previously found along with the TCP port. Once complete, click *Add*.

![ArcGIS Survey123 Sensor Settings](<img/ArcGIS/SparkFun RTK - ArcGIS Sensor Settings.png>)

*ArcGIS Survey123 Sensor Settings*

You may enter various sensor-specific settings including antenna height, if desired. To view real-time sensor information, click on the satellite icon in the upper right corner.

![ArcGIS Survey123 Sensor Data](<img/ArcGIS/SparkFun RTK - ArcGIS Sensor Data.png>)

*ArcGIS Survey123 Sensor Data*

The SparkFun RTK device's data should now be seen. Click on the *Map* icon to return to the mapping interface.

![ArcGIS Survey123 Map Interface](<img/ArcGIS/SparkFun RTK - ArcGIS Map Interface.png>)

*ArcGIS Survey123 Map Interface*

Returning to the map view, we can now begin gathering point information with millimeter accuracy.

## QField

![Opening page of QField](<img/QField/SparkFun RTK QField - Opening Page.png>)

[QField](https://apps.apple.com/us/app/qfield-for-qgis/id1531726814) is a free iOS app that runs QGIS.

![Modified NMEA messages on RTK Torch](<img/QField/SparkFun RTK QField - NMEA Messages.png>)

*Modified NMEA messages on RTK Torch*

First, configure the RTK device to output *only* the following NMEA messages:

* GPGGA
* GPGSA
* GPGST
* GPGSV

QField currently does not correctly parse other messages such as **GPRMC**, or **RTCM**. These messages will prevent communication if they are enabled.

These NMEA message settings can be found under the [Messages menu](menu_messages.md), using the [web config page](configure_with_wifi.md) or the [serial config interface](configure_with_serial.md).

![WiFi network setup via Web Config](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi Hotspot Web Config.png>)


![Adding WiFi network to settings](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi Hotspot.png>)

*Adding WiFi network to settings*

Next, the RTK device must use WiFi to connect to the data collector. Using a cellular hotspot or cellphone is recommended. In the above image, the device will attempt to connect to *iPhone* (a cell phone hotspot) when WiFi is needed.

![TCP Server Enabled on port 9000](<img/QField/SparkFun RTK QField - TCP Server.png>)

*TCP Server Enabled on port 9000*

Next, the RTK device must be configured as a *TCP Server*. QField uses a default port of 9000 so that is what we recommend using. See [TCP/UDP Menu](menu_tcp_udp.md) for more information.

![RTK device showing IP address](<img/ArcGIS/SparkFun RTK - ArcGIS WiFi IP Address.png>)

*RTK device showing IP address*

Once the RTK device connects to the WiFi hotspot, its IP address can be found in the [System Menu](menu_system.md). This is the number that needs to be entered into QField. You can now proceed to the QField app to set up the software connection.

![QField Opening Screen](<img/QField/SparkFun RTK QField - Opening Page.png>)

*QField Opening Screen*

Click on *QFieldCloud projects* to open your project that was previously created on the [QField Cloud](https://app.qfield.cloud/) or skip this step by using one of the default projects (*Bee Farming*, *Wastewater*, etc).

![QField Main Map](<img/QField/SparkFun RTK QField - Main Map.png>)

*QField Main Map*

From the main map, click on the 'hamburger' icon in the upper left corner.

![QField Settings Gear](<img/QField/SparkFun RTK QField - Settings Gear.png>)

*QField Settings Gear*

Click on the gear to open settings.

![QField Settings Menu](<img/QField/SparkFun RTK QField - Settings Menu.png>)

Click on the *Settings* menu.

![QField Positioning Menu](<img/QField/SparkFun RTK QField - Settings Positioning Menu.png>)

*QField Positioning Menu*

From the *Positioning* menu, click Add.

![QField Entering TCP Information](<img/QField/SparkFun RTK QField - TCP Connection Type.png>)

*QField Entering TCP Information*

Select TCP as the connection type. Enter the IP address of the RTK device and the port number. Finally, hit the small check box in the upper left corner (shown in pink above) to close the window.

Once this information is entered, QField will automatically attempt to connect to that IP and port.

![QField TCP Connected](<img/QField/SparkFun RTK QField - TCP Connected.png>)

*QField TCP Connected*

Above, we see the port is successfully connected. Exit out of all menus.

![QField Connected via TCP with RTK Fix](<img/QField/SparkFun RTK QField - Connected with RTK Fix.png>)

*QField Connected via TCP with RTK Fix*

Returning to the map view, we see an RTK Fix with 11mm positional accuracy.

## SW Maps

SWMaps is available for iOS [here](https://apps.apple.com/us/app/sw-maps/id6444248083).  

Make sure your RTK device is switched on and operating in Rover mode.

Make sure Bluetooth is enabled on your iOS device Settings.

The RTK device will not appear in the _OTHER DEVICES_ list. That is OK.

![iOS Settings Bluetooth](img/iOS/Screenshot1.PNG)

*iOS Settings Bluetooth*

Open SWMaps.

Open or continue a Project if desired.

SWMaps will show your approximate location based on your iOS device's location.

![iOS SWMaps Initial Location](img/iOS/Screenshot2.PNG)

*iOS SWMaps Initial Location*

Press the 'SWMaps' icon at the top left of the screen to open the menu.

![iOS SWMaps Menu](img/iOS/Screenshot3.PNG)

*iOS SWMaps Menu*

Select Bluetooth GNSS.

![iOS SWMaps Bluetooth Connection](img/iOS/Screenshot4.PNG)

*iOS SWMaps Bluetooth Connection*

Set the **Instrument Model** to **Generic NMEA (Bluetooth LE)**.

![iOS SWMaps Instrument Model](img/iOS/Screenshot5.PNG)

*iOS SWMaps Instrument Model*

Press 'Scan' and your RTK device should appear.

![iOS SWMaps Bluetooth Scan](img/iOS/Screenshot6.PNG)

*iOS SWMaps Bluetooth Scan*

Select (tick) the RTK device and press 'Connect'.

![iOS SWMaps Bluetooth Connected](img/iOS/Screenshot7.PNG)

*iOS SWMaps Bluetooth Connected*

Close the menu and your RTK location will be displayed on the map.

You can now use the other features of SWMaps, including the built-in NTRIP Client.

Re-open the menu and select 'NTRIP Client'.

Enter the details for your NTRIP Caster - as shown in the [SWMaps section above](#sw-maps).

![iOS SWMaps NTRIP Client](img/iOS/Screenshot8.PNG)

*iOS SWMaps NTRIP Client*

Click 'Connect'

At this point, you should see a Bluetooth Pairing Request. Select 'Pair' to pair your RTK with your iOS device.

![iOS Bluetooth Pairing](img/iOS/Screenshot9.PNG)

*iOS Bluetooth Pairing*

SWMaps will now receive NTRIP correction data from the caster and push it to your RTK over Bluetooth BLE.

From the SWMaps menu, open 'GNSS Status' to see your position, fix type and accuracy.

![iOS SWMaps GNSS Status](img/iOS/Screenshot10.PNG)

*iOS SWMaps GNSS Status*

If you return to the iOS Bluetooth Settings, you will see that your iOS and RTK devices are now paired.

![iOS Settings Bluetooth Paired](img/iOS/Screenshot11.PNG)

*iOS Settings Bluetooth - Paired*

## Other GIS Packages

Hopefully, these examples give you an idea of how to connect the RTK product line to most any GIS software. If there is other GIS software that you'd like to see configuration information about, please open an issue on the [RTK Firmware repo](https://github.com/sparkfun/SparkFun_RTK_Everywhere_Firmware/issues) and we'll add it.

## What's an NTRIP Caster? 

In a nutshell, it's a server that is sending out correction data every second. There are thousands of sites around the globe that calculate the perturbations in the ionosphere and troposphere that decrease the accuracy of GNSS accuracy. Once the inaccuracies are known, correction values are encoded into data packets in the RTCM format. You, the user, don't need to know how to decode or deal with RTCM, you simply need to get RTCM from a source within 10km of your location into the RTK device. The NTRIP client logs into the server (also known as the NTRIP caster) and grabs that data, every second, and sends it over Bluetooth to the RTK device.

## Where do I get RTK Corrections?

Be sure to see [Correction Sources](correction_sources.md). 

Don't have access to an NTRIP Caster or other RTCM correction source? There are a few options.

The [SparkFun RTK Facet L-Band](https://www.sparkfun.com/products/20000) gets corrections via an encrypted signal from geosynchronous satellites. This device gets RTK Fix without the need for a WiFi or cellular connection.

Also, you can use a 2nd RTK product operating in Base mode to provide the correction data. Check out [Creating a Permanent Base](permanent_base.md). 

If you're the DIY sort, you can create your own low-cost base station using an ESP32 and a ZED-F9P breakout board. Check out [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station). 

There are services available as well. [Syklark](https://www.swiftnav.com/skylark) provides RTCM coverage for $49 a month (as of writing) and is extremely easy to set up and use. [Point One](https://app.pointonenav.com/trial?utm_source=sparkfun) also offers RTK NTRIP service with a free 14 day trial and easy to use front end.
