#!/usr/bin/env python

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

This script is used to communicate with an OpenDPS device and can be used to
change all the settings possible with the buttons and dial on the device
directly. The device can be talked to via a serial interface or (if you added
an ESP8266) via wifi

dpsctl.py --help will provide enlightment.

Oh, and if you get tired of specifying the comms interface (TTY or IP) all the
time, add it tht the environment variable DPSIF.

"""

import argparse
import sys
import os
import socket
try:
    import serial
except:
    print("Missing dependency pyserial:")
    print(" sudo pip install pyserial")
    raise SystemExit()
import threading
import time
from uhej import uhej
from protocol import *
import uframe
import binascii
try:
    from PyCRC.CRCCCITT import CRCCCITT
except:
    print("Missing dependency pycrc:")
    print(" sudo pip install pycrc")
    raise SystemExit()
import json

"""
A abstract class that describes a comminucation interface
"""
class comm_interface(object):

    _if_name = None

    def __init__(self, if_name):
        self._if_name = if_name

    def open(self):
        return False

    def close(self):
        return False

    def write(self, bytes):
        return False

    def read(self):
        return bytearray()

    def name(self):
        return self._if_name

"""
A class that describes a serial interface
"""
class tty_interface(comm_interface):

    _port_handle = None

    def __init__(self, if_name):
        self._if_name = if_name

    def open(self):
        self._port_handle = serial.Serial(baudrate = 115200, timeout = 1.0)
        self._port_handle.port = self._if_name
        self._port_handle.open()
        return True

    def close(self):
        self._port_handle.port.close()
        self._port_handle = None
        return True

    def write(self, bytes):
        self._port_handle.write(bytes)
        return True

    def read(self):
        bytes = bytearray()
        sof = False
        while True:
            b = self._port_handle.read(1)
            if not b: # timeout
                break
            b = ord(b)
            if b == uframe._SOF:
                bytes = bytearray()
                sof = True
            if sof:
                bytes.append(b)
            if b == uframe._EOF:
                break
        return bytes

"""
A class that describes a UDP interface
"""
class udp_interface(comm_interface):

    _socket = None

    def __init__(self, if_name):
        self._if_name = if_name

    def open(self):
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._socket.settimeout(1.0)
        except socket.error:
            return False
        return True

    def close(self):
        self._socket.close()
        self._socket = None
        return True

    def write(self, bytes):
        try:
            self._socket.sendto(bytes, (self._if_name, 5005))
        except socket.error as msg:
            fail("%s (%d)" % (str(msg[0]), msg[1]))
        return True

    def read(self):
        reply = bytearray()
        try:
            d = self._socket.recvfrom(1000)
            reply = bytearray(d[0])
            addr = d[1]
        except socket.timeout:
            pass
        except socket.error:
            pass
        return reply

"""
Print error message and exit with error
"""
def fail(message):
        print("Error: %s." % (message))
        sys.exit(1)


"""
Handle a response frame from the device.
Return a dictionaty of interesting information.
"""
def handle_response(command, frame, args):
    ret_dict = {}
    resp_command = frame.get_frame()[0]
    if resp_command & cmd_response:
        resp_command ^= cmd_response
        success = frame.get_frame()[1]
        if resp_command != command:
            print("Warning: sent command %02x, response was %02x." % (command, resp_command))
        if resp_command !=  cmd_upgrade_start and resp_command !=  cmd_upgrade_data and not success:
            fail("command failed according to device")

    if args.json:
        _json = {}
        _json["cmd"] = resp_command;
        _json["status"] = 1; # we're here aren't we?

    if resp_command == cmd_status:
        v_in, v_out_setting, v_out, i_out, i_limit, power_enabled = unpack_status_response(frame)
        if power_enabled:
            enable_str = "on"
        else:
            enable_str = "off"
        v_in_str = "%d.%02d" % (v_in/1000, (v_in%1000)/10)
        v_out_str = "%d.%02d" % (v_out/1000, (v_out%1000)/10)
        v_set_str = "%d.%02d" % (v_out_setting/1000, (v_out_setting%1000)/10)
        i_lim_str = "%d.%03d" % (i_limit/1000, i_limit%1000)
        i_out_str = "%d.%03d" % (i_out/1000, i_out%1000)
        if args.json:
            _json["V_in"] = v_in_str;
            _json["V_out"] = v_out_str;
            _json["V_set"] = v_set_str;
            _json["I_lim"] = i_lim_str;
            _json["I_out"] = i_out_str;
            _json["enable"] = power_enabled;
        else:
            print("V_in  : %s V" % (v_in_str))
            print("V_set : %s V" % (v_set_str))
            print("V_out : %s V (%s)" % (v_out_str, enable_str))
            print("I_lim : %s A" % (i_lim_str))
            print("I_out : %s A" % (i_out_str))
    elif resp_command == cmd_upgrade_start:
    #  *  DPS BL: [cmd_response | cmd_upgrade_start] [<upgrade_status_t>] [<chunk_size:16>]
        cmd = frame.unpack8()
        status = frame.unpack8()
        chunk_size = frame.unpack16()
        ret_dict["status"] = status
        ret_dict["chunk_size"] = chunk_size
    elif resp_command == cmd_upgrade_data:
        cmd = frame.unpack8()
        status = frame.unpack8()
        ret_dict["status"] = status

    if args.json:
        print(json.dumps(_json, indent=4, sort_keys=True))
 
    return ret_dict

"""
Communicate with the DPS device according to the user's whishes
"""
def communicate(comms, frame, args):
    bytes = frame.get_frame()

    if not comms:
        fail("no communication interface specified")
    if not comms.open():
        fail("could not open %s" % (comms.name()))
    if args.verbose:
        print("Communicating with %s" % (comms.name()))
        print("TX %2d bytes [%s]" % (len(bytes), " ".join("%02x" % b for b in bytes)))
    if not comms.write(bytes):
        fail("write failed on %s" % (comms.name()))
    resp = comms.read()
    if len(resp) == 0:
        fail("timeout talking to device %s" % (comms._if_name))
    elif args.verbose:
        print("RX %d bytes [%s]\n" % (len(resp), " ".join("%02x" % b for b in resp)))
    if not comms.close:
        print("Warning: could not close %s" % (comms.name()))

    f = uFrame()
    res = f.set_frame(resp)
    if res < 0:
        fail("protocol error (%d)" % (res))
    else:
        return handle_response(frame.get_frame()[1], f, args)

"""
Communicate with the DPS device according to the user's whishes
"""
def handle_commands(args):
    if args.scan:
        uhej_scan()
        return

    comms = create_comms(args)

    # The ping command
    if args.ping:
        communicate(comms, create_ping(), args)

    # The upgrade
    if args.firmware:
        run_upgrade(comms, args.firmware, args)

    # The lock and unlock commands
    if args.lock:
        communicate(comms, create_lock(1), args)
    if args.unlock:
        communicate(comms, create_lock(0), args)

    # The V_out set command
    if args.voltage != None:
        v_out = int(args.voltage)
        communicate(comms, create_vout(v_out), args)

    # The I_max set command
    if args.current != None:
        i_limit = int(args.current)
        communicate(comms, create_ilimit(i_limit), args)

    # The power enable command
    if args.power != None:
        if args.power == "on" or args.power == "1":
            pwr = 1
        elif args.power == "off" or args.power == "0":
            pwr = 0
        else:
            pwr = None
        if pwr == None:
            fail("please say on/off or 1/0")
        else:
            communicate(comms, create_power_enable(pwr), args)

    # The status set command
    if args.status:
        communicate(comms, create_status(), args)

"""
Return True if the parameter if_name is an IP address.
"""
def is_ip_address(if_name):
    try:
        socket.inet_aton(if_name)
        return True
    except socket.error:
        return False

# Darn beautiful, from SO: https://stackoverflow.com/a/1035456
def chunk_from_file(filename, chunk_size):
    with open(filename, "rb") as f:
        while True:
            chunk = f.read(chunk_size)
            if chunk:
                yield bytearray(chunk)
            else:
                break

"""
Run OpenDPS firmware upgrade
"""
def run_upgrade(comms, fw_file_name, args):
    with open(fw_file_name, mode='rb') as file:
        #crc = binascii.crc32(file.read()) % (1<<32)
        content = file.read()
        if content.encode('hex')[6:8] != "20" and not args.force:
            fail("The firmware file does not seem valid, use --force to force upgrade")
        crc = CRCCCITT().calculate(content)
    chunk_size = 1024
    ret_dict = communicate(comms, create_upgrade_start(chunk_size, crc), args)
    if ret_dict["status"] == upgrade_continue:
        if chunk_size != ret_dict["chunk_size"]:
            print("Device selected chunk size %d" % (ret_dict["chunk_size"]))
        counter = 0
        for chunk in chunk_from_file(fw_file_name, chunk_size):
            counter += len(chunk)
            sys.stdout.write("\rDownload progress: %d%% " % (counter*1.0/len(content)*100.0) )
            sys.stdout.flush()
#            print(" %d bytes" % (counter))

            ret_dict = communicate(comms, create_upgrade_data(chunk), args)
            status = ret_dict["status"]
            if status == upgrade_continue:
                pass
            elif status == upgrade_crc_error:
                print("")
                fail("device reported CRC error")
            elif status == upgrade_erase_error:
                print("")
                fail("device reported erasing error")
            elif status == upgrade_flash_error:
                print("")
                fail("device reported flashing error")
            elif status == upgrade_overflow_error:
                print("")
                fail("device reported firmware overflow error")
            elif status == upgrade_success:
                print("")
            else:
                print("")
                fail("device reported an unknown error (%d)" % status)
    else:
        fail("Device rejected firmware upgrade")
    sys.exit(os.EX_OK)

"""
Create and return a comminications interface object or None if no comms if
was specified.
"""
def create_comms(args):
    if_name = None
    comms = None
    if args.device:
        if_name = args.device
    elif 'DPSIF' in os.environ and len(os.environ['DPSIF']) > 0:
        if_name = os.environ['DPSIF']

    if if_name != None:
        if is_ip_address(if_name):
            comms = udp_interface(if_name)
        else:
            comms = tty_interface(if_name)
    else:
        fail("no comms interface specified")
    return comms

"""
The worker thread used by uHej for service discovery
"""
def uhej_worker_thread():
    global discovery_list
    global sock
    while 1:
        try:
            data, addr = sock.recvfrom(1024)
            port = addr[1]
            addr = addr[0]
            frame = bytearray(data)
            try:
                f = uhej.decode_frame(frame)
                f["source"] = addr
                f["port"] = port
                types = ["UDP", "TCP", "mcast"]
                if uhej.ANNOUNCE == f["frame_type"]:
                    for s in f["services"]:
                        key = "%s:%s:%s" % (f["source"], s["port"], s["type"])
                        if not key in discovery_list:
                            if s["service_name"] == "opendps":
                                discovery_list[key] = True # Keep track of which hosts we have seen
                                print("%s" % (f["source"]))
#                            print("%16s:%-5d  %-8s %s" % (f["source"], s["port"], types[s["type"]], s["service_name"]))
            except uhej.IllegalFrameException as e:
                pass
        except socket.error as e:
            print('Exception'), e

"""
Scan for OpenDPS devices on the local network
"""
def uhej_scan():
    global discovery_list
    global sock
    discovery_list = {}

    ANY = "0.0.0.0"
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    except AttributeError:
        pass
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
    sock.bind((ANY, uhej.MCAST_PORT))

    thread = threading.Thread(target = uhej_worker_thread)
    thread.daemon = True
    thread.start()

    sock.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(uhej.MCAST_GRP) + socket.inet_aton(ANY))

    run_time_s = 6 # Run query for this many seconds
    query_interval_s = 2 # Send query this often
    last_query = 0
    start_time = time.time()

    while time.time() - start_time < run_time_s:
        if time.time() - last_query > query_interval_s:
            f = uhej.query(uhej.UDP, "*")
            sock.sendto(f, (uhej.MCAST_GRP, uhej.MCAST_PORT))
            last_query = time.time()
        time.sleep(1)

    num_found = len(discovery_list)
    if num_found == 0:
        print("No OpenDPS devices found")
    elif num_found == 1:
        print("1 OpenDPS device found")
    else:
        print("%d OpenDPS devices found" % (num_found))


"""
Ye olde main
"""
def main():
    global args
    parser = argparse.ArgumentParser(description='Instrument an OpenDPS device')

    parser.add_argument('-d', '--device', help="OpenDPS device to connect to. Can be a /dev/tty device or an IP number. If omitted, dpsctl.py will try the environment variable DPSIF", default='')
    parser.add_argument('-S', '--scan', action="store_true", help="Scan for OpenDPS wifi devices")
    parser.add_argument('-u', '--voltage', type=int, help="Set voltage (millivolt)")
    parser.add_argument('-i', '--current', type=int, help="Set maximum current (milliampere)")
    parser.add_argument('-p', '--power', help="Power 'on' or 'off'")
    parser.add_argument('-P', '--ping', action='store_true', help="Ping device")
    parser.add_argument('-L', '--lock', action='store_true', help="Lock device keys")
    parser.add_argument('-l', '--unlock', action='store_true', help="Unlock device keys")
    parser.add_argument('-s', '--status', action='store_true', help="Read voltage/current settings and measurements")
    parser.add_argument('-j', '--json', action='store_true', help="Output status as JSON")
    parser.add_argument('-v', '--verbose', action='store_true', help="Verbose communications")
    parser.add_argument('-U', '--upgrade', type=str, dest="firmware", help="Perform upgrade of OpenDPS firmware")
    parser.add_argument('-f', '--force', action='store_true', help="Force upgrade even if dpsctl complains about the firmware")

    args, unknown = parser.parse_known_args()

    try:
        handle_commands(args)
    except KeyboardInterrupt:
        print("")

if __name__ == "__main__":
    main()
