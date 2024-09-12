# WiFi Menu

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Partially Supported](img/Icons/YellowDot.png)

![WiFi Menu in AP Config page](img/WiFi Config/SparkFun%20RTK%20AP%20WiFi%20Menu.png)

*WiFi Menu in the WiFi config page*

![WiFi Network Entry](img/Terminal/SparkFun%20RTK%20WiFi%20Menu%20Terminal.png)


*WiFi Menu containing one network*

Beginning in firmware version 3.0, the WiFi menu allows a user to input credentials of up to four WiFi networks. WiFi is used for a variety of features on the RTK device. When WiFi is needed, the RTK device will attempt to connect to any network on the list of WiFi networks. For example, if you enter your home WiFi, work WiFi, and the WiFi for a mobile hotspot, the RTK device will automatically detect and connect to the network with the strongest signal.

Additionally, the device will continue to try to connect to WiFi if a connection is not made. The connection timeout starts at 15 seconds and increases by 15 seconds with each failed attempt. For example, 15, 30, 45, etc seconds are delayed between each new WiFi connection attempt. Once a successful connection is made, the timeout is reset.

WiFi is used for the following features:

* NTRIP Client or Server
* TCP Client or Server
* Firmware Updates
* Device Configuration (WiFi mode only)
* PointPerfect Key renewal (RTK Facet L-Band only)

## Configure Mode: AP vs WiFi

![Configure Mode in WiFi menu](img/WiFi%20Config/SparkFun%20RTK%20Config%20-%20Configure%20Mode.png)

![WiFi Network Entry](img/Terminal/SparkFun%20RTK%20WiFi%20Menu%20Terminal.png)

By default, the device will become an Access Point when the user selects 'Config' from the front panel controls. This is handy for in-field device configuration. Alternatively, changing this setting to 'WiFi' will cause the device to connect to local WiFi. 

![Configuring RTK device over local WiFi](img/WiFi%20Config/SparkFun%20RTK%20AP%20Main%20Page%20over%20Local%20WiFi.png)

Configuring over WiFi allows the device to be configured from any desktop computer that has access to the same WiFi network. This method allows for greater control from a full size keyboard and mouse.

![RTK display showing local IP and SSID](img/Displays/SparkFun%20RTK%20WiFi%20Config%20IP.png)

When the device enters WiFi config mode it will display the WiFi network it is connected to as well as its assigned IP address.

## MDNS

![Access using rtk.local](img/WiFi Config/SparkFun%20RTK%20WiFi%20MDNS.png)

Multicast DNS or MDNS allows the RTK device to be discovered over wireless networks without needing to know the IP. For example, when MDNS is enabled, simply type 'rtk.local' into a browser to connect to the RTK Config page. This feature works both for 'WiFi Access Point' or direct WiFi config. Note: When using WiFi config, you must be on the same subdomain (in other words, the same WiFi or Ethernet network) as the RTK device.
