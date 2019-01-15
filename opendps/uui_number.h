/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Johan Kanflo (github.com/kanflo)
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

#ifndef __UUI_NUMBER_H__
#define __UUI_NUMBER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tft.h"
#include "uui.h"

/**
 * A UI item describing an editable number formatted as <num_digits>.<num_decimals>
 * The number has a min and max value and cur_digit keeps track of which digit
 * is edited in the UI.
 * @todo: Add support for negative numbers
 */
typedef struct ui_number_t {
    ui_item_t ui;
    unit_t unit;
    uint16_t color;
    tft_font_size_t font_size;
    ui_text_alignment_t alignment;
    bool pad_dot; /** Make the '.' character the same width as the digits? */
    si_prefix_t si_prefix;
    uint8_t num_digits;
    uint8_t num_decimals;
    uint8_t cur_digit;
    int32_t value;
    int32_t min;
    int32_t max;
    void (*changed)(struct ui_number_t *item);
} ui_number_t;

/**
 * @brief      Initialize number UI item
 *
 * @param      item  The item
 */
void number_init(ui_number_t *item);

#endif // __UUI_NUMBER_H__
