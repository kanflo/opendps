/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Johan Kanflo (github.com/kanflo)
 * Modifications made by Leo Leung <leo@steamr.com>
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
#include <stdlib.h>
#include <string.h>
#include "gfx-iconpower.h"
#include "gfx-cvbar.h"
#include "gfx-ccbar.h"
#include "gfx-ppbar.h"
#include "gfx-oppbar.h"
#include "gfx-tmbar.h"
#include "gfx-m1bar.h"
#include "gfx-m2bar.h"
#include "font-meter_large.h"
#include "font-full_small.h"
#include "hw.h"
#include "func_dpsmode.h"
#include "event.h"
#include "uui.h"
#include "uui_number.h"
#include "uui_time.h"
#include "dbg_printf.h"
#include "mini-printf.h"
#include "dps-model.h"
#include "ili9163c.h"
#include "opendps.h"

/*
 * This is the implementation of the DPS look-alike screen. It has 3 editable
 * properties.
 *   Voltage limit (constant voltage)
 *   Current limit (constant current)
 *   Power limit (over power protection, 0 to disable)
 */

static void dpsmode_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void power_changed(ui_number_t *item);
static void watthour_changed(ui_number_t *item);
static void timer_changed(ui_time_t *item);
static void dpsmode_tick(void);
static void activated(void);
static void deactivated(void);
static bool event(uui_t *ui, event_t event, uint8_t data);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

static void clear_bars(bool all);
static void draw_bars(void);
static void determine_focused_item(uui_t *ui, int8_t direction);
static void clear_third_region(void);

/* We need to keep copies of the user settings as the value in the UI will
 * be replaced with measurements when output is active
 */
static int32_t saved_v, saved_i, saved_p, saved_t;
// the M1, M2 recall values
static int32_t[2] recall_v, recall_i, recall_p, recall_t = {0, 0};

// single edit mode, with M1/M2 buttons, not select.
// pressing any other button when in this mode will exit the edit mode
static bool single_edit_mode;
static bool select_mode;

// timer
static int64_t tick_since_count;

// what is displayed on the 3rd row.
static int8_t third_row = 0;
ui_item_t *third_item;
static bool third_invalidate;

enum {
    CUR_GFX_NOT_DRAWN = 0, 
    CUR_GFX_CV = 1,
    CUR_GFX_CC = 2,
    CUR_GFX_PP  = 4,
    CUR_GFX_OPP = 8,
    CUR_GFX_TM = 16,

    CUR_GFX_M1_RECALL = 1024,
    CUR_GFX_M2_RECALL = 2048,
} dpsmode_graphics; 

#define SCREEN_ID  (6)
#define PAST_V     (0)
#define PAST_I     (1)
#define PAST_P     (2)
#define PAST_T     (3)
#define XPOS_CCCV  (25)

#define XPOS_METER   (117)
#define YPOS_VOLTAGE (10)
#define YPOS_CURRENT (45)
#define YPOS_POWER   (80)

/* Overriding white color */
#define COLOR_VOLTAGE   GREEN
#define COLOR_AMPERAGE  YELLOW
#define COLOR_WATTAGE   MAGENTA

/* This is the definition of the voltage item in the UI */
ui_number_t dpsmode_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = XPOS_METER,
        .y = YPOS_VOLTAGE,
        .can_focus = true,
    },
    .font_size = FONT_METER_LARGE, 
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = COLOR_VOLTAGE,
    .value = 0,
    .min = 0,
    .max = 0, /** Set at init, continously updated in the tick callback */
    .si_prefix = si_milli,
    .num_digits = 2,
    .num_decimals = 2,
    .unit = unit_volt, /** Affects the unit printed on screen */
    .changed = &voltage_changed,
};

