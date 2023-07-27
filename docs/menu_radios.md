# Radios Menu

## ESP-Now

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Partially Supported](img/Icons/YellowDot.png)

![Radio menu during AP-Config](img/WiFi Config/SparkFun%20RTK%20Radio%20Config.png)

*Radio configuration through WiFi*

![Radio menu showing ESP-Now](img/Terminal/SparkFun%20RTK%20Radio%20Menu.png)

*Radio menu showing ESP-Now*

Pressing 'r' from the main menu will open the Configure Radios menu. This allows a user to enable or disable the use of the internal ESP-Now radio.

ESP-Now is a 2.4GHz protocol that is built into the internal ESP32 microcontroller; the same microcontroller that provides Bluetooth and WiFi. ESP-Now does not require WiFi or an Access Point. This is most useful for connecting a Base to Rover without the need for an external radio. Simply turn two SparkFun RTK products on, enable their radios, pair them, and data will be passed between units.

Additionally, ESP-Now supports point-to-multipoint transmissions. This means a Base can transmit to multiple Rovers simultaneously.

The ESP-Now radio feature was added in firmware release v2.4. If the **Configure Radio** menu is not visible, consider upgrading your firmware.

ESP-Now is a free radio included in every RTK product, and works well, but it has a few limitations: 

1. Limited use with Bluetooth SPP. The ESP32 is capable of [simultaneously transmitting](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/coexist.html) ESP-Now and Bluetooth LE, but not classic Bluetooth SPP. Unfortunately SPP (Serial Port Profile) is the most common method for moving data between a GNSS receiver and the GIS software. Because of this, using ESP-Now while connecting to the RTK product using Bluetooth SPP is not stable. SparkFun RTK products support Bluetooth LE and ESP-Now works flawlessly with Bluetooth LE. There are a few GIS applications that support Bluetooth LE including SW Maps.

    ![Max transmission range of about 250m](img/Radios/SparkFun%20RTK%20ESP-Now%20Distance%20Testing.png)

2. Limited range. You can expect two RTK devices to be able to communicate approximately 250m (845 ft) line of sight but any trees, buildings, or objects between the Base and Rover will degrade reception. This range is useful for many applications but may not be acceptable for some applications. We recommend using ESP-Now as a quick, free, and easy way to get started with Base/Rover setups. If your application needs longer RF distances consider cellular NTRIP, WiFi NTRIP, or an external serial telemetry radio plugged into the **RADIO** port.

3. Bug limited to Point to Point. There is a known bug in the ESP32 Arduino core that prevents a base from [transmitting to multiple rovers](https://github.com/espressif/esp-idf/issues/8992). This will be fixed in future releases of the RTK firmware once the ESP32 core is updated. For now, a base can be paired successfully with a single rover.

## Pairing

![Pairing Menu](img/Displays/SparkFun%20RTK%20Radio%20E-Pair.png)

Pressing the Setup button (Express or Express Plus) or the Power/Setup button (Facet or Facet L-Band) will display the various submenus. Pausing on E-Pair will put the unit into ESP-Now pairing mode. If another RTK device is detected nearby in pairing mode, they will exchange MAC addresses and pair with each other. Multiple Rover units can be paired to a Base in the same fashion.

![Radio menu during AP-Config](img/WiFi Config/SparkFun%20RTK%20Radio%20Config.png)

*Radio configuration through WiFi*

The radio system can be configured over WiFi. The radios subsystem is disabled by default. Enabling the radio to ESP-Now will expose the above options. The unit's radio MAC can be seen as well as a button to forget all paired radios. This button is disabled until the 'Enable Forget All Radios' checkbox is checked. The 'Broadcast Override' function changes all data transmitted by this radio to be sent to all radios in the vicinity, instead of only the radios it is paired with. This override feature is helpful if using a base that has not been paired: a base can transmit to multiple rovers regardless if they are paired or not.

![Serial Radio menu](img/Terminal/SparkFun%20RTK%20Radio%20Menu.png)

A serial menu is also available. This menu allows users to enter pairing mode, view the unit's current Radio MAC, the MAC addresses of any paired radios, as well as the ability to remove all paired radios from memory.

## Reference Station

There is not enough RAM available on the Reference Station to run ESP-Now and Bluetooth simultaneously. ESP-Now does work on the Reference Station but you need to disable Bluetooth in order to use it.

* Disable Bluetooth via the System Menu. Select "b" twice to: first select BLE mode; and then to disable Bluetooth completely

* Restart the system using the System Menu \ Debug Menu: enter "s" followed by "d" followed by "r" to restart the Reference Station. This ensures the RAM used by Bluetooth is released

* Select the E-Pair option by pressing the MODE button until "E-Pair" is displayed

* Pair the Reference Station Base with an RTK Rover and the Rover will achieve RTK-Fix
