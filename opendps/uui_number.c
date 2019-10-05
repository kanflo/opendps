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
#include "ili9163c.h"
#include "font-full_small.h"
#include "font-meter_small.h"
#include "font-meter_medium.h"
#include "font-meter_large.h"

#define MAX(a,b) (((a)>(b))?(a):(b))

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
            uint32_t diff = my_pow(10, (item->si_prefix * -1) - item->num_decimals + item->cur_digit);
            item->value -= diff;
            if (item->value < item->min) {
                item->value = item->min;
            }
            _item->needs_redraw = true;
            value_changed = true;
            break;
        }
        case event_rot_right: {
            uint32_t diff = my_pow(10, (item->si_prefix * -1) - item->num_decimals + item->cur_digit);
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
 * @brief      Get the width of the number if it were to be drawn
 *
 * @param      _item  The item
 *
 * @return     The width in pixels
 */
static uint32_t number_draw_width(ui_item_t *_item)
{
    ui_number_t *item = (ui_number_t*) _item;
    uint32_t digit_w, max_w, dot_width, spacing;
    uint32_t total_width = 0;

    switch (item->font_size) {
      case FONT_FULL_SMALL:
        digit_w = FONT_FULL_SMALL_MAX_DIGIT_WIDTH;
        max_w = FONT_FULL_SMALL_MAX_GLYPH_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_FULL_SMALL_MAX_DIGIT_WIDTH, FONT_FULL_SMALL_DOT_WIDTH) : FONT_FULL_SMALL_DOT_WIDTH;
        spacing = FONT_FULL_SMALL_SPACING;
        break;
      case FONT_METER_SMALL:
        digit_w = FONT_METER_SMALL_MAX_DIGIT_WIDTH;
        max_w = FONT_METER_SMALL_MAX_GLYPH_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_METER_SMALL_MAX_DIGIT_WIDTH, FONT_METER_SMALL_DOT_WIDTH) : FONT_METER_SMALL_DOT_WIDTH;
        spacing = FONT_METER_SMALL_SPACING;
        break;
      case FONT_METER_MEDIUM:
        digit_w = FONT_METER_MEDIUM_MAX_DIGIT_WIDTH;
        max_w = FONT_METER_MEDIUM_MAX_GLYPH_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_METER_MEDIUM_MAX_DIGIT_WIDTH, FONT_METER_MEDIUM_DOT_WIDTH) : FONT_METER_MEDIUM_DOT_WIDTH;
        spacing = FONT_METER_MEDIUM_SPACING;
        break;
      case FONT_METER_LARGE:
        digit_w = FONT_METER_LARGE_MAX_DIGIT_WIDTH;
        max_w = FONT_METER_LARGE_MAX_GLYPH_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_METER_LARGE_MAX_DIGIT_WIDTH, FONT_METER_LARGE_DOT_WIDTH) : FONT_METER_LARGE_DOT_WIDTH;
        spacing = FONT_METER_LARGE_SPACING;
        break;
      default:
        /* Can't do anything if the wrong font size was supplied. */
        assert(0);
    }

    /** Start printing from left to right */

    /** Digits before the decimal point */
    for (uint32_t i = item->num_digits - 1; i < item->num_digits; --i) {
        total_width += digit_w + spacing;
    }

    /** The decimal point if there is decimal places */
    if (item->num_decimals)
    {
        total_width += dot_width + spacing;
    }

    /** Digits after the decimal point */
    for (uint32_t i = 0; i < item->num_decimals; ++i) {
        total_width += digit_w + spacing;
    }

    /** The unit */
    switch(item->unit) {
        case unit_none:
            break;
        case unit_volt:
        case unit_ampere:
        case unit_watt:
            total_width += max_w;
            break;
        case unit_percent:
            total_width += FONT_FULL_SMALL_MAX_GLYPH_WIDTH;
            break;
        case unit_hertz:
            total_width += 2*FONT_FULL_SMALL_MAX_GLYPH_WIDTH;
            break;
        case unit_watthour:
            total_width += 3*FONT_FULL_SMALL_MAX_GLYPH_WIDTH;
            break;
        default:
            assert(0);
    }

    return total_width;
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

