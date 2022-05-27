SparkFun RTK Firmware
===========================================================

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/18443"><img src="https://cdn.sparkfun.com//assets/parts/1/6/4/0/1/17369-SparkFun_RTK_Surveyor-14.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18442"><img src="https://cdn.sparkfun.com//assets/parts/1/7/2/4/1/18019-SparkFun_RTK_Express-09.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18590"><img src="https://cdn.sparkfun.com//assets/parts/1/8/0/7/5/18590-SparkFun_RTK_Express_Plus-04.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18590"><img src="https://cdn.sparkfun.com//assets/parts/1/8/6/3/0/RTK_Facet_Photos-01.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/18443">SparkFun RTK Surveyor (GPS-18443)</a></td>
    <td><a href="https://www.sparkfun.com/products/18442">SparkFun RTK Express (GPS-18442)</a></td>
    <td><a href="https://www.sparkfun.com/products/18590">SparkFun RTK Express Plus (GPS-18590)</a></td>
    <td><a href="https://www.sparkfun.com/products/19029">SparkFun RTK Facet (GPS-19029)</a></td>
  </tr>
</table>

The [SparkFun RTK Surveyor](https://www.sparkfun.com/products/18443), [SparkFun RTK Express](https://www.sparkfun.com/products/18442), [SparkFun RTK Express Plus](https://www.sparkfun.com/products/18590), and [SparkFun RTK Facet](https://www.sparkfun.com/products/19029) are centimeter-level GNSS receivers. With RTK enabled, these devices can output your location with 14mm horizontal and vertical *accuracy* at up to 20Hz!

This repo houses the [RTK Product Manual](https://sparkfun.github.io/SparkFun_RTK_Firmware/intro/) and the firmware that runs on the SparkFun RTK product line including:

* [SparkFun RTK Surveyor](https://www.sparkfun.com/products/18443)
* [SparkFun RTK Express](https://www.sparkfun.com/products/18442)
* [SparkFun RTK Express Plus](https://www.sparkfun.com/products/18590)
* [SparkFun RTK Facet](https://www.sparkfun.com/products/19029)

If you're interested in the PCB, enclosure, or overlay on each product please see the hardware repos:

* [SparkFun RTK Surveyor Hardware](https://github.com/sparkfun/SparkFun_RTK_Surveyor)
* [SparkFun RTK Express Hardware](https://github.com/sparkfun/SparkFun_RTK_Express)
* [SparkFun RTK Express Plus Hardware](https://github.com/sparkfun/SparkFun_RTK_Express_Plus)
* [SparkFun RTK Facet Hardware](https://github.com/sparkfun/SparkFun_RTK_Facet)

Thanks:

* Special thanks to [Avinab Malla](https://github.com/avinabmalla) for the creation of [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps&hl=en_US&gl=US) and for pointers on handling the ESP32 read/write tasks.

Repository Contents
-------------------

* **/Binaries** - Loadable firmware either over USB or via SD card
* **/Firmware** - Main firmware as well as various feature unit tests
* **/Graphics** - Original bitmap icons for the display
* **/Uploader_GUI** - A python and Windows executable GUI for updating the firmware on RTK units. See [Updating Firmware From GUI](https://sparkfun.github.io/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-gui)
* **/docs** - Markdown pages for the [RTK Product Manual](https://sparkfun.github.io/SparkFun_RTK_Firmware/intro/)

Documentation
--------------

* **[RTK Product Manual](https://sparkfun.github.io/SparkFun_RTK_Firmware/intro/) - A detail guide describing all the various software features of the RTK product line.
* **[RTK Surveyor Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-surveyor-hookup-guide)** - Hookup guide for the SparkFun RTK Surveyor.
* **[RTK Express Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide)** - Hookup guide for the SparkFun RTK Express and Express Plus.
* **[RTK Facet Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-hookup-guide)** - Hookup guide for the SparkFun RTK Facet.

License Information
-------------------

This product is _**open source**_!

Various bits of the code have different licenses applied. Anything SparkFun wrote is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
