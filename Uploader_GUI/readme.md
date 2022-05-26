This folder contains the python based GUI for updating firmware. There is an executable available for Windows and a python script available for Linux, Mac OS, and Raspberry Pi.

![SparkFun RTK Firmware GUI](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/release_candidate/Uploader_GUI/SparkFun RTK Firmware Uploader.jpg)

The tool is based on the esptool.py and loads RTK_Surveyor.ino.bootloader.bin and RTK_Surveyor.ino.partitions.bin files along side the chosen firmware file. This tool was created to aid users in updating their firmware. In general, the SD firmware update method is recommended, but for some firmware updates (for example from version v1.x to v2.x) a serial connection via USB is required. This GUI makes it easy to point and click your way through a firmware update.

### To Use

* Attach the RTK device to your computer using a USB cable. 
* Turn the RTK device on.
* Open Device Manager to confirm which COM port the device is operating on.

![Device Manager showing USB Serial port on COM27](https://github.com/sparkfun/SparkFun_RTK_Firmware/blob/release_candidate/Uploader_GUI/SparkFun RTK Firmware Uploader COM Port.jpg)

-> *Device Manager showing USB Serial port on COM27* <-

* Get the latest binary firmware file from the *[Binaries](https://github.com/sparkfun/SparkFun_RTK_Firmware/tree/main/Binaries)* folder.
* Run *RTK_Firmware_Uploader_GUI.exe* (it takes a few seconds to start)
* Click *Browse* and select the binary file to upload
* Select the COM port previously seen in the Device Manager.
* Click *Upload Firmware*

Once complete, the device will reset and power down.
