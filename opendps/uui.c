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
#include "uui.h"
#include "tft.h"
#include "opendps.h"


/**
 * @brief      Callback for got focus because of M1/M2 presses
 *
 * @param      item  The ui item
 */
static void item_got_focus(ui_item_t *item)
{
    assert(item);
    assert(item->can_focus);
    item->has_focus = true;
    item->needs_redraw = true;
}

/**
 * @brief      Callback for lost focus because of M1/M2 presses
 *
 * @param      item  The ui item
 */
static void item_lost_focus(ui_item_t *item)
{
    assert(item);
    assert(item->can_focus);
    item->has_focus = false;
    item->needs_redraw = true;
}

void uui_init(uui_t *ui, past_t *past)
{
    assert(ui);
    ui->past = past;
    ui->num_screens = ui->cur_screen = 0;
    ui->is_visible = true;
}

void uui_add_screen(uui_t *ui, ui_screen_t *screen)
{
    assert(ui);
    assert(screen);
    if (screen->past_restore) {
        screen->past_restore(ui->past);
    }
    if (ui->num_screens < MAX_SCREENS) {
        ui->screens[ui->num_screens++] = screen;
        screen->cur_item = 0;
        screen->is_enabled = false;
        for (uint8_t i = 0; i < screen->num_items; i++) {
            screen->items[i]->screen = screen;
            screen->items[i]->needs_redraw = true;
        }
    }
}

void uui_refresh(uui_t *ui, bool force)
{
    assert(ui);
    ui_screen_t *screen = ui->screens[ui->cur_screen];
    assert(screen);
    for (uint8_t i = 0; i < screen->num_items; i++) {
        ui_item_t *item = screen->items[i];
        if (force || item->needs_redraw) {
            assert(item->draw);
            item->draw(item);
            item->needs_redraw = false;
        }
    }
    tft_blit((uint16_t*) screen->icon_data, screen->icon_width, screen->icon_height, XPOS_ICON, 128-screen->icon_height);
}

void uui_activate(uui_t *ui)
{
    assert(ui);
    assert(ui->num_screens);
    if (ui->num_screens > 0) {
        ui_screen_t *screen = ui->screens[ui->cur_screen];
        /** Find the first focusable item */
        for (uint32_t i = 0; i < screen->num_items; i++) {
            if (screen->items[i]->can_focus) {
                screen->cur_item = i;
                break;
            }
        }
        /** @todo: add activation callback for each screen allowing for updating of U/I settings */
        uui_refresh(ui, true);
        tft_blit((uint16_t*) screen->icon_data, screen->icon_width, screen->icon_height, XPOS_ICON, 128-screen->icon_height);
        if (screen->activated) {
            screen->activated();
        }
    }
}

static void focus_switch(ui_item_t *item)
{
    if (item->has_focus) {
        MCALL(item, lost_focus);
    } else {
        MCALL(item, got_focus);
    }
}

void uui_handle_screen_event(uui_t *ui, event_t event)
{
    assert(ui);
    ui_screen_t *screen = ui->screens[ui->cur_screen];
    assert(screen);
    ui_item_t *item = screen->items[screen->cur_item];
    assert(item);

    if (!ui->is_visible) {
        return;
    }

    switch(event) {
        case event_rot_left_set:
            uui_prev_screen(ui);
            break;

        case event_rot_right_set:
            uui_next_screen(ui);
            break;

        case event_rot_left:
        case event_rot_right:
        case event_rot_press:
            if (item->has_focus) {
                MCALL(item, got_event, event);
            }
            break;

        case event_button_sel:
            if (item->can_focus) {
                focus_switch(item);
            }
            break;

        case event_button_m1:
            if (item->has_focus) {
                ui_item_t *old_item = item;
                do {
                    screen->cur_item = screen->cur_item ? screen->cur_item - 1 : screen->num_items - 1;
                } while(!screen->items[screen->cur_item]->can_focus);
                ui_item_t *new_item = screen->items[screen->cur_item];
                if (old_item != new_item) {
                    focus_switch(old_item);
                    focus_switch(new_item);
                }
            }
            break;

        case event_button_m2:
            if (item->has_focus) {
                ui_item_t *old_item = item;
                do {
                    screen->cur_item = (screen->cur_item + 1) % screen->num_items;
                } while(!screen->items[screen->cur_item]->can_focus);
                ui_item_t *new_item = screen->items[screen->cur_item];
                if (old_item != new_item) {
                    focus_switch(old_item);
                    focus_switch(new_item);
                }
            }
            break;

        case event_button_enable:
        case event_ocp:
        case event_ovp:
            /** If current screen can be enabled */
            if (screen->enable) {
                screen->is_enabled = !screen->is_enabled;
                if (screen->is_enabled && screen->past_save) {
                    screen->past_save(ui->past);
                }
                screen->enable(screen->is_enabled);
                opendps_update_power_status(screen->is_enabled); /** @todo: move */
            }
            break;

        default:
            assert(0);
    }
}

void uui_next_screen(uui_t *ui)
{
    uint32_t new_screen = (ui->cur_screen + 1) % ui->num_screens;
    if (ui->num_screens > 1) {
        if (ui->screens[ui->cur_screen]->deactivated) {
            ui->screens[ui->cur_screen]->deactivated();
        }
        uui_set_screen(ui, new_screen);
    }
}

void uui_prev_screen(uui_t *ui)
{
    uint32_t new_screen = ui->cur_screen ? ui->cur_screen -1 : ui->num_screens - 1;
    if (ui->num_screens > 1) {
        if (ui->screens[ui->cur_screen]->deactivated) {
            ui->screens[ui->cur_screen]->deactivated();
        }
        uui_set_screen(ui, new_screen);
    }
}

void uui_set_screen(uui_t *ui, uint32_t screen_idx)
{
    assert(screen_idx < ui->num_screens);
    ui_screen_t *cur_screen = ui->screens[ui->cur_screen];
    assert(cur_screen);
    ui_item_t *item = cur_screen->items[cur_screen->cur_item];
    assert(item);
    ui->cur_screen = screen_idx;
    ui_screen_t *new_screen = ui->screens[ui->cur_screen];
    assert(new_screen);
    if (new_screen != cur_screen) {
//        cur_screen->enable(false); /** Alway disable current function when switching */
        opendps_update_power_status(false); /** @todo: move */
        if (cur_screen->is_enabled) {
            /** Disable the old screen as it will no longer be in control of power out */
            cur_screen->enable(false);
            cur_screen->is_enabled = false;
        }
        if (item->has_focus) {
            MCALL(item, lost_focus);
        }
        uui_activate(ui);
    }
}

void ui_item_init(ui_item_t *item)
{
    item->has_focus = false;
    item->got_focus = &item_got_focus;
    item->lost_focus = &item_lost_focus;
}

void uui_tick(uui_t *ui)
{
    ui->screens[ui->cur_screen]->tick();
}

void uui_show(uui_t *ui, bool show)
{
    ui->is_visible = show;
}

void uui_disable_cur_screen(uui_t *ui)
{
    ui_screen_t *screen = ui->screens[ui->cur_screen];
    if (screen->enable && screen->is_enabled) {
        screen->is_enabled = false;
        screen->enable(screen->is_enabled);
    }
}
