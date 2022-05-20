

# Micro OLED Setup

## Introduction

The [Qwiic Micro OLED](https://www.sparkfun.com/products/14532) is a Qwiic enabled version of our micro OLED display! This small monochrome, blue-on-black OLED display displays incredibly clear images

[![SparkFun Micro OLED Breakout (Qwiic)](https://cdn.sparkfun.com/r/500-500/assets/parts/1/2/6/2/1/14532-SparkFun_Micro_OLED_Breakout__Qwiic_-01.jpg "SparkFun Micro OLED Breakout (Qwiic)")](https://www.sparkfun.com/products/14532)


<!-- youtube(https://www.youtube.com/watch?v=OBOgxnctzwI) -->

This hookup guide will show you how to get started drawing objects and characters on your OLED.

###Required Materials 

To get started, you'll need a microcontroller to, well, control everything.

* [SparkFun RedBoard - Programmed with Arduino](https://www.sparkfun.com/products/13975)
* [SparkFun Thing Plus - Artemis](https://www.sparkfun.com/products/15574)
* [SparkFun Thing Plus - ESP32 WROOM](https://www.sparkfun.com/products/15663)
* [SparkFun Thing Plus - SAMD51](https://www.sparkfun.com/products/14713)
* [Raspberry Pi 4 Model B (2 GB)](https://www.sparkfun.com/products/15446)

If the controller you choose doesn't have a built in Qwiic connector, one of the following Qwiic shields that matches your preference of microcontroller is needed:

* [SparkFun Qwiic Shield for Arduino](https://www.sparkfun.com/products/14352)
* [SparkFun Qwiic Shield for Teensy](https://www.sparkfun.com/products/17119)
* [SparkFun Qwiic Shield for Arduino Nano](https://www.sparkfun.com/products/16789)
* [SparkFun Qwiic SHIM for Raspberry Pi](https://www.sparkfun.com/products/15794)

<!-- products_by_id(14352, 14477, 14459) -->

You will also need a Qwiic cable to connect the shield to your OLED, choose a length that suits your needs.

* [Flexible Qwiic Cable - 100mm](https://www.sparkfun.com/products/17259)
* [Flexible Qwiic Cable - 500mm](https://www.sparkfun.com/products/17257)
* [Flexible Qwiic Cable - 50mm](https://www.sparkfun.com/products/17260)
* [Flexible Qwiic Cable - 200mm](https://www.sparkfun.com/products/17258)

###Suggested Reading

If you aren't familiar with the Qwiic system, we recommend reading [here for an overview](https://www.sparkfun.com/qwiic).

<table class="table table-bordered">
  <tr align="center">
   <td><center><a href="https://www.sparkfun.com/qwiic"><img src="https://cdn.sparkfun.com/assets/custom_pages/2/7/2/qwiic-logo.png" alt="Qwiic Connect System" title="Qwiic Connect System"></a></center></td>
  </tr>
  <tr align="center">
    <td><i><a href="https://www.sparkfun.com/qwiic">Qwiic Connect System</a></i></td>
  </tr>
</table>

We would also recommend taking a look at the following tutorials if you aren't familiar with them.

* [I2C Overview](https://learn.sparkfun.com/tutorials/i2c)
* [Qwiic Shield for Arduino & Photon Hookup Guide](https://learn.sparkfun.com/tutorials/qwiic-shield-for-arduino--photon-hookup-guide)

### Hardware Overview

Listed below are some of the operating ranges and characteristics of the Qwiic Micro OLED.

| Characteristic | Range |
| :--- | :--- |
| Voltage | **3.3V** |
| Temperature | -40&deg;C to 85&deg;C |
| I<sup>2</sup>C Address | 0X3D (Default) or 0X3C (Closed Jumper) |


###Pins

| Pin | Description | Direction |
| :--- | :--- | :--- |
|GND | Ground | In | 
| 3.3V | Power | In |
| SDA | Data | In |
| SCL | Clock | In | 

##Optional Features

There are several jumpers on board that can be changed to facilitate several different functions. The first of which is the I<sup>2</sup>C pull-up jumper, highlighted below. If multiple boards are connected to the I<sup>2</sup>C bus, the equivalent resistance goes down, increasing your pull up strength. If multiple boards are connected on the same bus, make sure only one board has the pull-up resistors connected.

[![I2C Pull-Up Jumper](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/7/1/8/pu.PNG "I2C Pull-Up Jumper")](https://cdn.sparkfun.com/assets/learn_tutorials/7/1/8/pu.PNG)

The ADDR jumper (highlighted below) can be used to change the I<sup>2</sup>C address of the board. The default jumper is open by default, pulling the address pin high and giving us an I<sup>2</sup>C address of **0X3D**. Closing this jumper will ground the address pin, giving us an I<sup>2</sup>C address of 0X3C.

[![Address Jumper](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/7/1/8/addr.PNG "Address Jumper")](https://cdn.sparkfun.com/assets/learn_tutorials/7/1/8/addr.PNG)

## Hardware Assembly

If you haven't yet [assembled your Qwiic Shield](https://learn.sparkfun.com/tutorials/qwiic-shield-for-arduino--photon-hookup-guide), now would be the time to head on over to that tutorial.
With the shield assembled, Sparkfun's new Qwiic environment means that connecting the screen could not be easier. Just plug one end of the Qwiic cable into the OLED display, the other into the Qwiic Shield and you'll be ready to start displaying images on your little display.

[![Qwiic Micro OLED Connected to Arduino and Qwiic Shield](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/7/1/8/Qwiic_OLED-03.jpg "Qwiic Micro OLED Connected to Arduino and Qwiic Shield")](https://cdn.sparkfun.com/assets/learn_tutorials/7/1/8/Qwiic_OLED-03.jpg) 

The OLED screen itself is loosely attached to the breakout board initially, so be careful handling it! You can either use your own enclosure for the OLED display, or you can use some double sided foam tape for a less permanent solution.

[![Taped Screen](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/7/1/8/Qwiic_OLED-01.jpg "Taped Screen")](https://cdn.sparkfun.com/assets/learn_tutorials/7/1/8/Qwiic_OLED-01.jpg)

## Software

The SparkFun Micro OLED Breakout (Qwiic) uses the SparkFun QWIIC OLED Arduino Library. The [SparkFun Qwiic OLED library Getting Started guide](software.md) has library setup instructions and usage examples. Additionally, the full library API documentation is available in the [SparkFun Qwiic OLED Library API Reference guide](api_device.md).

## Resources and Going Further

Now that you've successfully got your OLED displaying things, it's time to incorporate it into your own project!

For more on the Qwiic Micro OLED, check out the links below:

* [Schematic (PDF)](https://cdn.sparkfun.com/assets/d/0/e/4/1/Qwiic_OLED_Breakout.pdf)
* [Eagle Files (ZIP)](https://cdn.sparkfun.com/assets/c/b/c/f/d/Qwiic_OLED_Breakout_1.zip) 
* [Datasheet (PDF)](https://cdn.sparkfun.com/assets/learn_tutorials/3/0/8/SSD1306.pdf) 
* [Bitmap Generator](http://en.radzio.dxp.pl/bitmap_converter/)
* [Qwiic System Landing Page](https://www.sparkfun.com/qwiic)
* [SparkFun Qwiic OLED Arduino Library](https://github.com/sparkfun/SparkFun_Qwiic_OLED_Arduino_Library/)
* [Qwiic Micro OLED Python Package](https://github.com/sparkfun/Qwiic_Micro_OLED_Py)
* [SparkFun Qwiic Micro OLED GitHub Repository](https://github.com/sparkfun/Qwiic_Micro_OLED) -- Board design files for the Qwiic Micro OLED.
* [Product Showcase: Qwiic Presence Sensor & OLED](https://www.youtube.com/watch?v=OBOgxnctzwI)

