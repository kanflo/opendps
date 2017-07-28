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
#include "uframe.h"
#include "crc16.h"


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
int32_t uframe_extract_payload(uint8_t *frame, uint32_t length)
{
    int32_t status = -1;
    bool seen_dle = false;
    uint16_t frame_crc, calc_crc = 0;
    do {
        if (length < 5) { // _SOF CRC16 _EOF is no usable frame
            status = -E_LEN;
            break;
        }
        if (frame[0] != _SOF || frame[length-1] != _EOF) {
            status = -E_FRM;
            break;
        }

        // First unescape the frame
        uint32_t w = 0;
        for (uint32_t r = 1; r < length-1; r++) {
            if (seen_dle) {
                seen_dle = false;
                frame[w++] = frame[r] ^_XOR;
            } else if (frame[r] == _DLE) {
                seen_dle = true;
            } else {
                frame[w++] = frame[r];
            }
        }

        // Then calculate CRC
        calc_crc = crc16((uint8_t*) frame, w - 2);

        frame_crc = (uint16_t) ((frame[w-2] << 8) | frame[w-1]);
        if (calc_crc == frame_crc) {
            status = (int32_t) w - 2; // omit crc from returned length
        } else {
            status = -E_CRC;
        }
    } while(0);

    return status;
}