static void number_draw(ui_item_t *_item)
{
    ui_number_t *item = (ui_number_t*) _item;
    uint32_t digit_w, max_w, h, dot_width, spacing;

    switch (item->font_size) {
      case FONT_FULL_SMALL:
        digit_w = FONT_FULL_SMALL_MAX_DIGIT_WIDTH;
        max_w = FONT_FULL_SMALL_MAX_GLYPH_WIDTH;
        h = FONT_FULL_SMALL_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_FULL_SMALL_MAX_DIGIT_WIDTH, FONT_FULL_SMALL_DOT_WIDTH) : FONT_FULL_SMALL_DOT_WIDTH;
        spacing = FONT_FULL_SMALL_SPACING;
        break;
      case FONT_METER_SMALL:
        digit_w = FONT_METER_SMALL_MAX_DIGIT_WIDTH;
        max_w = FONT_METER_SMALL_MAX_GLYPH_WIDTH;
        h = FONT_METER_SMALL_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_METER_SMALL_MAX_DIGIT_WIDTH, FONT_METER_SMALL_DOT_WIDTH) : FONT_METER_SMALL_DOT_WIDTH;
        spacing = FONT_METER_SMALL_SPACING;
        break;
      case FONT_METER_MEDIUM:
        digit_w = FONT_METER_MEDIUM_MAX_DIGIT_WIDTH;
        max_w = FONT_METER_MEDIUM_MAX_GLYPH_WIDTH;
        h = FONT_METER_MEDIUM_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_METER_MEDIUM_MAX_DIGIT_WIDTH, FONT_METER_MEDIUM_DOT_WIDTH) : FONT_METER_MEDIUM_DOT_WIDTH;
        spacing = FONT_METER_MEDIUM_SPACING;
        break;
      case FONT_METER_LARGE:
        digit_w = FONT_METER_LARGE_MAX_DIGIT_WIDTH;
        max_w = FONT_METER_LARGE_MAX_GLYPH_WIDTH;
        h = FONT_METER_LARGE_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_METER_LARGE_MAX_DIGIT_WIDTH, FONT_METER_LARGE_DOT_WIDTH) : FONT_METER_LARGE_DOT_WIDTH;
        spacing = FONT_METER_LARGE_SPACING;
        break;
      default:
        /* Can't do anything if the wrong font size was supplied. Drop out for safety. */
        return;
    }

    uint32_t xpos = _item->x;
    uint16_t color = item->color;
    uint32_t cur_digit = item->num_digits + item->num_decimals - 1; /** Which digit are we currently drawing? 0 is the right most digit */

    /** Adjust drawing position if right aligned */
    if (item->alignment == ui_text_right_aligned)
        xpos -= number_draw_width(_item);

    /** Start printing from left to right */

    /** Digits before the decimal point */
    for (uint32_t i = item->num_digits - 1; i < item->num_digits; --i) {
        bool highlight = _item->has_focus && item->cur_digit == cur_digit;
        uint8_t digit = item->value / my_pow(10, (item->si_prefix * -1) + i) % 10;

        if ( ! digit /** If current digit is a 0 */
            && !_item->has_focus /** and its not in focus (selected) */
            && cur_digit != item->num_decimals /** to prevent 0.123 becoming .123 */
            && my_pow(10, cur_digit + (item->si_prefix * -1)) > (uint32_t) item->value) /** to prevent 4023 becoming 4 23 */
        {
            /** To prevent from printing 00.123 */
            /** Black out any 0 that has previously been printed */
            if (spacing > 1)
            {
                tft_fill(xpos, _item->y, digit_w, h, BLACK);
                tft_rect(xpos-1, _item->y-1, digit_w+1, h+1, BLACK); /** Remove any potential frame highlight */
            }
            else
                tft_fill(xpos, _item->y, digit_w, h, BLACK); /** Black out any 0 that has previously been printed */
        }
        else
        {

            if (spacing > 1) /** Dont frame tiny fonts */
            {
                if (highlight) /** Draw an extra pixel wide border around the highlighted item */
                    tft_rect(xpos-1, _item->y-1, digit_w+1, h+1, WHITE);
                else
                    tft_rect(xpos-1, _item->y-1, digit_w+1, h+1, BLACK);
            }
            tft_putch(item->font_size, '0' + digit, xpos, _item->y, digit_w, h, color, highlight);
        }

        cur_digit--;
        xpos += digit_w + spacing;
    }

    /** Draw the decimal point if there is decimal places */
    if (item->num_decimals)
    {
        tft_putch(item->font_size, '.', xpos, _item->y, dot_width, h, color, false);
        xpos += dot_width + spacing;
    }

    /** Digits after the decimal point */
    for (uint32_t i = 0; i < item->num_decimals; ++i) {
        bool highlight = _item->has_focus && item->cur_digit == cur_digit;
        uint8_t digit = item->value / my_pow(10, (item->si_prefix * -1) -1 - i) % 10;
        if (spacing > 1) /** Dont frame tiny fonts */
        {
            if (highlight) /** Draw an extra pixel wide border around the highlighted item */
                tft_rect(xpos-1, _item->y-1, digit_w+1, h+1, WHITE);
            else
                tft_rect(xpos-1, _item->y-1, digit_w+1, h+1, BLACK);
        }
        tft_putch(item->font_size, '0' + digit, xpos, _item->y, digit_w, h, color, highlight);
        cur_digit--;
        xpos += digit_w + spacing;
    }

    /** The unit */
    switch(item->unit) {
        case unit_none:
            break;
        case unit_volt:
            tft_putch(item->font_size, 'V', xpos, _item->y, max_w, h, color, false);
            break;
        case unit_ampere:
            tft_putch(item->font_size, 'A', xpos, _item->y, max_w, h, color, false);
            break;
        case unit_hertz:
            tft_puts(FONT_FULL_SMALL, "Hz", xpos, _item->y + h, FONT_FULL_SMALL_MAX_GLYPH_WIDTH * 2, FONT_FULL_SMALL_MAX_GLYPH_HEIGHT, color, false);
            break;
        case unit_watthour:
            // units here are milliwatt hours
            tft_puts(FONT_FULL_SMALL, "mWh", xpos, _item->y + h, FONT_FULL_SMALL_MAX_GLYPH_WIDTH * 3, FONT_FULL_SMALL_MAX_GLYPH_HEIGHT, color, false);
            break;
        case unit_watt:
            tft_putch(item->font_size, 'W', xpos, _item->y, max_w, h, color, false);
            break;
        case unit_percent:
            tft_puts(FONT_FULL_SMALL, "%", xpos, _item->y + h, FONT_FULL_SMALL_MAX_GLYPH_WIDTH * 2, FONT_FULL_SMALL_MAX_GLYPH_HEIGHT, color, false);
            break;
        default:
            assert(0);
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
    item->cur_digit = item->num_decimals; // edit least significant digit first
    item->ui.needs_redraw = true;
}
