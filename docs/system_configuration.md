# System Configuration

Surveyor: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Express Plus: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Facet: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png) / Facet L-Band: ![Feature Supported](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Firmware/main/docs/img/GreenDot.png)

All device settings are stored both in internal memory and an SD card if one is detected. The device will load the latest settings at each power on. If there is a discrepancy between the internal settings and a settings file then the settings file will be used. This allows a collection of RTK products to be identically configured using one 'golden' settings file loaded onto an SD card.

There are multiple ways to configure an RTK product:

* [Bluetooth](docs\configure_with_bluetooth.md) - Good for in-field changes.
* [WiFi](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_with_wifi/) - Good for in-field changes
* [Serial Terminal](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_with_serial/) - Requires a computer but allows for all configuration settings
* [Settings File](https://sparkfun.github.io/SparkFun_RTK_Firmware/configure_with_settings_file/) - Error-Prone; for very advanced users only.

The Bluetooth or Serial Terminal methods are recommended for most advanced configurations. Most, but not all settings are also available over WiFi but can be tricky to input via mobile phone.

Device configuration options include:

* [Base](docs\configure_base.md)
* [GNSS Messages](docs\configure_messages.md)
* [GNSS Receiver](docs\configure_gnss.md)
* [Logging](docs\configure_data_logging.md)
* [Point Perfect](docs\configure_pointperfect.md)
* [Ports](docs\configure_ports.md)
* [Profiles](docs\configure_profiles.md)
* [Radio](docs\menu_radios.md)
* [Sensor](docs\menu_sensor.md)
* [WiFi](docs\menu_wifi.md)
