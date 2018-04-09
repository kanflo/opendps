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

#ifndef __UUI_H__
#define __UUI_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "event.h"
#include "tick.h"
#include "pwrctl.h"

#ifdef CONFIG_UI_MAX_SCREENS
 #define MAX_SCREENS (CONFIG_UI_MAX_SCREENS)
#else // CONFIG_UI_MAX_SCREENS
 #define MAX_SCREENS (5)
#endif // CONFIG_UI_MAX_SCREENS

/**
 * Describes units in the user interface
 */
typedef enum {
    unit_ampere,
    unit_volt,
    unit_watts,
    unit_seconds,
    unit_last = 0xff
} unit_t;

/**
 * UI item types
 */
typedef enum {
    ui_item_number, /** A control for setting a value (ui_number_t) */
    ui_item_last = 0xff
} ui_item_type_t;

/**
 * Base class for a UI item
 */

typedef struct ui_screen ui_screen_t;

typedef struct ui_item_t {
    uint8_t id;
    ui_item_type_t type;
    bool can_focus; /** A focusable item is one we can edit */
    bool has_focus;
    bool needs_redraw;
    uint16_t x, y;
    //uint16_t width, height;
    ui_screen_t *screen;

    void (*got_focus)(struct ui_item_t *item);
    void (*lost_focus)(struct ui_item_t *item);
    void (*got_event)(struct ui_item_t *item, event_t event);
    uint32_t (*get_value)(struct ui_item_t *item);
    void (*draw)(struct ui_item_t *item);
} ui_item_t;

/**
 * @brief      A macro used to call operations on UI elements
 */
#define MCALL(item, operation, ...) ((ui_item_t*) (item))->operation((ui_item_t*) item, ##__VA_ARGS__)

/**
 * A screen has a name and holds num_items UI items
 */
struct ui_screen {
    char *name;
    bool is_enabled;
    uint8_t num_items;
    uint8_t cur_item;
    void (*enable)(bool _enable); /** Called when the enable button is pressed */
    void (*tick)(void); /** Called periodically allowing the UI to do house keeping */
//    void (*update)(uint32_t v_in, uint32_t v_out, uint32_t i_out);
    ui_item_t *items[];
};

/**
 * A UI consists of several screens
 */
typedef struct {
    uint8_t num_screens;
    uint8_t cur_screen;
    ui_screen_t *screens[MAX_SCREENS];
} uui_t;

/**
 * @brief      Initialize the UUI instance
 *
 * @param      ui    The user interface
 */
void uui_init(uui_t *ui);

/**
 * @brief      Refresh all items on current screen in need of redrawing
 *
 * @param      screen  The UI
 * @param      force   If true, all items will be updated
 */
void uui_refresh(uui_t *ui, bool force);

/**
 * @brief      Activate current screen
 *
 * @param      ui    The user interface
 */
void uui_activate(uui_t *ui);

/**
 * @brief      Add screen to UI
 *
 * @param      ui      The user interface
 * @param      screen  The screen
 */
void uui_add_screen(uui_t *ui, ui_screen_t *screen);

/**
 * @brief      Process screen event
 *
 * @param      ui     The user interface
 * @param[in]  event  The event
 */
void uui_handle_screen_event(uui_t *ui, event_t event);

/**
 * @brief      Switch to next screen
 *
 * @param      ui    The user interface
 */
void uui_next_screen(uui_t *ui);

/**
 * @brief      Switch to previous screen
 *
 * @param      ui    The user interface
 */
void uui_prev_screen(uui_t *ui);

/**
 * @brief      Initialize UI item
 *
 * @param      item  The item
 */
void ui_item_init(ui_item_t *item);

/**
 * @brief      UI tick handler
 *
 * @param      ui    The user interface
 */
void uui_tick(uui_t *ui);


#endif // __UUI_H__
