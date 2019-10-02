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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uframe.h"
#include "crc16.h"

void set_frame_header(frame_t *frame)
{
    frame->buffer[0] = _SOF;
    frame->length = 1;
    frame->crc = 0;
    frame->unpack_pos = 0;
}

void start_frame_unpacking(frame_t *frame)
{
    frame->unpack_pos = 0;
}

/** Pack a byte with stuffing and crc updating */
void pack8(frame_t *frame, uint8_t data)
{
    if (data == _SOF || data == _DLE || data == _EOF) {
        if (frame->length + 2 >= MAX_FRAME_LENGTH)
            goto overflow;

        frame->buffer[frame->length++] = _DLE;
        frame->buffer[frame->length++] = data ^ _XOR;
    } else {
        if (frame->length + 1 >= MAX_FRAME_LENGTH)
            goto overflow;

        frame->buffer[frame->length++] = data;
    }

    frame->crc = crc16_add(frame->crc, data);

    return;

overflow:
    dbg_printf("uFrame overflow at %s:%d\n", __FILE__, __LINE__);
}

/** Like pack8 but does not compute crc */
void stuff8(frame_t *frame, uint8_t data)
{
    if (data == _SOF || data == _DLE || data == _EOF) {
        if (frame->length + 2 >= MAX_FRAME_LENGTH)
            goto overflow;

        frame->buffer[frame->length++] = _DLE;
        frame->buffer[frame->length++] = data ^ _XOR;
    } else {
        if (frame->length >= MAX_FRAME_LENGTH)
            goto overflow;

        frame->buffer[frame->length++] = data;
    }

    return;

overflow:
    dbg_printf("uFrame overflow at %s:%d\n", __FILE__, __LINE__);
}

void pack16(frame_t *frame, uint16_t data)
{
    pack8(frame, data >> 8);
    pack8(frame, data & 0xFF);
}

void pack32(frame_t *frame, uint32_t data)
{
    pack8(frame, data >> 24);
    pack8(frame, (data >> 16) & 0xFF);
    pack8(frame, (data >> 8) & 0xFF);
    pack8(frame, data & 0xFF);
}

void pack_float(frame_t *frame, float data)
{
    union {
        float f;
        uint32_t u32;
    } overlay;

    overlay.f = data;
    pack32(frame, overlay.u32);
}

/** Pack a c string with null terminator */
void pack_cstr(frame_t *frame, const char *data)
{
    while(*data) {
        pack8(frame, *data);
        data++;
    }
    pack8(frame, '\0');
}

/** Finish frame. Store crc and EOF */
void end_frame(frame_t *frame)
{
    stuff8(frame, (frame->crc >> 8) & 0xff);
    stuff8(frame, frame->crc & 0xff);
    if (frame->length < MAX_FRAME_LENGTH) {
        frame->buffer[frame->length++] = _EOF;
    }
}

uint32_t unpack8(frame_t *frame, uint8_t *data)
{
    if (frame->length >= 1) {
        frame->length--;
        *data = frame->buffer[frame->unpack_pos++];
        return 1; 
    }

    dbg_printf("uFrame underflow at %s:%d\n", __FILE__, __LINE__);
    *data = 0;
    return 0;
}

uint32_t unpack16(frame_t *frame, uint16_t *data)
{
    uint32_t bytes_read;
    uint8_t u8;

    bytes_read = unpack8(frame, &u8);
    *data = ((uint16_t) u8) << 8;

    bytes_read += unpack8(frame, &u8);
    *data |= ((uint16_t) u8);

    return bytes_read;
}

uint32_t unpack32(frame_t *frame, uint32_t *data)
{
    uint32_t bytes_read;
    uint8_t u8;

    bytes_read = unpack8(frame, &u8);
    *data = ((uint32_t) u8) << 24;

    bytes_read += unpack8(frame, &u8);
    *data |= ((uint32_t) u8) << 16;

    bytes_read += unpack8(frame, &u8);
    *data |= ((uint32_t) u8) << 8;

    bytes_read += unpack8(frame, &u8);
    *data |= ((uint32_t) u8);

    return bytes_read;
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
int32_t uframe_extract_payload(frame_t *frame, uint8_t *data, uint32_t length)
{
    int32_t status = -1;
    bool seen_dle = false;
    uint16_t frame_crc, calc_crc = 0;

    frame->length = 0;

    do {
        if (length < 5) { // _SOF CRC16 _EOF is no usable frame
            status = -E_LEN;
            break;
        }
        if (data[0] != _SOF || data[length-1] != _EOF) {
            status = -E_FRM;
            break;
        }

        // First unescape the frame
        uint32_t w = 0;
        for (uint32_t r = 1; r < length-1; r++) {
            if (seen_dle) {
                seen_dle = false;
                data[w++] = data[r] ^_XOR;
            } else if (data[r] == _DLE) {
                seen_dle = true;
            } else {
                data[w++] = data[r];
            }
        }

        // Then calculate CRC
        calc_crc = crc16((uint8_t*) data, w - 2);

        frame_crc = (uint16_t) ((data[w-2] << 8) | data[w-1]);
        if (calc_crc == frame_crc) {
            status = (int32_t) w - 2; // omit crc from returned length
        } else {
            status = -E_CRC;
        }
    } while(0);

    if (status != -E_CRC)
    {
        memcpy(frame->buffer, data, status);
        frame->length = status;
    }

    return status;
}
