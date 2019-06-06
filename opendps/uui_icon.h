/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Cyril Russo (github.com/X-Ryl669)
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

#ifndef __UUI_ICON_H__
#define __UUI_ICON_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tft.h"
#include "uui.h"

/**
 * A UI item describing an icon matching a number (the icon type)
 * The number lies in [0 num_icons] 
 */
typedef struct ui_icon_t {
    ui_item_t ui;
    uint16_t color;
    uint32_t icons_data_len;
    uint32_t icons_width;
    uint32_t icons_height;
    uint32_t value;
    uint32_t num_icons;
    void (*changed)(struct ui_icon_t *item);
    const uint8_t *icons[];
} ui_icon_t;

/**
 * @brief      Initialize icon UI item
 *
 * @param      item  The item
 */
void icon_init(ui_icon_t *item);

#endif // __UUI_ICON_H__
