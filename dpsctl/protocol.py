"""
The MIT License (MIT)

Copyright (c) 2017 Johan Kanflo (github.com/kanflo)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
"""

import struct

from uframe import uFrame

# command_t
CMD_PING = 1
# CMD_SET_VOUT = 2
# CMD_SET_ILIMIT = 3
CMD_QUERY = 4
# CMD_POWER_ENABLE = 5
CMD_WIFI_STATUS = 6
CMD_LOCK = 7
CMD_OCP_EVENT = 8
CMD_UPGRADE_START = 9
CMD_UPGRADE_DATA = 10
CMD_SET_FUNCTION = 11
CMD_ENABLE_OUTPUT = 12
CMD_LIST_FUNCTIONS = 13
CMD_SET_PARAMETERS = 14
CMD_LIST_PARAMETERS = 15
CMD_TEMPERATURE_REPORT = 16
CMD_VERSION = 17
CMD_CAL_REPORT = 18
CMD_SET_CALIBRATION = 19
CMD_CLEAR_CALIBRATION = 20
CMD_CHANGE_SCREEN = 21
CMD_SET_BRIGHTNESS = 22
CMD_RESPONSE = 0x80

# wifi_status_t
WIFI_OFF = 0
WIFI_CONNECTING = 1
WIFI_CONNECTED = 2
WIFI_ERROR = 3
WIFI_UPGRADING = 4

# upgrade_status_t
UPGRADE_CONTINUE = 0
UPGRADE_BOOTCOM_ERROR = 1
UPGRADE_CRC_ERROR = 2
UPGRADE_ERASE_ERROR = 3
UPGRADE_FLASH_ERROR = 4
UPGRADE_OVERFLOW_ERROR = 5
UPGRADE_SUCCESS = 16

# options for cmd_change_screen
CHANGE_SCREEN_MAIN = 0
CHANGE_SCREEN_SETTINGS = 1

# ########################################################################## #
# Helpers for creating frames.
# Each function returns a complete frame ready for transmission.
# ########################################################################## #

def create_response(command, success):
    f = uFrame()
    f.pack8(CMD_RESPONSE | command)
    f.pack8(success)
    f.end()
    return f


def create_cmd(cmd):
    f = uFrame()
    f.pack8(cmd)
    f.end()
    return f


def create_set_function(name):
    f = uFrame()
    f.pack8(CMD_SET_FUNCTION)
    f.pack_cstr(name)
    f.end()
    return f


def create_enable_output(activate):
    f = uFrame()
    f.pack8(CMD_ENABLE_OUTPUT)
    f.pack8(1 if activate == "on" else 0)
    f.end()
    return f


def create_set_parameter(parameter_list):
    f = uFrame()
    f.pack8(CMD_SET_PARAMETERS)
    for p in parameter_list:
        parts = p.split("=")
        if len(parts) != 2:
            return None
        else:
            f.pack_cstr(parts[0].lstrip().rstrip())
            f.pack_cstr(parts[1].lstrip().rstrip())
    f.end()
    return f


def create_set_calibration(parameter_list):
    f = uFrame()
    f.pack8(CMD_SET_CALIBRATION)
    for p in parameter_list:
        parts = p.split("=")
        if len(parts) != 2:
            return None
        else:
            f.pack_cstr(parts[0].lstrip().rstrip())
            for t in bytearray(struct.pack("f", float(parts[1].lstrip().rstrip()))):
                f.pack8(t)
    f.pack8(0)
    f.end()
    return f


def create_query_response(v_in, v_out_setting, v_out, i_out, i_limit, power_enabled):
    f = uFrame()
    f.pack8(CMD_RESPONSE | CMD_QUERY)
    f.pack16(v_in)
    f.pack16(v_out_setting)
    f.pack16(v_out)
    f.pack16(i_out)
    f.pack16(i_limit)
    f.pack8(power_enabled)
    f.end()
    return f


def create_wifi_status(wifi_status):
    f = uFrame()
    f.pack8(CMD_WIFI_STATUS)
    f.pack8(wifi_status)
    f.end()
    return f


def create_lock(locked):
    f = uFrame()
    f.pack8(CMD_LOCK)
    f.pack8(locked)
    f.end()
    return f


def create_ocp(i_cut):
    f = uFrame()
    f.pack8(CMD_OCP_EVENT)
    f.pack16(i_cut)
    f.end()
    return f


def create_upgrade_start(window_size, crc):
    f = uFrame()
    f.pack8(CMD_UPGRADE_START)
    f.pack16(window_size)
    f.pack16(crc)
    f.end()
    return f


