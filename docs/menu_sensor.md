# Sensor Menu

Surveyor: ![Feature Not Supported](img/Icons/RedDot.png) / Express: ![Feature Not Supported](img/Icons/RedDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Not Supported](img/Icons/RedDot.png) / Facet L-Band: ![Feature Not Supported](img/Icons/RedDot.png) / Reference Station: ![Feature Not Supported](img/Icons/RedDot.png)

![Sensor menu is shown in WiFi config](img/WiFi Config/SparkFun%20RTK%20Sensor%20Menu%20WiFi%20Config.png)

![Sensor menu from serial prompt](img/Terminal/SparkFun%20RTK%20-%20Sensor%20Menu.png)

*Setting the Sensor options over WiFi config and serial connections*

The [RTK Express Plus](https://www.sparkfun.com/products/18589) utilizes the ZED-F9R GNSS receiver with built-in IMU. This allows the RTK device to continue to output high-precision location information even if GNSS reception goes down or becomes unavailable. This was designed for and is especially helpful in automotive environments, such as tunnels or parking garages, where GNSS reception because sparse.

Enable 'Sensor Fusion' to begin using the onboard IMU when GNSS is avaialble. Sensor Fusion will only aid position information when used with an automobile and may lead to degraded position fixes when used in other situations (ie, surveying, pedestrian, etc).

'Automatic IMU-Mount Alignment' will allow the device to automatically determine how the product is mounted within the vehicle's frame of reference.

Additionally, wheel ticks should be provided to the unit to enhance the positional fixes. Please see [Mux Channel](menu_ports.md#mux-channel) of the Ports Menu for more information.


