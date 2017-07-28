# OpenDPS

#### Give your DPS5005 the upgrade it deserves

OpenDPS is a firmware replacement for the DPS5005 (and friends) that has the same functionality, has a less cluttered user interface and is remote controllable via wifi (ESP8266) or via a serial port.

<p align="center">
<img src="https://raw.githubusercontent.com/kanflo/opendps/master/image.jpg" alt="A DPS5005 with wifi"/>
</p>

There are three accompanying blog posts you might find of interest:

* [Part one](https://johan.kanflo.com/hacking-the-dps5005/) covers the reverse engineering of the DPS5005.
* [Part two](https://johan.kanflo.com/opendps-design/) describes the design of OpenDPS.
* [Part three](https://johan.kanflo.com/upgrading-your-dps5005/) covers the process of upgrading stock DPS5005:s to OpenDPS.

### Upgrading your DPS5005

If you are eager to upgrade your DPS5005, you may skip directly to part three. Oh, and of course you can use OpenDPS for more than a programmable power supply. Why not use it as an interface for your DIY sous vide cooker :D


### Cloning & building

```
git clone --recursive https://github.com/kanflo/opendps.git
cd opendps
make -C libopencm3
make -C opendps
make -C dpsboot
```

Check [the blog](https://johan.kanflo.com/upgrading-your-dps5005/) for instructions on how to unlock and flash your DPS5005.

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

### Upgrading

As newer DPS:es have 1.5mm spaced JTAG pins and limited space for running the JTAG signals towards the back of the device, a permanent soldered JTAG is somewhat cumbersome. People not activly developing OpenDPS will not need JTAG anyway. To facilitate upgrade, OpenDPS comes with a bootloader enabling upgrade over UART:

```
% make -C opendps bin
% dpsctl.py -d /dev/ttyUSB0 -U opendps/opendps.bin
```

If you accidentally upgrade to a really b0rken version, the bootloader can be forced to enter upgrade mode if you keep the SEL button pressed while enabling power.

The display will be black during the entire upgrade operation. If it stays black, the bootloader might refuse or fail to start the OpenDPS application, or the application crashed. If you attempt the upgrade operation again, and upgrading begins, the bootloader is running but is refusing to boot your firmware. But why? Well, let's find out. If you append the ```-v``` option to ```dpsctl.py``` you will get a dump of the UART traffic.

```
Communicating with /dev/ttyUSB0
TX  9 bytes 7e 09 04 00 27 86 0c b2 7f
RX 9 bytes 7e 89 00 04 00 03 66 0f 7f
```

The fourth byte from the end in the received data (0x03 in this example) will tell us why the bootloader refused to boot the firmware. See [protocol.h](https://github.com/kanflo/opendps/blob/master/opendps/protocol.h#L72) for the different reasons.

### Source code organisaton

The project consists of four parts:

* ```opendps/``` The OpenDPS firmware.
* ```dpsboot/``` The OpenDPS bootloader.
* ```esp8266-proxy/``` The ESP8266 firmware for wifi connected OpenDPS:es.
* ```dpsctl/``` A pyton script for controlling your OpenDPS via wifi or a serial port.

### What about other DPS:es?

In theory, OpenDPS should work for all the other models in the DPSx0xx series, such as the DPS3005, DPS3012 and DPS5015. I have none to test with but would be very surprised if the hardware differed that much. Please share any results. The maxium settable output current can be defined when building opendps, see the makefile.

---
Licensed under the MIT license. Have fun!
