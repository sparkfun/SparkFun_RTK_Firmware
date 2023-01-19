# WiFi Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

![WiFi Menu in AP Config page](img/SparkFun%20RTK%20AP%20WiFi%20Menu.png)

*WiFi Menu in the WiFi config page*

![WiFi Network Entry](img/SparkFun%20RTK%20WiFi%20Menu%20Terminal.png)

*WiFi Menu containing one network*

Beginning in firmware version 3.0, the WiFi menu allows a user to input credentials of up to four WiFi networks. WiFi is used for a variety of features on the RTK device. When WiFi is needed, the RTK device will attempt to connect to any network on the list of WiFi networks. For example, if you enter your home WiFi, work WiFi, and the WiFi for a mobile hotspot, the RTK device will automatically detect and connect to the network with the strongest signal.

WiFi is used for the following features:

* NTRIP Client or Server
* TCP Client or Server
* Firmware Updates
* Device Configuration (WiFi mode only)
* PointPerfect Key renewal (RTK Facet L-Band only)

## TCP Client and Server

The RTK device supports connection over TCP. Some Data Collector software (such as [Vespucci](docs\gis_software\#vespucci)) requires that the SparkFun RTK device connect as a TCP Client. Other software (such as [QGIS](docs\gis_software\#qgis)) requires that the SparkFun RTK device acts as a TCP Server. Both are supported.

**Note:** Currently TCP is only supported while connected to local WiFi, not AP mode. This means the device will need to be connected to a WiFi network, such as a mobile hotspot, before TCP connections can occur.

![TCP Port Entry](img/WiFi%20Config/SparkFun%20RTK%20Config%20-%20TCP%20Port.png)

If either Client or Server is enabled, a port can be designated. By default, the port is 2947 (registered as [*GPS Daemon request/response*](https://en.wikipedia.org/wiki/Gpsd)) but any port 0 to 65535 is supported.

## Configure Mode: AP vs WiFi

![Configure Mode in WiFi menu](img/WiFi%20Config/SparkFun%20RTK%20Config%20-%20Configure%20Mode.png)

![WiFi Network Entry](img/SparkFun%20RTK%20WiFi%20Menu%20Terminal.png)

By default, the device will become an Access Point when the user selects 'Config' from the front panel controls. This is handy for in-field device configuration. Alternatively, changing this setting to 'WiFi' will cause the device to connect to local WiFi. 

![Configuring RTK device over local WiFi](img/WiFi%20Config/SparkFun%20RTK%20AP%20Main%20Page%20over%20Local%20WiFi.png)

Configuring over WiFi allows the device to be configured from any desktop computer that has access to the same WiFi network. This method allows for greater control from a full size keyboard and mouse.

![RTK display showing local IP and SSID](img/WiFi%20Config/SparkFun%20RTK%20WiFi%20Config%20IP.png)

When the device enters WiFi config mode it will display the WiFi network it is connected to as well as its assigned IP address.