ui_number_t dpsmode_current = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = XPOS_METER,
        .y = YPOS_CURRENT,
        .can_focus = true,
    },
    .font_size = FONT_METER_LARGE,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = COLOR_AMPERAGE,
    .value = 0,
    .min = 0,
    .max = CONFIG_DPS_MAX_CURRENT,
    .si_prefix = si_milli,
    .num_digits = 1,
    .num_decimals = 3,
    .unit = unit_ampere,
    .changed = &current_changed,
};

// 3rd row items
ui_number_t dpsmode_power = {
    {
        .type = ui_item_number,
        .id = 12,
        .x = XPOS_METER,
        .y = YPOS_POWER,
        .can_focus = true,
    },
    .font_size = FONT_METER_LARGE,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = COLOR_WATTAGE,
    .value = 0,
    .min = 0,
    .max = 0, // set at init
    .si_prefix = si_micro,
    .num_digits = 2,
    .num_decimals = 2,
    .unit = unit_watt,
    .changed = &power_changed,
};

ui_number_t dpsmode_watthour = {
    {
        .type = ui_item_number,
        .id = 13,
        .x = XPOS_METER,
        .y = YPOS_POWER + 5, // +5 since we are using a smaller font
        .can_focus = true,
    },
    .font_size = FONT_METER_MEDIUM,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = 0,
    .max = 99999000,
    .si_prefix = si_milli, // milli watt hours
    .num_digits = 5,
    .num_decimals = 1,
    .unit = unit_watthour,
    .changed = &watthour_changed,
};

ui_time_t dpsmode_timer = {
    {
        .type = ui_item_time,
        .id = 14,
        .x = XPOS_METER,
        .y = YPOS_POWER + 5, // +5 since we are using smaller font
        .can_focus = true,
    },
    .font_size = FONT_METER_MEDIUM,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .changed = &timer_changed,
};


/* This is the screen definition */
ui_screen_t dpsmode_screen = {
    .id = SCREEN_ID,
    .name = "dpsmode",
    .icon_data = (uint8_t *) gfx_iconpower,
    .icon_data_len = sizeof(gfx_iconpower),
    .icon_width = GFX_ICONPOWER_WIDTH,
    .icon_height = GFX_ICONPOWER_HEIGHT,
    .event = &event,
    .activated = &activated,
    .deactivated = &deactivated,
    .enable = &dpsmode_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &dpsmode_tick,
    .set_parameter = &set_parameter,
    .get_parameter = &get_parameter,
    .num_items = 5,
    .items = { 
        (ui_item_t*) &dpsmode_voltage, 
        (ui_item_t*) &dpsmode_current, 
        (ui_item_t*) &dpsmode_power,
        (ui_item_t*) &dpsmode_watthour,
        (ui_item_t*) &dpsmode_timer,
    },
    .parameters = {
        {
            .name = "voltage",
            .unit = unit_volt,
            .prefix = si_milli
        },
        {
            .name = "current",
            .unit = unit_ampere,
            .prefix = si_milli
        },
        {
            .name = "power",
            .unit = unit_watt,
            .prefix = si_milli // or micro?
        },
        {
            .name = {'\0'} /** Terminator */
        },
    }
};

/**
 * @brief      Set function parameter
 *
 * @param[in]  name   name of parameter
 * @param[in]  value  value of parameter as a string - always in SI units
 *
 * @retval     set_param_status_t status code
 */
