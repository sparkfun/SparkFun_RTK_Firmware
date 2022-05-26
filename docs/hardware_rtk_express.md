# RTK Express

The RTK Express is a fully enclosed, preprogrammed device. There are very few things to worry about or configure but we will cover the basics.

### **Buttons**

[![RTK Express Buttons](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/RTK_Express_-_Buttons.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/RTK_Express_-_Buttons.jpg)

The RTK Express uses two buttons, **Power** and **Setup** for in-field configuration.

#### **Setup**

This device can be used in four modes:

* GNSS Positioning (~30cm accuracy) - also known as 'Rover'
* GNSS Positioning with RTK (1.4cm accuracy) - also known as 'Rover with RTK Fix'
* GNSS Base Station
* GNSS Base Station NTRIP Server

At power on the device will enter Rover or Base mode; whichever state the device was in at the last power down. When the SETUP button is pressed the RTK Express will toggle between *Rover* and *Base* mode. The display will indicate the change with a small car or flag icon.

In *Rover* mode the RTK Express will receive L1 and L2 GNSS signals from the four constellations (GPS, GLONASS, Galileo, and BeiDou) and calculate the position based on these signals. Similar to a standard grade GPS receiver, the RTK Express will output industry standard NMEA sentences at 4Hz and broadcast them over any paired Bluetooth device. The end user will need to parse the NMEA sentences using commonly available mobile apps, GIS products, or embedded devices (there are many open source libraries). Unlike standard grade GPS receivers that have 2500m accuracy, the accuracy in this mode is approximately 300mm horizontal positional accuracy with a good grade L1/L2 antenna.

When the device is in *Rover* mode and RTCM correction data is sent into the radio port or over Bluetooth, the device will automatically enter **Positioning with RTK** mode. In this mode RTK Express will receive L1/L2 signals from the antenna and correction data from a base station. The receiver will quickly (within a few seconds) obtain RTK float, then fix. The NMEA sentences will have increased accuracy of 14mm horizontal and 10mm vertical accuracy. The RTCM correction data can be obtained from a cellular link to online correction sources or over a radio link to a 2nd RTK Express setup as a base station.

In *Base* mode the device will enter *Base Station* mode. This is used when the device is mounted to a fixed position (like a tripod or roof). The RTK Express will initiate a survey. After 60 to 120 seconds the survey will complete and the RTK Express will begin transmitting RTCM correction data out the radio port. A base is often used in conjunction with a second RTK Express (or RTK Surveyor) unit set to 'Rover' to obtain the 14mm accuracy. Said differently, the Base sits still and sends correction data to the Rover so that the Rover can output a really accurate position. You’ll create an RTK system without any other setup.

#### **Power**

[![RTK Express startup display with firmware version number](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Startup_Shut_Down.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Startup_Shut_Down.jpg)

*RTK Express startup display with firmware version number*


The Power button turns on and off the unit. Press and hold the power button until the display illuminates. Press and hold the power button at any time to turn the unit off. 

[![RTK Express showing the battery level](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_Fixed.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_Fixed.jpg)

*RTK Express showing the battery level*

The RTK Express has a built-in 1300mAh lithium polymer battery that will enable over 5 hours of field use between charging. If more time is needed a common USB power bank can be attached boosting the field time to 40 hours.

### **Charge LED**

[![RTK Express Charge LED](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Charge_LED.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Charge_LED.jpg)

The Charge LED is located above the **Power** button. It will illuminate any time there is an external power source and will turn off when the internal battery is charged. With the unit fully powered down, charging takes approximately 1.5 hours from a 1A wall supply or 3 hours from a standard USB port. The RTK Express can run while being charged but it increases the charge time. Using an external USB battery bank to run the device for extended periods or running the device on a permanent wall power source is supported.

### **Connectors**

[![RTK Express Connectors and label](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors.jpg)

*The SparkFun RTK Express has a variety of connectors*


#### **Antenna:**

This SMA connector is used to connect an L1/L2 type GNSS antenna to the RTK Express. Please realize that a standard GPS antenna does not receive the L2 band signals and will greatly impede the performance of the RTK Express (RTK fixes are nearly impossible). Be sure to use a proper [L1/L2 antenna](https://www.sparkfun.com/products/17751).

[![RTK Express SMA connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-SMA.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-SMA.jpg)

#### **Configure u-blox:**

This USB C connector is used for charging the device and/or directly configuring and inspecting the ZED-F9P GNSS receiver using [u-center](https://learn.sparkfun.com/tutorials/getting-started-with-u-center-for-u-blox/all). It’s not necessary in normal operation but is handy for tailoring the receiver to specific applications. As an added perk, the ZED-F9P can be detected automatically by some mobile phones and tablets. If desired, the receiver can be directly connected to a compatible phone or tablet removing the need for a Bluetooth connection.

[![RTK Express u-blox connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-u-blox.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-u-blox.jpg)


#### **Configure ESP32:**

This USB C connector is used for charging the device, configuring the device, and reprogramming the ESP32. Various debug messages are printed to this port at 115200bps and a serial menu can be opened to configure advanced settings. 

[![RTK Express ESP32 connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-ESP32.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-ESP32.jpg)
 
#### **Radio:**

This 4-pin JST connector is used to allow RTCM correction data to flow into the device when it is acting as a rover or out of the device when it is acting as a base. The connector is a 4-pin locking 1.25mm JST SMD connector (part#: SM04B-GHS-TB, mating connector part#: GHR-04V-S). The RTK Express comes with a cable to interface to this connector but [additional cables](https://www.sparkfun.com/products/17239) can be purchased. You will most likely connect this port to one of our [Serial Telemetry Radios](https://www.sparkfun.com/products/19032) if you don’t have access to a correction source on the internet. The pinout is **3.5-5.5V** / TX / RX / GND  from left to right as pictured ([pin labels are shown on the board itself](https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide#hardware-overview---advanced-features)). **3.5V to 5.5V** is provided by this connector to power a radio with a voltage that depends on the power source. If USB is connected to the RTK Express then voltage on this port will be **5V** (+/-10%). If running off of the internal battery then voltage on this port will vary with the battery voltage (**3.5V** to **4.2V** depending on the state of charge). While the port is capable of sourcing up to 2 amps, we do not recommend more than 500mA. This port should not be connected to a power source.

[![RTK Express Radio connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-Radio.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-Radio.jpg) 

#### **Data:**

This 4-pin JST connector is used to output and input a variety of data to the RTK Express. The connector is a 4-pin locking 1.25mm JST SMD connector (part#: SM04B-GHS-TB, mating connector part#: GHR-04V-S). The RTK Express comes with a cable to interface to this connector but [additional cables](https://www.sparkfun.com/products/17240) can be purchased. 

Internally the Data connector is connected to a digital mux allowing one of four software selectable setups:

* **NMEA** - The TX pin outputs any enabled messages (NMEA, UBX, and RTCM) at a default of 460,800bps (configurable 9600 to 921600bps). The RX pin can receive RTCM for RTK and can also receive UBX configuration commands if desired.
* **PPS/Trigger** - The TX pin outputs the pulse-per-second signal that is accurate to 30ns RMS. The RX pin is connected to the EXTINT pin on the ZED-F9P allowing for events to be measured with incredibly accurate nano-second resolution. Useful for things like audio triangulation. See the Timemark section of the ZED-F9P integration for more information.
* **I2C** - The TX pin operates as SCL, RX pin as SDA on the I2C bus. This allows additional sensors to be connected to the I2C bus.
* **GPIO** - The TX pin operates as a DAC capable GPIO on the ESP32. The RX pin operates as a ADC capable input on the ESP32. This is useful for custom applications.

Most applications do not need to utilize this port and will send the NMEA position data over Bluetooth. This port can be useful for sending position data to an embedded microcontroller or single board computer. The pinout is **3.3V** / TX / RX / GND. **3.3V** from left to right as pictured ([pin labels are shown on the board itself](https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide#hardware-overview---advanced-features)), which is provided by this connector to power a remote device if needed. While the port is capable of sourcing up to 600mA, we do not recommend more than 300mA. This port should not be connected to a power source.

[![RTK Express Data connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-Data.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-Data.jpg)

#### **microSD:**

This slot accepts standard microSD cards up to 32GB formatted for FAT16 or FAT32. Logging any of 67 messages at up to 4Hz is supported for all constellations.

[![RTK Express microSD connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-microSD.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-microSD.jpg)

The following 67 messages are supported for logging:

* NMEA-DTM
* NMEA-GBS
* NMEA-GGA
* NMEA-GLL
* NMEA-GNS
* NMEA-GRS
* NMEA-GSA
* NMEA-GST
* NMEA-GSV
* NMEA-RMC
* NMEA-VLW
* NMEA-VTG
* NMEA-ZDA
* NAV-CLOCK
* NAV-DOP
* NAV-EOE
* NAV-GEOFENCE
* NAV-HPPOSECEF
* NAV-HPPOSLLH
* NAV-ODO
* NAV-ORB
* NAV-POSECEF
* NAV-POSLLH
* NAV-PVT
* NAV-RELPOSNED
* NAV-SAT
* NAV-SIG
* NAV-STATUS
* NAV-SVIN
* NAV-TIMEBDS
* NAV-TIMEGAL
* NAV-TIMEGLO
* NAV-TIMEGPS
* NAV-TIMELS
* NAV-TIMEUTC
* NAV-VELECEF
* NAV-VELNED
* RXM-MEASX
* RXM-RAWX
* RXM-RLM
* RXM-RTCM
* RXM-SFRBX
* MON-COMMS
* MON-HW2
* MON-HW3
* MON-HW
* MON-IO
* MON-MSGPP
* MON-RF
* MON-RXBUF
* MON-RXR
* MON-TXBUF
* TIM-TM2
* TIM-TP
* TIM-VRFY
* RTCM3x-1005
* RTCM3x-1074
* RTCM3x-1077
* RTCM3x-1084
* RTCM3x-1087
* RTCM3x-1094
* RTCM3x-1097
* RTCM3x-1124
* RTCM3x-1127
* RTCM3x-1230
* RTCM3x-4072-0
* RTCM3x-4072-1

#### **Qwiic:**

This 4-pin [Qwiic connector](https://www.sparkfun.com/qwiic) exposes the I2C bus of the ESP32 WROOM module. Currently, there is no firmware support for adding I2C devices to the RTK Express but support may be added in the future.

[![RTK Surveyor Qwiic connector](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-Qwiic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Connectors-Qwiic.jpg)

### **Power**

The RTK Express has a built in 1300mAh battery and consumes approximately 240mA worst case with Bluetooth connection active, GNSS fully tracking, and a 500mW radio broadcasting. This will allow for around 5.5 hours of use in the field. If more time is needed in the field a standard USB power bank can be attached. If a 10,000mAh bank is attached one can estimate 30 hours of run time assuming 25% is lost to efficiencies of the power bank and charge circuit within RTK Express.

The RTK Express can be charged from any USB port or adapter. The charge circuit is rated for 1000mA so USB 2.0 ports will charge at 500mA and USB 3.0+ ports will charge at 1A. 

To quickly view the state of charge, turn on the unit. The battery icon will indicate the following:

* 3 bars: >75% capacity remain
* 2 bars: >50% capacity remain
* 1 bar: >25% capacity remain
* 0 bars: <25% capacity remain

[![RTK Express Display showing three battery bars](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)

*RTK Express Display showing three battery bars*

## Advanced Features

The RTK Express is a hacker’s delight. Under the hood of the RTK Express is an ESP32 WROOM connected to a ZED-F9P as well as some peripheral hardware (LiPo fuel gauge, microSD, etc). It is programmed in Arduino and can be tailored by the end user to fit their needs.


[![RTK Express Schematic](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Schematic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Schematic.pdf)

->_Click on the image to get a closer look at the Schematic!_<-

### ZED-F9P GNSS Receiver

The [ZED-F9P GNSS receiver](https://www.sparkfun.com/products/16481) is configured over I<sup>2</sup>C and uses two UARTs to output NMEA (UART1) and input/output RTCM (UART2). In general, the ESP32 harvests the data from the ZED-F9Ps UART1 for Bluetooth transmission and logging to SD.

[![ZED-F9P GNSS Receiver](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_ZED-F9P.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_ZED-F9P.jpg)

### ESP32

The [ESP32](https://www.sparkfun.com/products/15663) uses a standard USB to serial conversion IC ([CH340](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all)) to program the device. You can use the ESP32 core for Arduino or Espressif’s [IoT Development Framework (IDF)](https://www.espressif.com/en/support/download/all).

The CH340 automatically resets and puts the ESP32 into bootload mode as needed. However, the reset pin of the ESP32 is brought out to an external 2-pin 0.1” footprint if an external reset button is needed.

<div class="alert alert-info" role="alert">
<strong>Note:</strong> 
If you've never connected a CH340 device to your computer before, you may need to install drivers for the USB-to-serial converter. Check out our section on <a href="https://learn.sparkfun.com/tutorials/sparkfun-serial-basic-ch340c-hookup-guide#drivers-if-you-need-them">"How to Install CH340 Drivers"</a> for help with the installation.
</div>

[![ESP32 on SparkFun RTK Express](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_ESP32.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_ESP32.jpg)

### Measurement Jumpers

To facilitate the measurement of run, charge, and quiescent currents, two measurement jumpers are included. These are normally closed jumpers combined with a 2-pin 0.1” footprint. To take a measurement, cut the jumper and install a 2-pin header and use [banana to IC hook](https://www.sparkfun.com/products/506) cables to a DMM. These can then be closed with a [2-pin jumper](https://www.sparkfun.com/products/9044).

[![Measurement Jumpers on SparkFun RTK Express](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Measurement_Jumpers.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Measurement_Jumpers.jpg)

### LiPo and Charging

The RTK Express houses a standard [1300mAh 3.7V LiPo](https://www.sparkfun.com/products/17748). The charge circuit is set to 1A so with an appropriate power source, charging an empty battery should take a little over one hour. USB C on the RTK Express is configured for 2A draw so if the user attaches to a USB 3.0 port, the charge circuit should operate near the 1A max. If a user attaches to a USB 2.0 port, the charge circuit will operate at 500mA. This charge circuit also incorporates a 42C upper temperature cutoff to insure the LiPo cannot be charged in dangerous conditions.

[![LiPo and Charging on SparkFun RTK Express](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_LiPo_Battery.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_LiPo_Battery.jpg)

### Fuel Gauge and Accelerometer

The [MAX17048](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/MAX17048-MAX17049.pdf) is a simple to use fuel gauge IC that gives the user a statement of charge (SOC) that is basically a 0 to 100% report. The MAX17048 has a sophisticated algorithm to figure out what the SOC is based on cell voltage that is beyond the scope of this tutorial but for our purposes, allows us to reliably view the battery level when the unit is on. 

The RTK Express also incorporates a the [LIS2DH12](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/lis2dh12_accelerometer_datasheet.pdf) triple-axis accelerometer to aid in leveling in the field.

[![Fuel Gauge and Accelerometer on SparkFun RTK Express](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Accel_Fuel_Gauge.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Accel_Fuel_Gauge.jpg)

### Qwiic

Two [Qwiic connectors](https://www.sparkfun.com/qwiic) are included in the unit. The internal Qwiic connector connects to the OLED display attached to the upper lid. The lower Qwiic connector is exposed on the end of the unit. These allow connection to the I<sup>2</sup>C bus on the ESP32. Currently the stock RTK Express does not support any additional Qwiic sensors or display but users may add support for their own application.

[![Dual Qwiic Connector on SparkFun RTK Express](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Qwiic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Qwiic.jpg)

### microSD

A microSD socket is situated on the ESP32 SPI bus. Any microSD up to 32GB is supported. RTK Express supports RAWX and NMEA logging to the SD card. Max logging time can also be set (default is 10 hours) to avoid multi-gigabyte text files. For more information about RAWX and doing PPP please see [this tutorial](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data).

[![microSD socket on SparkFun RTK Express](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_microSD.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_microSD.jpg)

### Data Port and Digital Mux

The 74HC4052 analog mux controls which digital signals route to the external Data port. This allows a variety of custom end user applications. The most interesting of which is event logging. Because the ZED-F9P has microsecond accuracy of the incoming digital signal, custom firmware can be created to triangulate an event based on the receiver's position and the time delay between multiple captured events. Currently, TM2 event logging is supported. 

Additionally, this mux can be configured to connect ESP pin 26 (DAC capable) and pin 39 (ADC capable) for end user custom applications.

[![SparkFun RTK Express Data port and mux](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Data_Port.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Internal_-_Data_Port.jpg)