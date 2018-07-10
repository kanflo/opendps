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

from uframe import *

# command_t
cmd_ping = 1
#cmd_set_vout = 2
#cmd_set_ilimit = 3
cmd_query = 4
#cmd_power_enable = 5
cmd_wifi_status = 6
cmd_lock = 7
cmd_ocp_event = 8
cmd_upgrade_start = 9
cmd_upgrade_data = 10
cmd_set_function = 11
cmd_enable_output = 12
cmd_list_functions = 13
cmd_set_parameters = 14
cmd_list_parameters = 15
cmd_temperature_report = 16
cmd_response = 0x80

# wifi_status_t
wifi_off = 0
wifi_connecting = 1
wifi_connected = 2
wifi_error = 3
wifi_upgrading = 4

# upgrade_status_t
upgrade_continue = 0
upgrade_bootcom_error = 1
upgrade_crc_error = 2
upgrade_erase_error = 3
upgrade_flash_error = 4
upgrade_overflow_error = 5
upgrade_success = 16


"""
 Helpers for creating frames.
 Each function returns a complete frame ready for transmission.

"""
def create_response(command, success):
    f = uFrame()
    f.pack8(cmd_response | command)
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
    f.pack8(cmd_set_function)
    f.pack_cstr(name)
    f.end()
    return f

def create_enable_output(activate):
    f = uFrame()
    f.pack8(cmd_enable_output)
    f.pack8(1 if activate == "on" else 0)
    f.end()
    return f

def create_set_parameter(parameter_list):
    f = uFrame()
    f.pack8(cmd_set_parameters)
    for p in parameter_list:
        parts = p.split("=")
        if len(parts) != 2:
            return None
        else:
            f.pack_cstr(parts[0].lstrip().rstrip())
            f.pack_cstr(parts[1].lstrip().rstrip())
    f.end()
    return f

def create_query_response(v_in, v_out_setting, v_out, i_out, i_limit, power_enabled):
    f = uFrame()
    f.pack8(cmd_response | cmd_query)
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
    f.pack8(cmd_wifi_status)
    f.pack8(wifi_status)
    f.end()
    return f

def create_lock(locked):
    f = uFrame()
    f.pack8(cmd_lock)
    f.pack8(locked)
    f.end()
    return f

def create_ocp(i_cut):
    f = uFrame()
    f.pack8(cmd_ocp_event)
    f.pack16(i_cut)
    f.end()
    return f

def create_upgrade_start(window_size, crc):
    f = uFrame()
    f.pack8(cmd_upgrade_start)
    f.pack16(window_size)
    f.pack16(crc)
    f.end()
    return f

def create_upgrade_data(data):
    f = uFrame()
    f.pack8(cmd_upgrade_data)
    for d in data:
        f.pack8(d)
    f.end()
    return f

def create_temperature(temperature):
    print("Sending temperature %.1f and %.1f" % (temperature, -temperature))
    temperature = int(10 * temperature)
    f = uFrame()
    f.pack8(cmd_temperature_report)
    f.pack16(temperature)
    f.pack16(-temperature)
    f.end()
    return f

"""
Helpers for unpacking frames.

These functions will unpack the content of the unframed payload and return
true. If the command byte of the frame does not match the expectation or the
frame is too short to unpack the expected payload, false will be returned.
"""

# Returns success
def unpack_response():
    return uframe.unpack8()

# Returns enable
def unpack_power_enable(uframe):
    return uframe.unpack8()

# Returns V_out
def unpack_vout(uframe):
    return uframe.unpack16()

# Returns I_limit
def unpack_ilimit(uframe):
    return uframe.unpack16()

# Returns a dictionary of the frame contents
def unpack_query_response(uframe):
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
        data['temp1'] = float(temp1)/10
    temp2 = int(uframe.unpack16())
    if temp2 != 0xffff:
        if temp2 & 0x8000:
            temp2 -= 0x10000
        data['temp2'] = float(temp2)/10
    data['temp_shutdown'] = uframe.unpack8()
    data['cur_func'] = uframe.unpack_cstr()
    data['params'] = {}
    while not uframe.eof():
        key = uframe.unpack_cstr()
        value = uframe.unpack_cstr()
        data['params'][key] = value
    return data

# Returns wifi_status
def unpack_wifi_status(uframe):
    return uframe.unpack8()

# Returns locked
def unpack_lock(uframe):
    return uframe.unpack8()

# Returns i_cut
def unpack_ocp(uframe):
    return uframe.unpack16()

# Returns a dictionary of the frame contents
def unpack_temperature_report(uframe):
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

