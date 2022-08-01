# Configure with WiFi

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

Starting with firmware v1.7, WiFi-based configuration is supported. For more information about updating the firmware on your device, please see [Updating RTK Firmware](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/).

The RTK device will present a webpage that is viewable from either a desktop/laptop with WiFi or a cell phone. For advanced configurations, a desktop is recommended. For quick in-field changes, a cell phone works great.

[![Desktop vs Phone display size configuration](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Desktop_vs_Phone_Config.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Desktop_vs_Phone_Config.jpg)

*Desktop vs Phone display size configuration*

## RTK Express / Express Plus / Facet
To get into WiFi configuration follow these steps:

1. Power on the RTK Express, Express Plus, or Facet.
2. Once the device has started press the Setup button repeatedly until the *Config* menu is highlighted.
3. The display will blink a WiFi icon indicating it is waiting for incoming connections.
4. Connect to WiFi network named ‘RTK Config’.
5. Open a browser (Chrome is preferred) and type **192.168.4.1** into the address bar.

[![Display showing IP address](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Display_WiFi_Config.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Display_WiFi_Config.jpg)

*Device ready for cellphone configuration*

## RTK Surveyor

To get into WiFi configuration follow these steps:

1. Power the RTK Surveyor on in Rover mode.
2. Once the device has started the Bluetooth status LED should be blinking at 1Hz. Now toggle the **SETUP** switch from Base and back to Rover within 1 second. If successful, the Bluetooth status LED will begin fading in/out. The device is now broadcasting as a WiFi access point.
3. Connect to WiFi network named ‘RTK Config’.
4. Open a browser (Chrome is preferred) and type **192.168.4.1** into the address bar.

## Connecting to WiFi Network

[![Discovered WiFi networks](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_Networks.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_Networks.jpg)

*The WiFi network RTK Config as seen from a cellphone*

Note: Upon connecting, your phone may warn you that this WiFi network has no internet. That's ok. Stay connected to the network and open a browser. If you still have problems turn off Mobile Data so that the phone does not default to cellular for internet connectivity and instead connects to the RTK Device.

[![Webpage showing the RTK Configuration options](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Main_Page.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Main_Page.jpg)

*Connected to the RTK WiFi Setup Page*

Clicking on the category 'carrot' will open or close that section. Clicking on an ‘i’ will give you a brief description of the options within that section.

[![Firmware highlighted](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Main_Page_-_Firmware.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Main_Page_-_Firmware.jpg)

*This unit has firmware version 1.8 and a ZED-F9P receiver*

Please note that the firmware for the RTK device and the firmware for the ZED receiver is shown at the top of the page. This can be helpful when troubleshooting or requesting new features.

## Saving and Exit

[![Save and Exit buttons](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_System_Save_Exit.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_System_Save_Exit.jpg)

Once settings are input, please press ‘Save Configuration’. This will validate any settings, show any errors that need adjustment, and send the settings to the unit. The page will remain active until the user presses ‘Exit to Rover Mode’ at which point the unit will exit WiFi configuration and return to standard Rover mode.
