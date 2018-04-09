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
#include "hw.h"
#include "func_cv.h"
#include "uui.h"
#include "uui_number.h"

/*
 * This is the implementation of the CV screen. It has two editable values, 
 * constant voltage and current limit. When power is enabled it will continously
 * display the current output voltage and current draw. If the user edits one
 * of the values when power is eabled, the other will continue to be updated.
 * Thid allows for ramping the voltage and obsering the current increase.
 */

static void cv_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void cv_tick(void);

/* This is the definition of the voltage item in the UI */
ui_number_t cv_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 120,
        .y = 15,
        .can_focus = true,
    },
    .font_size = 1, /** The bigger one, try 0 for kicks */
    .value = 0,
    .min = 0,
    .max = 0, /** Set at init, continously updated in the tick callback */
    .num_digits = 2,
    .num_decimals = 2, /** 2 decimals => value is in centivolts */
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
ui_screen_t cv_screen = {
    .name = "cv",
    .enable = &cv_enable,
    .tick = &cv_tick,
    .num_items = 2,
    .items = { (ui_item_t*) &cv_voltage, (ui_item_t*) &cv_current }
};

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void cv_enable(bool enabled)
{
    if (enabled) {
        (void) pwrctl_set_vout(10 * cv_voltage.value);
        pwrctl_enable_vout(true);
    } else {
        pwrctl_enable_vout(false);

        /** Make sure we're displaying the settings and not the current 
          * measurements when the power output is switched off */
        cv_voltage.value = pwrctl_get_vout() / 10;
        cv_voltage.ui.draw(&cv_voltage.ui);
        cv_current.value = pwrctl_get_ilimit();
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
    (void) pwrctl_set_vout(10 * item->value);
}

/**
 * @brief      Callback for when value of the current item is changed
 *
 * @param      item  The current item
 */
static void current_changed(ui_number_t *item)
{
    (void) pwrctl_set_ilimit(item->value);
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
    if (pwrctl_vout_enabled()) {
        uint16_t i_out_raw, v_in_raw, v_out_raw;
        hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
        /** Continously update max voltage output value */
        cv_voltage.max = pwrctl_calc_vin(v_in_raw) / 10;
        if (cv_voltage.ui.has_focus) {
            /** If the voltage setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cv_voltage.value != (int32_t) pwrctl_get_vout() / 10) {
                cv_voltage.value = pwrctl_get_vout() / 10;
                cv_voltage.ui.draw(&cv_voltage.ui);
            }
        } else {
            /** No focus, update display if necessary */
            int32_t new_u = pwrctl_calc_vout(v_out_raw) / 10;
            if (new_u != cv_voltage.value) {
                cv_voltage.value = new_u;
                cv_voltage.ui.draw(&cv_voltage.ui);
            }
        }

        if (cv_current.ui.has_focus) {
            /** If the currelt setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cv_current.value != (int32_t) pwrctl_get_ilimit()) {
                cv_current.value = pwrctl_get_ilimit();
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
 * @brief      Function init. Initialise the CV module and add its screen to
 *             the UI
 *
 * @param      ui    The user interface
 */
void func_cv_init(uui_t *ui)
{
    cv_voltage.value = pwrctl_get_vout() / 10;
    cv_current.value = pwrctl_get_ilimit();
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    cv_voltage.max = pwrctl_calc_vin(v_in_raw) / 10; /** @todo: subtract for LDO */
    number_init(&cv_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from 
        accidentally cranking up the setting 10V or more */
    cv_voltage.cur_digit = 2;
    number_init(&cv_current);
    uui_add_screen(ui, &cv_screen);
}
