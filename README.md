SparkFun OpenLog Artemis
===========================================================

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/16832"><img src="https://cdn.sparkfun.com//assets/parts/1/5/7/5/3/16832-SparkFun_OpenLog_Artemis-02a.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/15846"><img src="https://cdn.sparkfun.com//assets/parts/1/4/4/8/0/15846-OpenLog_Artemis-04.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/16832">SparkFun OpenLog Artemis (DEV-16832)</a></td>
    <td><a href="https://www.sparkfun.com/products/15846">SparkX OpenLog Artemis (SPX-15846)</a></td>
  </tr>
</table>

The OpenLog Artemis is an open source datalogger that comes preprogrammed to automatically log IMU, GPS, serial data, and various pressure, humidity, and distance sensors. All without writing a single line of code! OLA automatically detects, configures, and logs Qwiic sensors. OLA is designed for users who just need to capture a bunch of data to a CSV and get back to their larger project.

Included on every OpenLog Artemis is an IMU for built-in logging of triple axis accelerometer, gyro, and magnetometer. Whereas the original [9DOF Razor](https://www.sparkfun.com/products/14001) used the old MPU-9250, the OpenLog Artemis uses the latest [ICM-20948](https://www.sparkfun.com/products/15335) capable of nearly 1kHz logging of all 9 axis. We then took over a decade of experience with the original [OpenLog](https://www.sparkfun.com/products/13712) and took it much farther. Simply power up OpenLog Artemis and all incoming serial data is automatically recorded to a log file. Baud rates up to 921600bps are supported! Additionally, based on feedback from users we've added an onboard RTC so that all data can be time stamped.

OpenLog Artemis is highly configurable over an easy to use serial interface. Simply plug in a USB C cable and open a terminal at 115200kbps. The logging output is automatically streamed to both the terminal and the microSD. Pressing any key will open the configuration menu.

The OpenLog Artemis automatically scans, detects, configures, and logs various Qwiic sensors plugged into the board (no soldering required!). Currently, auto-detection is supported on the following Qwiic products:

* uBlox GPS Modules (Lat/Long, Altitude, Velocity, SIV, Time, Date):
  * [ZED-F9P](https://www.sparkfun.com/products/15136) 1cm High Precision GPS
  * [NEO-M8P-2](https://www.sparkfun.com/products/15005) 2.5cm High Precision GPS
  * [SAM-M8Q](https://www.sparkfun.com/products/15210) 1.5m 72 Channel GPS
  * [ZOE-M8Q](https://www.sparkfun.com/products/15193) 1.5m Compact GPS
  * [NEO-M9N](https://www.sparkfun.com/products/15712) 1.5m GPS
* [MCP9600 Thermocouple Amplifier](https://www.sparkfun.com/products/16294)
* [NAU7802 Load Cell Amplifier](https://www.sparkfun.com/products/15242)
* [LPS25HB Barometric Pressure Sensor](https://www.sparkfun.com/products/14767)
* [BME280 Humidity and Barometric Pressure Sensor](https://www.sparkfun.com/products/15440)
* [MS5637 Barometric Pressure Sensor](https://www.sparkfun.com/products/14688)
* [MS8607 Pressure Humidity Temperature Sensor](https://www.sparkfun.com/products/16298)
* [TMP117 High Precision Temperature Sensor](https://www.sparkfun.com/products/15805)
* [AHT20 Humidity and Temperature Sensor](https://www.sparkfun.com/products/16618)
* [SHTC3 Humidity and Temperature Sensor](https://www.sparkfun.com/products/16467)
* [CCS811 Air Quality Sensor](https://www.sparkfun.com/products/14348)
* [SGP30 Air Quality Sensor](https://www.sparkfun.com/products/16531)
* [SCD30 CO<sub>2</sub> and Air Quality Sensor](https://www.sparkfun.com/products/15112)
* [VEML6075 UV Sensor](https://www.sparkfun.com/products/15089)
* [VCNL4040 Proximity Sensor](https://www.sparkfun.com/products/15177)
* [VL53L1X LIDAR Distance Sensor](https://www.sparkfun.com/products/14722)
* [ADS122C04 ADC PT100 Sensor](https://www.sparkfun.com/products/16770)
* [Qwiic Mux](https://www.sparkfun.com/products/16784) allowing for the chaining of up to 64 unique buses!
* More boards are being added all the time!

Very low power logging is supported. OpenLog Artemis can be configured to take readings at 500 times a second, or as slow as 1 reading every 24 hours. You choose! When there is more than 2 seconds between readings OLA will automatically power down itself and the sensors on the bus resulting in a sleep current of approximately 18uA. This means a normal [2Ah battery](https://www.sparkfun.com/products/13855) will enable logging for more than 4,000 days! OpenLog Artemis has built-in LiPo charging set at 450mA/hr.

New features are constantly being added so we’ve released an easy to use firmware upgrade tool. No need to install Arduino or a bunch of libraries, simply open the [Artemis Firmware Upload GUI](https://github.com/sparkfun/Artemis-Firmware-Upload-GUI), load the latest OLA firmware, and add features to OpenLog Artemis as they come out! Full instructions are available in [UPGRADE.md](UPGRADE.md).

The OLA can be tailored to many different applications and we will be releasing custom versions of the firmware for those too:

* [Latest OLA firmware](https://github.com/sparkfun/OpenLog_Artemis/tree/master/Binaries)
* [Geophone Logger firmware](https://github.com/sparkfun/OpenLog_Artemis_Geophone_Logger) for logging seismic activity
* [GNSS Logger](https://github.com/sparkfun/OpenLog_Artemis_GNSS_Logger) for advanced data logging with the uBlox F9 and M9 GNSS modules including support for RAWX and RELPOSNED

Artemis Firmware Uploader GUI
Latest OLA firmware: v1.5
Geophone Logger firmware for logging seismic activity
GNSS Logger for advanced data logging with the uBlox F9 and M9 GNSS modules including support for RAWX and RELPOSNED

Repository Contents
-------------------

* **/Binaries** - The binary files for the different versions of the OLA firmware
* **/Firmware** - The main sketch that runs OpenLog Artemis as well as a variety of sketches to test various sensor interfaces and power saving states.
* **/Hardware** - Eagle files

Documentation
--------------

* **[UPGRADE.md](UPGRADE.md)** - contains full instructions on how to upgrade the firmware on the OLA using the [Artemis Firmware Upload GUI](https://github.com/sparkfun/Artemis-Firmware-Upload-GUI)
* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - OLA includes a large number of libraries that will need to be installed before compiling will work.
* **[Hookup Guide](https://learn.sparkfun.com/tutorials/openlog-artemis-hookup-guide)** - Basic hookup guide for the OLA.

License Information
-------------------

This product is _**open source**_!

Various bits of the code have different licenses applied. Anything SparkFun wrote is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
