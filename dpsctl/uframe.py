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

_SOF = 0x7e
_DLE = 0x7d
_XOR = 0x20
_EOF = 0x7f

# Errors returned by uframe_unescape(...)
E_LEN = 1 # Received frame is too short to be a uframe
E_FRM = 2 # Received data has no framing
E_CRC = 3 # CRC mismatch

# https://stackoverflow.com/a/30357446
def crc16_ccitt(crc, data):
    msb = crc >> 8
    lsb = crc & 255
    x = data ^ msb
    x ^= (x >> 4)
    msb = (lsb ^ (x >> 3) ^ (x << 4)) & 255
    lsb = (x ^ (x << 5)) & 255
    return (msb << 8) + lsb

"""
Describes a class for simple serial protocols
"""
class uFrame(object):
    _valid = False
    _crc = 0
    _frame = None
    _unpack_pos = 0

    def __init__(self):
        self._frame = bytearray()
        self._frame.append(_SOF)
        self._crc = 0

    """
    Pack a byte into the frame, update CRC
    """
    def pack8(self, byte, update_crc = True):
        byte &= 0xff
        if update_crc:
            self._crc = crc16_ccitt(self._crc, byte)
        if byte in [_SOF, _DLE, _EOF]:
            self._frame.append(_DLE)
            self._frame.append(byte ^ _XOR)
        else:
            self._frame.append(byte)

    def pack_cstr(self, str):
        for ch in str:
            self.pack8(ord(ch))
        self.pack8(0)

    def pack16(self, halfword):
        halfword &= 0xffff
        h1 = (halfword >> 8) & 0xff
        h2 = halfword & 0xff
        self.pack8(h1)
        self.pack8(h2)

    def pack32(self, word):
        word &= 0xffffffff
        self.pack8((word >> 24) & 0xff)
        self.pack8((word >> 16) & 0xff)
        self.pack8((word >>  8) & 0xff)
        self.pack8((word >>  0) & 0xff)

    """
    End packing
    """
    def end(self):
        crc1 = (self._crc >> 8) & 0xff
        crc2 = self._crc & 0xff
        self.pack8(crc1, False)
        self.pack8(crc2, False)
        self._frame.append(_EOF)
        self._valid = True

    """
    Return frame binary data
    """
    def get_frame(self):
        return self._frame

    """
    Set frame to given (escaped) frame, unescape, check crc and extract payload
    Return -E_* if error or 0 if frame is valid.
    """
    def set_frame(self, escaped_frame):
        self._frame = escaped_frame
        res = self._unescape()
        if res == 0:
            res = self._calc_crc()
        return res

    """
    Return a string describing the data in the frame
    """
    def frame_str(self):
        return ' '.join(format(x, '02x') for x in self._frame)

    """
    Unsecape frame data (internal function)
    """
    def _unescape(self):
        length = len(self._frame)
        if length < 4:
            return -E_LEN
        if self._frame[0] != _SOF or self._frame[length-1] != _EOF:
            return -E_FRM
        f = bytearray()
        seen_dle = False
        for b in self._frame:
            if b == _DLE:
                seen_dle = True
            elif seen_dle:
                f.append(b ^ _XOR)
                seen_dle = False
            else:
                f.append(b)
        self._frame = f[1:-1]
        return 0

    """
    Check crc of frame data and chop crc off payload if valid (internal function)
    """
    def _calc_crc(self):
        self._crc = 0
        for b in self._frame[:-2]:
            self._crc = crc16_ccitt(self._crc, b)
        self._crc &= 0xffff
        crc = (self._frame[-2] << 8) | self._frame[-1]
        self._valid = crc == self._crc
        if not self._valid:
            return -E_CRC
        else:
            self._frame = self._frame[:-2] # Chop of crc
            return 0

    def unpack8(self):
        b = self._frame[self._unpack_pos]
        self._unpack_pos += 1
        return b

    # Unpack signed 8 bit
    def unpacks8(self):
        b = self._frame[self._unpack_pos]
        self._unpack_pos += 1
        return b - 256

    def unpack16(self):
        h = self.unpack8() << 8 | self.unpack8()
        return h

    def unpack32(self):
        h = self.unpack8() << 24 | self.unpack8() << 16 | self.unpack8() << 8 | self.unpack8()
        return h

    def unpack_cstr(self):
        string = ""
        if self._unpack_pos < len(self._frame):
            b = self._frame[self._unpack_pos]
            self._unpack_pos += 1
            while self._unpack_pos < len(self._frame) and b != 0:
                string += '%c' % b
                b = self._frame[self._unpack_pos]
                self._unpack_pos += 1
        return string

    def eof(self):
        return self._unpack_pos >= len(self._frame) 

