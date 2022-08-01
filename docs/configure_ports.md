# Ports Menu

Surveyor: ![Feature Partially Supported](img/YellowDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

[![Setting the baud rate of the ports](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/RTK_Surveyor_-_WiFi_Config_-_Express_Ports_Config.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/RTK_Surveyor_-_WiFi_Config_-_Express_Ports_Config.jpg)

*Setting the baud rates of the two available external ports*

[![Baud rate configuration of Radio and Data ports](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Ports_Menu.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Ports_Menu.jpg)

*Baud rate configuration of Radio and Data ports*

## Radio Port

By default, the **Radio** port is set to 57600bps to match the [Serial Telemetry Radios](https://www.sparkfun.com/products/19032) that are recommended to be used with the RTK Facet (it is a plug-and-play solution). This can be set from 4800bps to 921600bps.

## Mux Channel

The **Data** port on the RTK Facet, Express, and Express Plus is very flexible. Internally the **Data** connector is connected to a digital mux allowing one of four software-selectable setups. By default, the Data port will be connected to the UART1 of the ZED-F9P and output any messages via serial.

* **NMEA** - The TX pin outputs any enabled messages (NMEA, UBX, and RTCM) at a default of 460,800bps (configurable 9600 to 921600bps). The RX pin can receive RTCM for RTK and can also receive UBX configuration commands if desired.
* **PPS/Trigger** - The TX pin outputs the pulse-per-second signal that is accurate to 30ns RMS. This pin can be configured as an extremely accurate time base. The pulse length and time between pulses are configurable down to 1us. The RX pin is connected to the EXTINT pin on the ZED-F9P allowing for events to be measured with incredibly accurate nano-second resolution. Useful for things like audio triangulation. See the Timemark section of the [ZED-F9P Integration Manual](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/ZED-F9P_IntegrationManual__UBX-18010802_.pdf) for more information.
* **I2C** - The TX pin operates as SCL, RX pin as SDA on the I2C bus. This allows additional sensors to be connected to the I2C bus.
* **GPIO** - The TX pin operates as a DAC-capable GPIO on the ESP32. The RX pin operates as an ADC-capable input on the ESP32. This is useful for custom applications.

![Configuring the External Pulse and External Events](img/SparkFun%20RTK%20Ports%20PPS%20Config.png)

*Configuring the External Pulse and External Events over WiFi*

[![RTK Facet Mux Menu](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Ports_Menu_Mux.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Ports_Menu_Mux.jpg)

*Port menu showing mux data port connections*

## Data Port

By default, the **Data** port is set to 460800bps and can be configured from 4800bps to 921600bps. The 460800bps baud rate was chosen to support applications where a large number of messages are enabled and a large amount of data is being sent. If you need to decrease the baud rate to 115200bps or other, be sure to monitor the MON-COMM message within u-center for buffer overruns. A baud rate of 115200bps and the NMEA+RXM default configuration at 4Hz *will* cause buffer overruns.

[![Monitoring the COM ports on the ZED-F9P](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Ports_Menu_MON-COMM_Overrun.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Ports_Menu_MON-COMM_Overrun.jpg)

*Monitoring the COM ports on the ZED-F9P*

If you must run the data port at lower than 460800bps, and you need to enable a large number of messages and/or increase the fix frequency beyond 4Hz, be sure to verify that UART1 usage stays below 99%. The image above shows the UART1 becoming overwhelmed because the ZED cannot transmit at 115200bps fast enough.

Most applications do not need to plug anything into the **Data** port. Most users will get their NMEA position data over Bluetooth. However, this port can be useful for sending position data to an embedded microcontroller or single-board computer. The pinout is 3.3V / TX / RX / GND. **3.3V** is provided by this connector to power a remote device if needed. While the port is capable of sourcing up to 600mA, we do not recommend more than 300mA. This port should not be connected to a power source.

## Surveyor Data Port

By default, the Data port is set to 460800bps and can be configured from 4800bps to 921600bps. 

Note: The Data port does not output NMEA by default. The unit must be opened and the *Serial NMEA Connection* switch must be moved to 'Ext Connector'. See [Hardware Overview - Advanced Features](https://sparkfun.github.io/SparkFun_RTK_Firmware/hardware_rtk_surveyor/#advanced-features) for the location of the switch.