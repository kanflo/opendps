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
#include "hw.h"
#include "func_cc.h"
#include "uui.h"
#include "uui_number.h"
#include "cc.h"
#include "dbg_printf.h"
#include "mini-printf.h"

/*
 * This is the implementation of the CC screen. 
 * 
 * It has one editable value, 
 * constant voltage and current limit. When power is enabled it will continously
 * display the current output voltage and current draw. If the user edits one
 * of the values when power is eabled, the other will continue to be updated.
 * Thid allows for ramping the voltage and obsering the current increase.
 */

static void cc_enable(bool _enable);
static void current_changed(ui_number_t *item);
static void cc_tick(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

/* We need to keep copies of the user settings as the value in the UI will 
 * be replaced with measurements when output is active
 */
static int32_t saved_i;

#define SCREEN_ID  (2)
#define PAST_I     (0)

/* This is the definition of the voltage item in the UI */
ui_number_t cc_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 120,
        .y = 15,
        .can_focus = false,
    },
    .font_size = 1, /** The bigger one, try 0 for kicks */
    .value = 0,
    .min = 0,
    .max = 0, /** Set at init, continously updated in the tick callback */
    .num_digits = 2,
    .num_decimals = 2, /** 2 decimals => value is in centivolts */
    .unit = unit_volt, /** Affects the unit printed on screen */
};

/* This is the definition of the current item in the UI */
ui_number_t cc_current = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 120,
        .y = 60,
        .can_focus = true,
    },
    .font_size = 1,
    .value = 0,
    .min = 0,
    .max = CONFIG_DPS_MAX_CURRENT,
    .num_digits = 1,
    .num_decimals = 3, /** 3 decimals => value is in milliapmere */
    .unit = unit_ampere,
    .changed = &current_changed,
};

/* This is the screen definition */
ui_screen_t cc_screen = {
    .id = SCREEN_ID,
    .name = "cc",
    .icon_data = (uint8_t *) cc,
    .icon_data_len = sizeof(cc),
    .icon_width = cc_width,
    .icon_height = cc_height,
    .enable = &cc_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .set_parameter = &set_parameter,
    .get_parameter = &get_parameter,
    .tick = &cc_tick,
    .num_items = 2,
    .parameters = {
        {
            .name = "current",
            .unit = unit_ampere,
            .prefix = si_milli
        },
        {
            .name = {'\0'} /** Terminator */
        },
    },
    .items = { (ui_item_t*) &cc_voltage, (ui_item_t*) &cc_current }
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
    if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < cc_current.min || ivalue > cc_current.max) {
            emu_printf("[CC] Current %d is out of range (min:%d max:%d)\n", ivalue, cc_current.min, cc_current.max);
            return ps_range_error;
        }
        emu_printf("[CC] Setting current to %d\n", ivalue);
        cc_current.value = ivalue;
        current_changed(&cc_current);
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
    if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", pwrctl_vout_enabled() ? saved_i : cc_current.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void cc_enable(bool enabled)
{
    if (enabled) {
        saved_i = cc_current.value;
        (void) pwrctl_set_vout(10 * cc_voltage.max - 1000);
        (void) pwrctl_set_ilimit(CONFIG_DPS_MAX_CURRENT);
        (void) pwrctl_set_iout(cc_current.value);
        pwrctl_enable_vout(true);
    } else {
        pwrctl_enable_vout(false);
        /** Make sure we're displaying the settings and not the current 
          * measurements when the power output is switched off */
        cc_voltage.value = 0;
        cc_voltage.ui.draw(&cc_voltage.ui);
        cc_current.value = saved_i;
        cc_current.ui.draw(&cc_current.ui);
    }
}

/**
 * @brief      Callback for when value of the current item is changed
 *
 * @param      item  The current item
 */
static void current_changed(ui_number_t *item)
{
    saved_i = item->value;
    (void) pwrctl_set_iout(item->value);
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    /** @todo: past bug causes corruption for units smaller than 4 bytes (#27) */
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4 /* sizeof(cc_current.value) */)) {
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
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = cc_current.value = *p;
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
static void cc_tick(void)
{    
    if (pwrctl_vout_enabled()) {
        uint16_t i_out_raw, v_in_raw, v_out_raw;
        hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
        /** Continously update max voltage output value */
        cc_voltage.max = pwrctl_calc_vin(v_in_raw) / 10;
        int32_t new_u = pwrctl_calc_vout(v_out_raw) / 10;
        if (new_u != cc_voltage.value) {
            cc_voltage.value = new_u;
            cc_voltage.ui.draw(&cc_voltage.ui);
        }

        if (cc_current.ui.has_focus) {
            /** If the current setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cc_current.value != saved_i) { // (int32_t) pwrctl_get_ilimit()) {
                cc_current.value = saved_i; // pwrctl_get_ilimit();
                cc_current.ui.draw(&cc_current.ui);
            }
        } else {
            /** No focus, update display if necessary */
            int32_t new_i = pwrctl_calc_iout(i_out_raw);
            if (new_i != cc_current.value) {
                cc_current.value = new_i;
                cc_current.ui.draw(&cc_current.ui);
            }
        }
    }
}

/**
 * @brief      Function init. Initialise the cc module and add its screen to
 *             the UI
 *
 * @param      ui    The user interface
 */
void func_cc_init(uui_t *ui)
{
    cc_voltage.value = pwrctl_get_vout() / 10;
    cc_current.value = pwrctl_get_ilimit();
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    cc_voltage.max = pwrctl_calc_vin(v_in_raw) / 10; /** @todo: subtract for LDO */
    number_init(&cc_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from 
        accidentally cranking up the setting 10V or more */
//    cc_voltage.cur_digit = 2;
    number_init(&cc_current);
    uui_add_screen(ui, &cc_screen);
}
