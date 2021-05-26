/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Johan Kanflo (github.com/kanflo)
 * Copyright (c) 2021 Richard Taylor (github.com/art103)
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
#include "gfx-mppt.h"
#include "hw.h"
#include "func_mppt.h"
#include "uui.h"
#include "uui_number.h"
#include "dbg_printf.h"
#include "mini-printf.h"
#include "dps-model.h"
#include "ili9163c.h"
#include "opendps.h"

/*
 * This is the implementation of the MPPT screen. It has two editable values,
 * voltage and current limit. When the screen is selected, it will automatically
 * enable the output and perform a binary tree search algorithm to determine the
 * maximum output current (up to the set limit) that the input can support.
 * This is useful in combination with a solar panel to find the optimum operating
 * point.
 */

static void mppt_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void mppt_tick(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

/* We need to keep copies of the user settings as the value in the UI will
 * be replaced with measurements when output is active
 */
static int32_t saved_u;
static int32_t saved_i;

#define SCREEN_ID  (1)
#define PAST_U     (0)
#define PAST_I     (1)

/** Delay after adjusting current to wait for the input to settle.
  * Value is in UI ticks (250ms). */
#define SWEEP_DELAY (12)
#define INCREASE_DELAY (8)

/* This is the definition of the voltage item in the UI */
ui_number_t mppt_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 64,
        .y = 6,
        .can_focus = true,
    },
    .font_size = FONT_METER_LARGE, /** The bigger one, try FONT_SMALL or FONT_MEDIUM for kicks */
    .alignment = ui_text_center_aligned,
    .pad_dot = false,
    .color = COLOR_VOLTAGE,
    .value = 0,
    .min = 0,
    .max = 0,
    .si_prefix = si_milli,
    .num_digits = 2,
    .num_decimals = 2,
    .unit = unit_volt, /** Affects the unit printed on screen */
    .changed = &voltage_changed,
};

/* This is the definition of the current item in the UI */
ui_number_t mppt_current = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 64,
        .y = 40,
        .can_focus = true,
    },
    .font_size = FONT_METER_LARGE,
    .alignment = ui_text_center_aligned,
    .pad_dot = false,
    .color = COLOR_AMPERAGE,
    .value = 0,
    .min = 0,
    .max = CONFIG_DPS_MAX_CURRENT,
    .si_prefix = si_milli,
    .num_digits = CURRENT_DIGITS,
    .num_decimals = CURRENT_DECIMALS,
    .unit = unit_ampere,
    .changed = &current_changed,
};

/* This is the definition of the power item in the UI */
ui_number_t mppt_power = {
    {
        .type = ui_item_number,
        .id = 12,
        .x = 64,
        .y = 72,
        .can_focus = false,
    },
    .font_size = FONT_METER_LARGE,
    .alignment = ui_text_center_aligned,
    .pad_dot = false,
    .color = COLOR_WATTAGE,
    .value = 0,
    .min = 0,
    .max = 0,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 1,
    .unit = unit_watt,
};

