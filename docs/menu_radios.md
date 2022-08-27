# Radios Menu

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

![Radio menu showing ESP-Now](img/SparkFun%20RTK%20Radio%20Menu.png)

*Radio menu showing ESP-Now*

Pressing 'r' from the main menu will open the Configure Radios menu. This allows a user to enable or disable the use of the internal ESP-Now radio.

ESP-Now is a 2.4GHz protocol that is built into the internal ESP32 microcontroller; the same microcontroller that provides Bluetooth and WiFi. ESP-Now does not require WiFi or an Access Point. This is most useful for connecting a Base to Rover without the need for an external radio. Simply turn two SparkFun RTK products on, enable their radios, pair them, and data will be passed between units.

Additionally, ESP-Now supports point to multipoint transmissions. This means a Base can transmit to multiple Rovers simultaneously.

The downside to ESP-Now is limited range. You can expect your RTK device to be able to communicate approximately 1km (3200 ft) line of sight but any trees, buildings, or objects between the Base and Rover will degrade reception. We recommend using ESP-Now as a quick, free, and easy way to get started with Base/Rover setups. If your application needs longer RF distances consider an external serial telemetry radio plugged into the **Radio**.

The ESP-Now radio feature was added in firmware release v2.4. If the **Configure Radio** menu is not visible, consider upgrading your firmware.

## Pairing

![Pairing Menu](img/Radios/SparkFun%20RTK%20Radio%20E-Pair.png)

Pressing the Setup button (Express or Express Plus) or the Power button (Facet or Facet L-Band) will display the various submenus. Pausing on E-Pair will put the unit into ESP-Now pairing mode. If another RTK device is detected nearby in pairing mode, they will exchange MAC addresses and pair with each other. Multiple Rover units can be paired to a Base in the same fashion.

![Serial Radio menu](img/SparkFun%20RTK%20Radio%20Menu.png)

A serial menu is also available. This menu allows users to enter pairing mode, view the unit's current Radio MAC, the MAC addresses of any paired radios, as well as the ability to remove all paired radios from memory.