# Debug Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

![System Debug Menu](img/SparkFun%20RTK%20Debug%20Menu.png)

*Showing the debug menu*

The Debug menu enables the user to enable and disable various debug features. None of these options are needed for normal users or daily use. These are provided for faster software development and troubleshooting.

1. **I2C Debugging Output** - Enable additional ZED-F9P interface debug messages
2. **Heap Reporting** - Display currently available bytes, lowest value and the largest block
3. **Task Highwater Reporting** - Shows stack usage of select tasks
4. **Set the SPI / microSD card frequency** - SD card interface speed. Default is 16MHz. 
5. **Set SPP RX buffer size** - Default 128 bytes
6. **Set SPP TX buffer size** - Controls how large the buffer used to communicate over Bluetooth
7. **Throttle Bluetooth transmissions during SPP congestion** - Reduce bytes transmitted if Bluetooth link becomes busy
8. **Display reset counter** - Enable to display a small number indicating non-power on reset count
9. **Set GNSS serial timeout in seconds** - Sets the number of milliseconds before reporting serial available
10. Periodically display WiFi IP Address
11. Periodically display system states
12. Periodically display WiFi states
13. Periodically display NTRIP Client states
14. Periodically display NTRIP Server states

* **t** - Display the test screen
* **e** - Erase LittleFS: Clear settings and profiles saved internally (not on microSD card)
* **r** - Reset the system
* **x** - Exit the debug menu
