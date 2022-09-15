# Updating RTK Firmware

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

The device has two primary firmwares:

* Firmware on the ESP32 microcontroller. Keep reading.
* Firmware on the u-blox ZED-F9P, ZED-F9P, or NEO-D9S Receiver. [See below](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-u-blox-firmware).

![[Main Menu showing RTK Firmware v1.8-Oct 7 2021](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Serial_Config_-_Main.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Serial_Config_-_Main.jpg)

*Main Menu showing RTK Firmware v1.8-Oct 7 2021*

You can check your firmware by opening the main menu by pressing a key at any time.

From time to time SparkFun will release new firmware for the RTK product line to add and improve functionality. For most users, firmware can be upgraded by loading the appropriate binary file located on the [releases page](https://github.com/sparkfun/SparkFun_RTK_Firmware/releases) or from the [binaries repo](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries). Once the firmware is downloaded, loading the firmware onto an RTK product can be achieved by using one of the following methods:

* [GUI Method](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-using-windows-gui): Use the [Windows GUI](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/raw/main/Uploader_GUI/Windows_exe/RTK_Firmware_Uploader_GUI.exe) and a USB cable. (This method is python based which can also be used on Linux, Mac OS, and Raspberry Pi)
* [SD Method](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-the-sd-card): Load the firmware on an SD card, then use a serial terminal with the *Firmware Upgrade* menu
* [WiFi Method](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-wifi): Load the firmware over WiFi when the device is in WiFi AP Config Mode
* [CLI Method](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-cli): Use the command line *batch_program.bat*

The GUI method is generally recommended. For more information see [here](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-using-windows-gui).

Remember, all SparkFun RTK devices are open source hardware meaning you have total access to the [firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware) and [hardware](https://github.com/sparkfun/SparkFun_RTK_Facet). Be sure to check out each repo for the latest firmware and hardware information.

## Updating Firmware Using Windows GUI

![RTK Firmware GUI](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/blob/main/u-blox_Update_GUI/SparkFun_RTK_u-blox_Update_GUI.jpg?raw=true)

*RTK Firmware GUI*

This GUI makes it easy to point and click your way through a firmware update.

Download the GUI [here](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/raw/main/Uploader_GUI/Windows_exe/RTK_Firmware_Uploader_GUI.exe). 

Download the latest binary file located on the [releases page](https://github.com/sparkfun/SparkFun_RTK_Firmware/releases) or from the [binaries repo](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries).

**To Use**

* Attach the RTK device to your computer using a USB cable. 
* Turn the RTK device on.
* Open Windows Device Manager to confirm which COM port the device is operating on.

![Device Manager showing USB-Serial CH340 port on COM27](img/SparkFun%20RTK%20Firmware%20Uploader%20COM%20Port.jpg)

*Device Manager showing 'USB-Serial CH340' port on COM27*

* Get the latest binary file located on the [releases page](https://github.com/sparkfun/SparkFun_RTK_Firmware/releases) or from the [binaries repo](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries).
* Run *[RTK_Firmware_Uploader_GUI.exe](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/raw/main/Uploader_GUI/Windows_exe/RTK_Firmware_Uploader_GUI.exe)* (it takes a few seconds to start)
* Click *Browse* and select the binary file to upload
* Select the COM port previously seen in the Device Manager
* Click *Upload Firmware*

Once complete, the device will reset and power down.

## Updating Firmware From the SD Card

[![Firmware update menu](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Firmware_Update.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Firmware_Update.jpg)

*Firmware update taking place*

Download the latest binary file located on the [releases page](https://github.com/sparkfun/SparkFun_RTK_Firmware/releases) or from the [binaries repo](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries).

The firmware upgrade menu will only display files that have the "RTK_Surveyor_Firmware*.bin" file name format so don't change the file names once loaded onto the SD card. Select the firmware you'd like to load and the system will proceed to load the new firmware, then reboot.

**Note:** The firmware is called `RTK_Surveyor_Firmware_vXX.bin` even though there are various RTK products (Facet, Express, Surveyor, etc). We united the different platforms into one. The [RTK Firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware) runs on all our RTK products.

### Force Firmware Loading

In the rare event that a unit is not staying on long enough for new firmware to be loaded into a COM port, the RTK Firmware (as of version 1.2) has an override function. If a file named *RTK_Surveyor_Firmware_Force.bin* is detected on the SD card at boot that file will be used to overwrite the current firmware, and then be deleted. This update path is generally not recommended. Use the [GUI](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-using-windows-gui) or [WiFi OTA](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-wifi) methods as the first resort.

## Updating Firmware From WiFi

**Note:** Firmware versions 1.1 to 1.9 have an issue that severely limits firmware upload over WiFi and is not recommended; use the 'Updating Firmware From Windows GUI' method instead. Firmware versions v1.10 and beyond support direct firmware updates via WiFi.

[![Advanced system settings](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Firmware_Update_Button.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_WiFi_Config_Firmware_Update_Button.jpg)

*Advanced system settings*

**Versions 1.10 and Greater:** Firmware may be uploaded to the unit by clicking on 'Choose File', selecting the binary such as 'RTK_Surveyor_Firmware_v1_xx.bin' and pressing upload. The unit will automatically reset once the firmware upload is complete.

## Updating Firmware From CLI

The command-line interface is also available. You’ll need to download the [RTK Firmware Binaries](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries) repo. This repo contains the binaries but also various supporting tools including esptool.exe and the three binaries required along with the firmware (bootloader, partitions, and app0).

### Windows

Connect a USB A to C cable from your computer to the ESP32 port on the RTK device. Turn the unit on. Now identify the COM port the RTK enumerated at. The easiest way to do this is to open the Device Manager:

[![CH340 is on COM6 as shown in Device Manager](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_COM_Port.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_COM_Port.jpg)

*CH340 is on COM6 as shown in Device Manager*

If the COM port is not showing be sure the unit is turned **On**. If an unknown device is appearing, you’ll need to [install drivers for the CH340](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all). Once you know the COM port, open a command prompt (Windows button + r then type ‘cmd’).

![batch_program.bat running esptool](img/SparkFun%20RTK%20Firmware%20Update%20CLI.png)

*batch_program.bat running esptool*

Once the correct COM is identified, run 'batch_program.bat' along with the binary file name and COM port. For example *batch_program.bat RTK_Surveyor_Firmware_v2_0.bin COM6*. COM6 should be replaced by the COM port you identified earlier.

The batch file runs the following commands:

esptool.exe --chip esp32 --port COM6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 ./bin/RTK_Surveyor.ino.bootloader.bin 0x8000 ./bin/RTK_Surveyor_Partitions_16MB.bin 0xe000 ./bin/boot_app0.bin 0x10000 ./RTK_Surveyor_Firmware_vxx.bin

Where *COM6* is replaced with the COM port that the RTK product enumerated at and *RTK_Surveyor_Firmware_vxx.bin* is the firmware you would like to load.

**Note:** Some users have reported the 921600bps baud rate does not work. Decrease this to 115200 as needed.

Upon completion, your RTK device will reset and power down.

### macOS / Linux

Get [esptool.py](https://github.com/espressif/esptool). Connect a USB A to C cable from your computer to the ESP32 port on the RTK device. Turn the unit on. Now identify the COM port the RTK enumerated at. 

If the COM port is not showing be sure the unit is turned **On**. If an unknown device is appearing, you’ll need to [install drivers for the CH340](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all). Once you know the COM port, run the following command:

py esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 ./bin/RTK_Surveyor.ino.bootloader.bin 0x8000 ./bin/RTK_Surveyor_Partitions_16MB.bin 0xe000 ./bin/boot_app0.bin 0x10000 ./RTK_Surveyor_Firmware_vxx.bin

Where */dev/ttyUSB0* is replaced with the port that the RTK product enumerated at and *RTK_Surveyor_Firmware_vxx.bin* is the firmware you would like to load.

**Note:** Some users have reported the 921600bps baud rate does not work. Decrease this to 115200 as needed.

Upon completion, your RTK device will reset and power down.

## Updating 4MB Surveyors

RTK Surveyors sold before September 2021 may have an ESP32 WROOM module with 4MB flash instead of 16MB flash. These units still support all the functionality of other RTK products with the following limitations:

* There is not enough flash space for OTA. Upgrading the firmware must be done via [CLI](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-from-cli) or [GUI](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-firmware-using-windows-gui). WiFi or SD update paths are not possible.

The GUI (as of v1.3) will autodetect the ESP32's flash size and load the appropriate partition file. No user interaction is required.

If you are using the CLI method, be sure to point at the [4MB partition file](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/blob/main/bin/RTK_Surveyor_Partitions_4MB.bin?raw=true). For example:

esptool.exe --chip esp32 --port COM6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 ./bin/RTK_Surveyor.ino.bootloader.bin 0x8000 ./bin/**RTK_Surveyor_Partitions_4MB**.bin 0xe000 ./bin/boot_app0.bin 0x10000 ./RTK_Surveyor_Firmware_vxx.bin

### Determining Size of Flash

To determine if the device has a 4MB module:

* Use the esptool via CLI. Please see the [flash_id](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html#read-spi-flash-id-flash-id) command for usage.
* Use the GUI and attempt a firmware update. The output will auto-detect and show the flash size, as shown below:

![Module with 4MB flash](img/SparkFun%20RTK%20Firmware%20Update%20GUI%20-%204MB.png)

## Updating u-blox Firmware

The following products contain the following u-blox receivers:

* RTK Surveyor: ZED-F9P
* RTK Express: ZED-F9P
* RTK Express Plus: ZED-F9R
* RTK Facet: ZED-F9P
* RTK Facet L-Band: ZED-F9P and NEO-D9S

The firmware loaded onto the ZED-F9P, ZED-F9R, and NEO-D9S receivers is written by u-blox and can vary depending on the manufacture date. The RTK Firmware (that runs on the ESP32) is designed to flexibly work with any u-blox firmware. Upgrading the ZED-F9x/NEO-D9S is a good thing to consider but is not crucial to the use of RTK products.

Not sure what firmware is loaded onto your RTK product? Open the [System Status Menu](https://docs.sparkfun.com/SparkFun_RTK_Firmware/menu_system_status/) to display the module's current firmware version.

The firmware on u-blox devices can be updated using a [Windows-based GUI](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-using-windows-gui) or [u-center](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#updating-using-u-center). A CLI method is also possible using the ubxfwupdate.exe tool provided with u-center. Additionally, u-blox offers the source for the ubxfwupdate tool that is written in C. It is currently released only under an NDA so contact your local u-blox Field Applications Engineer if you need a different method.

## Updating Using Windows GUI

![SparkFun u-blox firmware update tool](img/SparkFun%20RTK%20Facet%20L-Band%20u-blox%20Firmware%20Update%20GUI.png)

*SparkFun RTK u-blox Firmware Update Tool*

The [SparkFun RTK u-blox Firmware Update Tool](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/tree/main/u-blox_Update_GUI) is a simple Windows GUI and python script that runs the ubxfwupdate.exe tool. This allows users to directly update module firmware without the need for u-center. Additionally, this tool queries the module to verify that the firmware type matches the module. Because the RTK Facet L-Band contains two u-blox modules that both appear as identical serial ports, it can be difficult and perilous to know which port to load firmware. This tool prevents ZED-F9P firmware from being accidentally loaded onto a NEO-D9S receiver and vice versa.

The SparkFun RTK u-blox Firmware Update Tool will only run on Windows as it relies upon u-blox's ubxfwupdate.exe. The full, integrated executable for Windows is available [here](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/raw/main/u-blox_Update_GUI/Windows_exe/RTK_u-blox_Update_GUI.exe).

* Attach the RTK device's USB port to your computer using a USB cable
* Turn the RTK device on
* Open Device Manager to confirm which COM port the device is operating on

![Device Manager showing USB Serial port on COM14](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/raw/main/u-blox_Update_GUI/SparkFun_RTK_u-blox_Updater_COM_Port.jpg)

*Device Manager showing USB Serial port on COM14*

* Get the latest binary firmware file from the [ZED Firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/tree/main/ZED%20Firmware), [NEO Firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/tree/main/NEO%20Firmware) folder, or the [u-blox](https://www.u-blox.com/) website
* Run *RTK_u-blox_Update_GUI.exe* (it takes a few seconds to start)
* Click the Firmware File *Browse* and select the binary file for the update
* Select the COM port previously seen in the Device Manager
* Click *Update Firmware*

Once complete, the u-blox module will restart.

### Updating Using u-center

If you're familiar with u-center a tutorial with step-by-step instructions for locating the firmware version as well as changing the firmware can be found in [How to Upgrade Firmware of a u-blox Receiver](https://learn.sparkfun.com/tutorials/how-to-upgrade-firmware-of-a-u-blox-gnss-receiver/all).

### ZED-F9P Firmware Changes

This module is used in the Surveyor, Express, and Facet. It is capable of both Rover *and* base modes.

Most of these binaries can be found in the [ZED Firmware/ZED-F9P](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/tree/main/ZED%20Firmware/ZED-F9P) folder.

All field testing and device-specific performance parameters were obtained with ZED-F9P v1.30.

* v1.12 has the benefit of working with SBAS and an operational RTK status signal (the LED illuminates correctly). See [release notes](https://content.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG112_RN_%28UBX-19026698%29.pdf).

* v1.13 has a few RTK and receiver performance improvements but introduces a bug that causes the RTK Status LED to fail when SBAS is enabled. See [release notes](https://content.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG113_RN_%28UBX-20019211%29.pdf).

* v1.30 has a few RTK and receiver performance improvements, I<sup>2</sup>C communication improvements, and most importantly support for SPARTN PMP packets. See [release notes](https://www.u-blox.com/sites/default/files/ZED-F9P-FW100-HPG130_RN_UBX-21047459.pdf).

* v1.32 has a few SPARTN protocol-specific improvements. See [release notes](https://www.u-blox.com/sites/default/files/documents/ZED-F9P-FW100-HPG132_RN_UBX-22004887.pdf). This firmware is required for use with the NEO-D9S and the decryption of PMP messages.

### ZED-F9R Firmware Changes

This module is used in the Express Plus. It contains an internal IMU and additional algorithms to support high-precision location fixes using dead reckoning. The ZED-F9R is not capable of operating in base mode.

Most of these binaries can be found in the [ZED Firmware/ZED-F9R](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/tree/main/ZED%20Firmware/ZED-F9R) folder.

* v1.00 Initial release.

* v1.21 SPARTN support as well as adding E-scooter and robotic lawnmower dynamic models. See [release notes](https://www.u-blox.com/sites/default/files/ZED-F9R-02B_FW1.00HPS1.21_RN_UBX-21035491_1.3.pdf).

### NEO-D9S Firmware Changes

This module is used in the Facet L-Band to receive encrypted PMP messages over ~1.55GHz broadcast via a geosynchronous Inmarsat.

This binary file can be found in the [NEO Firmware](https://github.com/sparkfun/SparkFun_RTK_Firmware_Binaries/tree/main/NEO%20Firmware) folder.

* v1.04 Initial release.

As of writing, no additional releases of the NEO-D9S firmware have been made.

## Compiling Source

### Windows

The SparkFun RTK firmware is compiled using Arduino (currently v1.8.15). To compile:

1. Install [Arduino](https://www.arduino.cc/en/software). 
2. Install ESP32 for Arduino. [Here](https://learn.sparkfun.com/tutorials/esp32-thing-hookup-guide#installing-via-arduino-ide-boards-manager) are some good instructions for installing it via the Arduino Boards Manager. **Note**: Use v2.0.2 of the core. **Note:** We use the 'ESP32 Dev Module' for pin numbering. Select the correct board under Tools->Board->ESP32 Arduino->ESP32 Dev Module.
3. Change the Partition table. Replace 'C:\Users\\[user name]\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.2\tools\partitions\app3M_fat9M_16MB.csv' with the app3M_fat9M_16MB.csv [file](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/main/Firmware/app3M_fat9M_16MB.csv?raw=true) found in the [Firmware folder](https://github.com/sparkfun/SparkFun_RTK_Firmware/tree/main/Firmware). This will increase the program partition from a maximum of 1.9MB to 3MB.
4. From the Arduino IDE, set the core settings from the **Tools** menu: 
    
    A. Set the 'Partition Scheme' to *16M Flash (3MB APP/9MB FATFS)*. This will use the 'app3M_fat9M_16MB.csv' updated partition table.
    
    B. Set the 'Flash Size' to 16MB (128mbit)

5. Obtain all the required libraries. **Note:** You should click on the link next to each of the #includes at the top of RTK_Surveyor.ino within the Arduino IDE to open the library manager and download them. Getting them directly from Github also works but may not be 'official' releases:
    * [ESP32Time](https://github.com/fbiego/ESP32Time)
    * [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) (not available via library manager)
    * [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) (not available via library manager)
    * [JC_Button](https://github.com/JChristensen/JC_Button)
    * [SdFat](https://github.com/greiman/SdFat)
    * [SparkFun u-blox GNSS Arduino Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library)
    * [SparkFun MAX1704x Fuel Gauge Arduino Library](https://github.com/sparkfun/SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library)
    * [SparkFun Micro OLED Arduino Library](https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library) -  Note the Arduino Library manager lists this as 'SparkFun Micro OLED Breakout'
    * [SparkFun LIS2DH12 Accelerometer Arduino Library](https://github.com/sparkfun/SparkFun_LIS2DH12_Arduino_Library)
    * [Arduino JSON](https://github.com/bblanchon/ArduinoJson)
    * [PubSub Client for MQTT](https://github.com/knolleary/pubsubclient)
    * [ESP32 BleSerial](https://github.com/avinabmalla/ESP32_BleSerial)

Once compiled, firmware can be uploaded directly to a unit when the RTK unit is on and the correct COM port is selected under the Arduino IDE Tools->Port menu.

If you are seeing the error:

> text section exceeds available space ...

You have not replaced the partition file correctly. See the 'Change Partition table' step inside the [Windows instructions](https://docs.sparkfun.com/SparkFun_RTK_Firmware/firmware_update/#windows_1).

**Note:** There are a variety of compile guards (COMPILE_WIFI, COMPILE_AP, etc) at the top of RTK_Surveyor.ino that can be commented out to remove them from compilation. This will greatly reduce the firmware size and allow for faster development of functions that do not rely on these features (serial menus, system configuration, logging, etc).

### Ubuntu 20.04

#### Virtual Machine

Execute the following commands to create the Linux virtual machine:

1. Using a browser, download the Ubuntu 20.04 Desktop image
2. virtualbox
    1. Click on the new button
    2. Specify the machine Name, e.g.: Sparkfun_RTK_20.04
    3. Select Type: Linux
    4. Select Version: Ubuntu (64-bit)
    5. Click the Next> button
    6. Select the memory size: 7168
    7. Click the Next> button
    8. Click on Create a virtual hard disk now
    9. Click the Create button
    10. Select VDI (VirtualBox Disk Image)
    11. Click the Next> button
    12. Select Dynamically allocated
    13. Click the Next> button
    14. Select the disk size: 128 GB
    15. Click the Create button
    16. Click on Storage
    17. Click the empty CD icon
    18. On the right-hand side, click the CD icon
    19. Click on Choose a disk file...
    20. Choose the ubuntu-20.04... iso file
    21. Click the Open button
    22. Click on Network
    23. Under 'Attached to:' select Bridged Adapter
    24. Click the OK button
    25. Click the Start button
2. Install Ubuntu 20.04
3. Log into Ubuntu
4. Click on Activities
5. Type terminal into the search box
6. Optionally install the SSH server
    1. In the terminal window
        1. sudo apt install -y net-tools openssh-server
        2. ifconfig

        Write down the IP address

    2. On the PC
        1. ssh-keygen -t rsa -f ~/.ssh/Sparkfun_RTK_20.04
        2. ssh-copy-id -o IdentitiesOnly=yes -i ~/.ssh/Sparkfun_RTK_20.04  &lt;username&gt;@&lt;IP address&gt;
        3. ssh -Y &lt;username&gt;@&lt;IP address&gt;

#### Build Environment

Execute the following commands to create the build environment for the SparkFun RTK Firmware:

1. sudo adduser $USER dialout
2. sudo shutdown -r 0

Reboot to ensure that the dialout privilege is available to the user

3. sudo apt update
4. sudo apt install -y  git  gitk  git-cola  minicom  python3-pip
5. sudo pip3 install pyserial
6. mkdir ~/SparkFun
7. mkdir ~/SparkFun/esptool
8. cd ~/SparkFun/esptool
9. git clone https://github.com/espressif/esptool .
10. cd ~/SparkFun
11. nano serial-port.sh

Insert the following text into the file:

    #!/bin/bash
    #   serial-port.sh
    #
    #   Shell script to read the serial data from the RTK Express ESP32 port
    #
    #   Parameters:
    #       1:  ttyUSBn
    #
    sudo minicom -b 115200 -8 -D /dev/$1 < /dev/tty

12. chmod +x serial-port.sh
13. nano new-firmware.sh

Insert the following text into the file:

    #!/bin/bash
    #   new-firmware.sh
    #
    #   Shell script to load firmware into the RTK Express via the ESP32 port
    #
    #   Parameters:
    #      1: ttyUSBn
    #      2: Firmware file
    #
    sudo python3 ~/.arduino15/packages/esp32/tools/esptool_py/*/esptool.py --chip esp32 --port /dev/$1 --baud 230400 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect \
    0x1000   ~/SparkFun/RTK/Binaries/bin/RTK_Surveyor.ino.bootloader.bin \
    0x8000   ~/SparkFun/RTK/Binaries/bin/RTK_Surveyor_Partitions_16MB.bin \
    0xe000   ~/SparkFun/RTK/Binaries/bin/boot_app0.bin \
    0x10000  $2

14. chmod +x new-firmware.sh

Get the SparkFun RTK Firmware sources

15. mkdir ~/SparkFun/RTK
16. cd ~/SparkFun/RTK
17. git clone https://github.com/sparkfun/SparkFun_RTK_Firmware .

Install the Arduino IDE

18. mkdir ~/SparkFun/arduino
19. cd ~/SparkFun/arduino
20. wget https://downloads.arduino.cc/arduino-1.8.15-linux64.tar.xz
21. tar -xvf ./arduino-1.8.15-linux64.tar.xz
22. cd arduino-1.8.15/
23. sudo ./install.sh

Add the ESP32 support

24. arduino
    1. Click on File in the menu bar
    2. Click on Preferences
    3. Go down to the Additional Boards Manager URLs text box
    4. Only if the textbox already has a value, go to the end of the value or values and add a comma
    5. Add the link: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    6. Note the value in Sketchbook location
    7. Click the OK button
    8. Click on File in the menu bar
    9. Click on Quit

Get the required external libraries, then add to the Sketchbook location from above

25. cd   ~/Arduino/libraries
26. mkdir AsyncTCP
27. cd AsyncTCP/
28. git clone https://github.com/me-no-dev/AsyncTCP.git .
29. cd ..
30. mkdir ESPAsyncWebServer
31. cd ESPAsyncWebServer
32. git clone https://github.com/me-no-dev/ESPAsyncWebServer .

Connect the Config ESP32 port of the RTK to a USB port on the computer

33. ls /dev/ttyUSB*

Enable the libraries in the Arduino IDE

34. arduino
    1. From the menu, click on File
    2. Click on Open...
    3. Select the ~/SparkFun/RTK/Firmware/RTK_Surveyor/RTK_Surveyor.ino file
    4. Click on the Open button

    Select the ESP32 development module

    5. From the menu, click on Tools
    6. Click on Board
    7. Click on Board Manager…
    8. Click on esp32
    9. Select version 2.0.2
    10. Click on the Install button in the lower right
    11. Close the Board Manager...
    12. From the menu, click on Tools
    13. Click on Board
    14. Click on ESP32 Arduino
    15. Click on ESP32 Dev Module

    Load the required libraries

    15. From the menu, click on Tools
    16. Click on Manage Libraries…
    17. For each of the following libraries:
        1. Locate the library
        2. Click on the library
        3. Click on the Install button in the lower right

        Library List:

        * ArduinoJson
        * ESP32Time
        * ESP32_BleSerial
        * JC_Button
        * MAX17048 - Used for “Test Sketch/Batt_Monitor”
        * PubSubClient
        * SdFat
        * SparkFun LIS2DH12 Accelerometer Arduino Library
        * SparkFun MAX1704x Fuel Gauge Arduino Library
        * SparkFun Qwiic OLED Graphics Library
        * SparkFun u-blox GNSS Arduino Library

    18. Click on the Close button

    Select the terminal port

    19. From the menu, click on Tools
    20. Click on Port, Select the port that was displayed in step 38 above
    21. Select /dev/ttyUSB0
    22. Click on Upload Speed
    23. Select 230400

    Setup the partitions for the 16 MB flash

    24. From the menu, click on Tools
    25. Click on Flash Size
    26. Select 16MB
    27. From the menu, click on Tools
    28. Click on Partition Scheme
    29. Click on 16M Flash (3MB APP/9MB FATFS)
    30. From the menu click on File
    31. Click on Quit

35. cd ~/SparkFun/RTK/
36. cp  Firmware/app3M_fat9M_16MB.csv  ~/.arduino15/packages/esp32/hardware/esp32/2.0.2/tools/partitions/app3M_fat9M_16MB.csv

### Arduino CLI

The firmware can be compiled using [Arduino CLI](https://github.com/arduino/arduino-cli). This makes compilation fairly platform independent and flexible. All release candidates and firmware releases are compiled using Arduino CLI using a github action. You can see the source of the action [here](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/main/.github/workflows/compile-release.yml), and use it as a starting point for Arduino CLI compilation.
