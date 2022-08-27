# Displays

Surveyor: ![Feature Partially Supported](img/YellowDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

The RTK Facet, Facet L-Band, Express, and Express Plus utilize a 0.96" high-contrast OLED display. While small, it packs various situational data that can be helpful in the field. We will walk you through each display.

## Power On/Off

[![Startup and Shutdown Screens](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Display_On_Off.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Display_On_Off.jpg)

*RTK Facet Startup and Shutdown Screens*

Press and hold the power button until the display illuminates to turn on the device. Similarly, press and hold the power button to turn off the device.

The device's firmware version is shown during the Power On display.

## Rover Fix

[![Rover with location fix](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)

*Rover with location fix*

Upon power up the device will enter either Rover mode or Base mode. Above, the Rover mode is displayed.

* **MAC:** The MAC address of the internal Bluetooth module. This is helpful knowledge when attempting to connect to the device from your phone. This will change to a Bluetooth symbol once connected.
* **HPA:** Horizontal positional accuracy is an estimate of how accurate the current positional readings are. This number will decrease rapidly after the first power-up and settle around 0.3m depending on your antenna and view of the sky. When RTK fix is achieved this icon will change to a double circle and the HPA number will decrease even further to as low as 0.014m.
* **SIV:** Satellites in view is the number of satellites used for the fix calculation. This symbol will blink before a location fix is generated and become solid when the device has a good location fix. SIV is a good indicator of how good of a view the antenna has. This number will vary but anything above 10 is adequate. We've seen as high as 31.
* **Model:** This icon will change depending on the selected dynamic model: Portable (default) Pedestrian, Sea, Bike, Stationary, etc.
* **Log:** This icon will remain animated while the log file is increasing. This is a good visual indication that you have an SD card inserted and RTK Facet can successfully record to it. There are three logging icons ![Logging icons](img/Radios/SparkFun%20RTK%20Logging%20Types.png)
    * Standard (three lines) is shown when the standard 5 NMEA messages are being logged
    * PPP (capital P) is shown when the standard 5 NMEA + RAWX and SFRBX messages are recorded. This is most often used for post process positioning (PPP) and 12 to 24-hour logs for [fixed permanent bases](https://docs.sparkfun.com/SparkFun_RTK_Firmware/permanent_base/).
    * Custom (capital C) is shown when a custom set of messages are being recorded (not standard, and not PPP).

## Rover RTK Fix

[![Rover with RTK Fix and Bluetooth connected](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)

*Rover with RTK Fix and Bluetooth connected*

Once NTRIP is enabled on your phone or RTCM data is being streamed into the **Radio** port the device will gain an RTK Fix. You should see the HPA drop to 14mm with a double circle bulls-eye as shown above.

## Base Survey-In

[![RTK Facet in Survey-In Mode](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Survey-In.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_Survey-In.jpg)

*RTK device in Survey-In Mode*

Pressing the Setup button will change the device to Base mode. If the device is configured for *Survey-In* base mode, a flag icon will be shown and the survey will begin. The mean standard deviation will be shown as well as the time elapsed. For most Survey-In setups, the survey will complete when both 60 seconds have elapsed *and* a mean of 5m or less is obtained.

## Base Transmitting

[![RTK Facet in Fixed Transmit Mode](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_FixedBase-Xmitting.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_FixedBase-Xmitting.jpg)

*RTK Facet in Fixed Transmit Mode*

Once the *survey-in* is complete the device enters RTCM Transmit mode. The number of RTCM transmissions is displayed. By default, this is one per second.

The *Fixed Base* mode is similar but uses a structure icon (shown above) to indicate a fixed base.

## Base Transmitting NTRIP

If the NTRIP server is enabled the device will first attempt to connect over WiFi. The WiFi icon will blink until a WiFi connection is obtained. If the WiFi icon continually blinks be sure to check your SSID and PW for the local WiFi. 

[![RTK Facet in Transmit Mode with NTRIP](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_FixedBase-Casting.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/8/5/7/SparkFun_RTK_Express_-_Display_-_FixedBase-Casting.jpg)

*RTK Facet in Transmit Mode with NTRIP*


Once WiFi connects the device will attempt to connect to the NTRIP mount point. Once successful the display will show 'Casting' along with a solid WiFi icon. The number of successful RTCM transmissions will increase every second.

Note: During NTRIP transmission WiFi is turned on and Bluetooth is turned off. You should not need to know the location information of the base so Bluetooth should not be needed. If necessary, USB can be connected to the USB port to view detailed location and ZED-F9P configuration information.

## L-Band

L-Band decryption keys are valid for a maximum of 56 days. During that time, the RTK Facet L-Band can operate normally without the need for WiFi access. However, when the keys are set to expire in 28 days or less, the RTK Facet L-Band will attempt to log in to the 'Home' WiFi at each power on. If WiFi is not available, it will continue normal operation. 

[![Display showing 14 days until L-Band Keys Expire](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_LBand_DayToExpire.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_LBand_DayToExpire.jpg)

*Display showing 14 days until L-Band Keys Expire*

The unit will display various messages to aid the user in obtaining keys as needed.

[![Three-pronged satellite dish indicating L-Band reception](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_LBand_Indicator.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_LBand_Indicator.jpg)

*Three-pronged satellite dish indicating L-Band reception*

Upon successful reception and decryption of L-Band corrections, the satellite dish icon will increase to a three-pronged icon. As the unit's fix increases the cross-hair will indicate a basic 3D solution, a double blinking cross-hair will indicate a floating RTK solution, and a solid double cross-hair will indicate a fixed RTK solution.

## Adding a Display to the RTK Surveyor

While the RTK Surveyor works very well using only LEDs, it is possible to add an external display. The [SparkFun Micro OLED Breakout (Qwiic)](https://www.sparkfun.com/products/14532) can be attached to the Qwiic connector on the end of the Surveyor. At power on, the display will be automatically detected and used.