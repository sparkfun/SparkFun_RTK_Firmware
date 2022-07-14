# Correction Transport

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Not Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/YellowDot.png)

Once a [correction source](https://docs.sparkfun.com/SparkFun_RTK_Firmware/correction_sources/) is chosen, the correction data must be transported from the base to the rover. The RTCM serial data is approximately 530 bytes per second and is transmitted at 57600bps out of the **RADIO** port on a SparkFun RTK device.

There are a variety of ways to move data from a base to a rover. We will cover the most common below. 

Note: RTK calculations require RTCM data to be delivered approximately once per second. If RTCM data is lost or not received by a rover, RTK Fix can still be maintained for many seconds before the device will enter RTK Float mode. This is beneficial where devices like Serial Radios may drop packets due to RF congestion.

**Note:** The RTK Facet L-Band is capable of receiving RTCM corrections from a terrestrial source but because it has a built-in L-Band receiver, we recommend using the satellite-based corrections.

## WiFi

![NTRIP Server setup](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/RTK_Surveyor_-_WiFi_Config_-_Base_Config2.jpg)

Any SparkFun RTK device can be set up as an [NTRIP Server](https://docs.sparkfun.com/SparkFun_RTK_Firmware/configure_base/#ntrip-server). This means the device will connect to local WiFi and broadcast its correction data to the internet. The data is delivered to something called an NTRIP Caster. Any number of rovers can then access this data using something called an NTRIP Client. Nearly *every* GIS application has an NTRIP Client built into it so this makes it very handy.

WiFi broadcasting is the most common transport method of getting RTCM correction data to the internet and to rovers via NTRIP Clients.

![RTK product in NTRIP Client mode](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_Rover_NTRIP_Client_Connection.png)

Similarly, any SparkFun RTK device can be set up as an [NTRIP Client](https://docs.sparkfun.com/SparkFun_RTK_Firmware/configure_gnss/#ntrip-client). The RTK device will connect to the local WiFi and begin downloading the RTCM data from the given NTRIP Caster and RTK Fix will be achieved. This is useful only if the Rover remains in RF range of the WiFi access point. Because of the limited range, we recommend using a cellphone rather than WiFi for NTRIP Clients.

## Cellular

![SW Maps NTRIP Client](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SW_Maps_-_NTRIP_Client.jpg)

Using a cellphone is the most common way of transporting correction data from the internet to a rover. This method uses the cell phone's built-in internet connection to obtain data from an NTRIP Caster and then pass those corrections over Bluetooth to the RTK device.

Shown above are SW Map's NTRIP Client Settings. Nearly all GIS applications have an NTRIP Client built in so we recommend leveraging the device you already own to save money. Additionally, a cell phone gives your rover incredible range: a rover can obtain RTCM corrections anywhere there is cellular coverage.

Cellular can even be used in Base mode. We have seen some very inventive users use an old cell phone as a WiFi access point. The base unit is configured as an NTRIP Server with the cellphone's WiFi AP credentials. The base performs a survey-in, connects to the WiFi, and the RTCM data is pushed over WiFi, over cellular, to an NTRIP Caster.

## L-Band

What if you are in the field, far away from WiFi, cellular, radio, or any other data connection? Look to the sky! 

A variety of companies provide GNSS RTK corrections broadcast from satellites over a spectrum called L-Band. [L-Band](https://en.wikipedia.org/wiki/L_band) is any frequency from 1 to 2 GHz. These frequencies have the ability to penetrate clouds, fog, and other natural weather phenomena making them particularly useful for location applications.

These corrections are not as accurate as a fixed base station, and the corrections can require a monthly subscription fee, but you cannot beat the ease of use!

L-Band reception requires specialized RF receivers capable of demodulating the satellite transmissions. Currently, the [RTK Facet L-Band](https://www.sparkfun.com/products/20000) is the only product that supports L-Band correction reception.

## Serial Radios

![Two serial radios](https://cdn.sparkfun.com//assets/parts/1/8/6/3/4/19032-SiK_Telemetry_Radio_V3_-_915MHz__100mW-01.jpg)

Serial radios, sometimes called telemetry radios, provide what is essentially a serial cable between the base and rover devices. Transmission distance, frequency, maximum data rate, configurability, and price vary widely, but all behave functionally the same. SparkFun recommends the [HolyBro 100mW](https://www.sparkfun.com/products/19032) and the [SparkFun LoRaSerial 1W](https://www.sparkfun.com/products/19311) radios for RTK use.

![Serial radio cable](https://cdn.sparkfun.com//assets/parts/1/6/2/2/2/17239-GHR-04V-S_to_GHR-06V-S_Cable_-_150mm-01.jpg)

All SparkFun RTK products include a [4-pin to 6-pin cable](https://www.sparkfun.com/products/17239) that will allow you to connect the HolyBro branded radio or the SparkFun LoRaSerial radios to a base and rover RTK device.

![Radio attached to RTK device](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_Radio.jpg)

These radios attach nicely to the back or bottom of an RTK device.

The benefit of a serial telemetry radio link is that you do not need to configure anything; simply plug two radios onto two RTK devices and turn them on. 

The downside to serial telemetry radios is that they generally have a much shorter range (often slightly more than a 1-kilometer functional range) than a cellular link can provide.
