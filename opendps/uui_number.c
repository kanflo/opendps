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

#include <stdint.h>
#include <stdbool.h>
#include "my_assert.h"
#include "uui_number.h"
#include "tft.h"
#include "font-18.h"
#include "font-24.h"
#include "font-48.h"

/** @todo: why is pow missing from my -lm ? */
static uint32_t my_pow(uint32_t a, uint32_t b)
{
    uint32_t r = 1;
    while(b--) {
        r *= a;
    }
    return r;
}

/**
 * @brief      Handle event and update our state and value accordingly
 *
 * @param      _item  The item
 * @param[in]  event  The event
 */
static void number_got_event(ui_item_t *_item, event_t event)
{
    assert(_item);
    ui_number_t *item = (ui_number_t*) _item;
    bool value_changed = false;
    switch(event) {
        case event_rot_left: {
            uint32_t diff = my_pow(10, item->cur_digit);
            item->value -= diff;
            if (item->value < item->min) {
                item->value = item->min;
            }
            _item->needs_redraw = true;
            value_changed = true;
            break;
        }
        case event_rot_right: {
            uint32_t diff = my_pow(10, item->cur_digit);
            item->value += diff;
            if (item->value > item->max) {
                item->value = item->max;
            }
            _item->needs_redraw = true;
            value_changed = true;
            break;
        }
        case event_rot_press:
            if (item->cur_digit == 0) {
                item->cur_digit = item->num_digits + item->num_decimals - 1;
            } else {
                item->cur_digit--;
            }
            _item->needs_redraw = true;
            break;
        default:
            assert(0);
    }
    if (value_changed && item->changed) {
        item->changed(item);
    }
}

/**
 * @brief      Getter of our value
 *
 * @param      _item  The item
 *
 * @return     value?
 */
static uint32_t number_get_value(ui_item_t *_item)
{
    assert(_item);
    ui_number_t *item = (ui_number_t*) _item;
    return item->value;
}

/**
 * @brief      Draw the number from right to left (unit first)
 *
 * @param      _item  The item
 */
static void number_draw(ui_item_t *_item)
{
    uint32_t cur_digit = 0;
    ui_number_t *item = (ui_number_t*) _item;
    uint32_t value = item->value;
    uint32_t w, h;
    switch (item->font_size) {
      case 18:
        w = font_18_widths[4]; /** @todo: Find the widest glyph */
        h = font_18_height;
        break;
      case 24:
        w = font_24_widths[4] + 2; /** @todo: Find the widest glyph */
        h = font_24_height + 2;
        break;
      case 48:
        w = font_48_widths[4] + 2; /** @todo: Find the widest glyph */
        h = font_48_height + 2;
        break;
    }
    uint16_t xpos = _item->x - w;
    switch(item->unit) {
        case unit_volt:
            tft_putch(item->font_size, 'V', xpos, _item->y, w, h, false);
            break;
        case unit_ampere:
            tft_putch(item->font_size, 'A', xpos, _item->y, w, h, false);
            break;
        default:
            assert(0);
    }
    xpos -= item->font_size == 18 ? 2 : 4;
    for (uint32_t i = 0; i < item->num_decimals; i++) {
        bool highlight = _item->has_focus && item->cur_digit == cur_digit;
        xpos -= w;
        tft_putch(item->font_size, '0'+(value%10), xpos, _item->y, w, h, highlight);
        value /= 10;
        cur_digit++;
    }
    xpos -= item->font_size == 18 ? 4 : 10;
    tft_putch(item->font_size, '.', xpos, _item->y, item->font_size == 0 ? 4 : 10, h, false);
    for (uint32_t i = 0; i < item->num_digits; i++) {
        bool highlight = _item->has_focus && item->cur_digit == cur_digit;
        xpos -= w;
        tft_putch(item->font_size, '0'+(value%10), xpos, _item->y, w, h, highlight);
        value /= 10;
        cur_digit++;
        if (!value && !_item->has_focus) { /** To prevent from printing 00.123 */
            xpos -= w;
            tft_fill(xpos, _item->y, w, h, 0);
            break;
        }
    }
}

/**
 * @brief      Initialize number item
 *
 * @param      item  The item
 */
void number_init(ui_number_t *item)
{
    assert(item);
    ui_item_init(&item->ui);
    item->ui.got_event = &number_got_event;
    item->ui.get_value = &number_get_value;
    item->ui.draw = &number_draw;
    item->cur_digit = item->num_digits + item->num_decimals - 1; /** Most signinficant digit */
    item->ui.needs_redraw = true;
}
