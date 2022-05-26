# RTK Surveyor

The RTK Surveyor is a fully enclosed, preprogrammed device. There are very few things to worry about or configure but we will cover the basics.

## Switches

[![RTK Surveyor Switches](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Switches.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Switches.jpg)

### Setup

This device can be used in three modes:

* GNSS Positioning (~30cm accuracy)
* GNSS Positioning with RTK (1.4cm accuracy)
* GNSS Base Station

When the SETUP switch is set to *Rover* the device will enter **Position** mode. RTK Surveyor will receive L1 and L2 GNSS signals from the four constellations (GPS, GLONASS, Galileo, and BeiDou) and calculate the position based on these signals. Similar to a standard grade GPS receiver, the RTK Surveyor will output industry standard NMEA sentences at 4Hz and broadcast them over any paired Bluetooth device at 115200bps. The end user will need to parse the NMEA sentences using commonly available mobile apps, GIS products, or embedded devices (there are many open source libraries). Unlike standard grade GPS receivers that have 2500m accuracy, the accuracy in this mode is approximately 300mm horizontal positional accuracy with a good grade L1/L2 antenna.

When the SETUP switch is set to *Rover* and RTCM correction data is sent into the radio port or over Bluetooth, the device will automatically enter **Positioning with RTK** mode. In this mode RTK Surveyor will receive L1/L2 signals from the antenna and correction data from a base station. The receiver will quickly (within a few seconds) obtain RTK float, then fix. The NMEA sentences will have increased accuracy of 14mm horizontal and 10mm vertical accuracy. The RTCM correction data can be obtained from a cellular link to online correction sources or over a radio link to a 2nd RTK Surveyor setup as a base station.

When the SETUP switch is set to *Base* the device will enter **Base Station** mode. This is used when the device is mounted to a fixed position (like a tripod or roof). The RTK Surveyor will initiate a survey. After 60 to 120 seconds the survey will complete and the RTK Surveyor will begin transmitting RTCM correction data out the radio port. A base is often used in conjunction with a second unit set to 'Rover' to obtain the 14mm accuracy. Said differently, if you’ve got a radio attached to the base and the rover, you’ll create an RTK system without any other setup and the Rover will output super accurate readings.

### Power

The Power switch is self explanatory. When turned on the LED will turn Green, Yellow, or Red indicating battery level. The RTK Surveyor has a built-in 1000mAh lithium polymer battery that will enable up to 4 hours of field use between charging. If more time is needed a common USB power bank can be attached boosting the field time to 40 hours.

## LEDs

[![RTK Surveyor LEDs](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_LEDs1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_LEDs1.jpg)

There are a variety of LEDs:

* **Power** - Blue when attached to power and charging / off when fully charged. Green/Yellow/Red when the Power switch is turned on indicating the state of charge of the internal battery.
* **RTK** - This white LED will be off when no RTCM correction data is received. Blinking indicates RTK Float is achieved. Solid when RTK Fix is achieved.
* **PPS** - Pulse per second. This yellow LED will blink at 1Hz when GNSS fix is achieved. You’ll see this LED begin blinking anytime the receiver detects enough satellites to obtain a rough location.
* **PAIR** - Blinks blue when waiting to be paired with over Bluetooth. Solid when a connection is active.
* **Horizontal Accuracy 100cm/10cm/1cm** - These green LEDs illuminate as the horizontal positional accuracy increases. 100cm will often be achieved in normal positioning mode with a good L1/L2 antenna. 10cm will often be achieved as the first few seconds of RTCM correction data is received, and 1cm will be achieved when a full RTK fix is calculated.
* **BASE** - This LED will blink red when the SETUP switch is set to Base and a survey is being conducted. It will turn solid red once the survey is complete and the unit begins broadcasting RTCM correction data.

## Connectors

[![RTK Surveyor Connectors and label](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors1.jpg)

*The SparkFun RTK Surveyor has a variety of connectors*

### Antenna

This SMA connector is used to connect an L1/L2 type GNSS antenna to the RTK Surveyor. Please realize that a standard GPS antenna does not receive the L2 band signals and will greatly impede the performance of the RTK Surveyor (RTK fixes are nearly impossible). Be sure to use a proper [L1/L2 antenna](https://www.sparkfun.com/products/17751).

[![RTK Surveyor SMA connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Antenna.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Antenna.jpg)

### Configure u-blox

This USB C connector is used for charging the device and/or directly configuring and inspecting the ZED-F9P GNSS receiver using [u-center](https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox/all). It’s not necessary in normal operation but is handy for tailoring the receiver to specific applications. As an added perk, the ZED-F9P can be detected automatically by some mobile phones and tablets. If desired, the receiver can be directly connected to a compatible phone or tablet removing the need for a Bluetooth connection.

[![RTK Surveyor u-blox connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_u-blox.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_u-blox.jpg)


### USB Configure ESP32

This USB C connector is used for charging the device, configuring the device, and reprogramming the ESP32. Various debug messages are printed to this port at 115200bps and a serial menu can be opened to configure advanced settings. 

[![RTK Surveyor ESP32 connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_ESP32.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_ESP32.jpg)

 
### Radio

This 4-pin JST connector is used to allow RTCM correction data to flow into the device when it is acting as a rover or out of the device when it is acting as a base. You will most likely connect this port to one of our [Serial Telemetry Radios](https://www.sparkfun.com/products/19032) if you don’t have access to a correction source on the internet. The pinout is <b>3.5-5.5V</b> / TX / RX / GND. The connector is a 4-pin locking 1.25mm JST SMD connector (part#: SM04B-GHS-TB, mating connector part#: GHR-04V-S). <b>3.5V to 5.5V</b> is provided by this connector to power a radio with a voltage that depends on the power source. If USB is connected to the RTK Surveyor then voltage on this port will be <b>5V</b> (+/-10%). If running off of the internal battery then voltage on this port will vary with the battery voltage (<b>3.5V</b> to <b>4.2V</b> depending on the state of charge). While the port is capable of sourcing up to 2 amps, we do not recommend more than 500mA. This port should not be connected to a power source.


[![RTK Surveyor Radio connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Radio.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Radio.jpg)
 

### Data

This 4-pin JST connector is used to output NMEA sentences over 115200bps serial. Most applications will send the NMEA position data over Bluetooth. Alternatively, this port can be useful for sending position data to an embedded microcontroller or single board computer. The pinout is <b>3.3V</b> / TX / RX / GND. The connector is a 4-pin locking 1.25mm JST SMD connector (part#: SM04B-GHS-TB, mating connector part#: GHR-04V-S). <b>3.3V</b> is provided by this connector to power a remote device if needed. While the port is capable of sourcing up to 600mA, we do not recommend more than 300mA. This port should not be connected to a power source.

[![RTK Surveyor Data connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Data.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Data.jpg)

### microSD

This slot accepts standard microSD cards up to 32GB formatted for FAT16 or FAT32. Logging NMEA and RAWX data at up to 4Hz is supported for all constellations.

[![RTK Surveyor microSD connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_microSD.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_microSD.jpg)

### Qwiic

This 4-pin [Qwiic connector](https://www.sparkfun.com/qwiic) exposes the I2C bus of the ESP32 WROOM module. Currently, there is no firmware support for adding I2C devices to the RTK Surveyor but support may be added in the future.

[![RTK Surveyor Qwiic connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Qwiic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Connectors_-_Qwiic.jpg)

## Power

The RTK Surveyor has a built in 1000mAh battery and consumes approximately 240mA worst case with Bluetooth connection active, GNSS fully tracking, and a 500mW radio broadcasting. This will allow for 4 hours of use in the field. If more time is needed in the field a standard USB power bank can be attached. If a 10,000mAh bank is attached one can estimate 30 hours of run time assuming 25% is lost to efficiencies of the power bank and charge circuit within RTK Surveyor.

The RTK Surveyor can be charged from any USB port or adapter. The charge circuit is rated for 1000mA so USB 2.0 ports will charge at 500mA and USB 3.0+ ports will charge at 1A. 

To quickly view the state of charge, turn on the unit. A green LED indicates > 50% charge remaining. A yellow LED indicates > 10% charge remaining. A red LED indicates less than 10% charge remaining.

## Advanced Features

The RTK Surveyor is a hacker’s delight. Under the hood of the RTK Surveyor is an ESP32 WROOM connected to a ZED-F9P as well as some peripheral hardware (LiPo fuel gauge, microSD, etc). It is programmed in Arduino and can be tailored by the end user to fit their needs.

[![RTK Survayor Schematic](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Schematic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_GPS_RTK_Surveyor-v13.pdf)

*Click on the image to get a closer look at the Schematic*

### ZED-F9P GNSS Receiver

The [ZED-F9P GNSS receiver](https://www.sparkfun.com/products/16481) is configured over I<sup>2</sup>C and uses two UARTs to output NMEA (UART1) and input/output RTCM (UART2). 

[![ZED-F9P GNSS Receiver](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_ZED-F9P.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_ZED-F9P.jpg)

Two internal slide switches control the flow of NMEA and RTCM traffic between the external connectors and the internal BT UART used on the ESP32. Ostensibly the *Bluetooth Broadcast* switch can be set to pipe RTCM data to the ESP32’s UART (instead of NMEA) so that correction data can be transmitted over Bluetooth. Point to point Bluetooth radio support is not supported because the useful range of Bluetooth is too short for most RTK applications but may be helpful in some advanced applications.

[![Communication switches](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_NMEA_Switches.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_NMEA_Switches.jpg)

### ESP32

The [ESP32](https://www.sparkfun.com/products/15663) uses a standard USB to serial conversion IC ([CH340](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all)) to program the device. You can use the ESP32 core for Arduino or Espressif’s [IoT Development Framework (IDF)](https://www.espressif.com/en/support/download/all).

The CH340 automatically resets and puts the ESP32 into bootload mode as needed. However, the reset pin of the ESP32 is brought out to an external 2-pin 0.1” footprint if an external reset button is needed.

<div class="alert alert-info" role="alert">
<strong>Note:</strong> 
If you've never connected a CH340 device to your computer before, you may need to install drivers for the USB-to-serial converter. Check out our section on <a href="https://learn.sparkfun.com/tutorials/sparkfun-serial-basic-ch340c-hookup-guide#drivers-if-you-need-them">"How to Install CH340 Drivers"</a> for help with the installation.
</div>

[![ESP32 on SparkFun RTK Surveyor](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_ESP32-1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_ESP32-1.jpg)


### Measurement Jumpers

To facilitate the measurement of run, charge, and quiescent currents, two measurement jumpers are included. These are normally closed jumpers combined with a 2-pin 0.1” footprint. To take a measurement, cut the jumper and install a 2-pin header and use [banana to IC hook](https://www.sparkfun.com/products/506) cables to a DMM.

[![Measurement Jumpers on SparkFun RTK Surveyor](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_Measurement_Jumpers.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_Measurement_Jumpers.jpg)

### LiPo and Charging

The RTK Surveyor houses a standard [1000mAh 3.7V LiPo](https://www.sparkfun.com/products/13813). The charge circuit is set to 1A so with an appropriate power source, charging an empty battery should take roughly one hour. USB C on the RTK Surveyor is configured for 2A draw so if the user attaches to a USB 3.0 port, the charge circuit should operate near the 1A max. If a user attaches to a USB 2.0 port, the charge circuit will operate at 500mA. 

[![LiPo and Charging on SparkFun RTK Surveyor](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_LiPo_Charging1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_LiPo_Charging1.jpg)


### MAX17048 Fuel Gauge

The [MAX17048](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/MAX17048-MAX17049.pdf) is a simple to use fuel gauge IC that gives the user a statement of charge (SOC) that is basically a 0 to 100% report. The MAX17048 has a sophisticated algorithm to figure out what the SOC is based on cell voltage that is beyond the scope of this tutorial but for our purposes, allows us to control the color of the power LED.

[![MAX17048 Fuel Gauge on SparkFun RTK Surveyor](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_MAX17048_Fuel_Gauge1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_MAX17048_Fuel_Gauge1.jpg)

### Qwiic

A [Qwiic connector](https://www.sparkfun.com/qwiic) is exposed on the end of the unit. This allows connection to the I<sup>2</sup>C bus on the ESP32. Currently the stock RTK Surveyor does not support any additional Qwiic sensors or display but users may add support for their own application.

[![Qwiic Connector on SparkFun RTK Surveyor](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_Qwiic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_Qwiic.jpg)

### microSD

A microSD socket is situated on the ESP32 SPI bus. Any microSD up to 32GB is supported. RTK Surveyor supports RAWX and NMEA logging to the SD card. Max logging time can also be set (default is 10 hours) to avoid multi-gigabyte text files. For more information about RAWX and doing PPP please see [this tutorial](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data).

[![microSD socket on SparkFun RTK Surveyor](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_microSD.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_Internal_-_microSD.jpg)