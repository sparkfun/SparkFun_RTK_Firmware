# System Configuration

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

All device settings are stored both in internal memory and an SD card if one is detected. The device will load the latest settings at each power on. If there is a discrepancy between the internal settings and a settings file then the settings file will be used. This allows a collection of RTK products to be identically configured using one 'golden' settings file loaded onto an SD card.

There are multiple ways to configure an RTK product:

* [Bluetooth](configure_with_bluetooth.md) - Good for in-field changes.
* [WiFi](configure_with_wifi) - Good for in-field changes
* [Serial Terminal](configure_with_serial) - Requires a computer but allows for all configuration settings
* [Settings File](configure_with_settings_file) - Error-Prone; for very advanced users only.

The Bluetooth or Serial Terminal methods are recommended for most advanced configurations. Most, but not all settings are also available over WiFi but can be tricky to input via mobile phone.

Device configuration options include:

* [Base](configure_base.md)
* [GNSS Messages](configure_messages.md)
* [GNSS Receiver](configure_gnss.md)
* [Logging](configure_data_logging.md)
* [Point Perfect](configure_pointperfect.md)
* [Ports](configure_ports.md)
* [Profiles](configure_profiles.md)
* [Radio](menu_radios.md)
* [Sensor](menu_sensor.md)
* [WiFi](menu_wifi.md)