static set_param_status_t set_parameter(char *name, char *value)
{
    int32_t ivalue = atoi(value);
    if (strcmp("voltage", name) == 0 || strcmp("u", name) == 0) {
        if (ivalue < dpsmode_voltage.min || ivalue > dpsmode_voltage.max) {
            emu_printf("[DPS] Voltage %d is out of range (min:%d max:%d)\n", ivalue, dpsmode_voltage.min, dpsmode_voltage.max);
            return ps_range_error;
        }
        emu_printf("[DPS] Setting voltage to %d\n", ivalue);
        dpsmode_voltage.value = ivalue;
        voltage_changed(&dpsmode_voltage);
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < dpsmode_current.min || ivalue > dpsmode_current.max) {
            emu_printf("[DPS] Current %d is out of range (min:%d max:%d)\n", ivalue, dpsmode_current.min, dpsmode_current.max);
            return ps_range_error;
        }
        emu_printf("[DPS] Setting current to %d\n", ivalue);
        dpsmode_current.value = ivalue;
        current_changed(&dpsmode_current);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Get function parameter
 *
 * @param[in]  name       name of parameter
 * @param[in]  value      value of parameter as a string - always in SI units
 * @param[in]  value_len  length of value buffer
 *
 * @retval     set_param_status_t status code
 */
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len)
{
    if (strcmp("voltage", name) == 0 || strcmp("u", name) == 0) {
        /** value returned in millivolt, module internal representation is centivolt */
        (void) mini_snprintf(value, value_len, "%d", (pwrctl_vout_enabled() ? saved_v : dpsmode_voltage.value));
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", pwrctl_vout_enabled() ? saved_i : dpsmode_current.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void dpsmode_enable(bool enabled)
{
    emu_printf("[DPS] %s output\n", enabled ? "Enable" : "Disable");

    if (enabled) {
        /** Display will now show the current values, keep the user setting saved */
        saved_v = dpsmode_voltage.value;
        saved_i = dpsmode_current.value;
        saved_p = dpsmode_power.value;
        saved_t = dpsmode_timer.value;

        (void) pwrctl_set_vout(dpsmode_voltage.value);
        (void) pwrctl_set_iout(dpsmode_current.value);
        (void) pwrctl_set_ilimit(0xFFFF); /** Set the current limit to the maximum to prevent OCP (over current protection) firing */
        pwrctl_enable_vout(true);

        // clear all bars, including over power protection
        clear_bars(true);

        // reset timer counter and start.
        tick_since_count = get_ticks();
        if (saved_t > 0)
            dpsmode_timer.value--;

    } else {
        // already off, turning off again will reset any pp warnings
        if ( ! pwrctl_vout_enabled()) {
            clear_bars(true);
        } else {
            // when disabled first, clear only cc/cv, not power
            clear_bars(false);
        }
        
        pwrctl_enable_vout(false);

        /** Make sure we're displaying the settings and not the current
          * measurements when the power output is switched off */
        dpsmode_voltage.value = saved_v;
        dpsmode_current.value = saved_i;
        dpsmode_power.value = saved_p;
        dpsmode_timer.value = saved_t;

        // disable timer graphic
        dpsmode_graphics &= ~CUR_GFX_TM;
    }
}

/**
 * @brief      Callback for when value of the voltage item is changed
 *
 * @param      item  The voltage item
 */
static void voltage_changed(ui_number_t *item)
{
    dpsmode_graphics &= ~CUR_GFX_M1_RECALL & ~CUR_GFX_M2_RECALL;
    saved_v = item->value;
    (void) pwrctl_set_vout(item->value);
}

/**
 * @brief      Callback for when value of the current item is changed
 *
 * @param      item  The current item
 */
static void current_changed(ui_number_t *item)
{
    dpsmode_graphics &= ~CUR_GFX_M1_RECALL & ~CUR_GFX_M2_RECALL;
    saved_i = item->value;
    (void) pwrctl_set_iout(item->value);
}

/**
 * @brief      Callback for when value of the power item is changed
 *
 * @param      item  The current item
 */
static void power_changed(ui_number_t *item)
{
    dpsmode_graphics &= ~CUR_GFX_M1_RECALL & ~CUR_GFX_M2_RECALL;
    saved_p = item->value;
    // (void) pwrctl_set_iout(item->value);
}

/**
 * @brief      Callback for when value of the watthour item is changed
 *
 * @param      item  The current item
 */
static void watthour_changed(ui_number_t *item) {
    //
}

static void timer_changed(ui_time_t *item) {
    // do nothing yet...
    saved_t = item->value;
}


static bool event(uui_t *ui, event_t event, uint8_t data) {

    switch(event) {
        case event_button_sel:
        case event_button_m1:
        case event_button_m2:

            // do recall on press_long event_button_m1 or event_button_m2
            if (event == event_button_m1 && data == press_long) {
                saved_v = recall_v[0];
                saved_i = recall_i[0];
                saved_p = recall_p[0];
                saved_t = recall_t[0];

                // Turn off power
                dpsmode_enable(false);

                // show the M1 recall graphics
                dpsmode_graphics |= CUR_GFX_M1_RECALL;
                return true;
            }
            if (event == event_button_m2 && data == press_long) {
                saved_v = recall_v[1];
                saved_i = recall_i[1];
                saved_p = recall_p[1];
                saved_t = recall_t[1];

                // Turn off power
                dpsmode_enable(false);

                // show the M2 recall graphics
                dpsmode_graphics |= CUR_GFX_M2_RECALL;
                return true;
            }


            // leave single edit mode on any button press
            if (single_edit_mode) {
                single_edit_mode = false;

                // toggle focus on anything that is in focus (to unfocus)
                if (dpsmode_voltage.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_voltage);
                if (dpsmode_current.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_current);
                return true;
            }


            break;

        case event_rot_left_m1:
        case event_rot_right_m1:
            // change what's visible on the 3rd row
            // only if not in select mode, as we may be editing fields
            if ( ! select_mode) {
                ui_screen_t *screen = ui->screens[ui->cur_screen];

                // rotate around the 3rd row objects (skip the 1st two)
                if (event == event_rot_right_m1) {
                    third_row = (third_row + 1) % (screen->num_items - 2);
                } else {
                    if (third_row == 0) third_row = screen->num_items - 3;
                    else third_row--;
                }

                third_item = screen->items[2 + third_row];
                third_invalidate = true;
            }
            break;

        default:
            break;
    }


    switch(event) {
        case event_button_m1:
            // if in normal select mode, let parent handle it
            if (select_mode) {
                // third item focused may have changed
                determine_focused_item(ui, -1);

                return false;
            }

            // otherwise, enter single edit mode
            single_edit_mode = true;

            // focus on voltage if not already focused
            if ( ! dpsmode_voltage.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_voltage);

            // we handled it, parent should do nothing
            return true;

        case event_button_m2:
            if (select_mode) { 
                determine_focused_item(ui, 1);
                return false;
            }

            single_edit_mode = true;
            if ( ! dpsmode_current.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_current);
            return true;

        case event_button_sel:
            // toggle select mode, so parent can deal with other UI elements
            // keep track, so this screen will not do anything until we leave this mode
            select_mode = ! select_mode;

            if (select_mode) { 
                determine_focused_item(ui, 0);
                return false;
            }

            return false;

        default:
            return false;
    }

    return false;
}

/**
 * @brief      Callback when screen is activated
 */
static void activated(void) {
    clear_bars(true);
    clear_third_region();

    // reset watthour value when we leave the screen.
    dpsmode_watthour.value = 0;
}

static void determine_focused_item(uui_t *ui, int8_t direction) {
    // determine what is now focused and change third to it.
    ui_screen_t *screen = ui->screens[ui->cur_screen];

    uint8_t focus_index = 0;

    for (uint8_t i = 0; i < screen->num_items; i++) {
        if (((ui_number_t *)screen->items[i])->ui.has_focus) {
            focus_index = i;
        }
    }

    if (direction > 0) {
        // if searching ahead
        for (focus_index++; focus_index < screen->num_items; focus_index++) {
            if (((ui_number_t *)screen->items[focus_index])->ui.can_focus) {
                break;
            }
        }
    } else if (direction < 0) {
        // if searching behind
        for (; ; focus_index--) {
            if (focus_index == 0) {
                focus_index = screen->num_items;
            }

            if (((ui_number_t *)screen->items[focus_index - 1])->ui.can_focus) {
                focus_index--;
                break;
            }
        }

    // the current item on focus by the screen
    } else {
        // this item may not be a third item. The check below should catch it.
        focus_index = screen->cur_item;
    }


    if (focus_index >= 2 && focus_index != screen->num_items) {
        third_item = (ui_item_t *)screen->items[focus_index];
        third_row = focus_index - 2;
        third_invalidate = true;
        clear_third_region();
    }
}

/**
 * @brief      Do any required clean up before changing away from this screen
 */
static void deactivated(void)
{
    tft_clear();
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    if (   past_write_unit(past, (SCREEN_ID << 24) | PAST_V, (void*) &saved_v, 4)
        && past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4)
        && past_write_unit(past, (SCREEN_ID << 24) | PAST_P, (void*) &saved_p, 4)
        && past_write_unit(past, (SCREEN_ID << 24) | PAST_T, (void*) &saved_t, 4)) {
        // write successful
    }

    // recall m1
    if (   past_write_unit(past, (SCREEN_ID << 24) | (PAST_V << 2), (void*) &recall_v[0], 4)
        && past_write_unit(past, (SCREEN_ID << 24) | (PAST_I << 2), (void*) &recall_i[0], 4)
        && past_write_unit(past, (SCREEN_ID << 24) | (PAST_P << 2), (void*) &recall_p[0], 4)
        && past_write_unit(past, (SCREEN_ID << 24) | (PAST_T << 2), (void*) &recall_t[0], 4)) {
        // write successful
    }

    // recall m2
    if (   past_write_unit(past, (SCREEN_ID << 24) | (PAST_V << 4), (void*) &recall_v[1], 4)
        && past_write_unit(past, (SCREEN_ID << 24) | (PAST_I << 4), (void*) &recall_i[1], 4)
        && past_write_unit(past, (SCREEN_ID << 24) | (PAST_P << 4), (void*) &recall_p[1], 4)
        && past_write_unit(past, (SCREEN_ID << 24) | (PAST_T << 4), (void*) &recall_t[1], 4)) {
        // write successful
    }
}

/**
 * @brief      Restore persistent parameters
 *
 * @param      past  The past
 */
static void past_restore(past_t *past)
{
    uint32_t length;
    uint32_t *p = 0;

    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_V, (const void**) &p, &length)) {
        saved_v = dpsmode_voltage.value = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = dpsmode_current.value = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_P, (const void**) &p, &length)) {
        saved_p = dpsmode_power.value = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_T, (const void**) &p, &length)) {
        saved_t = dpsmode_timer.value = *p;
    }

    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_V << 2), (const void**) &p, &length)) {
        recall_v[0] = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_I << 2), (const void**) &p, &length)) {
        recall_i[0] = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_P << 2), (const void**) &p, &length)) {
        recall_p[0] = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_T << 2), (const void**) &p, &length)) {
        recall_t[0] = *p;
    }

    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_V << 4), (const void**) &p, &length)) {
        recall_v[1] = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_I << 4), (const void**) &p, &length)) {
        recall_i[1] = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_P << 4), (const void**) &p, &length)) {
        recall_p[1] = *p;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | (PAST_T << 4), (const void**) &p, &length)) {
        recall_t[1] = *p;
    }

}

