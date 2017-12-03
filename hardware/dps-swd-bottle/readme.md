# The OpenDPS SWD Bottle

On newer DPS:es, the SWD connector is a JST-GH (1.25mm spacing that is) which translates to "really tiny". The annular rings where you need to apply solder and heat for adding wires are even smaller. This is why the OpenDPS SWD Bottle is handy. Add three P50-E2 pogo pins, and connect to your favourite SWD debugger.

<p align="center">
  <img src="https://raw.githubusercontent.com/kanflo/opendps/master/hardware/dps-swd-bottle/pogo-pins-ftw.jpg" alt="The OpenDPS SWD Bottle"/>
</p>

## BOM

* 1x "DPS SWD BTL" PCB
* 3x P50-E2 pogo pins (about €5 for 100pcs on AliExpress)
* 1x 3 pin 0.1" male right angle header
* 3x F-F dupont cable

## Mounting

1. Add solder to each of the three exposed pads where the pogo pins will be mounted.
2. Hold one pogo pin with a tweezer or small plier an align along the pad.
3. Apply heat on the pad and when the solder reflows, gently push the pin in place and wait for the solder to cool down.
4. Repeat for the second pin and check alignment of the two pins on your DPS.
5. Repeat for the third pin and check alignment on your DPS.
6. Solder the 0.1" header *on the reverse side of the pogo pins*.

## Using

The pinout is described in the silk and you should be able to make out which is GND, SWCLK and SWDIO with a little imagination :) The DPS BTL shall be interfaced with the DPS with ground to the left, on the three rightmost rings of the SWD connector where GND is in the middle.

Soldering the right angle header on the suggested side helps you in pressing the adapter against the DPS with one hand only.

SWD *Bottle*? Well it looks like one, doesn't it?
