# Introduction

The SparkFun RTK products are exceptional GNSS receivers out-of-box and can be used with little or no configuration. This RTK Product Manual provides detailed descriptions of all the available features of the RTK products.

The line of RTK products offered by SparkFun all run identical firmware. The [RTK firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware) and this guide cover the following products:

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/20000"><img src="img/SparkFun%20RTK%20Facet%20L-Band.png"></a></td>
   <td><a href="https://www.sparkfun.com/products/19029"><img src="img/SparkFun%20RTK%20Facet.png"></a></td>
   <td><a href="https://www.sparkfun.com/products/22429"><img src="img/SparkFun%20RTK%20Reference%20Station.png"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/20000">SparkFun RTK Facet L-Band (GPS-20000)</a></td>
    <td><a href="https://www.sparkfun.com/products/19029">SparkFun RTK Facet (GPS-19029)</a></td>
    <td><a href="https://www.sparkfun.com/products/22429">SparkFun RTK Reference Station (GPS-22429)</a></td>
  </tr>
  <tr align="center">
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-l-band-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-reference-station-hookup-guide">Hookup Guide</a></td>
  </tr>
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/18590"><img src="img/SparkFun%20RTK%20Express%20Plus.png"></a></td>
   <td><a href="https://www.sparkfun.com/products/18442"><img src="img/SparkFun%20RTK%20Express.png"></a></td>
   <td><a href="https://www.sparkfun.com/products/18443"><img src="img/SparkFun%20RTK%20Surveyor.png"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/18590">SparkFun RTK Express Plus (GPS-18590)</a></td>
    <td><a href="https://www.sparkfun.com/products/18442">SparkFun RTK Express (GPS-18442)</a></td>
    <td><a href="https://www.sparkfun.com/products/18443">SparkFun RTK Surveyor (GPS-18443)</a></td>
  </tr>
  <tr align="center">
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-surveyor-hookup-guide">Hookup Guide</a></td>
  </tr>
</table>

Depending on the hardware platform different features may or may not be supported. We will denote each product in each section so that you know what is supported.

There are multiple ways to configure an RTK product:

* [Bluetooth](configure_with_bluetooth.md) - Good for in-field changes
* [WiFi](configure_with_wifi.md) - Good for in-field changes
* [Serial Terminal](configure_with_serial.md) - Requires a computer but allows for all configuration settings
* [Settings File](configure_with_settings_file.md) - Used for configuring multiple RTK devices identically

The Bluetooth or Serial Terminal methods are recommended for most advanced configurations. Most, but not all settings are also available over WiFi but can be tricky to input via mobile phone.

If you have an issue, feature request, bug report, or a general question about the RTK firmware specifically we encourage you to post your comments on the [firmware's repository](https://github.com/sparkfun/SparkFun_RTK_Firmware/issues). If you feel like bragging or showing off what you did with your RTK product, we'd be thrilled to hear about it on the issues list as well!

Things like how to attach an antenna or other hardware-specific topics are best read on the Hookup Guides for the individual products.
