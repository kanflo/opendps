/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019
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

#ifndef __UUI_TIME_H__
#define __UUI_TIME_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tft.h"
#include "uui.h"


// HHH:MM:SS == 7
#define DIGITS 7 

// 999:59:59 == 3600000-1 seconds
#define MAX_TIME 3600000-1

/**
 * A UI item describing an editable time value formatted as HH:MM:SS
 */
typedef struct ui_time_t {
    ui_item_t ui;
    uint16_t color;
    tft_font_size_t font_size;
    ui_text_alignment_t alignment;
    bool pad_dot; /** Make the '.' character the same width as the digits? */
    uint8_t cur_digit;
    int32_t value;
    void (*changed)(struct ui_time_t *item);
} ui_time_t;

/**
 * @brief      Initialize number UI item
 *
 * @param      item  The item
 */
void time_init(ui_time_t *item);

#endif // __UUI_NUMBER_H__
