# coding=utf-8

import logging
import socket
import struct

logger = logging.getLogger(__name__)

# The multicast IP and port uHej resides on
MCAST_GRP = "225.0.1.250"
MCAST_PORT = 4242

rx_timeout_s = 1

# uHej frame magic
MAGIC = 0xfedebeda

# Frame types
HELLO = 0
ANNOUNCE = 1
QUERY = 2
BEACON = 3

# Service types
UDP = 0
TCP = 1
MCAST = 2


class IllegalFrameException(Exception):
    pass


# Utility function to convert IP address to 32 bit integer
def ip2int(addr):
    return struct.unpack("!I", socket.inet_aton(addr))[0]


# Utility function to convert 32 bit integer to IP address
def int2ip(addr):
    return socket.inet_ntoa(struct.pack("!I", addr))


# Decode a received frame as an uhej frame (a byte array)
# Raises IllegalFrameException if the frame is not a valud uhej frame
def decode_frame(frame):
    f = {}
    try:
        magic = _decode_int32(frame, 0)
        if MAGIC != magic:
            raise IllegalFrameException
        type = frame[4]
        if HELLO == type:
            f = _decode_hello(frame)
        elif ANNOUNCE == type:
            f = _decode_announce(frame)
        elif QUERY == type:
            f = _decode_query(frame)
        elif BEACON == type:
            f = _decode_beacon(frame)
        else:
            raise IllegalFrameException
    except IndexError as e:
        print("Index error")
        raise IllegalFrameException
    return f


# Create and return a frame (a byte array) with given type (UDP, TCP or MCAST)
def create_frame(type):
    frame = bytearray()
    frame.append((MAGIC >> 24) & 0xff)
    frame.append((MAGIC >> 16) & 0xff)
    frame.append((MAGIC >> 8) & 0xff)
    frame.append((MAGIC) & 0xff)
    frame.append(type & 0xff)
    return frame


# Add byte array payload to frame, return new frame
def add_payload(frame, payload):
    #   frame.append(len(payload))
    frame += payload
    return frame


# Create and return a hello frame
def hello(node_id, ip_addr_str, macaddr_str, name):
    f = create_frame(HELLO)
    p = bytearray()
    ip = ip2int(ip_addr_str)

    # Append node id
    p.append((node_id >> 24) & 0xff)
    p.append((node_id >> 16) & 0xff)
    p.append((node_id >> 8) & 0xff)
    p.append((node_id) & 0xff)

    # Append IP
    p.append((ip >> 24) & 0xff)
    p.append((ip >> 16) & 0xff)
    p.append((ip >> 8) & 0xff)
    p.append((ip) & 0xff)

    # Apped MAC address
    for b in macaddr_str.split(":"):
        p.append(int(b, 16))

    # Append node name
    p.extend(name.encode('ascii'))
    p.append(0)

    f = add_payload(f, p)
    return f


# Create and return a query framefor a service of type UDP, TCP or MULTICAST with given name
def query(type, name):
    f = create_frame(QUERY)
    p = bytearray()
    p.append(type & 0xff)
    p.extend(name.encode('ascii'))
    p.append(0)
    f = add_payload(f, p)
    return f


# Create and return an announce frame. Services is an array each item being a dictionary of
# 'type' : int8     type of service (UDP, TCP or MCAST)
# 'port' : int16    port where service resides
# 'name' : string   name of servier
def announce(services):
    f = create_frame(ANNOUNCE)
    p = bytearray()
    for s in services:
        p.append(s["type"] & 0xff)
        p.append((s["port"] >> 8) & 0xff)
        p.append((s["port"]) & 0xff)
        p.extend(s["name"].encode('ascii'))
        p.append(0)
    f = add_payload(f, p)
    return f


# Create beacon frame. Will be used if beacons ever get implemented
# def beacon():
#    f = create_frame(BEACON)
#    return f

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(('8.8.8.8', 53))  # connecting to a UDP address doesn't send packets
    local_ip_address = s.getsockname()[0]
    return local_ip_address


### Internal functions below ###

# Decode an int32 in network byte order in the byte array 'data' startging at 'offset'
def _decode_int32(data, offset):
    return (data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | (data[offset + 3])


# Decode an int16 in network byte order in the byte array 'data' startging at 'offset'
def _decode_int16(data, offset):
    return (data[offset] << 8) | (data[offset + 1])


# Find the nexto zero byte in the byte array 'data' starting at 'start'
# Return found location or -1 if none found
def _find_zero(data, start):
    i = start
    while i < len(data):
        if data[i] == 0:
            return i;
        i += 1
    return -1


# Decode a 'hello' frame
def _decode_hello(frame):
    f = {}
    f['frame_type'] = frame[4]
    f['node_id'] = _decode_int32(frame, 5)
    f['ip'] = int2ip(_decode_int32(frame, 9))
    f['mac'] = "%02x:%02x:%02x:%02x:%02x:%02x" % (frame[13], frame[14], frame[15], frame[16], frame[17], frame[18])
    f['name'] = frame[19:-1].decode("ascii")
    return f


# Decode a 'announce' frame
def _decode_announce(frame):
    f = {}
    f['frame_type'] = frame[4]
    i = 5
    services = []
    while i < len(frame):
        s = {}
        s['type'] = frame[i]
        s['port'] = _decode_int16(frame, i + 1)
        j = _find_zero(frame, i + 3)
        s['service_name'] = frame[i + 3:j].decode("utf8")
        i = j + 1
        services.append(s)

    f['services'] = services

    return f


# Decode a 'query' frame
def _decode_query(frame):
    f = {}
    f['frame_type'] = frame[4]
    f['service_type'] = frame[5]
    f['service_name'] = frame[6:-1].decode("utf8")
    return f


# Decode a 'beacon' frame
def _decode_beacon(frame):
    f = {}
    f['frame_type'] = frame[4]
    return f
