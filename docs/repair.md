# Disassembly / Repair

Surveyor: ![Feature Supported](img/Icons/GreenDot.png) / Express: ![Feature Supported](img/Icons/GreenDot.png) / Express Plus: ![Feature Supported](img/Icons/GreenDot.png) / Facet: ![Feature Supported](img/Icons/GreenDot.png) / Facet L-Band: ![Feature Supported](img/Icons/GreenDot.png) / Reference Station: ![Feature Supported](img/Icons/GreenDot.png)

The RTK product line is fully open-source hardware. This allows users to view schematics, code, and repair manuals. This section documents how to safely disassemble the RTK Facet and [Reference Station](#reference-station).

Repair Parts:

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/24675"><img src="https://cdn.sparkfun.com//assets/parts/2/5/0/0/0/SPX-24675_RTK_Facet_L-Band_Main_v14_-_1.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/24064"><img src="https://cdn.sparkfun.com//assets/parts/2/4/3/8/7/SPX-24064_RTK_Facet_Main_v13_-_1.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/24673"><img src="https://cdn.sparkfun.com//assets/parts/2/4/9/9/8/SPX-24673_RTK_Facet_Housing.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/24675">SparkFun RTK Replacement Parts - Facet L-Band Main Board v14 (SPX-24675)</a></td>
    <td><a href="https://www.sparkfun.com/products/24064">SparkFun RTK Replacement Parts - Facet Main Board v13 (SPX-24064)</a></td>
    <td><a href="https://www.sparkfun.com/products/24673">SparkFun RTK Replacement Parts - Facet Housing (SPX-24673)</a></td>
  </tr>
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/24674"><img src="https://cdn.sparkfun.com//assets/parts/2/4/9/9/9/SPX-24674_RTK_Facet_L-Band_Housing.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/24707"><img src="https://cdn.sparkfun.com//assets/parts/1/7/6/9/8/SPX-24707_RTK_Facet_Rubber_Sock.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/products/24706"><img src="https://cdn.sparkfun.com//assets/parts/2/5/0/3/0/SPX-24706_RTK_Facet_Connector_Board_-_1.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/24674">SparkFun RTK Replacement Parts - Facet L-Band Housing (SPX-24674)</a></td>
    <td><a href="https://www.sparkfun.com/products/24707">SparkFun RTK Replacement Parts - Facet Rubber Sock (SPX-24707)</a></td>
    <td><a href="https://www.sparkfun.com/products/24706">SparkFun RTK Replacement Parts - Facet Connector Assembly v12 (SPX-24706)</a></td>
  </tr>
  <tr align="center">
   <td><a href="https://www.sparkfun.com/products/24705"><img src="https://cdn.sparkfun.com//assets/parts/2/5/0/2/9/SPX-24705_RTK_Facet_Display_Assembly_v12_-_1.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/products/24705">SparkFun RTK Replacement Parts - Facet Display/Button (SPX-24705)</a></td>
  </tr>
</table>

Tools Needed:

* [Small Philips Head Screwdriver](https://www.sparkfun.com/products/9146)
* [Curved Tweezers](https://www.sparkfun.com/products/10602)
* [U.FL Puller](https://www.sparkfun.com/products/20687) - *Recommended*
* [Wire Cutters](https://www.sparkfun.com/products/10447) - *Recommended*

## Opening Facet

![Remove the protective silicone boot](img/Repair/SparkFun-RTK-Repair-1.jpg)

Starting from the back of the unit, remove the protective silicone boot. If your boot has gotten particularly dirty from field use, rinse it with warm water and soap to clean it up.

![Remove four Philips head screws](img/Repair/SparkFun-RTK-Repair-2.jpg)

Remove the four Philips head screws. They may not come all the way out of the lower enclosure.

![Lid removed](img/Repair/SparkFun-RTK-Repair-3.jpg)

The top lid should then come off. The front overlay is adhesive and may adhere slightly to the 'tooth' on the lid. You will not damage anything by gently prying it loose from the lid as you lift the lid.

**Tip:** The lid makes a great screw bin.

![Remove the antenna](img/Repair/SparkFun-RTK-Repair-4.jpg)

Note the antenna orientation so that it can be re-mounted in the same way. A sharpie dot towards the display is a handy method. 

Remove the four screws holding the antenna in place. 

Be aware that the antenna material is susceptible to fingerprints. You won't likely damage the reception but it's best to just avoid touching the elements.

![Antenna to the side](img/Repair/SparkFun-RTK-Repair-5.jpg)

The antenna will be attached to the main board and must stay that way for the next few steps. Without pulling on the thin RF cable, gently set the antenna to the side.

![Remove battery boat](img/Repair/SparkFun-RTK-Repair-6.jpg)

The battery and vertical PCBs are held in place using a retention PCB. Remove the four screws holding the PCB in place and lift off the foam top of the battery holder. 

**Note:** v1.0 of the retention plate is not symmetrical. Meaning, if the plate is installed in reverse, the retention PCB will be just short of the connector board and will not properly hold it in place. Reinstall the retention plate as shown in the picture.

**Note:** The foam is held to the PCB using an adhesive. Some of that adhesive is exposed to catch material that may enter into the enclosure. Try to avoid getting stuck.

![Holder PCB to the side](img/Repair/SparkFun-RTK-Repair-7.jpg)

Set the retention PCB to the side.

## Removing Antenna Connection

![Removing the antenna connection](img/Repair/SparkFun-RTK-Repair-8.jpg)

This is the most dangerous step. The cable connecting the antenna to the main board uses something called a U.FL or IPEX connector. These tend to be fragile. You can damage the connector rendering the unit inoperable. Just be sure to take your time.

Using the U.FL removal tool, slide the tool onto the U.FL connector and gently pull away from the main board. If it won't give, you may need to angle the tool slightly while pulling.

**Note:** If you do not have a U.FL tool this [tutorial on U.FL connectors](https://learn.sparkfun.com/tutorials/three-quick-tips-about-using-ufl/all#disconnect) has three alternative methods using tweezers, wire cutters, and a skinny PCB that may also work. 

![U.FL connector removed](img/Repair/SparkFun-RTK-Repair-9.jpg)

The U.FL connector will disconnect. The antenna can now be set to the side.

## Opening Backflip Connectors

![Flipping connector](img/Repair/SparkFun-RTK-Repair-10.jpg)

Many of the connections made within the RTK product line use this 'back flip' style of FPC connector. To open the connector and release the flex printed circuit (FPC) cable, use a curved pair of tweezers to gently flip up the arm. The arm in the connector above has been flipped, the FPC can now be removed.

As shown above, remove the FPC connecting to the 4th connector on the main board. The connector is labeled 'SD Display'. Leave all other FPCs in place.

![Removing main board](img/Repair/SparkFun-RTK-Repair-13.jpg)

The main board is attached to the battery and the connector board. Lift the mainboard and connector board together, bringing the battery with the assembly.

## Removing the Battery

![Removing the JST connector](img/Repair/SparkFun-RTK-Repair-14.jpg)

**Note:** This step is not needed for general repair. Only disconnect the battery if you are replacing the battery.

The battery is plugged into the mainboard using a JST connector. These are very strong connectors. *Do not* pull on the wires. We recommend using the mouth of wire cutters (also known as diagonal cutters) to pry the connector sideways. 

Once removed, the battery can be set aside.

## Removing the Front Overlay

![Disconnecting overlay](img/Repair/SparkFun-RTK-Repair-15.jpg)

The front overlay (the sticker with the Power button) is connected to the display board using the same style 'back flip' FPC connector. Flip up the arm and disconnect the overlay.

![Peeling off the overlay](img/Repair/SparkFun-RTK-Repair-16.jpg)

Gently peel off the adhesive overlay from the front face. This cannot be saved.

## Inserting New Display Board

![New display board in place](img/Repair/SparkFun-RTK-Repair-17.jpg)

Slide the old display board out. Remove the brown FPC from the old display board and move over to the new display board. Insert the new display board into the slot.

![Remove film](img/Repair/SparkFun-RTK-Repair-18.jpg)

With the new display board in place, remove the protective film from the display.

![Apply new overlay](img/Repair/SparkFun-RTK-Repair-19.jpg)

Remove the backing from the new overlay. Stick the overlay into the center of the front face area.

![Insert tab into connector](img/Repair/SparkFun-RTK-Repair-20.jpg)

Be sure to flip up the arm on the overlay connector before trying to insert the new overlay FPC.

Using tweezers, and holding the FPC by the cable stiffener, insert the overlay FPC into the display board. 

![Modern display board with 6-pin friction connector](img/Repair/RTK-Facet-Repair-3.jpg)

Shown above, the modern display boards use a 6-pin friction fit connector. There is no backflip arm that needs to be raised. Hold the FPC by the cable stiffener and push it down into the black connector on the display board.

## Closing The Backflip Connector

![Closing the arm on an FPC](img/Repair/SparkFun-RTK-Repair-21.jpg)

Use the nose of the tweezers to press the arm down, securing the FPC in place.

![FPC in the display board connector](img/Repair/SparkFun-RTK-Repair-22.jpg)

If you haven't already done so, move the brown FPC from the original display board over to the new display board. Be sure to open the connector before inserting the FPC, and then press down on the arm to secure it in place.

## Reinstalling Main Board

![Reinstalling Main board](img/Repair/SparkFun-RTK-Repair-23.jpg)

Slide the main board and connector boards back into place along with the battery. We find it easier to partially insert the connector board, then the main board, and then adjust them down together.

![Handling FPCs](img/Repair/SparkFun-RTK-Repair-11.jpg)

Reconnect the display board to the main board. Be sure to close the arm on the main board to secure the FPC in place. 

## Testing the Overlay

![Internal Power Button](img/Repair/SparkFun-RTK-Repair-24.jpg)

The RTK Facet has two power buttons: the external button on the overlay and an internal button on the back of the display board (shown above). Pressing and holding the internal button will verify the connection between the display board and the main board.

If the internal button is not working, remove and reinsert the FPC connecting the display board to the main board.

Press and hold the internal power button to power down the unit.

![Using overlay power button](img/Repair/SparkFun-RTK-Repair-26.jpg)

Repeat the process using the overlay button to verify the external power button is working. 

If the external overlay button is not working, but the internal button is, remove and reinsert the FPC connecting the overlay to the display board.

If the external button is working, proceed with re-assembling the unit.

## Reassembly

Confirm that all FPC armatures are in the down and locked position.

![Attach U.FL connector](img/Repair/SparkFun-RTK-Repair-9.jpg)

Carefully line the U.FL connector up with the main board and gently press the connector in place. A tool is useful in this step but an index finger works just as reliably.

![Protect the battery](img/Repair/SparkFun-RTK-Repair-5.jpg)

Place the retention plate and foam over the battery. The battery may need to be nudged slightly to align with the upper cavity.

**Note:** v1.0 of the retention plate is not symmetrical. Meaning, if the plate is installed in reverse, the retention PCB will be just short of the connector board and will not properly hold it in place. Reinstall the retention plate as shown in the picture above.

Secure the retention plate with the four *small* screws.

![Reattach antenna](img/Repair/SparkFun-RTK-Repair-4.jpg)

Place the antenna over top of the retention plate in the same orientation as it was removed. Secure in place with the four *large* screws.

![Dome showing the front tooth](img/Repair/SparkFun-RTK-Repair-3.jpg)

Plate the dome over the antenna with the front 'tooth' aligning over the display.

![Insert four screws into dome](img/Repair/SparkFun-RTK-Repair-2.jpg)

Secure the dome in place using four *small* screws.

Replace the silicone boot around the device.

Power on the RTK Facet and take outside to confirm SIV reaches above ~20 satellites and HPA is below ~1.0m.

## Reference Station

Taking the Reference Station apart is really easy:

* Disconnect all cables

* Unplug the green 10-way 3.5mm I/O connector
    * This makes it easy to remove the main PCB from the enclosure
    * The connector is a firm fit. You may need to rock it from side to side as you unplug it
  
* Unscrew the four screws holding the front panel in place
    * We recommend removing the front panel first, so you can unplug the OLED display
  
* Remove the front panel

![Image of the Reference Station with the front panel removed](img/Repair/Ref_Station_Disassembly.png)

* Unplug the OLED Qwiic cable

* Slide out the main PCB

## Surveyor

Disassembly of the RTK Surveyor is achieved by removing two Philips head screws and gently lifting the cover.

![Internal Surveyor Switches](img/Repair/RTK_Surveyor_Internal_-_NMEA_Switches.jpg)

Within the RTK Surveyor, two internal slide switches control the flow of NMEA data over Bluetooth and should be in the position shown above. If these switches get moved, the device will fail to correctly push NMEA data over Bluetooth. Placing a bit of tape over the top of the switches can help keep them in place.




