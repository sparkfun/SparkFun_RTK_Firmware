

# Transparent OLED Setup

## Introduction

The future is here! You asked and we delivered - our [Qwiic Transparent Graphical OLED Breakout](https://www.sparkfun.com/products/15173) allows you to display custom images on a transparent screen using either I<sup>2</sup>C or SPI connections. 

With Qwiic connectors it's quick (ha ha) and easy to get started with your own images. However, we still have broken out 0.1"-spaced pins in case you prefer to use a breadboard. Brilliantly lit in the dark and still visible by daylight, this OLED sports a display area of 128x64 pixels, 128x56 of which are completely transparent. Control of the OLED is based on our new HyperDisplay library.


[![SparkFun Transparent Graphical OLED Breakout (Qwiic)](https://cdn.sparkfun.com//assets/parts/1/3/5/8/8/15173-SparkFun_Transparent_Graphical_OLED_Breakout__Qwiic_-01a.jpg "SparkFun Transparent Graphical OLED Breakout (Qwiic)")](https://www.sparkfun.com/products/15173)


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

## Hardware Overview

Listed below are some of the operating ranges and characteristics of the Qwiic Micro OLED.

| Characteristic | Range |
| :--- | :--- |
| Voltage | **1.65V-3.3V** |
| Supply Current | 400 mA |
| I<sup>2</sup>C Address | **0X3C (Default)**, 0X3D  |

###Graphical Display

The graphical display is where all the fun stuff happens. The glass itself measures 42mm x 27.16mm, with a pixel display that is 35.5 x 18mm. It houses 128x64 pixels, 128x56 of which are transparent. 

[![Display Screen](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/GraphicalDisplay1.jpg "Dispaly Screen")](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/GraphicalDisplay1.jpg) 

_Graphical Display_

###Qwiic Connectors

There are two Qwiic connectors on the board such that you can daisy-chain the boards should you choose to do so. If you're unfamiliar with our Qwiic system, head on over to our [Qwiic page](https://www.sparkfun.com/qwiic) to see the advantages! 

[![Qwiic Connectors](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/QwiicConnectors1.jpg "Qwiic Connectors")](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/QwiicConnectors1.jpg) <-

_Qwiic Connectors_

###GPIO Pins

When you look at the GPIO pins, you'll notice that the labels are different from one side to the other. One side is labeled for I<sup>2</sup>C, the other side is labeled for SPI. 

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/GPIOPinsFront1.jpg"><img src="https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/GPIOPinsFront1.jpg" title="I<sup>2</sup>C Pins"></a></td>
   <td><a href="https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/15173-GPIOBackCorrected.jpg"><img src="https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/15173-GPIOBackCorrected.jpg" title="SPI Pins"></a> <br /></td>
  </tr>
  <tr align="center">
    <td><i>I<sup>2</sup>C Labels</i></td>
    <td><i>SPI Labels</i></td>
  </tr>
</table>

###Power LED

This bad boy will light up when the board is powered up correctly. 

[![Power LED](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/PowerLED1.jpg "Power LED")](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/PowerLED1.jpg) <-

_Power LED_

You can disable the power LED by cutting the LED jumpers on the back of the board. 

[![Power LED Jumpers](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/15173-LEDJumper.jpg "Power LED Jumpers")](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/15173-LEDJumper.jpg)

_Power LED Jumpers_


###JPX Jumpers

The JPX jumpers are used to either change the I<sup>2</sup>C address or configure the board to use SPI communications. The other two jumpers allow you to disconnect the power LED and to disconnect the I<sup>2</sup>C pull-up resistors when chaining several Qwiic devices.

| Jumper | Function |
| :--- | :--- |
| JP1 | Holds the Chip Select line low when closed. Close for I<sup>2</sup>C, open for SPI |
| JP2 | Selects the address in I<sup>2</sup>C mode. Closed for <b>0x30 by default</b> and open for 0x31. Open for SPI mode to release the D/C pin |
| JP3 | Used to select I<sup>2</sup>C or SPI mode. Close for I<sup>2</sup>C, open for SPI |
| JP4 | This jumper should be closed for I<sup>2</sup>C and open for SPI. This connection allows SDA to be bi-directional|


[![JP1-JP4](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/JPJumpersBackCorrected.jpg "JP1-JP4")](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/JPJumpersBackCorrected.jpg) 

_JPX Jumpers_


###I<sup>2</sup>C Pull-Up Jumper

I<sup>2</sup>C devices contain open drains so we include resistors on our boards to allow these devices to pull pins high. This becomes a problem if you have a large number of I<sup>2</sup>C devices chained together. If you plan to daisy chain more than a few Qwiic boards together, you'll need to <a href="https://learn.sparkfun.com/tutorials/how-to-work-with-jumper-pads-and-pcb-traces#cutting-a-trace-between-jumper-pads">cut this I<sup>2</sup>C pull-up jumper</a>. 

[![I2C Pullup jumper](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/15173-I2CPUJumper.jpg "I2C Pullup jumper")](https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/15173-I2CPUJumper.jpg) <-

_I<sup>2</sup>C PU Jumpers_

## Hardware Hookup

Now that you know what's available on your breakout board we can check out the options for connecting it to the brains of your project. There are two options to use - either I<sup>2</sup>C or SPI - and they each have their own advantages and drawbacks. Read on to choose the best option for your setup.

!!! warning
    Reminder!</b> This breakout can only handle up to <b>3.3V</b> on the pins, so make sure to do some <a href="https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide"><b>level shifting</b></a> if you're using a 5V microcontroller.

### I<sup>2</sup>C (Qwiic)

The easiest way to start using the Transparent Graphical OLED is to use a [Qwiic Cable](https://www.sparkfun.com/products/15081) along with a Qwiic compatible microcontroller (such as the [ESP32 Thing Plus](https://www.sparkfun.com/products/14689)). You can also use the [Qwiic Breadboard Cable](https://www.sparkfun.com/products/14425) to attach any I<sup>2</sup>C capable microcontroller, or take the scenic route and [solder](https://learn.sparkfun.com/tutorials/how-to-solder---through-hole-soldering) in all the I<sup>2</sup>C wires to the plated-through connections on the board. 

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/QwiicConnectors1.jpg"><img src="https://cdn.sparkfun.com/r/300-300/assets/learn_tutorials/6/1/2/QwiicConnectors1.jpg" alt="Qwiic Connector"></a></td>
   <td><a href="https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/GPIOPinsFront.jpg"><img src="https://cdn.sparkfun.com/r/300-300/assets/learn_tutorials/6/1/2/GPIOPinsFront.jpg" alt="I2C Pinout"></a></td>
  </tr>
</table>

So why use I<sup>2</sup>C? It's easy to connect with the Qwiic system, and you can put up to two of the Transparent Graphical Breakouts on the same bus without using any more microcontroller pins. That simplicity comes at a cost to performance though. The maximum clock speed of the I<sup>2</sup>C bus is 400 kHz, and there is additional overhead in data transmission to indicate which bytes are data and which are commands. This means that the I<sup>2</sup>C connection is best for showing static images.



| Breakout Pin | Microcontroller Pin Requirements|
| :--- | :--- |
| GND | Ground pin. Connect these so the two devices agree on voltages |
| 3V3 | 3.3V supply pin, capable of up to 400 mA output |
| SDA | SDA - the bi-directional data line of your chosen I2C port |
| SCL | SCL - the clock line of your chosen I2C port |
| SA0 | _Optional_ : change the I2C address of the breakout. Make sure to cut JP2 |
| RST | _Optional_ : reset the breakout to a known state by pulsing this low |



### SPI

SPI solves the I<sup>2</sup>C speed problems. With SPI there is a control signal that indicates data or command and the maximum clock speed is 10 MHz -- giving SPI 50x more speed! However,  it doesn't have the same conveniences of the polarized Qwiic connector and low pin usage. You'll need to [solder](https://learn.sparkfun.com/tutorials/how-to-solder---through-hole-soldering) to the pins.

[![SPI Pinout](https://cdn.sparkfun.com/r/300-300/assets/learn_tutorials/6/1/2/15173-GPIOBackCorrected.jpg "SPI Pinout")](https://cdn.sparkfun.com/assets/learn_tutorials/6/1/2/15173-GPIOBackCorrected.jpg)

_SPI Pinout_

You can use SPI to connect as many breakouts as you want. For N displays you will need to use at least N + 3 data pins. That's because the MOSI, SCLK, and D/C pins can be shared between displays but each breakout needs its own dedicated Chip Select (CS) pin.

| Breakout Pin | Microcontroller Pin Requirements |
| :--- | :--- |
| CS | A GPIO pin, set low when talking to the breakout |
| D/C | A GPIO pin, indicates if bytes are data or commands | 
| SCLK | The clock output of your chosen SPI port |
| MOSI | The data output of your chosen SPI port |
| 3V3 | 3.3V supply pin, capable of up to 400 mA output | 
| GND | Ground pin. Connect these so the two devices agree on voltages |

</table>

!!! Warning
    
    Make sure to cut jumpers JP1, JP2, JP3, and JP4 when using SPI mode!

    <div class="text-center"><a href="https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/15173-CutJumpers.jpg"><img src="https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/6/1/2/15173-CutJumpers.jpg" alt="Cut Jumpers for SPI Mode"></a></div>






## Software

The Transparent OLED Breakout (Qwiic) uses the SparkFun QWIIC OLED Arduino Library. The [SparkFun Qwiic OLED library Getting Started guide](software.md) has library setup instructions and usage examples. Additionally, the full library API documentation is available in the [SparkFun Qwiic OLED Library API Reference guide](api_device.md).

## Resources and Going Further

For more information on the Transparent Graphical OLED Breakout, check out some of the links here: 

* [Schematic (PDF)](https://cdn.sparkfun.com/assets/5/e/7/b/5/SparkFun_Transparent_Graphical_OLED_Breakout.pdf)
* [Eagle Files (ZIP)](https://cdn.sparkfun.com/assets/2/0/2/c/b/TransparentGraphicalOLEDBreakout.zip)
* [SparkFun Qwiic OLED Arduino Library](https://github.com/sparkfun/SparkFun_Qwiic_OLED_Arduino_Library/)
* [GitHub Repo](https://github.com/sparkfun/Qwiic_Transparent_Graphical_OLED)
* [SFE Product Showcase](https://youtu.be/vzFuVbxBfXI)

