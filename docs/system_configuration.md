# System Configuration

Surveyor: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express Plus: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Facet: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Facet L-Band: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png)

All device settings are stored both in internal memory and an SD card if one is detected. The device will load the latest settings at each power on. If there is a discrepancy between the internal settings and a settings file then the settings file will be used. This allows a collection of RTK products to be identically configured using one 'golden' settings file loaded onto an SD card.

There are three ways to configure an RTK product:

* [WiFi](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_with_wifi/) - Good for in-field changes
* [Serial Terminal](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_with_serial/) - Requires a computer but allows for all configuration settings
* [Settings File](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_with_settings_file/) - Error-Prone; for very advanced users only.

The serial terminal method is recommended for most advanced configurations. Most, but not all settings are also available over WiFi but can be tricky to input via mobile phone.

Firmware configuration options include:

* [Base](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_base/)
* [GNSS Messages](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_messages/)
* [GNSS Receiver](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_gnss/)
* [Logging](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_data_logging/)
* [Point Perfect](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_pointperfect/)
* [Ports](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_ports/)
* [Profiles](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_profiles/)

Hardware configuration:
* [Bluetooth Connection](https://sparkfun.github.io/SparkFun_RTK_Firmware/connecting_bluetooth/)
* [Embedded System Connection](https://sparkfun.github.io/SparkFun_RTK_Firmware/embeddedsystem_connection/)

Software configuration options include:
* [GIS Software](https://sparkfun.github.io/SparkFun_RTK_Firmware/gis_software/)
* [u-center](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_ucenter/)
