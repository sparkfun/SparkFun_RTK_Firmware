# Connecting Bluetooth

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

SparkFun RTK products transmit full NMEA sentences over Bluetooth serial port profile (SPP) at 4Hz and 115200bps. This means that nearly any GIS application that can receive NMEA data over a serial port (almost all do) can be used with the RTK Express. As long as your device can open a serial port over Bluetooth (also known as SPP) your device can retrieve industry-standard NMEA positional data. The following steps show how to connect an external tablet, or cell phone to the RTK device so that any serial port-based GIS application can be used.

## Android

[![Pairing with the RTK Express over Bluetooth](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/8/5/7/RTK_Express_-_Bluetooth_Connect.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/RTK_Express_-_Bluetooth_Connect.jpg)

*Pairing with the 'Express Rover-5556' over Bluetooth*

Open Android's system settings and find the 'Bluetooth' or 'Connected devices' options. Scan for devices and pair with the device in the list that matches the Bluetooth MAC address on your RTK device.

When powered on, the RTK product will broadcast itself as either '[Platform] Rover-5556' or '[Platform] Base-5556' depending on which state it is in. [Platform] is Facet, Express, Surveyor, etc. Discover and pair with this device from your phone or tablet. Once paired, open SW Maps. 

![Bluetooth MAC address B022 is shown in the upper left corner](img/SparkFun%20RTK%20Rover%20Display.png)

*Bluetooth MAC address B022 is shown in the upper left corner*

**Note:** *B022* is the last four digits of your unit's MAC address and will be unique to the device in front of you. This is helpful in case there are multiple RTK devices within Bluetooth range.

## Windows

Open settings and navigate to Bluetooth. Click **Add device**.

![Adding Bluetooth Device](img/Bluetooth/SparkFun%20RTK%20Software%20-%20Add%20Bluetooth%20Device.jpg)

*Adding Bluetooth Device*

Click Bluetooth 'Mice, Keyboards, ...'

![Viewing available Bluetooth Devices](img/Bluetooth/SparkFun%20RTK%20Software%20-%20Add%20Bluetooth%20Device%202.jpg)

*Viewing available Bluetooth Devices*

Click on the RTK device. When powered on, the RTK product will broadcast itself as either '[Platform] Rover-5556' or '[Platform] Base-5556' depending on which state it is in. [Platform] is Facet, Express, Surveyor, etc. Discover and pair with this device from your phone or tablet. Once paired, open SW Maps. 

![Bluetooth MAC address B022 is shown in the upper left corner](img/SparkFun%20RTK%20Rover%20Display.png)

*Bluetooth MAC address B022 is shown in the upper left corner*

**Note:** *B022* is the last four digits of your unit's MAC address and will be unique to the device in front of you. This is helpful in case there are multiple RTK devices within Bluetooth range.

![Bluetooth Connection Success](img/Bluetooth/SparkFun%20RTK%20Software%20-%20Add%20Bluetooth%20Device%203.jpg)

*Bluetooth Connection Success*

The device will begin pairing. After a few seconds, Windows should report that you are ready to go. 

![Bluetooth COM ports](img/Bluetooth/SparkFun%20RTK%20Software%20-%20Add%20Bluetooth%20Device%204.jpg)

*Bluetooth COM ports*

The device is now paired and a series of COM ports will be added under 'Device Manager'. 

![NMEA received over the Bluetooth COM port](img/Bluetooth/SparkFun%20RTK%20Software%20-%20Add%20Bluetooth%20Device%205.jpg)

*NMEA received over the Bluetooth COM port*

If necessary, you can open a terminal connection to one of the COM ports. Because the Bluetooth driver creates multiple COM ports, it's impossible to tell which is the serial stream so it's easiest to just try each port until you see a stream of NMEA sentences (shown above). You're all set! Be sure to close out the terminal window so that other software can use that COM port.
