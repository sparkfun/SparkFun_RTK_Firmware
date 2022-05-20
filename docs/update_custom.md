# Creating Custom Firmware

The RTK Facet is an ESP32 and high-precision GNSS hackers’s delight. Writing custom firmware can be done using Arduino. 

[![Selecting ESP32 Dev Module](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_-_Select_ESP32_Dev_Module.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Firmware_Update_-_Select_ESP32_Dev_Module.jpg)

*Selecting ESP32 Dev Module*

Please see the [ESP32 Thing Plus Hookup Guide](https://learn.sparkfun.com/tutorials/esp32-thing-plus-hookup-guide/all#software-setup) for information about getting Arduino setup. The only difference is that you will need to select *ESP32 Dev Module* as your board.

[![Arduino Library Links](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Arduino_Setup_-_Library_Link.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_Arduino_Setup_-_Library_Link.jpg)

*Arduino Library Links*

Pull the entire [RTK Firmware repo](https://github.com/sparkfun/SparkFun_RTK_Firmware) and open `/Firmware/RTK_Surveyor/RTK_Surveyor.ino` and Arduino will open all the sub-files in new tabs. We’ve broken the functional pieces into smaller tabs to help users navigate it. There are a handful of libraries that will need to be installed. To make this easier, we’ve placed a link next to each library that will automatically open the Arduino Library Manager with that library ready for download.

After connecting a USB C cable to the ESP32 Config connector and selecting the correct COM port you should be able to upload new firmware through the Arduino IDE. Note: The RTK Facet must be turned on for it to enumerate as a COM port.
