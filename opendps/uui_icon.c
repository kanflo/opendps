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

#include <stdint.h>
#include <stdbool.h>
#include "my_assert.h"
#include "uui_icon.h"
#include "tft.h"
#include "ili9163c.h" /* For WHITE/BLACK */

/**
 * @brief      Handle event and update our state and value accordingly
 *
 * @param      _item  The item
 * @param[in]  event  The event
 */
static void icon_got_event(ui_item_t *_item, event_t event)
{
    assert(_item);
    ui_icon_t *item = (ui_icon_t*) _item;
    bool value_changed = false;
    switch(event) {
        case event_rot_left: {
            if (item->value == 0) {
                item->value = item->num_icons - 1;
            } else {
                --item->value;
            }
            _item->needs_redraw = true;
            value_changed = true;
            break;
        }
        case event_rot_right: {
            if (item->value == item->num_icons - 1) {
                item->value = 0;
            } else {
                ++item->value;
            }
            _item->needs_redraw = true;
            value_changed = true;
            break;
        }
        case event_rot_press: break;
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
static uint32_t icon_get_value(ui_item_t *_item)
{
    assert(_item);
    ui_icon_t *item = (ui_icon_t*) _item;
    return item->value;
}

static void icon_draw(ui_item_t *_item)
{
    ui_icon_t *item = (ui_icon_t*) _item;
    assert(item->value < item->num_icons);
    /* Frame the icon */
    tft_rect(_item->x-1, _item->y-1, item->icons_width+2, item->icons_height+2, _item->has_focus ? WHITE : BLACK);
    tft_blit((uint16_t*) item->icons[item->value], item->icons_width, item->icons_height, _item->x, _item->y);
}

/**
 * @brief      Initialize number item
 *
 * @param      item  The item
 */
void icon_init(ui_icon_t *item)
{
    assert(item);
    ui_item_init(&item->ui);
    item->ui.got_event = &icon_got_event;
    item->ui.get_value = &icon_get_value;
    item->ui.draw      = &icon_draw;
    item->ui.needs_redraw = true;
}
