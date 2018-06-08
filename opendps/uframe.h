/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Johan Kanflo (github.com/kanflo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __UFRAME_H__
#define __UFRAME_H__

#include "crc16.h"
#include "dbg_printf.h"

#define _SOF 0x7e
#define _DLE 0x7d
#define _XOR 0x20
#define _EOF 0x7f

/** Errors returned by uframe_unescape(...) */
#define E_LEN 1 // Received frame is too short to be a uframe
#define E_FRM 2 // Received data has no framing
#define E_CRC 3 // CRC mismatch

/**  Max size given a payload of 'size' bytes
  * (SOF + every byte of payload escaped + 2x escaped crc bytes + EOF)
*/
#define FRAME_OVERHEAD(size) (1 + 2*(size) + 4 + 1)


/** Declare a frame of length 'length' for dumping data into */
#define DECLARE_FRAME(length) \
    uint8_t _buffer[ FRAME_OVERHEAD(length) ]; \
    _buffer[0] = _SOF; \
    uint32_t _length = 1; \
    uint16_t _crc = 0;

/** Pack a byte with stuffing and crc updating */
#define PACK8(b) \
    if (_length < sizeof(_buffer)) { \
        _crc = crc16_add(_crc, b); \
        uint8_t _byte = (b); \
        if (_byte == _SOF || _byte == _DLE || _byte == _EOF) { \
            _buffer[_length++] = _DLE; \
            _buffer[_length++] = _byte ^ _XOR; \
        } else { \
            _buffer[_length++] = _byte; \
        } \
    } else { \
        dbg_printf("uFrame overflow at %s:%d\n", __FILE__, __LINE__); \
    }

/** Pack a c string with null terminator */
#define PACK_CSTR(s) \
{ \
    uint8_t *_tmp = (uint8_t*) (s); \
    while(*_tmp) { \
        PACK8(*_tmp); \
        _tmp++; \
    } \
    PACK8(0); \
}

#define PACK16(h) \
    PACK8((h) >> 8); \
    PACK8((h) & 0xff);

#define PACK32(w) \
    PACK8((w) >> 24); \
    PACK8(((w) >> 16) & 0xff); \
    PACK8(((w) >> 8) & 0xff); \
    PACK8((w) & 0xff);

/** Like PACK8 but does not compute crc */
#define STUFF8(b) \
    if (_length+2 < sizeof(_buffer)) { \
        uint8_t _byte = (b); \
        if (_byte == _SOF || _byte == _DLE || _byte == _EOF) { \
            _buffer[_length++] = _DLE; \
            _buffer[_length++] = _byte ^ _XOR; \
        } else { \
            _buffer[_length++] = _byte; \
        } \
    } else { \
        dbg_printf("uFrame overflow at %s:%d\n", __FILE__, __LINE__); \
    }

/** Finish frame. Store crc and EOF. You now have the frame in _buffer of length _length */
#define FINISH_FRAME() \
    STUFF8((_crc >> 8) & 0xff); \
    STUFF8(_crc & 0xff); \
    if (_length < sizeof(_buffer)) { \
        _buffer[_length++] = _EOF; \
    }

/** Declare an unpacker for the frame of given length */
#define DECLARE_UNPACK(payload, length) \
    uint8_t *_buffer = (uint8_t*) payload; \
    uint32_t _pos = 0; \
    uint32_t _remain = length;

#define UNPACK8(b) \
    if (_remain >= 1) { \
        (b) = _buffer[_pos++]; \
        _remain--; \
    } else { \
        (b) = 0; \
    }

#define UNPACK16(h) \
    if (_remain >= 2) { \
        (h)  = _buffer[_pos++] << 8; \
        (h) |= _buffer[_pos++]; \
        _remain -= 2; \
    } else { \
        (h) = 0; \
    }

#define UNPACK32(h) \
    if (_remain >= 4) { \
        (h)  = _buffer[_pos++] << 24; \
        (h) |= _buffer[_pos++] << 16; \
        (h) |= _buffer[_pos++] << 8; \
        (h) |= _buffer[_pos++]; \
        _remain -= 4; \
    } else { \
        (h) = 0; \
    }

/**
  * @brief Extract payload from frame following deframing, unescaping and
  *       crc checking.
  * @note The frame is processed 'in place', when exiting the payload will be
  *       the first byte of 'frame'. If the frame contains two valid frames,
  *       -E_CRC will be returned.
  * @param frame the frame to process, payload on exit
  * @param length length of frame
  * @retval length of payload or -E_* in case of errors (see uframe.h)
  */
int32_t uframe_extract_payload(uint8_t *frame, uint32_t length);

#endif // __UFRAME_H__