/* This is the screen definition */
ui_screen_t mppt_screen = {
    .id = SCREEN_ID,
    .name = "mppt",
    .icon_data = (uint8_t *) gfx_mppt,
    .icon_data_len = sizeof(gfx_mppt),
    .icon_width = GFX_MPPT_WIDTH,
    .icon_height = GFX_MPPT_HEIGHT,
    .activated = NULL,
    .deactivated = NULL,
    .enable = &mppt_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &mppt_tick,
    .set_parameter = &set_parameter,
    .get_parameter = &get_parameter,
    .num_items = 3,
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
            .prefix = si_milli
        },
        {
            .name = {'\0'} /** Terminator */
        },
    },
    .items = { (ui_item_t*) &mppt_voltage, (ui_item_t*) &mppt_current, (ui_item_t*) &mppt_power }
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
        if (ivalue < mppt_voltage.min || ivalue > mppt_voltage.max) {
            emu_printf("[MPPT] Voltage %d is out of range (min:%d max:%d)\n", ivalue, mppt_voltage.min, mppt_voltage.max);
            return ps_range_error;
        }
        emu_printf("[MPPT] Setting voltage to %d\n", ivalue);
        mppt_voltage.value = ivalue;
        voltage_changed(&mppt_voltage);
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < mppt_current.min || ivalue > mppt_current.max) {
            emu_printf("[MPPT] Current %d is out of range (min:%d max:%d)\n", ivalue, mppt_current.min, mppt_current.max);
            return ps_range_error;
        }
        emu_printf("[MPPT] Setting current to %d\n", ivalue);
        mppt_current.value = ivalue;
        current_changed(&mppt_current);
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
        (void) mini_snprintf(value, value_len, "%d", (pwrctl_vout_enabled() ? saved_u : mppt_voltage.value));
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", pwrctl_vout_enabled() ? saved_i : mppt_current.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void mppt_enable(bool enabled)
{
    emu_printf("[MPPT] %s output\n", enabled ? "Enable" : "Disable");
    if (enabled) {
        /** Display will now show the current values, keep the user setting saved */
        saved_u = mppt_voltage.value;
        saved_i = mppt_current.value;
        (void) pwrctl_set_vout(saved_u);
        (void) pwrctl_set_iout(0);
        (void) pwrctl_set_ilimit(CONFIG_DPS_MAX_CURRENT);
        (void) pwrctl_set_vlimit(0xFFFF); /** Set the voltage limit to the maximum to prevent OVP (over voltage protection) firing */
        pwrctl_enable_vout(true);
    } else {
        pwrctl_enable_vout(false);
        /** Make sure we're displaying the settings and not the current
          * measurements when the power output is switched off */
        mppt_voltage.value = saved_u;
        mppt_voltage.ui.draw(&mppt_voltage.ui);
        mppt_current.value = saved_i;
        mppt_current.ui.draw(&mppt_current.ui);
        mppt_power.value = saved_i * saved_u / 1000;
        mppt_power.ui.draw(&mppt_power.ui);
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
    (void) pwrctl_set_iout(0);
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    /** @todo: past bug causes corruption for units smaller than 4 bytes (#27) */
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_U, (void*) &saved_u, 4 /* sizeof(mppt_voltage.value) */ )) {
        /** @todo: handle past write failures */
    }
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4 /* sizeof(mppt_current.value) */ )) {
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
        saved_u = mppt_voltage.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = mppt_current.value = *p;
        (void) length;
    }
}

/**
 * @brief      Update the UI. We need to be careful about the values shown
 *             as they will differ depending on the current state of the UI
 *             and the current power output mode. 250ms ticks.
 *             Power off: always show current setting
 *             Power on : show current output value unless the item has focus
 *                        in which case we shall display the current setting.
 */
