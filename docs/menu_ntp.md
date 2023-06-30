# Network Time Protocol Menu

Surveyor: ![Feature Not Supported](img/Icons/RedDot.png) / Express: ![Feature Not Supported](img/Icons/RedDot.png) / Express Plus: ![Feature Not Supported](img/Icons/RedDot.png) / Facet: ![Feature Not Supported](img/Icons/RedDot.png) / Facet L-Band: ![Feature Not Supported](img/Icons/RedDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

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

![Animation of selecting NTP mode](img/Displays/SparkFun RTK - NTP Select.gif)

*Selecting NTP mode*

The Reference Station will first synchronize its Real Time Clock (RTC) using the very accurate time provided by the u-blox GNSS module. The module's Time Pulse (Pulse-Per-Second) signal is connected to the ESP32 as an interrupt. The ESP32's RTC is synchronized to Universal Time Coordinate (UTC) on the rising edge of the TP signal using the time contained in the UBX-TIM-TP message.

The WIZnet W5500 interrupt signal is also connected to the ESP32, allowing the ESP32 to accurately log when each NTP request arrives.

The Reference Station will respond to each NTP request within a few 10s of milliseconds.

If desired, you can log all NTP requests to a file on the microSD card, and/or print them as diagnostic messages. The log and messages contain the NTP timing information and the IP Address and port of the Client.

[![The system debug menu showing how to enable the NTP diagnostics](img/NTP/NTP_Diagnostics.png)](img/NTP/NTP_Diagnostics.png)

*System Debug Menu - NTP Diagnostics (Click for a closer look)*

[![The logging menu showing how to log the NTP requests](img/NTP/NTP_Logging.png)](img/NTP/NTP_Logging.png)

*Logging Menu - Log NTP Requests*

### Logged NTP Requests

![NTP requests log](img/NTP/NTP_Log.png)

NTP uses its own epoch - midnight January 1st, 1900. This is different than the standard Unix epoch - midnight January 1st, 1970 - and the GPS epoch - midnight January 6th, 1980. The times shown in the log and diagnostic messages use the NTP epoch. You can use online calculators to convert between the different epochs:

* [https://weirdo.cloud/](https://weirdo.cloud/)

* [https://www.unixtimestamp.com/](https://www.unixtimestamp.com/)

* [https://www.labsat.co.uk/index.php/en/gps-time-calculator](https://www.labsat.co.uk/index.php/en/gps-time-calculator)

### NTP on Windows

If you want to synchronize your Windows PC to a Reference Station NTP Server, here's how to do it:

* Install [Meinberg NTP](https://www.meinbergglobal.com/english/sw/ntp.htm) - this replaces the Windows built-in Time Service

![Meinberg NTP initial configuration](img/NTP/NTP_Install_1.png)

* During the installation, select "Create an initial configuration file" and select the NTP Pool server for your location
* Select "Use fast initial sync mode" for faster first synchronization

![Meinberg NTP service settings](img/NTP/NTP_Install_2.png)

* The next step is to edit the NTP Configuration File
    * Editing the file requires Administrator privileges
    * Open the *Start* menu, navigate to *Meinberg*, right-click on *Edit NTP Configuration* and select *Run as administrator*

[![Meinberg NTP configuration](img/NTP/NTP_Config_1_small.png)](img/NTP/NTP_Config_1.png)

* Comment the lines in *ntp.conf* which name the pool.ntp servers
* Add an extra *server* line and include the IP Address for your Reference Station. It helps to give your Reference Station a fixed IP Address first - see [Menu Ethernet](menu_ethernet.md)
* Save the file

[![Meinberg NTP configuration](img/NTP/NTP_Config_2_small.png)](img/NTP/NTP_Config_2.png)

* Finally, restart the NTP Service
    * Again this needs to be performed with Administrator privileges
    * Open the *Start* menu, navigate to *Meinberg*, right-click on *Restart NTP Service* and select *Open file loctaion*

[![Meinberg NTP configuration](img/NTP/NTP_Config_3_small.png)](img/NTP/NTP_Config_3.png)

* Right-click on the *Restart NTP Service* and select *Run as administrator*

![Meinberg NTP configuration](img/NTP/NTP_Config_4.png)

* You can check if your PC clock synchronized successfully by opening a *Command Prompt (cmd)* and running *ntpq -pd*

![Meinberg NTP configuration](img/NTP/NTP_Config_5.png)

If enabled, your Windows PC NTP requests will be printed and logged by the reference station. See [above](#logged-ntp-requests).

