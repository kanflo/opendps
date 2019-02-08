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

dpsctl.py --help will provide enlightenment.

Oh, and if you get tired of specifying the comms interface (TTY or IP) all the
time, add it tht the environment variable DPSIF.

"""

from __future__ import division

import argparse
import json
import os
import socket
import sys
import threading
import time

import protocol
import uframe
from protocol import (create_cmd, create_enable_output, create_lock, create_set_calibration,
                      create_set_function, create_set_parameter, create_temperature,
                      create_upgrade_data, create_upgrade_start, create_change_screen,
                      unpack_cal_report, unpack_query_response, unpack_version_response)
from uhej import uhej

try:
    from PyCRC.CRCCCITT import CRCCCITT
except ImportError:
    print("Missing dependency pycrc:")
    print(" sudo pip{} install pycrc"
          .format("3" if sys.version_info.major == 3 else ""))
    raise SystemExit()
try:
    import serial
except ImportError:
    print("Missing dependency pyserial:")
    print(" sudo pip{} install pyserial"
          .format("3" if sys.version_info.major == 3 else ""))
    raise SystemExit()

parameters = []


class comm_interface(object):
    """
    An abstract class that describes a communication interface
    """

    _if_name = None

    def __init__(self, if_name):
        self._if_name = if_name

    def open(self):
        return False

    def close(self):
        return False

    def write(self, bytes_):
        return False

    def read(self):
        return bytearray()

    def name(self):
        return self._if_name


class tty_interface(comm_interface):
    """
    A class that describes a serial interface
    """

    _port_handle = None
    _baudrate = None

    def __init__(self, if_name, baudrate):
        super(tty_interface, self).__init__(if_name)
        self._if_name = if_name
        self._baudrate = baudrate

    def open(self):
        if not self._port_handle:
            self._port_handle = serial.Serial(baudrate=self._baudrate, timeout=1.0)
            self._port_handle.port = self._if_name
            self._port_handle.open()
        return True

    def close(self):
        self._port_handle.port.close()
        self._port_handle = None
        return True

    def write(self, bytes_):
        self._port_handle.write(bytes_)
        return True

    def read(self):
        bytes_ = bytearray()
        sof = False
        while True:
            b = self._port_handle.read(1)
            if not b:  # timeout
                break
            b = ord(b)
            if b == uframe._SOF:
                bytes_ = bytearray()
                sof = True
            if sof:
                bytes_.append(b)
            if b == uframe._EOF:
                break
        return bytes_


class udp_interface(comm_interface):
    """
    A class that describes a UDP interface
    """

    _socket = None

    def __init__(self, if_name):
        super(udp_interface, self).__init__(if_name)

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

    def write(self, bytes_):
        try:
            self._socket.sendto(bytes_, (self._if_name, 5005))
        except socket.error as msg:
            fail("{} ({:d})".format(str(msg[0]), msg[1]))
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


def fail(message):
    """
    Print error message and exit with error
    """
    print("Error: {}.".format(message))
    sys.exit(1)


def unit_name(unit):
    """
    Return name of unit (must of course match unit_t in opendps/uui.h)
    """
    if unit == 0:
        return "unitless"  # none
    if unit == 1:
        return "A"  # ampere
    if unit == 2:
        return "V"  # volt
    if unit == 3:
        return "W"  # watt
    if unit == 4:
        return "s"  # second
    if unit == 5:
        return "Hz"  # hertz
    return "unknown"


def prefix_name(prefix):
    """
    Return SI prefix
    """
    if prefix == -6:
        return "u"
    if prefix == -3:
        return "m"
    if prefix == -2:
        return "c"
    if prefix == -1:
        return "d"  # TODO: is this correct (deci?
    if prefix == 0:
        return ""
    if prefix == 1:
        return "D"  # TODO: is this correct (deca)?
    if prefix == 2:
        return "hg"
    if prefix == 3:
        return "k"
    if prefix == 4:
        return "M"
    return "e{:d}".format(prefix)


def handle_response(command, frame, args):
    """
    Handle a response frame from the device.
    Return a dictionary of interesting information.
    """
    ret_dict = {}
    resp_command = frame.get_frame()[0]
    if resp_command & protocol.CMD_RESPONSE:
        resp_command ^= protocol.CMD_RESPONSE
        success = frame.get_frame()[1]
        if resp_command != command:
            print("Warning: sent command {:02x}, response was {:02x}.".format(command, resp_command))
        if resp_command != protocol.CMD_UPGRADE_START and resp_command != protocol.CMD_UPGRADE_DATA and not success:
            fail("command failed according to device")

    if args.json:
        _json = {}
        _json["cmd"] = resp_command
        _json["status"] = 1  # we're here aren't we?

    if resp_command == protocol.CMD_PING:
        print("Got pong from device")
    elif resp_command == protocol.CMD_QUERY:
        data = unpack_query_response(frame)
        enable_str = "on" if data['output_enabled'] else "temperature shutdown" if data['temp_shutdown'] == 1 else "off"
        v_in_str = "{:.2f}".format(data['v_in'] / 1000)
        v_out_str = "{:.2f}".format(data['v_out'] / 1000)
        i_out_str = "{:.3f}".format(data['i_out'] / 1000)
        if args.json:
            _json = data
        else:
            print("{:<10} : {} ({})".format('Func', data['cur_func'], enable_str))
            for key, value in data['params'].items():
                print("  {:<8} : {}".format(key, value))
            print("{:<10} : {} V".format('V_in', v_in_str))
            print("{:<10} : {} V".format('V_out', v_out_str))
            print("{:<10} : {} A".format('I_out', i_out_str))
            if 'temp1' in data:
                print("{:<10} : {:.1f}".format('temp1', data['temp1']))
            if 'temp2' in data:
                print("{:<10} : {:.1f}".format('temp2', data['temp2']))

    elif resp_command == protocol.CMD_UPGRADE_START:
        #  *  DPS BL: [cmd_response | cmd_upgrade_start] [<upgrade_status_t>] [<chunk_size:16>]
        cmd = frame.unpack8()
        status = frame.unpack8()
        chunk_size = frame.unpack16()
        ret_dict["status"] = status
        ret_dict["chunk_size"] = chunk_size
    elif resp_command == protocol.CMD_UPGRADE_DATA:
        cmd = frame.unpack8()
        status = frame.unpack8()
        ret_dict["status"] = status
    elif resp_command == protocol.CMD_SET_FUNCTION:
        cmd = frame.unpack8()
        status = frame.unpack8()
        if not status:
            print("Function does not exist.")  # Never reached due to status == 0
        else:
            print("Changed function.")
    elif resp_command == protocol.CMD_LIST_FUNCTIONS:
        cmd = frame.unpack8()
        status = frame.unpack8()
        if status == 0:
            print("Error, failed to list available functions")
        else:
            functions = []
            name = frame.unpack_cstr()
            while name != "":
                functions.append(name)
                name = frame.unpack_cstr()
            if args.json:
                _json["functions"] = functions
            else:
                if len(functions) == 0:
                    print("Selected OpenDPS supports no functions at all, which is quite weird when you think about it...")
                elif len(functions) == 1:
                    print("Selected OpenDPS supports the {} function.".format(functions[0]))
                else:
                    temp = ", ".join(functions[:-1])
                    temp = "{} and {}".format(temp, functions[-1])
                    print("Selected OpenDPS supports the {} functions.".format(temp))
    elif resp_command == protocol.CMD_SET_PARAMETERS:
        cmd = frame.unpack8()
        status = frame.unpack8()
        for p in args.parameter:
            status = frame.unpack8()
            parts = p.split("=")
            # TODO: handle json output
            print("{}: {}".format(parts[0], "ok" if status == 0 else "unknown parameter" if status == 1 else "out of range" if status == 2 else "unsupported parameter" if status == 3 else "unknown error {:d}".format(status)))
    elif resp_command == protocol.CMD_SET_CALIBRATION:
        cmd = frame.unpack8()
        status = frame.unpack8()
        for p in args.calibration_set:
            status = frame.unpack8()
            parts = p.split("=")
            # TODO: handle json output
            print("{}: {}".format(parts[0], "ok" if status == 0 else "unknown coefficient" if status == 1 else "out of range" if status == 2 else "unsupported coefficient" if status == 3 else "flash write error" if status == 4 else "unknown error {:d}".format(status)))
    elif resp_command == protocol.CMD_LIST_PARAMETERS:
        cmd = frame.unpack8()
        status = frame.unpack8()
        if status == 0:
            print("Error, failed to list available parameters")
        else:
            cur_func = frame.unpack_cstr()
            parameters = []
            while not frame.eof():
                parameter = {}
                parameter['name'] = frame.unpack_cstr()
                parameter['unit'] = unit_name(frame.unpack8())
                parameter['prefix'] = prefix_name(frame.unpacks8())
                parameters.append(parameter)
            if args.json:
                _json["current_function"] = cur_func
                _json["parameters"] = parameters
            else:
                if len(parameters) == 0:
                    print("Selected OpenDPS supports no parameters at all for the {} function".format(cur_func))
                elif len(parameters) == 1:
                    print("Selected OpenDPS supports the {} parameter ({}{}) for the {} function.".format(parameters[0]['name'], parameters[0]['prefix'], parameters[0]['unit'], cur_func))
                else:
                    temp = ""
                    for p in parameters:
                        temp += p['name'] + ' ({}{})'.format(p['prefix'], p['unit']) + " "
                    print("Selected OpenDPS supports the {}parameters for the {} function.".format(temp, cur_func))
    elif resp_command == protocol.CMD_ENABLE_OUTPUT:
        cmd = frame.unpack8()
        status = frame.unpack8()
        if status == 0:
            print("Error, failed to enable/disable output.")
    elif resp_command == protocol.CMD_TEMPERATURE_REPORT:
        pass
    elif resp_command == protocol.CMD_LOCK:
        pass
    elif resp_command == protocol.CMD_VERSION:
        data = unpack_version_response(frame)
        print("BootDPS GIT Hash: {}".format(data['boot_git_hash']))
        print("OpenDPS GIT Hash: {}".format(data['app_git_hash']))
    elif resp_command == protocol.CMD_CAL_REPORT:
        ret_dict = unpack_cal_report(frame)
    elif resp_command == protocol.CMD_CLEAR_CALIBRATION:
        pass
    elif resp_command == protocol.CMD_CHANGE_SCREEN:
        pass
    else:
        print("Unknown response {:d} from device.".format(resp_command))

    if args.json:
        print(json.dumps(_json, indent=4, sort_keys=True))

    return ret_dict


def communicate(comms, frame, args):
    """
    Communicate with the DPS device according to the user's wishes
    """
    bytes_ = frame.get_frame()

    if not comms:
        fail("no communication interface specified")
    if not comms.open():
        fail("could not open {}".format(comms.name()))
    if args.verbose:
        print("Communicating with {}".format(comms.name()))
        print("TX {:2d} bytes [{}]".format(len(bytes_), " ".join("{:02x}".format(b) for b in bytes_)))
    if not comms.write(bytes_):
        fail("write failed on {}".format(comms.name()))
    resp = comms.read()
    if len(resp) == 0:
        fail("timeout talking to device {}".format(comms._if_name))
    elif args.verbose:
        print("RX {:2d} bytes [{}]\n".format(len(resp), " ".join("{:02x}".format(b) for b in resp)))
    if not comms.close:
        print("Warning: could not close {}".format(comms.name()))

    f = uframe.uFrame()
    res = f.set_frame(resp)
    if res < 0:
        fail("protocol error ({:d})".format(res))
    else:
        return handle_response(frame.get_frame()[1], f, args)


def handle_commands(args):
    """
    Communicate with the DPS device according to the user's wishes
    """
    if args.scan:
        uhej_scan()
        return

    comms = create_comms(args)

    if args.ping:
        communicate(comms, create_cmd(protocol.CMD_PING), args)

    if args.firmware:
        run_upgrade(comms, args.firmware, args)

    if args.lock:
        communicate(comms, create_lock(1), args)
    if args.unlock:
        communicate(comms, create_lock(0), args)

    if args.list_functions:
        communicate(comms, create_cmd(protocol.CMD_LIST_FUNCTIONS), args)

    if args.list_parameters:
        communicate(comms, create_cmd(protocol.CMD_LIST_PARAMETERS), args)

    if args.function:
        communicate(comms, create_set_function(args.function), args)

    if args.enable:
        if args.enable == 'on' or args.enable == 'off':
            communicate(comms, create_enable_output(args.enable), args)
        else:
            fail("enable is 'on' or 'off'")

    if args.parameter:
        payload = create_set_parameter(args.parameter)
        if payload:
            communicate(comms, payload, args)
        else:
            fail("malformed parameters")

    if args.query:
        communicate(comms, create_cmd(protocol.CMD_QUERY), args)

    if args.version:
        communicate(comms, create_cmd(protocol.CMD_VERSION), args)

    if args.calibration_report:
        data = communicate(comms, create_cmd(protocol.CMD_CAL_REPORT), args)
        print("Calibration Report:")
        print("\tA_ADC_K = {}".format(data['cal']['A_ADC_K']))
        print("\tA_ADC_C = {}".format(data['cal']['A_ADC_C']))
        print("\tA_DAC_K = {}".format(data['cal']['A_DAC_K']))
        print("\tA_DAC_C = {}".format(data['cal']['A_DAC_C']))
        print("\tV_ADC_K = {}".format(data['cal']['V_ADC_K']))
        print("\tV_ADC_C = {}".format(data['cal']['V_ADC_C']))
        print("\tV_DAC_K = {}".format(data['cal']['V_DAC_K']))
        print("\tV_DAC_C = {}".format(data['cal']['V_DAC_C']))
        print("\tVIN_ADC_K = {}".format(data['cal']['VIN_ADC_K']))
        print("\tVIN_ADC_C = {}".format(data['cal']['VIN_ADC_C']))
        print("\tVIN_ADC = {}".format(data['vin_adc']))
        print("\tVOUT_ADC = {}".format(data['vout_adc']))
        print("\tIOUT_ADC = {}".format(data['iout_adc']))
        print("\tIOUT_DAC = {}".format(data['iout_dac']))
        print("\tVOUT_DAC = {}".format(data['vout_dac']))

    if args.calibration_set:
        payload = create_set_calibration(args.calibration_set)
        if payload:
            communicate(comms, payload, args)
        else:
            fail("malformed parameters")

    if hasattr(args, 'temperature') and args.temperature:
        communicate(comms, create_temperature(float(args.temperature)), args)

    if args.calibration_reset:
        communicate(comms, create_cmd(protocol.CMD_CLEAR_CALIBRATION), args)

    if args.switch_screen:
        if (args.switch_screen.lower() == "main"):
            communicate(comms, create_change_screen(protocol.CHANGE_SCREEN_MAIN), args)
        elif (args.switch_screen.lower() == "settings"):
            communicate(comms, create_change_screen(protocol.CHANGE_SCREEN_SETTINGS), args)
        else:
            fail("please specify either 'settings' or 'main' as parameters")


def is_ip_address(if_name):
    """
    Return True if the parameter if_name is an IP address.
    """
    try:
        socket.inet_aton(if_name)
        return True
    except socket.error:
        return False


def chunk_from_file(filename, chunk_size):
    # Darn beautiful, from SO: https://stackoverflow.com/a/1035456

    with open(filename, "rb") as f:
        while True:
            chunk = f.read(chunk_size)
            if chunk:
                yield bytearray(chunk)
            else:
                break


def run_upgrade(comms, fw_file_name, args):
    """
    Run OpenDPS firmware upgrade
    """
    with open(fw_file_name, mode='rb') as file:
        # crc = binascii.crc32(file.read()) % (1<<32)
        content = file.read()
        if content.encode('hex')[6:8] != "20" and not args.force:
            fail("The firmware file does not seem valid, use --force to force upgrade")
        crc = CRCCCITT().calculate(content)
    chunk_size = 1024
    ret_dict = communicate(comms, create_upgrade_start(chunk_size, crc), args)
    if ret_dict["status"] == protocol.UPGRADE_CONTINUE:
        if chunk_size != ret_dict["chunk_size"]:
            print("Device selected chunk size {:d}".format(ret_dict["chunk_size"]))
            chunk_size = ret_dict["chunk_size"]
        counter = 0
        for chunk in chunk_from_file(fw_file_name, chunk_size):
            counter += len(chunk)
            sys.stdout.write("\rDownload progress: {:d}% ".format(int(counter / len(content) * 100)))
            sys.stdout.flush()
            # print(" {:d} bytes".format(counter))

            ret_dict = communicate(comms, create_upgrade_data(chunk), args)
            status = ret_dict["status"]
            if status == protocol.UPGRADE_CONTINUE:
                pass
            elif status == protocol.UPGRADE_CRC_ERROR:
                print("")
                fail("device reported CRC error")
            elif status == protocol.UPGRADE_ERASE_ERROR:
                print("")
                fail("device reported erasing error")
            elif status == protocol.UPGRADE_FLASH_ERROR:
                print("")
                fail("device reported flashing error")
            elif status == protocol.UPGRADE_OVERFLOW_ERROR:
                print("")
                fail("device reported firmware overflow error")
            elif status == protocol.UPGRADE_SUCCESS:
                print("")
            else:
                print("")
                fail("device reported an unknown error ({:d})".format(status))
    else:
        fail("Device rejected firmware upgrade")


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
            comms = udp_interface(if_name)
        else:
            comms = tty_interface(if_name, args.baudrate)
    else:
        fail("no comms interface specified")
    return comms


def uhej_worker_thread():
    """
    The worker thread used by uHej for service discovery
    """
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
                        key = "{}:{}:{}".format(f["source"], s["port"], s["type"])
                        if key not in discovery_list:
                            if s["service_name"] == "opendps":
                                discovery_list[key] = True  # Keep track of which hosts we have seen
                                print("{}".format(f["source"]))
                                # print("{:>16}:{:<5d}  {:<8} {}".format(f["source"], s["port"], types[s["type"]], s["service_name"]))
            except uhej.IllegalFrameException as e:
                pass
        except socket.error as e:
            print('Exception', e)


def uhej_scan():
    """
    Scan for OpenDPS devices on the local network
    """
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

    thread = threading.Thread(target=uhej_worker_thread)
    thread.daemon = True
    thread.start()

    sock.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(uhej.MCAST_GRP) + socket.inet_aton(ANY))

    run_time_s = 6  # Run query for this many seconds
    query_interval_s = 2  # Send query this often
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
        print("{:d} OpenDPS devices found".format(num_found))


def main():
    """
    Ye olde main
    """
    global args
    testing = '--testing' in sys.argv
    parser = argparse.ArgumentParser(description='Instrument an OpenDPS device')

    parser.add_argument('-d', '--device', help="OpenDPS device to connect to. Can be a /dev/tty device or an IP number. If omitted, dpsctl.py will try the environment variable DPSIF", default='')
    parser.add_argument('-b', '--baudrate', type=int, dest="baudrate", help="Set baudrate used for serial communications", default=115200)
    parser.add_argument('-S', '--scan', action="store_true", help="Scan for OpenDPS wifi devices")
    parser.add_argument('-f', '--function', nargs='?', help="Set active function")
    parser.add_argument('-F', '--list-functions', action='store_true', help="List available functions")
    parser.add_argument('-p', '--parameter', nargs='+', help="Set function parameter <name>=<value>")
    parser.add_argument('-P', '--list-parameters', action='store_true', help="List function parameters of active function")
    parser.add_argument('-c', '--calibration_set', nargs='+', help="Set the specified calibration coefficient <name>=<value>")
    parser.add_argument('-cr', '--calibration_report', action="store_true", help="Prints Calibration report")
    parser.add_argument('--calibration_reset', action='store_true', help="Resets the calibration to the default values")
    parser.add_argument('-o', '--enable', help="Enable output ('on' or 'off')")
    parser.add_argument('--ping', action='store_true', help="Ping device (causes screen to flash)")
    parser.add_argument('-L', '--lock', action='store_true', help="Lock device keys")
    parser.add_argument('-l', '--unlock', action='store_true', help="Unlock device keys")
    parser.add_argument('-q', '--query', action='store_true', help="Query device settings and measurements")
    parser.add_argument('-j', '--json', action='store_true', help="Output parameters as JSON")
    parser.add_argument('-v', '--verbose', action='store_true', help="Verbose communications")
    parser.add_argument('-V', '--version', action='store_true', help="Get firmware version information")
    parser.add_argument('-U', '--upgrade', type=str, dest="firmware", help="Perform upgrade of OpenDPS firmware")
    parser.add_argument('--screen', type=str, dest="switch_screen", help="Switch to 'settings' or 'main' screen")
    parser.add_argument('--force', action='store_true', help="Force upgrade even if dpsctl complains about the firmware")
    if testing:
        parser.add_argument('-t', '--temperature', type=str, dest="temperature", help="Send temperature report (for testing)")

    args, unknown = parser.parse_known_args()

    try:
        handle_commands(args)
    except KeyboardInterrupt:
        print("")


if __name__ == "__main__":
    main()
