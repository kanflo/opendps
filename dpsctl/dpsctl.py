#!/usr/bin/env python
# coding=utf-8

"""

Copyright (c) 2017 Johan Kanflo (github.com/kanflo)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


This script is used to communicate with an OpenDPS device and can be used to
change all the settings possible with the buttons and dial on the device
directly. The device can be talked to via a serial interface or (if you added
an ESP8266) via wifi

dpsctl.py --help will provide enlightment.

Oh, and if you get tired of specifying the comms interface (TTY or IP) all the
time, add it tht the environment variable DPSIF.

"""

from __future__ import absolute_import, print_function

import argparse
import os
import socket
import sys
import threading
import time

import serial

from . import protocol, uframe
from .uframe import uFrame
from .uhej import ANNOUNCE, IllegalFrameException, MCAST_GRP, MCAST_PORT, UDP, decode_frame, query


class CommInterface(object):
    """
    A abstract class that describes a communication interface
    """
    _if_name = None

    def __init__(self, if_name):
        self._if_name = if_name

    def open(self):
        return False

    def close(self):
        return False

    def write(self, some_bytes):
        return False

    def read(self):
        return bytearray()

    def name(self):
        return self._if_name


class TTYInterface(CommInterface):
    """
    A class that describes a serial interface
    """

    _port_handle = None

    def open(self):
        self._port_handle = serial.Serial(baudrate=115200, timeout=0.5)
        self._port_handle.port = self._if_name
        self._port_handle.open()
        return True

    def close(self):
        self._port_handle.port.close()
        self._port_handle = None
        return True

    def write(self, some_bytes):
        self._port_handle.write(some_bytes)
        return True

    def read(self):
        some_bytes = bytearray()
        sof = False
        while True:
            b = self._port_handle.read(1)
            if not b:  # timeout
                break
            b = ord(b)
            if b == uframe.SOF:
                some_bytes = bytearray()
                sof = True
            if sof:
                some_bytes.append(b)
            if b == uframe.EOF:
                break
        return some_bytes


class UDPInterface(CommInterface):
    """
    A class that describes a UDP interface
    """

    _socket = None

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

    def write(self, some_bytes):
        try:
            self._socket.sendto(some_bytes, (self._if_name, 5005))
        except socket.error as msg:
            fail("{} ({:d})".format(msg.strerror, msg.errno))
        return True

    def read(self):
        reply = bytearray()
        try:
            d = self._socket.recvfrom(1000)
            reply = bytearray(d[0])
            addr = d[1] # Unused
        except socket.timeout:
            pass
        except socket.error:
            pass
        return reply


def fail(message):
    """
    Print error message and exit with error
    """
    print("Error: {}.".format(message))
    sys.exit(os.EX_USAGE)


def handle_response(command, frame, args):
    """
    Handle a response frame from the device
    """
    resp_command = frame.get_frame()[0]
    if resp_command & protocol.cmd_response:
        resp_command ^= protocol.cmd_response
        success = frame.get_frame()[1]
        if resp_command != command:
            print("Warning: sent command {:02x}, response was {:02x}.".format(command, resp_command))
        if not success:
            fail("command failed according to device")

    json = {}
    if args.json:
        json["cmd"] = resp_command
        json["status"] = 1  # we're here aren't we?

    if resp_command == protocol.cmd_status:
        v_in, v_out_setting, v_out, i_out, i_limit, power_enabled = protocol.unpack_status_response(frame)
        if power_enabled:
            enable_str = "on"
        else:
            enable_str = "off"
        v_in_str = "{:d}.{:02d}".format(v_in / 1000, (v_in % 1000) / 10)
        v_out_str = "{:d}.{:02d}".format(v_out / 1000, (v_out % 1000) / 10)
        v_set_str = "{:d}.{:02d}".format(v_out_setting / 1000, (v_out_setting % 1000) / 10)
        i_lim_str = "{:d}.{:03d}".format(i_limit / 1000, i_limit % 1000)
        i_out_str = "{:d}.{:03d}".format(i_out / 1000, i_out % 1000)
        if args.json:
            json["V_in"] = v_in_str
            json["V_out"] = v_out_str
            json["V_set"] = v_set_str
            json["I_lim"] = i_lim_str
            json["I_out"] = i_out_str
            json["enable"] = power_enabled
        else:
            print("V_in  : {} V".format(v_in_str))
            print("V_set : {} V".format(v_set_str))
            print("V_out : {} V ({})".format(v_out_str, enable_str))
            print("I_lim : {} A".format(i_lim_str))
            print("I_out : {} A".format(i_out_str))

    if args.json:
        count = 0
        print("{", end=' ')
        for k in json:
            count += 1
            if count == len(json):
                print('"{}":"{}"'.format(k, json[k]), end=' ')
            else:
                print('"{}":"{}",'.format(k, json[k]), end=' ')
        print("}")


