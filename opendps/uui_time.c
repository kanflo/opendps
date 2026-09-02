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

#include <stdint.h>
#include <stdbool.h>
#include "my_assert.h"
#include "uui_time.h"
#include "tft.h"
#include "ili9163c.h"
#include "font-full_small.h"
#include "font-meter_small.h"
#include "font-meter_medium.h"
#include "font-meter_large.h"

#define MAX(a,b) (((a)>(b))?(a):(b))

static uint32_t digit_scale(ui_time_t *item, uint32_t i) {
    switch (item->cur_digit) {
        case 0:
            // seconds
            return i;
        case 1:
            // 10s of seconds
            return 10 * i;
        case 2:
            // minutes
            return 60 * i;
        case 3:
            // 10s of minutes
            return 600 * i;
        case 4:
            // hours
            return 3600 * i;
        case 5:
            // tens of hours
            return 36000 * i;
        case 6:
        default:
            // hundreds of hours
            return 360000 * i;
    }
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

    ui_time_t *item = (ui_time_t*) _item;
    bool value_changed = false;

    switch(event) {
        case event_rot_left:
            item->value -= digit_scale(item, 1);
            if (item->value < 0) item->value = 0;

            _item->needs_redraw = true;
            value_changed = true;

            break;

        case event_rot_right:
            item->value += digit_scale(item, 1);
            if (item->value > MAX_TIME) item->value = MAX_TIME;

            _item->needs_redraw = true;
            value_changed = true;

            break;

        case event_rot_press:
            if (item->cur_digit == 0) {
                item->cur_digit = 5; // item->num_digits + item->num_decimals - 1;
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
    ui_time_t *item = (ui_time_t*) _item;
    uint32_t digit_w, dot_width, spacing;
    uint32_t total_width = 0;

    switch (item->font_size) {
      case FONT_FULL_SMALL:
        digit_w = FONT_FULL_SMALL_MAX_DIGIT_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_FULL_SMALL_MAX_DIGIT_WIDTH, FONT_FULL_SMALL_DOT_WIDTH) : FONT_FULL_SMALL_DOT_WIDTH;
        spacing = FONT_FULL_SMALL_SPACING;
        break;
      case FONT_METER_SMALL:
        digit_w = FONT_METER_SMALL_MAX_DIGIT_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_METER_SMALL_MAX_DIGIT_WIDTH, FONT_METER_SMALL_DOT_WIDTH) : FONT_METER_SMALL_DOT_WIDTH;
        spacing = FONT_METER_SMALL_SPACING;
        break;
      case FONT_METER_MEDIUM:
        digit_w = FONT_METER_MEDIUM_MAX_DIGIT_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_METER_MEDIUM_MAX_DIGIT_WIDTH, FONT_METER_MEDIUM_DOT_WIDTH) : FONT_METER_MEDIUM_DOT_WIDTH;
        spacing = FONT_METER_MEDIUM_SPACING;
        break;
      case FONT_METER_LARGE:
        digit_w = FONT_METER_LARGE_MAX_DIGIT_WIDTH;
        dot_width = item->pad_dot ? MAX(FONT_METER_LARGE_MAX_DIGIT_WIDTH, FONT_METER_LARGE_DOT_WIDTH) : FONT_METER_LARGE_DOT_WIDTH;
        spacing = FONT_METER_LARGE_SPACING;
        break;
      default:
        /* Can't do anything if the wrong font size was supplied. */
        assert(0);
    }

    /** Start printing from left to right */

    // total digits HHH:MM:SS = 7, plus spacing, plus 2 :
    total_width += DIGITS * (digit_w + spacing);
    total_width += 2 * (dot_width + spacing);

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
    ui_time_t *item = (ui_time_t*) _item;
    return item->value;
}

static void number_draw(ui_item_t *_item)
{
    ui_time_t *item = (ui_time_t*) _item;
    uint32_t digit_w, h, dot_width, spacing;

    switch (item->font_size) {
      case FONT_FULL_SMALL:
        digit_w = FONT_FULL_SMALL_MAX_DIGIT_WIDTH;
        h = FONT_FULL_SMALL_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_FULL_SMALL_MAX_DIGIT_WIDTH, FONT_FULL_SMALL_DOT_WIDTH) : FONT_FULL_SMALL_DOT_WIDTH;
        spacing = FONT_FULL_SMALL_SPACING;
        break;
      case FONT_METER_SMALL:
        digit_w = FONT_METER_SMALL_MAX_DIGIT_WIDTH;
        h = FONT_METER_SMALL_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_METER_SMALL_MAX_DIGIT_WIDTH, FONT_METER_SMALL_DOT_WIDTH) : FONT_METER_SMALL_DOT_WIDTH;
        spacing = FONT_METER_SMALL_SPACING;
        break;
      case FONT_METER_MEDIUM:
        digit_w = FONT_METER_MEDIUM_MAX_DIGIT_WIDTH;
        h = FONT_METER_MEDIUM_MAX_GLYPH_HEIGHT;
        dot_width = item->pad_dot ? MAX(FONT_METER_MEDIUM_MAX_DIGIT_WIDTH, FONT_METER_MEDIUM_DOT_WIDTH) : FONT_METER_MEDIUM_DOT_WIDTH;
        spacing = FONT_METER_MEDIUM_SPACING;
        break;
      case FONT_METER_LARGE:
        digit_w = FONT_METER_LARGE_MAX_DIGIT_WIDTH;
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
    uint32_t cur_digit = DIGITS - 1; // Start at  left most  digit of HH:MM:SS

    /** Adjust drawing position if right aligned */
    if (item->alignment == ui_text_right_aligned)
        xpos -= number_draw_width(_item);


    uint16_t hours = item->value / 3600;
    uint8_t minutes = (item->value / 60) % 60;
    uint8_t seconds = item->value % 60;
    uint8_t digit = 0;
    bool highlight = false;

    // HHH
    digit = hours / 100;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 1;

    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;

    digit = (hours % 100) / 10;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 2;

    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;

    digit = hours % 10;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 3;
    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;

    // :
    tft_putch(item->font_size, '.', xpos, _item->y - (h>>2) + (h >> 1), dot_width, 3, color, false);
    tft_putch(item->font_size, '.', xpos, _item->y - (h>>2) , dot_width, 3, color, false);
    xpos += dot_width + spacing;

    // MM
    digit = minutes / 10;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 4;
    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;

    digit = minutes % 10;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 5;
    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;

    // :
    tft_putch(item->font_size, '.', xpos, _item->y - (h>>2) + (h >> 1), dot_width, 3, color, false);
    tft_putch(item->font_size, '.', xpos, _item->y - (h>>2) , dot_width, 3, color, false);
    xpos += dot_width + spacing;

    // SS
    digit = seconds / 10;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 6;
    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;

    digit = seconds % 10;
    highlight =_item->has_focus && item->cur_digit == DIGITS - 7;
    tft_putch(item->font_size, '0' + digit, 
            xpos, _item->y,
            digit_w, h,
            color, highlight);
    cur_digit--;
    xpos += digit_w + spacing;


}

/**
 * @brief      Initialize time item
 *
 * @param      item  The item
 */
void time_init(ui_time_t *item)
{
    assert(item);
    ui_item_init(&item->ui);
    item->ui.got_event = &number_got_event;
    item->ui.get_value = &number_get_value;
    item->ui.draw = &number_draw;
    item->cur_digit = DIGITS - 3; // hour first
    item->ui.needs_redraw = true;
}



