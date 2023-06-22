# Network Time Protocol (NTP)

Surveyor: ![Feature Not Supported](img/RedDot.png) / Express: ![Feature Not Supported](img/RedDot.png) / Express Plus: ![Feature Not Supported](img/RedDot.png) / Facet: ![Feature Not Supported](img/RedDot.png) / Facet L-Band: ![Feature Not Supported](img/RedDot.png) / Reference Station: ![Feature Supported](img/GreenDot.png)

The Reference Station can act as an Ethernet Network Time Protocol (NTP) server.

Network Time Protocol has been around since 1985. It is a simple way for computers to synchronize their clocks with each other, allowing the network latency (delay) to be subtracted:

* A client sends a NTP request (packet) to the chosen or designated server
  * The request contains the client's current clock time - for identification

* The server logs the time the client's request arrived and then sends a reply containing:
  * The client's clock time - for identification
  * The server's clock time - when the request arrived at the server
  * The server's clock time - when the reply is sent
  * The time the server's clock was last synchronized - providing the age of the synchronization

* The client logs the time the reply is received - using its own clock

When the client receives the reply, it can deduce the total round-trip delay which is the sum of:

* How long the request took to reach the server

* How long the server took to construct the reply

* How long the reply took to reach the client

This exchange is repeated typically five times, before the client synchronizes its clock to the server's clock, subtracting the latency (delay) introduced by the network.

Having your own NTP server on your network allows tighter clock synchronization as the network latency is minimized.

The Reference Station can be placed into its dedicated NTP mode, by pressing the **MODE** button until NTP is highlighted in the display and pausing there.

![Animation of selecting NTP mode](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/0/NTP.gif)](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/0/NTP.gif)

**Selecting NTP mode**

The Reference Station will first synchronize its Real Time Clock (RTC) using the very accurate time provided by the u-blox GNSS module. The module's Time Pulse (Pulse-Per-Second) signal is connect to the ESP32 as an interrupt. The ESP32's RTC is synchronized to Universal Time Coordinate (UTC) on the rising edge of the TP signal using the time contained in the UBX-TIM-TP message.

The WIZnet W5500 interrupt signal is also connected to the ESP32, allowing the ESP32 to accurately log when each NTP request arrives.

The Reference Station will respond to each NTP request within a few 10's of milliseconds.

If desired, you can log all NTP requests to a file on the microSD card, and/or print them as diagnostic messages. The log and messages contain the NTP timing information and the IP Address and port of the Client.

![The system debug menu showing how to enable the NTP diagnostics](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/3/2/1/0/NTP_Diagnostics.png)](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/0/NTP_Diagnostics.png)

**System Debug Menu - NTP Diagnostics (Click for a closer look)**

![The logging menu showing how to log the NTP requests](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/3/2/1/0/NTP_Logging.png)](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/0/NTP_Logging.png)

**Logging Menu - Log NTP Requests**

![NTP requests log](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/0/NTP_Log.png)](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/0/NTP_Log.png)

**Logged NTP Requests**

NTP uses its own epoch - midnight January 1st 1900. This is different to the standard Unix epoch - midnight January 1st 1970 - and the GPS epoch - midnight January 6th 1980. The times shown in the log and diagnostic messages use the NTP epoch. You can use online calculators to convert between the different epochs:

* [https://weirdo.cloud/](https://weirdo.cloud/)

* [https://www.unixtimestamp.com/](https://www.unixtimestamp.com/)

* [https://www.labsat.co.uk/index.php/en/gps-time-calculator](https://www.labsat.co.uk/index.php/en/gps-time-calculator)
