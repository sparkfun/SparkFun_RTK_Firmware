# Network Menu

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

![Client and Server settings](<img/Terminal/SparkFun RTK Network Menu.png>)

*Client and Server settings*

As an alternative to serial ports, serial over USB, or Bluetooth, the RTK device can send and receive GNSS data over TCP, and can send data over UDP.   These mechanisms sit on top of the network layer (WiFi or Ethernet).  The data could be NMEA, UBX, RTCM, and is the same data that would be sent and received over Bluetooth.  The mechanism can be used in Rover or Base mode.  There are three mechanisms, two TCP and one UDP, described below.

## TCP Client and Server

Configuring a TCP Client will cause the RTK device to open a TCP connection to a given address and port, and then send and receive data.  Configuring a TCP Server will cause the RTK device to listen on the given port for an incoming connection.  In either case, when a connection is established the device will send and receive data.
Some Data Collector software (such as [Vespucci](gis_software.md#vespucci)) requires that the SparkFun RTK device connect as a TCP Client. Other software (such as [QGIS](gis_software.md#qgis)) requires that the SparkFun RTK device acts as a TCP Server. Both are supported.

**Note:** Currently for WiFi: TCP is only supported while connected to local WiFi, not AP mode. This means the device will need to be connected to a WiFi network, such as a mobile hotspot, before TCP connections can occur.

![TCP Port Entry](img/WiFi%20Config/SparkFun%20RTK%20Config%20-%20TCP%20Port.png)

If either Client or Server is enabled, a port can be designated. By default, the port is 2948 (registered as [*GPS Daemon request/response*](https://tcp-udp-ports.com/port-2948.htm)) but any port 0 to 65535 is supported.

![Ethernet TCP Client connection](img/Terminal/TCP_Client.gif)

The above animation was generated using [TCP_Server.py](https://github.com/sparkfun/SparkFun_RTK_Everywhere_Firmware/blob/main/Firmware/Tools/TCP_Server.py).

## UDP Server

Data can also be broadcast via UDP on Ethernet and WiFi, rather than TCP. If enabled, the UDP Server will begin broadcasting NMEA data over the specific port (default 10110).
