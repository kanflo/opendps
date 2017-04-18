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
# Make sure these match protocol.h
cmd_ping = 1
cmd_set_vout = 2
cmd_set_ilimit = 3
cmd_status = 4
cmd_power_enable = 5
cmd_wifi_status = 6
cmd_lock = 7
cmd_ocp_event = 8
cmd_output_mode = 9
cmd_response = 0x80

# wifi_status_t
wifi_off = 0
wifi_connecting = 1
wifi_connected = 2
wifi_error = 3
wifi_upgrading = 4

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

def create_ping():
    f = uFrame()
    f.pack8(cmd_ping)
    f.end()
    return f

def create_power_enable(enable):
    f = uFrame()
    f.pack8(cmd_power_enable)
    f.pack8(enable)
    f.end()
    return f

def create_vout(vout_mv):
    f = uFrame()
    f.pack8(cmd_set_vout)
    f.pack16(vout_mv)
    f.end()
    return f

def create_ilimit(ilimit_ma):
    f = uFrame()
    f.pack8(cmd_set_ilimit)
    f.pack16(ilimit_ma)
    f.end()
    return f

def cmd_set_output_mode(mode):
    f = uFrame()
    f.pack8(cmd_output_mode)
    f.pack8(mode)
    f.end()
    return f

def create_status():
    f = uFrame()
    f.pack8(cmd_status)
    f.end()
    return f

def create_status_response(v_in, v_out_setting, v_out, i_out, i_limit, power_enabled):
    f = uFrame()
    f.pack8(cmd_response | cmd_status)
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

# Returns v_in, v_out_setting, v_out, i_out, i_limit, power_enabled
def unpack_status_response(uframe):
    command = uframe.unpack8()
    status = uframe.unpack8()
    return uframe.unpack16(), uframe.unpack16(), uframe.unpack16(), uframe.unpack16(), uframe.unpack16(), uframe.unpack8()

# Returns wifi_status
def unpack_wifi_status(uframe):
    return uframe.unpack8()

# Returns locked
def unpack_lock(uframe):
    return uframe.unpack8()

# Returns i_cut
def unpack_ocp(uframe):
    return uframe.unpack16()

