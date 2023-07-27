# Configure with Settings File

Surveyor: ![Feature Partially Supported](img/Icons/YellowDot.png) / Express: ![Feature Partially Supported](img/Icons/YellowDot.png) / Express Plus: ![Feature Partially Supported](img/Icons/YellowDot.png) / Facet: ![Feature Partially Supported](img/Icons/YellowDot.png) / Facet L-Band: ![Feature Partially Supported](img/Icons/YellowDot.png) / Reference Station: ![Feature Partially Supported](img/Icons/YellowDot.png)

![SparkFun RTK Facet Settings File](img/SparkFun_RTK_Express_-_Settings_File.jpg)

*SparkFun RTK Settings File*

All device settings are stored both in internal memory and an SD card if one is detected. The device will load the latest settings at each power on. If there is a discrepancy between the internal settings and a settings file then the settings file will be used. This allows a collection of RTK products to be identically configured using one 'golden' settings file loaded onto an SD card.

All system configuration can be done by editing the *SFE_[Platform]_Settings_0.txt* file (example shown above) where [Platform] is Facet, Express, Surveyor, etc and 0 is the profile number (0, 1, 2, 3). This file is created when a microSD card is installed. The settings are clear text but there are no safety guards against setting illegal states. It is not recommended to use this method unless You Know What You're Doing®.

Keep in mind: 

* The settings file contains hundreds of settings.
* The SD card file "SFE_Express_Settings_0.txt" is used for Profile 1, SD card file "SFE_Express_Settings_1.txt" is used for Profile 2, etc. (note that setting 0 is for profile 1, ...)
* When switching to a new profile, the settings file on the SD card with all settings will be created or updated. The internal settings will not be updated until you switch to the profile. Additionally, the file for a particular profile will not be created on the SD card until you switch to that profile.
* It is not necessary that the settings file on the SD card have all of the settings.

For example, if you only wanted to set up two wireless networks for profile 2, you could create a file named "SFE_Express_Settings_1.txt" that only contained the following settings:

    profileName=a name you choose
    enableTcpServer=1
    wifiNetwork0SSID=your SSID name 1
    wifiNetwork0Password=your SSID password 1
    wifiNetwork1SSID=your SSID name 2
    wifiNetwork1Password=your SSID password 2
    wifiConfigOverAP=0

These settings on the SD card will overwrite the settings in the RTK Express internal memory. Once you select this profile on your RTK Express, the SD card file will be overwritten with all of the merged settings.

## Forcing a Factory Reset

![Setting size of settings to -1 to force reset](<img/SparkFun RTK Settings File - Factory Reset.png>)

If the device has been configured into an unknown state the device can be reset to factory defaults. Power down the RTK device and remove the SD card. Using a computer and an SD card reader, open the SFE_[Platform]_Settings_0.txt file where [Platform] is Facet, Express, Surveyor, etc and 0 is the profile number (0, 1, 2, 3). Modify the **sizeOfSettings** value to -1 and save the file to the SD card. Reinsert the SD card into the RTK unit and power up the device. Upon power up, the device will display 'Factory Reset' while it clears the settings.

Note: A device may have multiple profiles, ie multiple settings files (SFE_Express_Settings_**0**.txt, SFE_Express_Settings_**1**.txt, etc). All settings files found on the SD card must be modified to guarantee the factory reset.