# OpenDPS (p1ngb4ck fork)

#### Give your DPS5005 the upgrade it deserves

> **This is a fork of [kanflo/opendps](https://github.com/kanflo/opendps)** with firmware improvements and an accompanying ESPHome component that replaces the original ESP8266 proxy with a modern ESP32-based integration.

---

## 🙏 Special thanks

A huge, heartfelt thank you to the people without whom this project would not exist in its current form:

**[Johan Kanflo (kanflo)](https://github.com/kanflo)** — for creating OpenDPS in the first place. Reverse-engineering the DPS5005, writing a clean open-source firmware replacement, and maintaining it for years is no small feat. This fork stands entirely on his shoulders.

**Mallow** (Discord) — for invaluable help with the LVGL UI configuration and for creating the [custom LVGL 9.5 component](https://github.com/youkorr/lvgl_9.5) that overrides ESPHome's bundled LVGL version, enabling LVGL 9.5 features on ESP32-P4. Without that work the display integration simply would not have been possible.

---

## What this fork is about

OpenDPS is awesome — so why not make the most of it? This fork pairs OpenDPS with a co-host ESP32 device to bring all of its functions together on a nice big display, with full network integration, datalogging, firmware management and plenty of room to extend further. Flash both devices once, and everything is there.

For the best experience, a **display-equipped ESP32-P4 tablet** (such as the Guition JC1060P470_I_W_Y) is recommended — but the ESPHome component works on any ESP32, including headless setups integrated into Home Assistant.

---

## Quick navigation

- [Getting started](#getting-started)
- [Our additions](#our-additions)
- [ToDo](#todo)
- [Screenshots & feature overview](#screenshots--feature-overview)

---

## Getting started

### Step 1 — Flash the DPS with OpenDPS firmware

Follow the original project's flashing guide ([Wiki](Wiki.md) or the [upstream blog post](https://johan.kanflo.com/upgrading-your-dps5005/)) to unlock and flash your DPS device.

**Recommended:** use the firmware from this fork — it includes baud rate switching, improved ADC accuracy and calibration fixes (see [Our additions](#our-additions) below). Build and flash with:

```bash
git clone --recursive https://github.com/p1ngb4ck/opendps.git
cd opendps
make -C libopencm3
make -C dpsboot flash   # only needed for a fresh device or to update the bootloader
make -C opendps flash
```

**Already running upstream OpenDPS?** That's fine — the ESPHome component is backwards compatible with any OpenDPS version that has a working bootloader. You can always upgrade the main firmware later directly from the ESPHome device UI (no PC, no JTAG required), as long as the original `dpsboot` bootloader is intact.

> **Note:** Reflashing the bootloader (`dpsboot`) requires JTAG/SWD access. The main application firmware can be upgraded over UART by the ESPHome device at any time.

---

### Step 2 — Flash the ESPHome device

Install [ESPHome](https://esphome.io) if you haven't already, then compile and flash using our config:

```bash
# Install ESPHome (if needed)
pip install esphome

# Compile and flash (USB connected)
esphome run esphome-config/esphome_config_opendps_JC1060P470_I_W_Y.yaml
```

Or use the [ESPHome web installer](https://web.esphome.io) / [ESPHome dashboard](https://esphome.io/guides/getting_started_hassio.html) if you prefer a GUI.

The config uses `external_components` to pull the `opendps` component from this project's companion ESPHome repository — no manual component installation needed.

---

### Step 3 — Connect DPS to ESP32

Wire the DPS serial interface to your ESP32:

| DPS pin | ESP32 pin |
|---------|-----------|
| TX      | RX (see config) |
| RX      | TX (see config) |
| GND     | GND |

Exact GPIO assignments are defined in the ESPHome config file. Logic level is 3.3V on the ESP32 side — check your DPS model's serial voltage before connecting.

---

### Step 4 (optional) — Adapt the config for your hardware

The provided config targets the **Guition JC1060P470_I_W_Y** (ESP32-P4, 10.6" MIPI DSI display, IP101 Ethernet). If you use a different device:

- **Another ESP32-P4 tablet with display:** adjust display, touch and pin assignments in the YAML. Be aware that the UI currently has hardcoded sizes and positions throughout the LVGL config, so it will require manual layout adjustments to fit a different display resolution. Displays of 1024×600 or larger (e.g. 10.1") are the intended target — see the ToDo section for planned auto-sizing support.
- **Any other ESP32 (no display):** strip out the `display`, `lvgl` and `touchscreen` sections and keep only the `opendps` component with your sensors and automations. All control and monitoring works headlessly via Home Assistant or MQTT.

The `opendps` ESPHome component itself has no display dependency — the UI is purely additive.

---

### ToDo

Features planned for future implementation (not yet available):

- **UI auto-sizing** — refactor the LVGL config to derive all sizes and positions from the configured display resolution, so any display of 1024×600 or larger works without manual layout adjustments
- **BLE remote / keyboard support** — control the DPS via Bluetooth keyboard or remote
- **USB-HID support** — use a USB keyboard or gamepad connected to the ESP32 for local control
- **Minor UI fixes** — ongoing polish of the LVGL interface

---

## Our additions

### Firmware changes

- **Configurable UART baud rate** — runtime baud rate switching; default changed to 115200 for faster host communication (`b92f6de`, `31a1114`)
- **ADC oversampling** — averages 420 ADC samples per reading (~50 Hz update rate, ~21 kHz ADC rate) for significantly smoother voltage/current readings (`cdf97dd`)
- **Current noise floor clamp** — output current is clamped to zero below the 10 mA noise floor, eliminating false micro-amp readings at idle (`dc26352`)
- **ADC calibration fixes** — corrected `ADC_CHA_IOUT_GOLDEN_VALUE` for the DPS5005 and fixed double-offset bug in current averaging (`963fe78`, `bca649f`, `1f3b9ab`)
- **Vin display** — input voltage now shown with 2 decimal places (`1f3b9ab`)
- **Splash screen disabled** — faster boot (`b92f6de`)
- **dpsctl `--set-baud` / `--upgrade-baud`** — CLI support for baud rate management and baud-rate-aware firmware upgrades (`b76bc6f`)
- **dpsboot LTO + size trim** — link-time optimisation enabled in the bootloader; `hw_set_baudrate_boot` trimmed to fit in the 5 KB flash budget (`4a9a172`)
- **Status response fix** — corrected status response code handling (`e53fd42`)

### ESPHome component

The original project used a separate ESP8266 as a Wi-Fi proxy. This fork adds a **native ESPHome component** (`opendps`) that talks directly to the DPS over UART from any ESP32, replacing the proxy entirely.

**Features:**
- Real-time V_in, V_out, I_out, power and temperature sensors
- Output enable/disable, voltage/current set, function selection, lock, brightness
- OTA firmware upgrade of the DPS from USB, SD, LittleFS or network storage (NFS/SMB/FTP) — directly from the device, no PC needed
- High-speed datalogger with PSRAM-backed triple-buffering (CSV or binary output)
- Full Home Assistant integration via native API or MQTT
- Ethernet support (e.g. IP101) for wired connectivity

The component lives in the [p1ngb4ck/esphome](https://github.com/p1ngb4ck/esphome) repository under `esphome/components/opendps/`. See its [README](https://github.com/p1ngb4ck/esphome/blob/dev/esphome/components/opendps/README.md) for full documentation, configuration schema and examples.

### ESPHome configuration — Guition JC1060P470_I_W_Y (ESP32-P4 10.6" tablet)

A complete, ready-to-use ESPHome configuration for the **Guition JC1060P470_I_W_Y** is provided in [`esphome-config/esphome_config_opendps_JC1060P470_I_W_Y.yaml`](esphome-config/esphome_config_opendps_JC1060P470_I_W_Y.yaml).

**Target hardware:**
- SoC: ESP32-P4 @ 400 MHz
- Display: 10.6" MIPI DSI, 1024×600, model JC1060P470
- Touch: GT911 (I2C)
- Flash: 16 MB
- PSRAM: Octal, 200 MHz
- Ethernet: IP101

**What the config provides:**
- Full LVGL UI with main control screen (voltage, current, power, Vin, temperatures, output toggle)
- Settings page with network config, calibration editor, OTA firmware upgrade browser (USB/SD), and system info
- Datalogger integration with start/stop from the UI
- Calibration read/write with per-field numpad editing
- Ethernet connectivity (no Wi-Fi dependency)

This config is the reference implementation and is actively running on real hardware.

---

## Screenshots & feature overview

> _Screenshots and photos coming soon._

### OpenDPS supported functions

The OpenDPS firmware (and by extension this ESPHome integration) supports the following operating modes on compatible DPS devices:

| Function | Description |
|----------|-------------|
| `cv` | Constant Voltage — hold output voltage at setpoint, current limited |
| `cc` | Constant Current — hold output current at setpoint, voltage limited |
| `cp` | Constant Power — hold output power at setpoint (model dependent) |

**Supported devices:** DPS3003, DPS3005, DPS5005, DPS5015, DPS5020, DP50V5A and variants. Hardware revisions may vary — see the [upstream project](https://github.com/kanflo/opendps) for compatibility notes.

**Monitored values (all modes):**
- V_in — input voltage
- V_out — output voltage
- I_out — output current
- Power — calculated output power (V_out × I_out)
- Temperature 1 & 2 — internal sensors

**Control:**
- Enable / disable output
- Set voltage setpoint
- Set current limit
- Switch operating function
- Lock / unlock front panel
- Adjust display brightness
- Upgrade DPS firmware over UART (from ESP32, no PC required)
- Calibrate ADC/DAC coefficients

---

## Original project

OpenDPS is a FOSS firmware replacement for the DPS5005 (and DPS3003, DPS3005, DPS5015, DP50V5A, DPS5020 and possibly others) that has the same functionality, has a less cluttered user interface and is remote controllable via WiFi (ESP8266) or via a serial port.

<p align="center">
<img src="https://raw.githubusercontent.com/kanflo/opendps/master/image.jpg" alt="A DPS5005 with OpenDPS"/>
</p>

There are three accompanying blog posts you might find of interest:

* [Part one](https://johan.kanflo.com/hacking-the-dps5005/) covers the reverse engineering of the DPS5005.
* [Part two](https://johan.kanflo.com/opendps-design/) describes the design of OpenDPS.
* [Part three](https://johan.kanflo.com/upgrading-your-dps5005/) covers the process of upgrading stock DPS5005:s to OpenDPS.

### Cloning & building (upstream)

```bash
git clone --recursive https://github.com/kanflo/opendps.git
cd opendps
make -C libopencm3
make -C opendps flash
make -C dpsboot flash
```

Check [the blog](https://johan.kanflo.com/upgrading-your-dps5005/) for instructions on how to unlock and flash your DPS5005.

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

### Upgrading DPS firmware over UART

OpenDPS comes with a bootloader enabling firmware upgrade over UART — no JTAG needed for the application firmware:

```
% make -C opendps bin
% dpsctl.py -d /dev/ttyUSB0 -U opendps/opendps.bin
```

With this fork's ESPHome integration, firmware upgrades can be triggered directly from the ESP32 device UI or via Home Assistant — no PC or `dpsctl.py` required.

If you accidentally upgrade to a broken version, the bootloader can be forced into upgrade mode by holding the SEL button while powering on.

### Source code organisation

* `dpsboot/` — The OpenDPS bootloader
* `opendps/` — The OpenDPS firmware
* `esp8266-proxy/` — The original ESP8266 Wi-Fi proxy firmware (superseded by the ESPHome component in this fork)
* `dpsctl/` — Python script for controlling OpenDPS via Wi-Fi or serial
* `emu/` — Xcode project and GNU makefile for running an emulated OpenDPS
* `esphome-config/` — Ready-to-use ESPHome configurations for supported devices

### What about other DPS:es?

OpenDPS has been verified to work with other models in the DPSx0xx series, such as the DPS3003, DPS3005, DPS5015 and DPS5020. The maximum settable output current can be defined when building opendps, see the makefile. Please note that the hardware design might change at any time without any notice (I am not affiliated with its designer). This will render OpenDPS unusable until fixed.

---

Licensed under the MIT license. Have fun!