/**
 * @brief      Update the UI. We need to be careful about the values shown
 *             as they will differ depending on the current state of the UI
 *             and the current power output mode.
 *             Power off: always show current setting
 *             Power on : show current output value unless the item has focus
 *                        in which case we shall display the current setting.
 */
static void dpsmode_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);

    /** Continously update max voltage output value
      * Max output voltage = Vin / VIN_VOUT_RATIO
      * Add 0.5f to ensure correct rounding when truncated */
    dpsmode_voltage.max = (float) pwrctl_calc_vin(v_in_raw) / VIN_VOUT_RATIO + 0.5f;

    // set the maximum power based on max voltage and max amps
    dpsmode_power.max = dpsmode_voltage.max * CONFIG_DPS_MAX_CURRENT;

    // power enabled
    if (pwrctl_vout_enabled()) {
        // get the actual voltage and current being supplied
        int32_t vout_actual = pwrctl_calc_vout(v_out_raw);
        int32_t cout_actual = pwrctl_calc_iout(i_out_raw);
        int32_t power_actual = vout_actual * cout_actual;

        // TODO: Issue where focus causes a brief frame where value is incorrect
        
        // Voltage setting has focus, update with the desired value and not output value
        if (dpsmode_voltage.ui.has_focus) {
            dpsmode_voltage.value = saved_v;
        }
        // Voltage setting is not focused, update with actual output voltage
        if ( ! dpsmode_voltage.ui.has_focus) {
            dpsmode_voltage.value = vout_actual;
        }

        // Same for amperage. update with desired value if focused
        if (dpsmode_current.ui.has_focus) {
            dpsmode_current.value = saved_i;
        } 
        // Update with actual output voltage if not in focus
        if ( ! dpsmode_current.ui.has_focus) {
            dpsmode_current.value = cout_actual;
        }

        // update the power with desired value if focused
        if (dpsmode_power.ui.has_focus) {
            dpsmode_power.value = saved_p;
        } 
        // Update with actual output power if not in focus
        if ( ! dpsmode_power.ui.has_focus) {
            dpsmode_power.value = power_actual;
        }

        /** Determine if we are in CV or CC mode and display it */
        int32_t vout_diff = abs(saved_v - vout_actual);
        int32_t cout_diff = abs(saved_i - cout_actual);

        if (cout_diff < vout_diff) {
            // current diff smaller than voltage diff (constant current)
            dpsmode_graphics |= CUR_GFX_CC;
            dpsmode_graphics &= ~CUR_GFX_CV;

        } else {
            // current diff larger than voltage diff (constant voltage)
            dpsmode_graphics |= CUR_GFX_CV;
            dpsmode_graphics &= ~CUR_GFX_CC;
        }


        // OPP / PP warnings
        // over power limits (if defined, or absolute maximum of this device)
        if ( (saved_p > 0 && power_actual >= saved_p) || power_actual >= dpsmode_power.max) {
            // show opp warning
            dpsmode_graphics |= CUR_GFX_OPP;
            dpsmode_graphics &= ~CUR_GFX_PP;
            // disable timer graphic
            dpsmode_graphics &= ~CUR_GFX_TM;

            // power off
            dpsmode_enable(false);
            opendps_update_power_status(false);
        }

        // over 80% power (if defined, or absolute maximum), show warning
        else if ( (saved_p > 0 && power_actual >= (saved_p * 0.8f)) || power_actual >= (dpsmode_power.max * 0.8f) ) {
            // show opp warning
            dpsmode_graphics |= CUR_GFX_PP;
            dpsmode_graphics &= ~CUR_GFX_OPP;

        } else {
            // no pp or opp warning
            dpsmode_graphics &= ~CUR_GFX_PP;
            dpsmode_graphics &= ~CUR_GFX_OPP;
        }

        // timer
        int64_t diff = get_ticks() - tick_since_count;
        int8_t secs = diff / 1000;

        // update every second
        if (secs > 0) {
            // update timer. If 0, use the timer as a clock. Otherwise, count down
            if (saved_t > 0) {
                dpsmode_timer.value -= secs;
                dpsmode_graphics |= CUR_GFX_TM;
            } else {
                dpsmode_timer.value += secs;

                // overflow timer
                if (dpsmode_timer.value > MAX_TIME) {
                    dpsmode_timer.value = MAX_TIME;
                }
            }
            diff = diff % 1000;

            tick_since_count = get_ticks() - diff;

            // calculate amount of power delivered as milli watt hours
            dpsmode_watthour.value += ((power_actual * 1000.0 / 3600.0f) * secs) / 1000;

            // 1000 == 1.0mWh
            /*
            if (dpsmode_watthour.value <= 1000000) {
                dpsmode_watthour.num_decimals = 1;
            }
            if (dpsmode_watthour.value >= 1000000) {
                dpsmode_watthour.num_decimals = 0;
            }
            */
        }

        // timer enabled, count down
        if (saved_t > 0 && dpsmode_timer.value <= 0) {
            // timer has counted down
            // show the timer
            third_item = &dpsmode_timer;
            third_invalidate = true;

            // power off
            dpsmode_enable(false);
            opendps_update_power_status(false);
        }

    }


    // redraw
    dpsmode_voltage.ui.draw(&dpsmode_voltage.ui);
    dpsmode_current.ui.draw(&dpsmode_current.ui);

    // draw 3rd row item...
    if ( third_item ) {
        // blank out the whole 3rd row area
        if (third_invalidate) {
            third_invalidate = false;
            clear_third_region();
        }

        ((ui_number_t *)third_item)->ui.draw(& ((ui_number_t *)third_item)->ui);

    } else {
        // dpsmode_power.ui.draw(&dpsmode_power.ui);
    }

    // draw bars on right
    draw_bars();
}

