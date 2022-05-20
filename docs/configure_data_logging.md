# Configure Data Logging Menu

-> [![RTK Facet Data Logging Configuration Menu](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_ExpressPlus_Logging_Cyclic.jpg)](https://cdn.sparkfun.com/assets/learn_tutorials/2/1/8/8/SparkFun_RTK_ExpressPlus_Logging_Cyclic.jpg) <-

-> *RTK Facet Data Logging Configuration Menu* <-

Pressing 5 will enter the Logging Menu. This menu will report the status of the microSD card. While you can enable logging, you cannot begin logging until a microSD card is inserted. Any FAT16 or FAT32 formatted microSD card up to 32GB will work. We regularly use the [SparkX brand 1GB cards](https://www.sparkfun.com/products/15107) but note that these log files can get very large (>500MB) so plan accordingly.

* Option 1 will enable/disable logging. If logging is enabled, all messages from the ZED-F9P will be recorded to microSD. A log file is created at power on with the format *SFE_Facet_YYMMDD_HHMMSS.txt* based on current GPS data/time.
* Option 2 allows a user to set the max logging time. This is convenient to determine the location of a fixed antenna or a receiver on a repeatable landmark. Set the RTK Facet to log RAWX data for 10 hours, convert to RINEX, run through an observation processing station and you’ll get the corrected position with <10mm accuracy. Please see the [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station) tutorial for more information.
* Option 3 allows a user to set the max logging length in minutes. Every 'max long length' amount of time the current log will be closed and a new log will be started. This is known as cyclic logging and is convenient on *very* long surveys (ie, months or years) to prevent logs from getting too unwieldy and helps limit the risk of log corruption. This will continue until the unit is powered down or the *max logging time* is reached.

Note: If you are wanting to log RAWX sentences to create RINEX files useful for post processing the position of the receiver please see the GNSS Configuration Menu. For more information on how to use a RAWX GNSS log to get higher accuracy base location please see the [How to Build a DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data) tutorial.