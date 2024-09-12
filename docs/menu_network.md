# Network Menu

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

![PVT Client and Server settings](<img/Terminal/SparkFun RTK Network Menu.png>)

*PVT Client and Server settings*

## PVT Client and Server

The RTK device supports connection over TCP (aka PVT Client and Server). The TCP Client sits on top of the network layer (WiFi or Ethernet) and sends position data to one or more computers or cell phones for display. Some Data Collector software (such as [Vespucci](gis_software.md#vespucci)) requires that the SparkFun RTK device connect as a TCP Client. Other software (such as [QGIS](gis_software.md#qgis)) requires that the SparkFun RTK device acts as a TCP Server. Both are supported.

**Note:** Currently for WiFi: TCP is only supported while connected to local WiFi, not AP mode. This means the device will need to be connected to a WiFi network, such as a mobile hotspot, before TCP connections can occur.

![TCP Port Entry](img/WiFi%20Config/SparkFun%20RTK%20Config%20-%20TCP%20Port.png)

If either Client or Server is enabled, a port can be designated. By default, the port is 2947 (registered as [*GPS Daemon request/response*](https://tcp-udp-ports.com/port-2948.htm)) but any port 0 to 65535 is supported.

![Ethernet TCP Client connection](img/Terminal/TCP_Client.gif)

The above animation was generated using [TCP_Server.py](https://github.com/sparkfun/SparkFun_RTK_Everywhere_Firmware/blob/main/Firmware/Tools/TCP_Server.py).

## UDP Server

NMEA messages can also be broadcast via UDP on Ethernet and WiFi, rather than TCP. If enabled, the UDP Server will begin broadcasting NMEA data over the specific port (default 10110).