static void clear_third_region() {
    tft_fill(0, YPOS_POWER,
            TFT_WIDTH, FONT_METER_LARGE_MAX_GLYPH_HEIGHT,
            BLACK);
}


/**
 * @brief draws bars on the right hand side of the screen.
 *        These include the CV / CC / PP and OPP warnings
 */
static void draw_bars() {
    // TODO: Keep track of what's drawn and what's not to optimze draws

    // draw cc bar
    if (dpsmode_graphics & CUR_GFX_CC) {
        tft_blit((uint16_t*) gfx_ccbar,
                GFX_CCBAR_WIDTH, GFX_CCBAR_HEIGHT,
                TFT_WIDTH - GFX_CCBAR_WIDTH,
                YPOS_CURRENT + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CCBAR_HEIGHT );
    } else {
        // clear cc bar
        tft_fill(TFT_WIDTH - GFX_CCBAR_WIDTH, YPOS_CURRENT + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CCBAR_HEIGHT,
                    GFX_CCBAR_WIDTH, GFX_CCBAR_HEIGHT,
                    BLACK);
    }

    // draw cv bar
    if (dpsmode_graphics & CUR_GFX_CV) {
        tft_blit((uint16_t*) gfx_cvbar,
                GFX_CVBAR_WIDTH, GFX_CVBAR_HEIGHT,
                TFT_WIDTH - GFX_CVBAR_WIDTH,
                YPOS_VOLTAGE + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CVBAR_HEIGHT );
    } else {
        tft_fill(TFT_WIDTH - GFX_CVBAR_WIDTH, YPOS_VOLTAGE + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CVBAR_HEIGHT,
                    GFX_CVBAR_WIDTH, GFX_CVBAR_HEIGHT,
                    BLACK);
    }


    // draw timer
    if (dpsmode_graphics & CUR_GFX_TM) {
        // blink the timer icon 
        if ((get_ticks() >> 9) & 1)
            tft_blit((uint16_t*) gfx_tmbar,
                    GFX_TMBAR_WIDTH, GFX_TMBAR_HEIGHT,
                    TFT_WIDTH - GFX_TMBAR_WIDTH,
                    YPOS_POWER + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_TMBAR_HEIGHT );
        else
            tft_fill(TFT_WIDTH - GFX_TMBAR_WIDTH, YPOS_POWER + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_TMBAR_HEIGHT,
                        GFX_TMBAR_WIDTH, GFX_TMBAR_HEIGHT,
                        BLACK);
    }


    // draw opp
    if (dpsmode_graphics & CUR_GFX_OPP) {
        // blink the opp warning
        if (((get_ticks() >> 9) & 1) == 0)
            tft_blit((uint16_t*) gfx_oppbar,
                    GFX_OPPBAR_WIDTH, GFX_OPPBAR_HEIGHT,
                    TFT_WIDTH - GFX_OPPBAR_WIDTH,
                    YPOS_POWER + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_OPPBAR_HEIGHT );
        // no timer (or other icons), paint black over it.
        else if ( (dpsmode_graphics & CUR_GFX_TM) != 1)
            tft_fill(TFT_WIDTH - GFX_OPPBAR_WIDTH, YPOS_POWER + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_OPPBAR_HEIGHT,
                        GFX_OPPBAR_WIDTH, GFX_OPPBAR_HEIGHT,
                        BLACK);
    }
    // draw pp
    else if (dpsmode_graphics & CUR_GFX_PP) {
        // if timer (or other icons), blink it so it is visible.
        if (((get_ticks() >> 9) & 1) == 0)
            tft_blit((uint16_t*) gfx_ppbar,
                    GFX_PPBAR_WIDTH, GFX_PPBAR_HEIGHT,
                    TFT_WIDTH - GFX_PPBAR_WIDTH,
                    YPOS_POWER + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_PPBAR_HEIGHT );
    }


    // no graphics at 3rd location, clear it.
    if (  ! (dpsmode_graphics & CUR_GFX_OPP || dpsmode_graphics & CUR_GFX_PP || dpsmode_graphics & CUR_GFX_TM)) {
        tft_fill(TFT_WIDTH - GFX_PPBAR_WIDTH, YPOS_POWER + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_PPBAR_HEIGHT,
                    GFX_PPBAR_WIDTH, GFX_PPBAR_HEIGHT,
                    BLACK);
    }


    // draw any recall icons
    if (dpsmode_graphics & CUR_GFX_M1_RECALL) {
        tft_blit((uint16_t*) gfx_m1bar,
                GFX_M1BAR_WIDTH, GFX_M1BAR_HEIGHT,
                5, 0);
    } else if (dpsmode_graphics & CUR_GFX_M2_RECALL) {
        tft_blit((uint16_t*) gfx_m2bar,
                GFX_M2BAR_WIDTH, GFX_M2BAR_HEIGHT,
                5, 0);
    }


}