def create_upgrade_data(data):
    f = uFrame()
    f.pack8(CMD_UPGRADE_DATA)
    for d in data:
        f.pack8(d)
    f.end()
    return f


def create_temperature(temperature):
    print("Sending temperature {:.1f} and {:.1f}".format(temperature, -temperature))
    temperature = int(10 * temperature)
    f = uFrame()
    f.pack8(CMD_TEMPERATURE_REPORT)
    f.pack16(temperature)
    f.pack16(-temperature)
    f.end()
    return f


def create_change_screen(screen):
    f = uFrame()
    f.pack8(CMD_CHANGE_SCREEN)
    f.pack8(screen)
    f.end()
    return f


def create_set_brightness(brightness):
    f = uFrame()
    f.pack8(CMD_SET_BRIGHTNESS)
    f.pack8(brightness)
    f.end()
    return f


# ########################################################################## #
# Helpers for unpacking frames.
#
# These functions will unpack the content of the unframed payload and return
# true. If the command byte of the frame does not match the expectation or the
# frame is too short to unpack the expected payload, false will be returned.
# ########################################################################## #


def unpack_response(uframe):
    """
    Returns success
    """
    return uframe.unpack8()


def unpack_power_enable(uframe):
    """
    Returns enable
    """
    return uframe.unpack8()


def unpack_vout(uframe):
    """
    Returns V_out
    """
    return uframe.unpack16()


def unpack_ilimit(uframe):
    """
    Returns I_limit
    """
    return uframe.unpack16()


def unpack_query_response(uframe):
    """
    Returns a dictionary of the frame contents
    """
    data = {}
    data['command'] = uframe.unpack8()
    data['status'] = uframe.unpack8()
    data['v_in'] = uframe.unpack16()
    data['v_out'] = uframe.unpack16()
    data['i_out'] = uframe.unpack16()
    data['output_enabled'] = uframe.unpack8()
    temp1 = int(uframe.unpack16())
    if temp1 != 0xffff:
        if temp1 & 0x8000:
            temp1 -= 0x10000
        data['temp1'] = temp1 / 10
    temp2 = int(uframe.unpack16())
    if temp2 != 0xffff:
        if temp2 & 0x8000:
            temp2 -= 0x10000
        data['temp2'] = temp2 / 10
    data['temp_shutdown'] = uframe.unpack8()
    data['cur_func'] = uframe.unpack_cstr()
    data['params'] = {}
    while not uframe.eof():
        key = uframe.unpack_cstr()
        value = uframe.unpack_cstr()
        data['params'][key] = value
    return data


def unpack_cal_report(uframe):
    """
    Returns ADC/DAC values and calibration values
    """
    data = {}
    data['cal'] = {}
    data['command'] = uframe.unpack8()
    data['status'] = uframe.unpack8()
    data['vout_adc'] = uframe.unpack16()
    data['vin_adc'] = uframe.unpack16()
    data['iout_adc'] = uframe.unpack16()
    data['iout_dac'] = uframe.unpack16()
    data['vout_dac'] = uframe.unpack16()
    data['cal']['A_ADC_K'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['A_ADC_C'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['A_DAC_K'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['A_DAC_C'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['V_ADC_K'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['V_ADC_C'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['V_DAC_K'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['V_DAC_C'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['VIN_ADC_K'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    data['cal']['VIN_ADC_C'] = struct.unpack("<f", struct.pack("<I", uframe.unpack32()))[0]
    return data


def unpack_wifi_status(uframe):
    """
    Returns wifi_status
    """
    return uframe.unpack8()


def unpack_lock(uframe):
    """
    Returns locked
    """
    return uframe.unpack8()


def unpack_ocp(uframe):
    """
    Returns i_cut
    """
    return uframe.unpack16()


def unpack_temperature_report(uframe):
    """
    Returns a dictionary of the frame contents
    """
    data = {}
    data['command'] = uframe.unpack8()
    data['status'] = uframe.unpack8()
    data['v_in'] = uframe.unpack16()
    data['v_out'] = uframe.unpack16()
    data['i_out'] = uframe.unpack16()
    data['output_enabled'] = uframe.unpack8()
    data['cur_func'] = uframe.unpack_cstr()
    data['params'] = {}
    while not uframe.eof():
        key = uframe.unpack_cstr()
        value = uframe.unpack_cstr()
        data['params'][key] = value
    return data


def unpack_version_response(uframe):
    """
    Returns the dpsBoot and OpenDPS git hash strings
    """
    data = {}
    data['command'] = uframe.unpack8()
    data['status'] = uframe.unpack8()
    data['boot_git_hash'] = uframe.unpack_cstr()
    data['app_git_hash'] = uframe.unpack_cstr()
    return data
