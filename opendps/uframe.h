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

#include "dbg_printf.h"

#define _SOF 0x7e
#define _DLE 0x7d
#define _XOR 0x20
#define _EOF 0x7f

/** Errors returned by uframe_unescape(...) */
#define E_LEN 1 // Received frame is too short to be a uframe
#define E_FRM 2 // Received data has no framing
#define E_CRC 3 // CRC mismatch

#define FRAME_OVERHEAD(size) (1 + 2*(size) + 4 + 1)
#define MAX_FRAME_LENGTH (128)

typedef struct 
{
    uint8_t buffer[MAX_FRAME_LENGTH];
    uint32_t length;
    uint16_t crc;
    uint32_t unpack_pos;
} frame_t;

void set_frame_header(frame_t *frame);
void end_frame(frame_t *frame);
void start_frame_unpacking(frame_t *frame);
void pack8(frame_t *frame, uint8_t data);
void stuff8(frame_t *frame, uint8_t data);
void pack16(frame_t *frame, uint16_t data);
void pack32(frame_t *frame, uint32_t data);
void pack_float(frame_t *frame, float data);
void pack_cstr(frame_t *frame, const char *data);
uint32_t unpack8(frame_t *frame, uint8_t *data);
uint32_t unpack16(frame_t *frame, uint16_t *data);
uint32_t unpack32(frame_t *frame, uint32_t *data);

/**
  * @brief Extract payload from frame following deframing, unescaping and
  *       crc checking.
  * @note The frame is processed 'in place', when exiting the payload will be
  *       the first byte of 'frame'. If the frame contains two valid frames,
  *       -E_CRC will be returned.
  * @param frame  the returned processed frame
  * @param data   the raw data to be processed
  * @param length length of frame
  * @retval length of payload or -E_* in case of errors (see uframe.h)
  */
int32_t uframe_extract_payload(frame_t *frame, uint8_t *data, uint32_t length);

#endif // __UFRAME_H__