/*
 * @brief Clears bars on the right hand side
 *
 * @param all if true will clear all bars. Otherwise, only cc/cv/pp
 */
static void clear_bars(bool all) {
    if (all) {
        // clears opp as well as the others
        dpsmode_graphics = CUR_GFX_NOT_DRAWN;
        return;
    }

    // clear just cc/cv/pp otherwise
    dpsmode_graphics = dpsmode_graphics & ~CUR_GFX_CC;
    dpsmode_graphics = dpsmode_graphics & ~CUR_GFX_CV;
    dpsmode_graphics = dpsmode_graphics & ~CUR_GFX_PP;
}


/**
 * @brief      Initialise the DPS Mode module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_dpsmode_init(uui_t *ui)
{
    dpsmode_voltage.value = 0; /** read from past */
    dpsmode_current.value = 0; /** read from past */
    dpsmode_power.value = 0; /** read from past */

    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;

    dpsmode_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */

    number_init(&dpsmode_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    dpsmode_voltage.cur_digit = 2;

    // set the maximum power based on max voltage and max amps
    dpsmode_power.max = dpsmode_voltage.max * CONFIG_DPS_MAX_CURRENT;

    number_init(&dpsmode_current);
    number_init(&dpsmode_power);
    number_init(&dpsmode_watthour);
    time_init(&dpsmode_timer);

    // third item initialize
    third_item = &dpsmode_power;
    third_invalidate = true;

    uui_add_screen(ui, &dpsmode_screen);
}
