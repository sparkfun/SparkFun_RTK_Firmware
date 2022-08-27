SparkFun RTK Firmware
===========================================================

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/20000"><img src="https://cdn.sparkfun.com//assets/parts/1/9/7/4/6/20000-SparkFun_RTK_Facet_L-Band-01.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18590"><img src="https://cdn.sparkfun.com//assets/parts/1/8/6/3/0/RTK_Facet_Photos-01.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18590"><img src="https://cdn.sparkfun.com//assets/parts/1/8/0/7/5/18590-SparkFun_RTK_Express_Plus-04.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18442"><img src="https://cdn.sparkfun.com//assets/parts/1/7/2/4/1/18019-SparkFun_RTK_Express-09.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/18443"><img src="https://cdn.sparkfun.com//assets/parts/1/6/4/0/1/17369-SparkFun_RTK_Surveyor-14.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/20000">SparkFun RTK Facet L-Band (GPS-20000)</a></td>
    <td><a href="https://www.sparkfun.com/products/19029">SparkFun RTK Facet (GPS-19029)</a></td>
    <td><a href="https://www.sparkfun.com/products/18590">SparkFun RTK Express Plus (GPS-18590)</a></td>
    <td><a href="https://www.sparkfun.com/products/18442">SparkFun RTK Express (GPS-18442)</a></td>
    <td><a href="https://www.sparkfun.com/products/18443">SparkFun RTK Surveyor (GPS-18443)</a></td>
  </tr>
  <tr align="center">
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-l-band-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide">Hookup Guide</a></td>
    <td><a href="https://learn.sparkfun.com/tutorials/sparkfun-rtk-surveyor-hookup-guide">Hookup Guide</a></td>
  </tr>
</table>

The [SparkFun RTK Surveyor](https://www.sparkfun.com/products/18443), [SparkFun RTK Express](https://www.sparkfun.com/products/18442), [SparkFun RTK Express Plus](https://www.sparkfun.com/products/18590), [SparkFun RTK Facet](https://www.sparkfun.com/products/19029), and [SparkFun RTK Facet L-Band](https://www.sparkfun.com/products/20000) are centimeter-level GNSS receivers. With RTK enabled, these devices can output your location with 14mm horizontal and vertical [*accuracy*](https://docs.sparkfun.com/SparkFun_RTK_Firmware/accuracy_verification/) at up to 20Hz!

This repo houses the [RTK Product Manual](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/) and the firmware that runs on the SparkFun RTK product line including:

* [SparkFun RTK Facet L-Band](https://www.sparkfun.com/products/20000)
* [SparkFun RTK Facet](https://www.sparkfun.com/products/19029)
* [SparkFun RTK Express Plus](https://www.sparkfun.com/products/18590)
* [SparkFun RTK Express](https://www.sparkfun.com/products/18442)
* [SparkFun RTK Surveyor](https://www.sparkfun.com/products/18443)

For compiled binaries of the firmware, please see [SparkFun RTK Firmware Binaries](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries).

If you're interested in the PCB, enclosure, or overlay on each product please see the hardware repos:

* [SparkFun RTK Facet L-Band Hardware](https://github.com/sparkfun/SparkFun_RTK_Facet)
* [SparkFun RTK Facet Hardware](https://github.com/sparkfun/SparkFun_RTK_Facet)
* [SparkFun RTK Express Plus Hardware](https://github.com/sparkfun/SparkFun_RTK_Express_Plus)
* [SparkFun RTK Express Hardware](https://github.com/sparkfun/SparkFun_RTK_Express)
* [SparkFun RTK Surveyor Hardware](https://github.com/sparkfun/SparkFun_RTK_Surveyor)

Thanks:

* Special thanks to [Avinab Malla](https://github.com/avinabmalla) for the creation of [SW Maps](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps&hl=en_US&gl=US) and for pointers on handling the ESP32 read/write tasks.

Documentation
--------------

* **[RTK Product Manual](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/)** - A detail guide describing all the various software features of the RTK product line.   Essentially it is a manual for the firmware in this repository.
* **[RTK Facet L-Band Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-l-band-hookup-guide)** - Hookup guide for the SparkFun RTK Facet L-Band.
* **[RTK Facet Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-facet-hookup-guide)** - Hookup guide for the SparkFun RTK Facet.
* **[RTK Express Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-express-hookup-guide)** - Hookup guide for the SparkFun RTK Express and Express Plus.
* **[RTK Surveyor Hookup Guide](https://learn.sparkfun.com/tutorials/sparkfun-rtk-surveyor-hookup-guide)** - Hookup guide for the SparkFun RTK Surveyor.

Repository Contents
-------------------

* **/Firmware** - Source code for SparkFun RTK firmware as well as various feature unit tests
* **/Graphics** - Original bitmap icons for the display
* **/docs** - Markdown pages for the [RTK Product Manual](https://docs.sparkfun.com/SparkFun_RTK_Firmware/intro/)

Repository Branch Structure
---------------------------

This repository has two long-term branches: `main` and `release_candidate`.

With respect to the firmware, `main` is a branch where only changes that are appropriate for all users are applied. Thus, following `main` means updating to normal releases, and perhaps bugfixes to those releases.

In contrast, `release_candidate` is where new code is added as it is developed.

The documentation source code is in docs/ on `main`.  It is built automatically on push and stored in the branch `gh-pages`, from which it is served at the above URL. Documentation changes are pushed directly to main.

Release Process
---------------

A release is made by merging `release_candidate` back to `main`, and then applying a tag to that commit on `main`.

A pre-release is often created using the latest stable release candidate. These binaries will have extra debug statements turned on that will not be present in a formal release, but should not affect behavior of the firmware.

Building from Source
--------------------

For building the firmware, see the [Firmware README](Firmware/readme.md).

For the documentation, see [mkdocs.yml](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/main/mkdocs.yml) and [/workflows/mkdocs.yml](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/main/.github/workflows/mkdocs.yml).

For building the Uploader_GUI see the header comments of [RTK_Firkware_Uploader_GUI.py](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/main/Uploader_GUI/RTK_Firmware_Uploader_GUI.py)

For building the u-blox_Update_GUI see the header comments of [RTK_u-blox_Update_GUI.py](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/main/u-blox_Update_GUI/RTK_u-blox_Update_GUI.py)

License Information
-------------------

This product is _**open source**_!  Please feel free to [contribute](https://docs.sparkfun.com/SparkFun_RTK_Firmware/contribute/) to both the firmware and documentation.

Various bits of the code have different licenses applied. Anything SparkFun wrote is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
