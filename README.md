# OpenDPS

#### Give your DPS5005 the upgrade it deserves

OpenDPS is a FOSS firmware replacement for the DPS5005 (and DPS3003, DPS3005, DPS5015, DP50V5A and possibly others) that has the same functionality, has a less cluttered user interface and is remote controllable via wifi (ESP8266) or via a serial port.

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

First build the OpenDPS firmware:

```
git clone --recursive https://github.com/kanflo/opendps.git
cd opendps
make -C libopencm3
make -C opendps flash
make -C dpsboot flash
```

*Please note that you currently MUST flash the bootloader last as OpenOCD overwrites the bootloader when flashing the firmware. Currently no idea why :-/ *

Check [the blog](https://johan.kanflo.com/upgrading-your-dps5005/) for instructions on how to unlock and flash your DPS5005.

Second, build and flash the ESP8266 firmware. First you need to create the file `esp8266-proxy/esp-open-rtos/include/private_ssid_config.h` with the following content:

```
#define WIFI_SSID "My SSID"
#define WIFI_PASS "Secret password"
```

Next:

```
make -C esp8266-proxy flash
```

### Setup dpsctl.py

The script runs with python2 and python3. The libraries pycrc and pyserial are required:

```
pip install -r requirements.txt
```

### Usage

A vanilla OpenDPS device will support two functions, constant voltage (cv) and constant current (cc). A tool called `dpsctl.py` can be used to talk to an OpenDPS device to query functionality and supported parameters for each function.

Once upgraded and connected to an ESP8266, type the following at the terminal to find its IP address:

```
% dpsctl.py --scan
172.16.3.203
1 OpenDPS device found
```

Enable constant voltage (cv) at 3.3V limited to 500mA:

```
% dpsctl.py -d 172.16.3.203 -f cv
Changed function.

% dpsctl.py -d 172.16.3.203 -p voltage=3300 current=500
voltage: ok
current: ok

% dpsctl.py -d 172.16.3.203 -o on
```

Query the status of the device:

```
% dpsctl.py -d 172.16.3.203 -q
Func       : cv (on)
  current  : 500
  voltage  : 3300
V_in       : 12.35 V
V_out      : 3.33 V
I_out      : 0.152 A
```

List supported functions of the device:

```
% dpsctl.py -d 172.16.3.203 -F
Selected OpenDPS supports the cv and cc functions.
```

List supported functions parameters of current function:

```
% dpsctl.py -d 172.16.3.203 -P
Selected OpenDPS supports the voltage (mV) current (mA) parameters for the cv function.
```

Additionally, `dpsctl.py` can return JSON, eg.:

```
% dpsctl.py -d 172.16.3.203 -q -j
{
    "command": 132,
    "cur_func": "cv",
    "i_out": 151,
    "output_enabled": 1,
    "params": {
        "current": "500",
        "voltage": "3300"
    },
    "status": 1,
    "v_in": 12355,
    "v_out": 3321
}
```

### Upgrading

As newer DPS:es have 1.25mm spaced JTAG pins (JST-GH) and limited space for running the JTAG signals towards the back of the device, a permanent soldered JTAG is somewhat cumbersome. People not activly developing OpenDPS will not need JTAG anyway. To facilitate upgrade, OpenDPS comes with a bootloader enabling upgrade over UART:

```
% make -C opendps bin
% dpsctl.py -d /dev/ttyUSB0 -U opendps/opendps.bin
```

If you accidentally upgrade to a really b0rken version, the bootloader can be forced to enter upgrade mode if you keep the SEL button pressed while enabling power.

The display will be black during the entire upgrade operation. If it stays black, the bootloader might refuse or fail to start the OpenDPS application, or the application crashed. If you attempt the upgrade operation again, and upgrading begins, the bootloader is running but is refusing to boot your firmware. But why? Well, let's find out. If you append the `-v` option to `dpsctl.py` you will get a dump of the UART traffic.

```
Communicating with /dev/ttyUSB0
TX  9 bytes 7e 09 04 00 27 86 0c b2 7f
RX 9 bytes 7e 89 00 04 00 03 66 0f 7f
```

The fourth byte from the end in the received data (0x03 in this example) will tell us why the bootloader refused to boot the firmware. See [protocol.h](https://github.com/kanflo/opendps/blob/master/opendps/protocol.h#L72) for the different reasons.

### Custom Fonts

If you would like to use a your own font for OpenDPS you may do so by doing the following:

```
% make -C opendps fonts \
    METER_FONT_FILE=<path_to_font> \
    METER_FONT_SMALL_SIZE=18 \
    METER_FONT_MEDIUM_SIZE=24 \
    METER_FONT_LARGE_SIZE=48 \
    FULL_FONT_FILE=<path_to_font> \
    FULL_FONT_SMALL_SIZE=15
```

Supported fonts are .ttf or .otf

### Source code organisation

The project consists of four parts:

* `dpsboot/` The OpenDPS bootloader.
* `opendps/` The OpenDPS firmware.
* `esp8266-proxy/` The ESP8266 firmware for wifi connected OpenDPS:es.
* `dpsctl/` A Python script for controlling your OpenDPS via wifi or a serial port.
* `emu/` Xcode project and GNU makefile for running an emulated OpenDPS.


### What about other DPS:es?

OpenDPS has been verified to work with other models in the DPSx0xx series, such as the DPS3003, DPS3005 and DPS5015. The maxium settable output current can be defined when building opendps, see the makefile. Plese note that the hardware design might change at any time without any notice (I am not affiliated with its designer). This will render OpenDPS unusable until fixed.

---
Licensed under the MIT license. Have fun!
