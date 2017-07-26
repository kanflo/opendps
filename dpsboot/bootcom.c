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

#include <stdio.h>
#include "bootcom.h"
#include "crc16.h"

/** If this magic is found at _bootcom_start, we have bootcom data */
#define BOOTCOM_MAGIC (0xfa1affe1)

/** Linker symbols */
extern uint32_t *_bootcom_start;
extern uint32_t *_bootcom_end;

/**
  * @brief Put data into bootcom buffer and set the bootcom magic
  * @param w1, w2 data to place into buffer
  * @retval void
  */
void bootcom_put(uint32_t w1, uint32_t w2)
{
    uint32_t *bootcom = (uint32_t*) &_bootcom_start;
    bootcom[0] = BOOTCOM_MAGIC;
    bootcom[1] = w1;
    bootcom[2] = w2;
    bootcom[3] = crc16((uint8_t*) bootcom, 12);
}

/**
  * @brief Get data from bootcom buffer
  * @param w1, w2 pointers to place bootcom data info
  * @retval true if bootcom data was found, false otherwise
  */
bool bootcom_get(uint32_t *w1, uint32_t *w2)
{
    bool success = false;
    uint32_t *bootcom = (uint32_t*) &_bootcom_start;
    if (w1 && w2 && bootcom[0] == BOOTCOM_MAGIC) {
        if (bootcom[3] == crc16((uint8_t*) bootcom, 12)) {
            *w1 = bootcom[1];
            *w2 = bootcom[2];
            bootcom[0] = bootcom[1] = bootcom[2] = bootcom[3] = 0;
            success = true;
        }
    }
	return success;
}