static void mppt_tick(void)
{
    static bool mppt_sweep = true;
    static uint32_t tick_count = 0;
    static uint32_t next_sweep = 0;

    static uint32_t mppt_wait;
    static int32_t mppt_max_i;
    static int32_t mppt_i_step;
    static int32_t mppt_v_in;

    bool editing = false;
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);

    tick_count++;
    if (tick_count > next_sweep) {
        mppt_sweep = true;

        /** Try and go to max current */
        mppt_max_i = saved_i;
        mppt_i_step = 0;
        mppt_v_in = 50000;

        /** Re-enable ourselves */
        opendps_enable_output(false);
        opendps_enable_output(true);

        /** Scan every 30 mins (1 tick == 0.25s) */
        next_sweep = tick_count + 4 * 60 * 30;
    }

    if (pwrctl_vout_enabled()) {
        int32_t new_u = pwrctl_calc_vout(v_out_raw);
        int32_t new_i = pwrctl_calc_iout(i_out_raw);
        int32_t new_p = new_u * new_i / 1000;
        int32_t new_v_in = pwrctl_calc_vin(v_in_raw);
        bool update_iout = false;

        if (mppt_sweep) {
            mppt_wait++;
            /** Wait for 2s to allow the system to stabilise. */
            if (mppt_wait > SWEEP_DELAY) {
                mppt_wait = 0;

                if (new_i < mppt_max_i - 10 ||
                    mppt_i_step < 10) {
                    /** We're at min input / min step.
                      * Abort the sweep */
                    mppt_sweep = false;
                } else {
                    /** Perform a binary search for the mppt */
                    mppt_max_i = mppt_max_i + mppt_i_step;
                    mppt_i_step /= 2;
                }

                /* We know this v_in is good */
                if (new_v_in < mppt_v_in)
                    mppt_v_in = new_v_in;
            }
            new_i = mppt_max_i;
            update_iout = true;
        } else {
            /** Keep tracking around the mpp input voltage */
            if (new_v_in < mppt_v_in) {
                new_i -= 100;
                if (new_i < 0) {
                    next_sweep = tick_count;
                    new_i = 0;
                }
                update_iout = true;
            } else if (new_v_in > mppt_v_in + 250) {
                mppt_wait++;
                if (mppt_wait > INCREASE_DELAY) {
                    mppt_wait = 0;
                    new_i += 10;
                    update_iout = true;
                }
            }
        }

        /** Make sure we limit the current to our set maximum */
        if (new_i > saved_i)
            new_i = saved_i;

        if (update_iout) {
            /** Apply the new current setting */
            pwrctl_set_iout(new_i);
        }

        if (mppt_voltage.ui.has_focus) {
            /** If the voltage setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (mppt_voltage.value != (int32_t) pwrctl_get_vout()) {
                mppt_voltage.value = pwrctl_get_vout();
                mppt_voltage.ui.draw(&mppt_voltage.ui);
            }
            editing = true;
        } else {
            /** No focus, update display if necessary */
            if (new_u != mppt_voltage.value) {
                mppt_voltage.value = new_u;
                mppt_voltage.ui.draw(&mppt_voltage.ui);
            }
        }

        if (mppt_current.ui.has_focus) {
            /** If the current setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (mppt_current.value != saved_i) {
                mppt_current.value = saved_i;
                mppt_current.ui.draw(&mppt_current.ui);
            }
            editing = true;
        } else {
            /** No focus, update display if necessary */
            if (new_i != mppt_current.value) {
                mppt_current.value = new_i;
                mppt_current.ui.draw(&mppt_current.ui);
            }
        }

        /** Update the power value */
        if (new_p != mppt_power.value) {
            mppt_power.value = new_p;
            mppt_power.ui.draw(&mppt_power.ui);
        }
    } else {
        if (mppt_sweep) {
            if (mppt_i_step == 0) {
                /** Ignore the first time through */
                mppt_i_step = saved_i / 2;
            } else {
                /** That current setting failed, next step! */
                mppt_max_i = mppt_max_i - mppt_i_step;
                mppt_i_step /= 2;
            }

            /** Apply the new setpoint */
            opendps_enable_output(true);
            pwrctl_set_iout(mppt_max_i);
        } else {
            /** Something went wrong, start a new sweep */
            next_sweep = tick_count;
        }
    }

    if (editing) {
        /** Trigger a new MPPT sweep in 2s */
        next_sweep = tick_count + SWEEP_DELAY;
    }
}

/**
 * @brief      Initialise the CV module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_mppt_init(uui_t *ui)
{
    mppt_voltage.value = 0; /** read from past */
    mppt_current.value = 0; /** read from past */
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    mppt_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */
    number_init(&mppt_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    mppt_voltage.cur_digit = 2;
    number_init(&mppt_current);
    number_init(&mppt_power);
    uui_add_screen(ui, &mppt_screen);
}
