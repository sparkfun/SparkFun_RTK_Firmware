# GIS Software

Surveyor: ![Feature Supported](img/GreenDot.png) / Express: ![Feature Supported](img/GreenDot.png) / Express Plus: ![Feature Supported](img/GreenDot.png) / Facet: ![Feature Supported](img/GreenDot.png) / Facet L-Band: ![Feature Supported](img/GreenDot.png)

While we recommend SW Maps for Android, there are a variety of 3rd party apps available for GIS and surveying. We will cover a few examples below that should give you an idea of how to get the incoming NMEA data over Bluetooth into the software of your choice.

## SW Maps

The best mobile app that we’ve found is the powerful, free, and easy-to-use _SW Maps_](https://play.google.com/store/apps/details?id=np.com.softwel.swmaps)* by Softwel. You’ll need an Android phone or tablet with Bluetooth. What makes SW Maps truly powerful is its built-in NTRIP client. This is a fancy way of saying that we’ll be showing you how to get RTCM correction data over the cellular network. 

Be sure your device is [paired over Bluetooth](https://sparkfun.github.io/SparkFun_RTK_Firmware/connecting_bluetooth/#android).


![List of BT Devices in SW Maps](img/SparkFun%20RTK%20SWMaps%20Bluetooth%20Connect.png)

*List of available Bluetooth devices*

From SW Map's main menu, select *Bluetooth GNSS*. This will display a list of available Bluetooth devices. Select the Rover or Base you just paired with. If you are taking height measurements (altitude) in addition to position (lat/long) be sure to enter the height of your antenna off the ground including any [ARP offsets](https://geodesy.noaa.gov/ANTCAL/FAQ.xhtml#faq4) of your antenna (this should be printed on the side).

Click on 'CONNECT' to open a Bluetooth connection. Assuming this process takes a few seconds, you should immediately have a location fix.

![SW Maps with RTK Fix](img/SparkFun%20RTK%20SWMaps%20GNSS%20Status.png)

*SW Maps with RTK Fix*

You can open the GNSS Status sub-menu to view the current data.

**NTRIP Client**

If you’re using a serial radio to connect a Base to a Rover for your correction data, or if you're using the RTK Facet L-Band with built-in corrections, you can skip this part.

We need to send RTCM correction data from the phone back to the RTK device so that it can improve its fix accuracy. This is the amazing power of the SparkFun RTK products and SW Maps. Your phone can be the radio link! From the main SW Maps menu select NTRIP Client. Not there? Be sure the 'SparkFun RTK' instrument was automatically selected connecting. Disconnect and change the instrument to 'SparkFun RTK' to enable the NTRIP Connection option.

[![SW Maps NTRIP Connection menu](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_SW_Maps_NTRIP_Connection.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SparkFun_RTK_Surveyor_-_SW_Maps_NTRIP_Connection.jpg)

*NTRIP Connection - Not there? Be sure to select 'SparkFun RTK' was selected as the instrument*

[![SW Maps NTRIP client](https://cdn.sparkfun.com/r/600-600/assets/learn_tutorials/1/4/6/3/SW_Maps_-_NTRIP_Client.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/1/4/6/3/SW_Maps_-_NTRIP_Client.jpg)

*Connecting to an NTRIP Caster*

Enter your NTRIP Caster credentials and click connect. You will see bytes begin to transfer from your phone to the RTK Express. Within a few seconds, the RTK Express will go from ~300mm accuracy to 14mm. Pretty nifty, no?

What's an NTRIP Caster? In a nutshell, it's a server that is sending out correction data every second. There are thousands of sites around the globe that calculate the perturbations in the ionosphere and troposphere that decrease the accuracy of GNSS accuracy. Once the inaccuracies are known, correction values are encoded into data packets in the RTCM format. You, the user, don't need to know how to decode or deal with RTCM, you simply need to get RTCM from a source within 10km of your location into the RTK Express. The NTRIP client logs into the server (also known as the NTRIP caster) and grabs that data, every second, and sends it over Bluetooth to the RTK Express.

Don't have access to an NTRIP Caster? You can use a 2nd RTK product operating in Base mode to provide the correction data. Checkout [Creating a Permanent Base](https://sparkfun.github.io/SparkFun_RTK_Firmware/permanent_base/). If you're the DIY sort, you can create your own low-cost base station using an ESP32 and a ZED-F9P breakout board. Check out [How to](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station) Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station). If you'd just like a service, [Syklark](https://www.swiftnav.com/skylark) provides RTCM coverage for $49 a month (as of writing) and is extremely easy to set up and use. Remember, you can always use a 2nd RTK device in *Base* mode to provide RTCM correction data but it will be less accurate than a fixed position caster.

Once you have a full RTK fix you'll notice the location bubble in SW Maps turns green. Just for fun, rock your rover monopole back and forth on a fixed point. You'll see your location accurately reflected in SW Maps. Millimeter location precision is a truly staggering thing.

## Field Genius

[Field Genius for Android](https://www.microsurvey.com/products/fieldgenius-for-android/) is another good solution, albeit a lot more expensive than free. 

Be sure your device is [paired over Bluetooth](https://sparkfun.github.io/SparkFun_RTK_Firmware/connecting_bluetooth/#android).

![Main Menu](img/FieldGenius/Field%20Genius%202.png)

From the Main Menu open `Select Instrument`.

![Add Profile](img/FieldGenius/Field%20Genius%203.png)

Click the 'Add Profile' button.

![New Instrument Profile](img/FieldGenius/Field%20Genius%204.png)

Click `GNSS Rover` and select *NMEA* as the Make. Set your Profile Name to something memorable like 'RTK-Express' then click the 'Create' button.

![Set up communication](img/FieldGenius/Field%20Genius%205.png)

Click on 'SET UP COMMUNICATION'.

![Bluetooth Search Button](img/FieldGenius/Field%20Genius%207.png)

From the Bluetooth communication page, click the 'Search' button.

![List of paired Bluetooth devices](img/FieldGenius/Field%20Genius%206.png)

You will be shown a list of paired devices. Select the RTK device you'd like to connect to then click 'Connect'. The RTK device will connect and the MAC address shown on the RTK device OLED will change to the Bluetooth icon indicating a link is open.

**NTRIP Client**

If you’re using a serial radio to connect a Base to a Rover for your correction data, or if you're using the RTK Facet L-Band with built-in corrections, you can skip this part.

![Set up corrections](img/FieldGenius/Field%20Genius%208.png)

We need to send RTCM correction data from the phone back to the RTK device so that it can improve its fix accuracy. Your phone can be the radio link! Click on 'SET UP CORRECTIONS'.

![RTK via Internet](img/FieldGenius/Field%20Genius%209.png)

Click on 'RTK via Internet' then 'SET UP INTERNET', then 'Done'.

![Set up NTRIP data source](img/FieldGenius/Field%20Genius%2010.png)

Click on 'SET UP DATA SOURCE'.

![Adding a new source](img/FieldGenius/Field%20Genius%2011.png)

Click 'Add New Source'.

![NTRIP Credential Entry](img/FieldGenius/Field%20Genius%2012.png)

Enter your NTRIP Caster credentials and click 'DONE'. 

What's an NTRIP Caster? In a nutshell, it's a server that is sending out correction data every second. There are thousands of sites around the globe that calculate the perturbations in the ionosphere and troposphere that decrease the accuracy of GNSS accuracy. Once the inaccuracies are known, correction values are encoded into data packets in the RTCM format. You, the user, don't need to know how to decode or deal with RTCM, you simply need to get RTCM from a source within 10km of your location into the RTK Express. The NTRIP client logs into the server (also known as the NTRIP caster) and grabs that data, every second, and sends it over Bluetooth to the RTK Express.

Don't have access to an NTRIP Caster? You can use a 2nd RTK product operating in Base mode to provide the correction data. Checkout [Creating a Permanent Base](https://sparkfun.github.io/SparkFun_RTK_Firmware/permanent_base/). If you're the DIY sort, you can create your own low-cost base station using an ESP32 and a ZED-F9P breakout board. Check out [How to](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station) Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station). If you'd just like a service, [Syklark](https://www.swiftnav.com/skylark) provides RTCM coverage for $49 a month (as of writing) and is extremely easy to set up and use. Remember, you can always use a 2nd RTK device in *Base* mode to provide RTCM correction data but it will be less accurate than a fixed position caster.

![Selecting data source](img/FieldGenius/Field%20Genius%2011.png)

Click 'My NTRIP1' then 'Done' and 'Connect'. 

You will then be presented with a list of Mount Points. Select the mount point you'd like to use then click 'Select' then 'Confirm'.

Select 'Done' then from the main menu select 'Survey' to begin using the device.

![Surveying Screen](img/FieldGenius/Field%20Genius%201.png)

Now you can begin using the SparkFun RTK device with Field Genius.

## SurvPC

Note: The company behind SurvPC, Carlson Software, is not always welcoming to competitors of their $18,000 devices, so be warned.

Be sure your device is [paired over Bluetooth](https://sparkfun.github.io/SparkFun_RTK_Firmware/connecting_bluetooth/#windows).

![Equip Sub Menu](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Equip%20Menu.jpg)

*Equip Sub Menu*

Select the *Equip* sub menu then `GPS Rover`

![Select NMEA GPS Receiver](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20NMEA.jpg)

*Select NMEA GPS Receiver*

From the drop down, select `NMEA GPS Receiver`.

![Select Model: DGPS](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20DGPS.jpg)

*Select Model: DGPS*

Select DGPS if you'd like to connect to an NTRIP Caster. If you are using the RTK Facet L-Band, or do not need RTK fix type precision, leave the model as Generic.

![Bluetooth Settings](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20Comms.jpg)

*Bluetooth Settings Button*

From the `Comms` submenu, click the Blueooth settings button.

![SurvPC Bluetooth Devices](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20Find%20Device.jpg)

*SurvPC Bluetooth Devices*

Click `Find Device`.

![List of Paired Bluetooth Devices](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20Select%20Bluetooth%20Device.jpg)

*List of Paired Bluetooth Devices*

You will be shown a list of devices that have been paired. Select the RTK device you want to connect to.

![Connect to Device](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20Select%20Bluetooth%20Device%20With%20MAC.jpg)

*Connect to Device*

Click the `Connect Bluetooth` button, shown in red in the top right corner. The software will begin a connection to the RTK device. You'll see the MAC address on the RTK device changes to the Bluetooth icon indicating it's connected. 

If SurvPC detects NMEA, it will report a successful connection.

![Receiver Submenu](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Rover%20Receiver.jpg)

*Receiver Submenu*

You are welcome to enter the ARP (antenna reference point) and surveying stick length for your particular setup.

**NTRIP Client**

Note: If you are using a radio to connect Base to Rover, or if you are using the RTK Facet L-Band you do not need to set up NTRIP; the device will achieve RTK fixes and output extremely accurate location data by itself. But if L-Band corrections are not available, or you are not using a radio link, the NTRIP Client can provide corrections to this Rover.

![RTK Submenu](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20NTRIP%20Client.jpg)

*RTK Submenu*

If you selected 'DGPS' as the Model type, the RTK submenu will be shown. This is where you give the details about your NTRIP Caster such as your mount point, user name/pw, etc. For more information about creating your own NTRIP mount point please see [Creating a Permanent Base](https://sparkfun.github.io/SparkFun_RTK_Firmware/permanent_base/)


Enter your NTRIP Caster credentials and click connect. You will see bytes begin to transfer from your phone to the RTK Express. Within a few seconds, the RTK Express will go from ~300mm accuracy to 14mm. Pretty nifty, no?

What's an NTRIP Caster? In a nutshell, it's a server that is sending out correction data every second. There are thousands of sites around the globe that calculate the perturbations in the ionosphere and troposphere that decrease the accuracy of GNSS accuracy. Once the inaccuracies are known, correction values are encoded into data packets in the RTCM format. You, the user, don't need to know how to decode or deal with RTCM, you simply need to get RTCM from a source within 10km of your location into the RTK Express. The NTRIP client logs into the server (also known as the NTRIP caster) and grabs that data, every second, and sends it over Bluetooth to the RTK Express.

Don't have access to an NTRIP Caster? You can use a 2nd RTK product operating in Base mode to provide the correction data. Checkout [Creating a Permanent Base](https://sparkfun.github.io/SparkFun_RTK_Firmware/permanent_base/). If you're the DIY sort, you can create your own low-cost base station using an ESP32 and a ZED-F9P breakout board. Check out [How to](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station) Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station). If you'd just like a service, [Syklark](https://www.swiftnav.com/skylark) provides RTCM coverage for $49 a month (as of writing) and is extremely easy to set up and use. Remember, you can always use a 2nd RTK device in *Base* mode to provide RTCM correction data but it will be less accurate than a fixed position caster.

Once everything is connected up, click the Green check in the top right corner.

![Storing Points](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Survey.jpg)

*Storing Points*

Now that we have a connection, you can use the device, as usual, storing points and calculating distances.

![SurvPC Skyplot](img/SurvPC/SparkFun%20RTK%20Software%20-%20SurvPC%20Skyplot.jpg)

*SurvPC Skyplot*

Opening the Skyplot will allow you to see your GNSS details in real-time.

If you are a big fan of SurvPC please contact your sales rep and ask them to include SparkFun products in their Manufacturer drop-down list.

## Survey Master

[Survey Master](https://www.comnavtech.com/companyfile/4/) by ComNam / SinoGNSS is an Android-based option. The download location can vary so google 'Survey Master ComNav Download' if the link above fails. Download the zip file, send the APK file to a phone and install the program.

![Startup wizard](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2001.png)

By default, a wizard will guide you through the setup. The Project step will ask you for the name of the project, the datum, etc. 

![Connection Setup
](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2027.png)

Next select your connection. 

![Connection specifics](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2029.png)

For the Device Model select 'NMEA Device'. 

![TOP106 Antenna Parameters](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2002.png)

If you are just getting started, use one of the default antenna types. If you are attempting to get sub-centimeter accuracy, enter the parameters of your antenna and add it. Above are the NGS-certified parameters for the [TOP106 antenna](https://www.sparkfun.com/products/17751).

![List of Bluetooth devices](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2003.png)

Click the 'Target Device' option to get a list of available Bluetooth devices. Make sure your RTK product is on and you should see the device. In this example 'Express Rover-B022' was chosen.

To finish, click 'Connect'. You should see the Bluetooth MAC address on your RTK product change to the Bluetooth icon indicating a connection is established.

![Rover Work Mode Configuration](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2004.png)

Next is configuring the 'Work mode' of the device. The step is where we set up our NTRIP correction source.

![Empty mode list](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2005.png)

Click 'Add' to create a new work mode.

![NTRIP Client](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2009.png)

Shown above, we configure the NTRIP Client. Survey Master calls this the 'SinoGNSS' Protocol. Click on the three bars to the right of 'Server' to enter a new NTRIP connection.

![List of Services](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2030.png)

Here you can add different NTRIP Caster providers. If you're using RTK2Go be sure to enter your contact email into the user name.

![Server and mount point selected](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2009.png)

Return to the 'Datalink type' window and select the Server you just entered. Re-enter the server address and port for your NTRIP Caster. Once complete, click on the down-pointing arrow. This will ping the Caster and obtain the mount point table. Select your mount point.

![Rover with work list in place](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2011.png)

Select the newly created work mode and press the 'Apply' button.

![Connecting to service](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2033.png)

Survey Master will attempt to connect to your specified RTK corrections source (NTRIP Caster). Upon success, you will be located on the Project menu.

Survey Master expects many more NMEA sentences than most GIS software. We must enable some additional messages on the RTK device to correctly communicate with Survey Master. 

![Configured NMEA messages](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2026%20.jpg)

Note above: There are 9 enabled messages and GSV is set to '1'.

Connect to the RTK device either over [WiFi AP config](/configure_with_wifi/) or via [Serial](/configure_with_serial/). Above is shown the serial method.

Open a terminal at 115200bps and press a key to open the serial configuration menu. Press '2' for GNSS Messages, press '1' for NMEA messages, now be sure to enable 9 messages to a rate of 1:

* GGA
* GLL
* GRS
* GSA
* GST
* GSV
* RMC
* VTG
* ZDA

Once complete, press x until you exit the serial menus. Now we may return to Survey Master.

![Survey Master showing the location of RTK Express](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2025.png)

Click on the 'Survey' menu and then 'Topo Survey'. Above we can see a device with RTK float, and 117mm horizontal positional accuracy.

Known Issues:

* Survey Master parses the GxGSV sentence improperly and will only indicate GPS satellites even though the fix solution is using all satellites.

![NMEA Sentences](img/SurveyMaster/SparkFun%20RTK%20Survey%20Master%20-%2015.png)

To verify the NMEA sentences are being delivered correctly, Survey Master has a built-in tool. Select the Device->Rover->More->'H-Terminal'. 

## Other
Hopefully, these examples give you an idea of how to connect the RTK product line to most any GIS software. If there is other GIS software that you'd like to see configuration information about, please open an issue on the [RTK Firmware repo](https://github.com/sparkfun/SparkFun_RTK_Firmware/issues) and we'll add it.
