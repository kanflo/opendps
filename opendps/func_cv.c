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
#include <stdlib.h>
#include <string.h>
#include "gfx-cv.h"
#include "hw.h"
#include "func_cv.h"
#include "uui.h"
#include "uui_number.h"
#include "dbg_printf.h"
#include "mini-printf.h"
#include "dps-model.h"
#include "ili9163c.h"

/*
 * This is the implementation of the CV screen. It has two editable values,
 * constant voltage and current limit. When power is enabled it will continously
 * display the current output voltage and current draw. If the user edits one
 * of the values when power is eabled, the other will continue to be updated.
 * This allows for ramping the voltage and obsering the current increase.
 */

static void cv_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void cv_tick(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

/* We need to keep copies of the user settings as the value in the UI will
 * be replaced with measurements when output is active
 */
static int32_t saved_u, saved_i;

#define SCREEN_ID  (1)
#define PAST_U     (0)
#define PAST_I     (1)

/* This is the definition of the voltage item in the UI */
ui_number_t cv_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 120,
        .y = 15,
        .can_focus = true,
    },
    .font_size = FONT_METER_LARGE, /** The bigger one, try FONT_SMALL or FONT_MEDIUM for kicks */
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

/* This is the definition of the current item in the UI */
ui_number_t cv_current = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 120,
        .y = 60,
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

/* This is the screen definition */
ui_screen_t cv_screen = {
    .id = SCREEN_ID,
    .name = "cv",
    .icon_data = (uint8_t *) gfx_cv,
    .icon_data_len = sizeof(gfx_cv),
    .icon_width = GFX_CV_WIDTH,
    .icon_height = GFX_CV_HEIGHT,
    .activated = NULL,
    .deactivated = NULL,
    .enable = &cv_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &cv_tick,
    .set_parameter = &set_parameter,
    .get_parameter = &get_parameter,
    .num_items = 2,
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
            .name = {'\0'} /** Terminator */
        },
    },
    .items = { (ui_item_t*) &cv_voltage, (ui_item_t*) &cv_current }
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
        if (ivalue < cv_voltage.min || ivalue > cv_voltage.max) {
            emu_printf("[CV] Voltage %d is out of range (min:%d max:%d)\n", ivalue, cv_voltage.min, cv_voltage.max);
            return ps_range_error;
        }
        emu_printf("[CV] Setting voltage to %d\n", ivalue);
        cv_voltage.value = ivalue;
        voltage_changed(&cv_voltage);
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < cv_current.min || ivalue > cv_current.max) {
            emu_printf("[CV] Current %d is out of range (min:%d max:%d)\n", ivalue, cv_current.min, cv_current.max);
            return ps_range_error;
        }
        emu_printf("[CV] Setting current to %d\n", ivalue);
        cv_current.value = ivalue;
        current_changed(&cv_current);
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
        (void) mini_snprintf(value, value_len, "%d", (pwrctl_vout_enabled() ? saved_u : cv_voltage.value));
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", pwrctl_vout_enabled() ? saved_i : cv_current.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void cv_enable(bool enabled)
{
    emu_printf("[CV] %s output\n", enabled ? "Enable" : "Disable");
    if (enabled) {
        /** Display will now show the current values, keep the user setting saved */
        saved_u = cv_voltage.value;
        saved_i = cv_current.value;
        (void) pwrctl_set_vout(cv_voltage.value);
        (void) pwrctl_set_iout(CONFIG_DPS_MAX_CURRENT);
        (void) pwrctl_set_ilimit(cv_current.value);
        (void) pwrctl_set_vlimit(0xFFFF); /** Set the voltage limit to the maximum to prevent OVP (over voltage protection) firing */
        pwrctl_enable_vout(true);
    } else {
        pwrctl_enable_vout(false);
        /** Make sure we're displaying the settings and not the current
          * measurements when the power output is switched off */
        cv_voltage.value = saved_u;
        cv_voltage.ui.draw(&cv_voltage.ui);
        cv_current.value = saved_i;
        cv_current.ui.draw(&cv_current.ui);
    }
}

/**
 * @brief      Callback for when value of the voltage item is changed
 *
 * @param      item  The voltage item
 */
static void voltage_changed(ui_number_t *item)
{
    saved_u = item->value;
    (void) pwrctl_set_vout(item->value);
}

/**
 * @brief      Callback for when value of the current item is changed
 *
 * @param      item  The current item
 */
static void current_changed(ui_number_t *item)
{
    saved_i = item->value;
    (void) pwrctl_set_ilimit(item->value);
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    /** @todo: past bug causes corruption for units smaller than 4 bytes (#27) */
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_U, (void*) &saved_u, 4 /* sizeof(cv_voltage.value) */ )) {
        /** @todo: handle past write failures */
    }
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4 /* sizeof(cv_current.value) */ )) {
        /** @todo: handle past write failures */
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
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_U, (const void**) &p, &length)) {
        saved_u = cv_voltage.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = cv_current.value = *p;
        (void) length;
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
static void cv_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    /** Continously update max voltage output value
      * Max output voltage = Vin / VIN_VOUT_RATIO
      * Add 0.5f to ensure correct rounding when truncated */
    cv_voltage.max = (float) pwrctl_calc_vin(v_in_raw) / VIN_VOUT_RATIO + 0.5f;
    if (pwrctl_vout_enabled()) {
        if (cv_voltage.ui.has_focus) {
            /** If the voltage setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cv_voltage.value != (int32_t) pwrctl_get_vout()) {
                cv_voltage.value = pwrctl_get_vout();
                cv_voltage.ui.draw(&cv_voltage.ui);
            }
        } else {
            /** No focus, update display if necessary */
            int32_t new_u = pwrctl_calc_vout(v_out_raw);
            if (new_u != cv_voltage.value) {
                cv_voltage.value = new_u;
                cv_voltage.ui.draw(&cv_voltage.ui);
            }
        }

        if (cv_current.ui.has_focus) {
            /** If the current setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cv_current.value != saved_i) {
                cv_current.value = saved_i;
                cv_current.ui.draw(&cv_current.ui);
            }
        } else {
            /** No focus, update display if necessary */
            int32_t new_i = pwrctl_calc_iout(i_out_raw);
            if (new_i != cv_current.value) {
                cv_current.value = new_i;
                cv_current.ui.draw(&cv_current.ui);
            }
        }
    }
}

/**
 * @brief      Initialise the CV module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_cv_init(uui_t *ui)
{
    cv_voltage.value = 0; /** read from past */
    cv_current.value = 0; /** read from past */
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    cv_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */
    number_init(&cv_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    cv_voltage.cur_digit = 2;
    number_init(&cv_current);
    uui_add_screen(ui, &cv_screen);
}
