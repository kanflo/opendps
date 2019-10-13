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
#include "past.h"

#ifdef CONFIG_UI_MAX_SCREENS
 #define MAX_SCREENS (CONFIG_UI_MAX_SCREENS)
#else // CONFIG_UI_MAX_SCREENS
 #define MAX_SCREENS (6)
#endif // CONFIG_UI_MAX_SCREENS

#ifdef CONFIG_UI_MAX_PARAMETERS
 #define MAX_PARAMETERS (CONFIG_UI_MAX_PARAMETERS)
#else // CONFIG_UI_MAX_PARAMETERS
 #define MAX_PARAMETERS (6)
#endif // CONFIG_UI_MAX_PARAMETERS

#ifdef CONFIG_UI_MAX_PARAMETER_NAME
 #define MAX_PARAMETER_NAME (CONFIG_UI_MAX_PARAMETER_NAME)
#else // CONFIG_UI_MAX_PARAMETER_NAME
 #define MAX_PARAMETER_NAME (10)
#endif // CONFIG_UI_MAX_PARAMETER_NAME

/** The position for the screen icon on the status bar */ 
#define XPOS_ICON    (43)   


/**
 * Describes units in the user interface
 * Please keep in sync with dpsctl/dpsctl.py: def unit_name(unit)
 */
typedef enum {
    unit_none = 0,
    unit_ampere,
    unit_volt,
    unit_watt,
    unit_watthour,
    unit_second,
    unit_hertz,
    unit_percent,
    unit_furlong,
    unit_last = 0xff
} unit_t;

/**
 * Describes SI prefixes to units
 */
typedef enum {
    si_micro = -6,
    si_milli = -3,
    si_centi = -2,
    si_deci = -1,
    si_none = 0,
    si_deca = 1,
    si_hecto = 2,
    si_kilo = 3,
    si_mega = 4,
} si_prefix_t;

/**
 * UI item types
 */
typedef enum {
    ui_item_number, /** A control for setting a value (ui_number_t) */
    ui_item_icon, /** A control for showing a icon (ui_icon_t) */
    ui_item_time, /** A control for showing time (ui_time_t) */
    ui_item_last = 0xff
} ui_item_type_t;

/**
 * UI text alignment
 */
typedef enum {
    ui_text_left_aligned,
    ui_text_right_aligned
} ui_text_alignment_t;

/**
 * Return values to set_parameter
 */
typedef enum {
    ps_ok = 0,
    ps_unknown_name,
    ps_range_error,
    ps_not_supported,
    ps_flash_error,
} set_param_status_t;

/**
 * Base class for a parameter
 */
typedef struct ui_parameter_t {
    char name[MAX_PARAMETER_NAME];
    unit_t unit;
    si_prefix_t prefix;
} ui_parameter_t;

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
 * A UI consists of several screens
 */
typedef struct {
    uint8_t num_screens;
    uint8_t cur_screen;
    bool is_visible;
    ui_screen_t *screens[MAX_SCREENS];
    past_t *past;
} uui_t;

/**
 * A screen has a name and holds num_items UI items
 */
struct ui_screen {
    uint8_t id; /** must be unique */
    char *name;
    uint8_t *icon_data;
    uint32_t icon_data_len;
    uint32_t icon_width;
    uint32_t icon_height;
    bool is_enabled;
    uint8_t num_items;
    uint8_t cur_item;
    ui_parameter_t parameters[MAX_PARAMETERS];
    void (*activated)(void); /** Called when the screen is switched to */
    void (*deactivated)(void); /** Called when the screen is about to be changed from */
    void (*enable)(bool _enable); /** Called when the enable button is pressed */
    bool (*event)(uui_t *ui, event_t event, uint8_t data); /** Called when an event occurs (eg. button press). Return false if unhandled so main UI can handle it */
    void (*tick)(void); /** Called periodically allowing the UI to do house keeping */
    void (*past_save)(past_t *past);
    void (*past_restore)(past_t *past);
    set_param_status_t (*set_parameter)(char *name, char *value);
    set_param_status_t (*get_parameter)(char *name, char *value, uint32_t value_len);
    ui_item_t *items[];
};

/**
 * @brief      Initialize the UUI instance
 *
 * @param      ui    The user interface
 */
void uui_init(uui_t *ui, past_t *past);

/**
 * @brief      Refresh all items on current screen in need of redrawing
 *
 * @param      ui      The UI
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
void uui_handle_screen_event(uui_t *ui, event_t event, uint8_t data);

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
 * @brief      Switch to  screen index
 *
 * @param      ui    The user interface
 */
void uui_set_screen(uui_t *ui, uint32_t screen_idx);

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

/**
 * @brief      Show or hide UUI
 *
 * @param      ui    The user interface
 * @param      show  true for show, false for hide
 */
void uui_show(uui_t *ui, bool show);

/**
 * @brief      Disable current screen of user interface
 *
 * @param      ui    The user interface
 */
void uui_disable_cur_screen(uui_t *ui);

/**
 * @brief      Focus on a given user interface item
 *
 * @param      ui    The user interface
 * @param      item  The user interface item to focus on
 */
void uui_focus(uui_t *ui, ui_item_t *item);


#endif // __UUI_H__
