# Facet Hardware

## Hardware Overview

The RTK Facet is a fully enclosed, preprogrammed device. There are very few things to worry about or configure but we will cover the basics.

### **Power/Setup Button**

[![RTK Facet Front Face](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Front_Face_All.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Front_Face_All.jpg)

The RTK Facet has one button used for both **Power** and **Setup** for in-field configuration changes. Pressing and holding the Power button will cause it to power on or off. Short pressing the button will cause the RTK Facet to change modes.

This device can be used in four modes:

* GNSS Positioning (~30cm accuracy) - also known as 'Rover'
* GNSS Positioning with RTK (1.4cm accuracy) - also known as 'Rover with RTK Fix'
* GNSS Base Station
* GNSS Base Station NTRIP Server

At *Power On* the device will enter Rover or Base mode; whichever state the device was in at the last power down. When the POWER/SETUP button is pressed momentarily, a menu is presented to change the RTK Facet to *Rover* or *Base* mode. The display will indicate the change with a small car or flag icon.

In *Rover* mode the RTK Facet will receive L1 and L2 GNSS signals from the four constellations (GPS, GLONASS, Galileo, and BeiDou) and calculate the position based on these signals. Similar to a standard grade GPS receiver, the RTK Facet will output industry standard NMEA sentences at 4Hz and broadcast them over any paired Bluetooth device. The end user will need to parse the NMEA sentences using commonly available mobile apps, GIS products, or embedded devices (there are many open source libraries). Unlike standard grade GPS receivers that have 2500m accuracy, the accuracy in this mode is approximately 300mm horizontal positional accuracy with a good grade L1/L2 antenna.

When the device is in *Rover* mode and RTCM correction data is sent over Bluetooth or into the radio port, the device will automatically enter **Positioning with RTK** mode. In this mode RTK Facet will receive L1/L2 signals from the antenna and correction data from a base station. The receiver will quickly (within a second) obtain RTK float, then fix. The NMEA sentences will have increased accuracy of 14mm horizontal and 10mm vertical accuracy. The RTCM correction data is most easily obtained over the internet using a free app on your phone (see SW Maps or Lefebure NTRIP) and sent over Bluetooth to the RTK Facet but RTCM can also be delivered over an external cellular or radio link to a 2nd RTK Facet setup as a base station.

In *Base* mode the device will enter *Base Station* mode. This is used when the device is mounted to a fixed position (like a tripod or roof). The RTK Facet will initiate a survey. After 60 to 120 seconds the survey will complete and the RTK Facet will begin transmitting RTCM correction data out the radio port. A base is often used in conjunction with a second RTK Facet (or RTK Surveyor) unit set to 'Rover' to obtain the 14mm accuracy. Said differently, the Base sits still and sends correction data to the Rover so that the Rover can output a really accurate position. You’ll create an RTK system without any other setup.

#### **Power**

[![RTK Facet startup display with firmware version number](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Display_On_Off.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Display_On_Off.jpg)

*RTK Facet startup display with firmware version number*

The Power button turns on and off the unit. Press and hold the power button until the display illuminates. Press and hold the power button at any time to turn the unit off. 

[![RTK Facet showing the battery level](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_Fixed.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_Fixed.jpg)

*RTK Facet showing the battery level*

The RTK Facet has a large, built-in 6000mAh lithium polymer battery that will enable over 25 hours of field use between charging. If more time is needed a common USB power bank can be attached boosting the field time to any amount needed.


### **Charge LED**

[![RTK Facet Charge LED](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Front_Face_-_Charge_LED.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Front_Face_-_Charge_LED.jpg)

The Charge LED is located on the front face. It will illuminate any time there is an external power source and will turn off when the internal battery is charged. With the unit fully powered down, charging takes approximately 6 hours from a 1A wall supply or 12 hours from a standard USB port. The RTK Facet can run while being charged but it increases the charge time. Using an external USB battery bank to run the device for extended periods or running the device on a permanent wall power source is supported.

### **Connectors**

[![RTK Facet Connectors](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports.jpg)

*The SparkFun RTK Facet connectors shown with the dust cover removed*

There are a variety of connectors protected by a dust flap.

#### **USB:**

[![RTK Facet USB C Connector](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_USB.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_USB.jpg)

This USB C connector is used for three purposes:

* Charging the device
* Configuring the RTK Facet, and reprogramming the ESP32
* Directly configuring and inspecting the ZED-F9P GNSS receiver

There is a USB hub built into the RTK Facet. When you attach the device to your computer it will enumerate as two COM ports.

[![Two COM ports from one USB device](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Multiple_COM_Ports.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Multiple_COM_Ports.jpg)

In the image above, the `USB Serial Device` is the ZED-F9P and the `USB-SERIAL CH340` is the ESP32.

<div class="alert alert-info" role="alert">
<strong>Don't See 'USB-Serial CH340'?</strong> If you've never connected a CH340 device to your computer before, you may need to install drivers for the USB-to-serial converter. Check out our section on <a href="https://learn.sparkfun.com/tutorials/sparkfun-serial-basic-ch340c-hookup-guide#drivers-if-you-need-them">"How to Install CH340 Drivers"</a> for help with the installation.
</div>

<div class="alert alert-info" role="alert">
<strong>Don't See 'USB Serial Device'?</strong> The first time a u-blox module is connected to a computer you may need to adjust the COM driver. Check out our section on <a href="https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox#install-drivers">"How to Install u-blox Drivers"</a> for help with the installation.
</div>

Configuring the RTK Facet can be done over the *USB-Serial CH340* COM port via serial text menu. Various debug messages are printed to this port at 115200bps and a serial menu can be opened to configure advanced settings. 

Configuring the ZED-F9P can be configured over the *USB Serial Device* port using [u-center](https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox/all). It’s not necessary in normal operation but is handy for tailoring the receiver to specific applications. As an added perk, the ZED-F9P can be detected automatically by some mobile phones and tablets. If desired, the receiver can be directly connected to a compatible phone or tablet removing the need for a Bluetooth connection.

#### **Radio:**

[![RTK Facet Radio Connector](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_Radio.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_Radio.jpg)

This port is used when an external cellular or radio link is needed. This port is *not* used if you transfer RTCM from your phone to the RTK Facet over Bluetooth.

This 4-pin JST connector can be used to allow RTCM correction data to flow into the device when it is acting as a rover or out of the device when it is acting as a base. The connector is a 4-pin locking 1.25mm JST SMD connector (part#: SM04B-GHS-TB, mating connector part#: GHR-04V-S). The RTK Facet comes with a cable to interface to this connector but [additional cables](https://www.sparkfun.com/products/17239) can be purchased. You will most likely connect this port to one of our [Serial Telemetry Radios](https://www.sparkfun.com/products/19032) if you don’t have access to a correction source on the internet. The pinout is **3.5-5.5V** / TX / RX / GND  from left to right as pictured. **3.5V to 5.5V** is provided by this connector to power a radio with a voltage that depends on the power source. If USB is connected to the RTK Facet then voltage on this port will be **5V** (+/-10%). If running off of the internal battery then voltage on this port will vary with the battery voltage (**3.5V** to **4.2V** depending on the state of charge). This port is capable of sourcing up to 600mA and is protected by a PTC (resettable fuse). This port should not be connected to a power source.

#### **Data:**

[![RTK Facet Data Port](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_Data.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_Data.jpg)

This port is used when an external system is connected such as a rover, car, timing equipment, camera triggers, etc. This port is *not* used if you transfer NMEA positional data to your phone from the RTK Facet over Bluetooth. 

This 4-pin JST connector is used to output and input a variety of data to the RTK Facet. The connector is a 4-pin locking 1.25mm JST SMD connector (part#: SM04B-GHS-TB, mating connector part#: GHR-04V-S). The RTK Facet comes with a cable to interface to this connector but [additional cables](https://www.sparkfun.com/products/17240) can be purchased. 

Internally the Data connector is connected to a digital mux allowing one of four software selectable setups:

* **NMEA** - The TX pin outputs any enabled messages (NMEA, UBX, and RTCM) at a default of 460,800bps (configurable 9600 to 921600bps). The RX pin can receive RTCM for RTK and can also receive UBX configuration commands if desired.
* **PPS/Trigger** - The TX pin outputs the pulse-per-second signal that is accurate to 30ns RMS. The RX pin is connected to the EXTINT pin on the ZED-F9P allowing for events to be measured with incredibly accurate nano-second resolution. Useful for things like audio triangulation. See the Timemark section of the ZED-F9P integration for more information.
* **I2C** - The TX pin operates as SCL, RX pin as SDA on the I2C bus. This allows additional sensors to be connected to the I2C bus.
* **GPIO** - The TX pin operates as a DAC capable GPIO on the ESP32. The RX pin operates as a ADC capable input on the ESP32. This is useful for custom applications.

Most applications do not need to utilize this port and will send the NMEA position data over Bluetooth. This port can be useful for sending position data to an embedded microcontroller or single board computer. The pinout is **3.3V** / TX / RX / GND. **3.3V** from left to right as pictured, which is provided by this connector to power a remote device if needed. While the port is capable of sourcing up to 600mA, we do not recommend more than 300mA. This port should not be connected to a power source.

#### **microSD:**

[![RTK Facet microSD connector](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_microSD.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_microSD.jpg)

This slot accepts standard microSD cards up to 32GB formatted for FAT16 or FAT32. Logging any of 67 messages at up to 4Hz is supported for all constellations.

The following 67 messages are supported for logging:

<table class="table">
 <table>
  <COLGROUP><COL WIDTH=200><COL WIDTH=200><COL WIDTH=200></COLGROUP>
  <tr>
	<td>&#8226; NMEA-GSA</td>
	<td>&#8226; NMEA-GST</td>
	<td>&#8226; NMEA-GSV</td>
  </tr>
  <tr>
	<td>&#8226; NMEA-RMC</td>
	<td>&#8226; NMEA-VLW</td>
	<td>&#8226; NMEA-VTG</td>
  </tr>
  <tr>
    <td>&#8226; NMEA-ZDA</td>
    <td>&#8226; NAV-CLOCK</td>
    <td>&#8226; NAV-DOP</td>
  </tr>
  <tr>
    <td>&#8226; NAV-EOE</td>
    <td>&#8226; NAV-GEOFENCE</td>
    <td>&#8226; NAV-HPPOSECEF</td>
  </tr>
  <tr>
    <td>&#8226; NAV-HPPOSLLH</td>
    <td>&#8226; NAV-ODO</td>
    <td>&#8226; NAV-ORB</td>
  </tr>
  <tr>
    <td>&#8226; NAV-POSECEF</td>
    <td>&#8226; NAV-POSLLH</td>
    <td>&#8226; NAV-PVT</td>
  </tr>
  <tr>
    <td>&#8226; NAV-RELPOSNED</td>
    <td>&#8226; NAV-SAT</td>
    <td>&#8226; NAV-SIG</td>
  </tr>
  <tr>
    <td>&#8226; NAV-STATUS</td>
    <td>&#8226; NAV-SVIN</td>
    <td>&#8226; NAV-TIMEBDS</td>
  </tr>
  <tr>
    <td>&#8226; NAV-TIMEGAL</td>
    <td>&#8226; NAV-TIMEGLO</td>
    <td>&#8226; NAV-TIMEGPS</td>
  </tr>
  <tr>
    <td>&#8226; NAV-TIMELS</td>
    <td>&#8226; NAV-TIMEUTC</td>
    <td>&#8226; NAV-VELECEF</td>
  </tr>
  <tr>
    <td>&#8226; NAV-VELNED</td>
    <td>&#8226; RXM-MEASX</td>
    <td>&#8226; RXM-RAWX</td>
  </tr>
  <tr>
    <td>&#8226; RXM-RLM</td>
    <td>&#8226; RXM-RTCM</td>
    <td>&#8226; RXM-SFRBX</td>
  </tr>
  <tr>
    <td>&#8226; MON-COMMS</td>
    <td>&#8226; MON-HW2</td>
    <td>&#8226; MON-HW3</td>
  </tr>
  <tr>
    <td>&#8226; MON-HW</td>
    <td>&#8226; MON-IO</td>
    <td>&#8226; MON-MSGPP</td>
  </tr>
  <tr>
    <td>&#8226; MON-RF</td>
    <td>&#8226; MON-RXBUF</td>
    <td>&#8226; MON-RXR</td>
  </tr>
  <tr>
    <td>&#8226; MON-TXBUF</td>
    <td>&#8226; TIM-TM2</td>
    <td>&#8226; TIM-TP</td>
  </tr>
  <tr>
    <td>&#8226; TIM-VRFY</td>
    <td>&#8226; RTCM3x-1005</td>
    <td>&#8226; RTCM3x-1074</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-1077</td>
    <td>&#8226; RTCM3x-1084</td>
    <td>&#8226; RTCM3x-1087</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-1094</td>
    <td>&#8226; RTCM3x-1097</td>
    <td>&#8226; RTCM3x-1124</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-1127</td>
    <td>&#8226; RTCM3x-1230</td>
    <td>&#8226; RTCM3x-4072-0</td>
  </tr>
  <tr>
    <td>&#8226; RTCM3x-4072-1</td>
    <td></td>
    <td></td>
  </tr>
</table></table>


#### **Qwiic:**

[![RTK Facet Qwiic connector](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_Qwiic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Ports_-_Qwiic.jpg)

This 4-pin [Qwiic connector](https://www.sparkfun.com/qwiic) exposes the I2C bus of the ESP32 WROOM module. Currently, there is no firmware support for adding I<sup>2</sup>C devices to the RTK Facet but support may be added in the future.

#### **Antenna:**

[![Internal RTK Facet Antenna](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_L1_L2_Antenna_-_1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_L1_L2_Antenna_-_1.jpg)

It's built in! Housed under the dome of the RTK Facet is a surveyor grade L1/L2 antenna. It is the same element found within our [GNSS Multi-Band L1/L2 Surveying Antenna](https://www.sparkfun.com/products/17751). Its datasheet is available [here](https://cdn.sparkfun.com/assets/b/4/6/d/e/TOP106_GNSS_Antenna.pdf).

[![SparkFun RTK Facet Antenna Reference Point](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_ARP-1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_ARP-1.jpg)

*SparkFun RTK Facet Antenna Reference Points*

The built in antenna has an [ARP](https://geodesy.noaa.gov/ANTCAL/FAQ.xhtml#faq4) of 61.4mm from the base to the measuring point of the L1 antenna and an ARP of 57.4mm to the measuring point of the L2 antenna.

### **Power**

[![RTK Facet Display showing three battery bars](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)

*RTK Facet Display showing three battery bars*

The RTK Facet has a built in 6000mAh battery and consumes approximately 240mA worst case with Bluetooth connection active and GNSS fully tracking. This will allow for around 25 hours of use in the field. If more time is needed in the field a standard USB power bank can be attached. If a 10,000mAh bank is attached one can estimate 56 hours of run time assuming 25% is lost to efficiencies of the power bank and charge circuit within RTK Facet.

The RTK Facet can be charged from any USB port or adapter. The charge circuit is rated for 1000mA so USB 2.0 ports will charge at 500mA and USB 3.0+ ports will charge at 1A. 

To quickly view the state of charge, turn on the unit. The battery icon will indicate the following:

* 3 bars: >75% capacity remain
* 2 bars: >50% capacity remain
* 1 bar: >25% capacity remain
* 0 bars: <25% capacity remain

## Advanced Features

[![RTK Facet Circuit Boards](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/RTK_Facet_Photos-07.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/RTK_Facet_Photos-07.jpg)

*The boards that make up the RTK Facet*

The RTK Facet is a hacker’s delight. Under the hood of the RTK Facet is an ESP32 WROOM connected to a ZED-F9P as well as some peripheral hardware (LiPo fuel gauge, microSD, etc). It is programmed in Arduino and can be tailored by the end user to fit their needs.

[![RTK Facet Schematic](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Main_Board_Schematic_Image.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Main_Schematic.pdf)

*Click on the image to get a closer look at the Schematic!*

[![Internal RTK Facet Antenna](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_L1_L2_Antenna_-_1.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_L1_L2_Antenna_-_1.jpg)

*The Facet with three sub boards, the battery, and antenna*

### ZED-F9P GNSS Receiver

The [ZED-F9P GNSS receiver](https://www.sparkfun.com/products/16481) is configured over I<sup>2</sup>C and uses two UARTs to output NMEA (UART1) and input/output RTCM (UART2). In general, the ESP32 harvests the data from the ZED-F9Ps UART1 for Bluetooth transmission and logging to SD.

### ESP32

The [ESP32](https://www.sparkfun.com/products/15663) uses a standard USB to serial conversion IC ([CH340](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all)) to program the device. You can use the ESP32 core for Arduino or Espressif’s [IoT Development Framework (IDF)](https://www.espressif.com/en/support/download/all).

The CH340 automatically resets and puts the ESP32 into bootload mode as needed. However, the reset pin of the ESP32 is brought out to an external 2-pin 0.1” footprint if an external reset button is needed.

<div class="alert alert-info" role="alert">
<strong>Note:</strong> 
If you've never connected a CH340 device to your computer before, you may need to install drivers for the USB-to-serial converter. Check out our section on <a href="https://learn.sparkfun.com/tutorials/sparkfun-serial-basic-ch340c-hookup-guide#drivers-if-you-need-them">"How to Install CH340 Drivers"</a> for help with the installation.
</div>

### LiPo and Charging

The RTK Facet houses a standard [6000mAh 3.7V LiPo](https://www.sparkfun.com/products/13856). The charge circuit is set to 1A so with an appropriate power source, charging an empty battery should take a little over six hours. USB C on the RTK Facet is configured for 2A draw so if the user attaches to a USB 3.0 port, the charge circuit should operate near the 1A max. If a user attaches to a USB 2.0 port, the charge circuit will operate at 500mA. This charge circuit also incorporates a 42C upper temperature cutoff to insure the LiPo cannot be charged in dangerous conditions.

### Fuel Gauge and Accelerometer

The [MAX17048](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/MAX17048-MAX17049.pdf) is a simple to use fuel gauge IC that gives the user a statement of charge (SOC) that is basically a 0 to 100% report. The MAX17048 has a sophisticated algorithm to figure out what the SOC is based on cell voltage that is beyond the scope of this tutorial but for our purposes, allows us to reliably view the battery level when the unit is on. 

The RTK Facet also incorporates a the [LIS2DH12](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/lis2dh12_accelerometer_datasheet.pdf) triple-axis accelerometer to aid in leveling in the field.

### Qwiic

An internal [Qwiic connector](https://www.sparkfun.com/qwiic) is included in the unit for future expansion. Currently the stock RTK Facet does not support any additional Qwiic sensors or display but users may add support for their own application.

### microSD

A microSD socket is situated on the ESP32 SPI bus. Any microSD up to 32GB is supported. RTK Facet supports RAWX and NMEA logging to the SD card. Max logging time can also be set (default is 24 hours) to avoid multi-gigabyte text files. For more information about RAWX and doing PPP please see [this tutorial](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data).

### Data Port and Digital Mux

The 74HC4052 analog mux controls which digital signals route to the external Data port. This allows a variety of custom end user applications. The most interesting of which is event logging. Because the ZED-F9P has microsecond accuracy of the incoming digital signal, custom firmware can be created to triangulate an event based on the receiver's position and the time delay between multiple captured events. Currently, TM2 event logging is supported. 

Additionally, this mux can be configured to connect ESP pin 26 (DAC capable) and pin 39 (ADC capable) for end user custom applications.