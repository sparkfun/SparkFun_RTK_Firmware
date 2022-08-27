# Configuring with u-center

Surveyor: ![Feature Partially Supported](img/YellowDot.png) / Express: ![Feature Partially Supported](img/YellowDot.png) / Express Plus: ![Feature Partially Supported](img/YellowDot.png) / Facet: ![Feature Partially Supported](img/YellowDot.png) / Facet L-Band: ![Feature Partially Supported](img/YellowDot.png)

The ZED-F9P module can be configured independently using the u-center software from u-blox by connecting a USB cable to the *Config u-blox* USB connector. Settings can be saved to the module between power cycles. For more information please see SparkFun’s [Getting Started with u-center by u-blox](https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox/all).

However, because the ESP32 does considerable configuration of the ZED-F9P at power on it is not recommended to modify the settings of the ZED-F9P using u-center. Nothing will break but your changes will likely be overwritten at the next power cycle.