def communicate(comms, frame, args):
    """
    Communicate with the DPS device according to the user's wishes
    """
    some_bytes = frame.get_frame()

    if not comms:
        fail("no communication interface specified")
    if not comms.open():
        fail("could not open {}".format(comms.name()))
    if args.verbose:
        print("Communicating with {}".format(comms.name()))
        print("TX {:2d} bytes [{}]".format(len(some_bytes), " ".join("{:02x}".format(b) for b in some_bytes)))
    if not comms.write(some_bytes):
        fail("write failed on {}".format(comms.name()))
    resp = comms.read()
    if len(resp) == 0:
        fail("timeout talking to device {}".format(comms._if_name))
    elif args.verbose:
        print("RX {:d} bytes [{}]\n".format(len(resp), " ".join("{:02x}".format(b) for b in resp)))
    if not comms.close:
        print("Warning: could not close {}".format(comms.name()))

    f = uFrame()
    res = f.set_frame(resp)
    if res < 0:
        fail("protocol error ({:d})".format(res))
    else:
        handle_response(frame.get_frame()[1], f, args)


def handle_commands(args):
    """
    Communicate with the DPS device according to the user's whishes
    """
    if args.scan:
        uhej_scan()
        return

    comms = create_comms(args)

    # The ping command
    if args.ping:
        communicate(comms, protocol.create_ping(), args)

    # The lock and unlock commands
    if args.lock:
        communicate(comms, protocol.create_lock(1), args)
    if args.unlock:
        communicate(comms, protocol.create_lock(0), args)

    # The V_out set command
    if args.voltage is not None:
        v_out = int(args.voltage)
        communicate(comms, protocol.create_vout(v_out), args)

    # The I_max set command
    if args.current is not None:
        i_limit = int(args.current)
        communicate(comms, protocol.create_ilimit(i_limit), args)

    # The power enable command
    if args.power is not None:
        if args.power == "on" or args.power == "1":
            pwr = 1
        elif args.power == "off" or args.power == "0":
            pwr = 0
        else:
            pwr = None
        if pwr is None:
            fail("please say on/off or 1/0")
        else:
            communicate(comms, protocol.create_power_enable(pwr), args)

    # The status set command
    if args.status:
        communicate(comms, protocol.create_status(), args)


def is_ip_address(if_name):
    """
    Return True if the parameter if_name is an IP address.
    """
    try:
        socket.inet_aton(if_name)
        return True
    except socket.error:
        return False


def create_comms(args):
    """
    Create and return a communications interface object or None if no comms if
    was specified.
    """
    if_name = None
    comms = None
    if args.device:
        if_name = args.device
    elif 'DPSIF' in os.environ and len(os.environ['DPSIF']) > 0:
        if_name = os.environ['DPSIF']

    if if_name is not None:
        if is_ip_address(if_name):
            comms = UDPInterface(if_name)
        else:
            comms = TTYInterface(if_name)
    else:
        fail("no comms interface specified")
    return comms


def uhej_worker_thread():
    """
    The worker thread used by uHej for service discovery
    """
    global discovery_list
    global sock
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            port = addr[1]
            addr = addr[0]
            frame = bytearray(data)
            try:
                f = decode_frame(frame)
                f["source"] = addr
                f["port"] = port
                types = ["UDP", "TCP", "mcast"] # Unused
                if ANNOUNCE == f["frame_type"]:
                    for s in f["services"]:
                        key = "{}:{}:{}".format(f["source"], s["port"], s["type"])
                        if key not in discovery_list:
                            if s["service_name"] == "opendps":
                                discovery_list[key] = True  # Keep track of which hosts we have seen
                                print("{}".format(f["source"]))
            except IllegalFrameException:
                pass
        except socket.error as e:
            print('Expection: {}'.format(e))


def uhej_scan():
    """
    Scan for OpenDPS devices on the local network
    """
    global discovery_list
    global sock
    discovery_list = {}

    any_ip = "0.0.0.0"
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    except AttributeError:
        pass
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
    sock.bind((any_ip, MCAST_PORT))

    thread = threading.Thread(target=uhej_worker_thread)
    thread.daemon = True
    thread.start()

    sock.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(MCAST_GRP) + socket.inet_aton(any_ip))

    run_time_s = 6  # Run query for this many seconds
    query_interval_s = 2  # Send query this often
    last_query = 0
    start_time = time.time()

    while time.time() - start_time < run_time_s:
        if time.time() - last_query > query_interval_s:
            f = query(UDP, "*")
            sock.sendto(f, (MCAST_GRP, MCAST_PORT))
            last_query = time.time()
        time.sleep(1)

    num_found = len(discovery_list)
    if num_found == 0:
        print("No OpenDPS devices found")
    elif num_found == 1:
        print("1 OpenDPS device found")
    else:
        print("{:d} OpenDPS devices found".format(num_found))


def main():
    """
    Ye olde main
    """
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

    args, unknown = parser.parse_known_args()

    handle_commands(args)


if __name__ == "__main__":
    main()
