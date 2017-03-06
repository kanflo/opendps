# OpenDPS

#### Give your DPS5005 the upgrade it deserves

OpenDPS is a firmware replacement for the DPS5005 (and friends) that has the same functionality, has a less cluttered user interface and is remote controllable via wifi (ESP8266) or via a serial port.

<p align="center">
<img src="https://raw.githubusercontent.com/kanflo/opendps/master/opendps.jpg" alt="A DPS5005 with wifi"/>
</p>

There are three accompanying blog posts you might find of interest:

* [Part one](https://johan.kanflo.com/hacking-the-dps5005/) covers the reverse engineering of the DPS5005.
* [Part two](https://johan.kanflo.com/opendps-design/) describes the design of OpenDPS.
* [Part three](https://johan.kanflo.com/upgrading-your-dps5005/) covers the process of upgrading stock DPS5005:s to OpenDPS.

### Upgrading your DPS5005

If you are eager to upgrade your DPS5005, you may skip directly to part three. Oh, and of course you can use OpenDPS for more than a programmable power supply. Why not use it as an interface for your DIY sous vide cooker :D


### Cloning & building

Assuming you have your ARM toolchain and OpenOCD installed, clone the opendps repository and build libopencm3:

```
git clone --recursive git@github.com:kanflo/opendps.git
cd opendps
make -C libopencm3
```

Fire up OpenOCD:

```
cd openocd/scripts
openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg
```

And in another terminal:

```
cd opendps/opendps
make -C opendps flash
```

Check [the blog](https://johan.kanflo.com/upgrading-your-dps5005/) for hints about the ARM toolchain and OpenOCD if those are new to you.

### Usage

Once upgraded and connected to an ESP8266, type the following at the terminal to find its IP address:

```
% dpsctl.py --scan
172.16.3.203
1 OpenDPS device found
```

Enable 3.3V limited to 500mA:

```
% dpsctl -d 172.16.3.203 --voltage 3300
% dpsctl -d 172.16.3.203 --current 500
% dpsctl -d 172.16.3.203 --power on
```
Query the status of the device:

```
% dpsctl.py -d 172.16.3.203 --status
V_in : 7.71 V
V_set : 3.30 V
V_out : 3.32 V (on)
I_lim : 0.500 A
I_out : 0.040 A
```

### Source code organisaton

The project consists of three parts:

* ```opendps/``` The DPS5005 firmware.
* ```esp8266-proxy/``` The ESP8266 firmware for wifi connected OpenDPS:es.
* ```dpsctl/``` A pyton script for controlling your OpenDPS via wifi or a serial port.

### What about other DPS:es?

In theory, OpenDPS should work for all the other models in the DPSx0xx series, such as the DPS3005, DPS3012 and DPS5015. I have none to test with but would be very surprised if the hardware differed that much. Please share any results. The maxium settable output current can be defined when building opendps, see the makefile. 

-
Licensed under the MIT license. Have fun!
