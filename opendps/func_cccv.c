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
#include "gfx-cccv.h"
#include "hw.h"
#include "func_cccv.h"
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

static void cccv_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void cccv_tick(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

/* We need to keep copies of the user settings as the value in the UI will
 * be replaced with measurements when output is active
 */
static int32_t saved_u;
static int32_t saved_i;

#define SCREEN_ID  (2)
#define PAST_U     (0)
#define PAST_I     (1)

/* This is the definition of the voltage item in the UI */
ui_number_t cccv_voltage = {
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
    .max = 0, /** Set at init, continously updated in the tick callback */
    .si_prefix = si_milli,
    .num_digits = 2,
    .num_decimals = 2,
    .unit = unit_volt, /** Affects the unit printed on screen */
    .changed = &voltage_changed,
};

/* This is the definition of the current item in the UI */
ui_number_t cccv_current = {
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
ui_number_t cccv_power = {
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
ui_screen_t cccv_screen = {
    .id = SCREEN_ID,
    .name = "cccv",
    .icon_data = (uint8_t *) gfx_cccv,
    .icon_data_len = sizeof(gfx_cccv),
    .icon_width = GFX_CCCV_WIDTH,
    .icon_height = GFX_CCCV_HEIGHT,
    .activated = NULL,
    .deactivated = NULL,
    .enable = &cccv_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &cccv_tick,
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
    .items = { (ui_item_t*) &cccv_voltage, (ui_item_t*) &cccv_current, (ui_item_t*) &cccv_power }
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
        if (ivalue < cccv_voltage.min || ivalue > cccv_voltage.max) {
            emu_printf("[CCCV] Voltage %d is out of range (min:%d max:%d)\n", ivalue, cccv_voltage.min, cccv_voltage.max);
            return ps_range_error;
        }
        emu_printf("[CCCV] Setting voltage to %d\n", ivalue);
        cccv_voltage.value = ivalue;
        voltage_changed(&cccv_voltage);
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < cccv_current.min || ivalue > cccv_current.max) {
            emu_printf("[CCCV] Current %d is out of range (min:%d max:%d)\n", ivalue, cccv_current.min, cccv_current.max);
            return ps_range_error;
        }
        emu_printf("[CCCV] Setting current to %d\n", ivalue);
        cccv_current.value = ivalue;
        current_changed(&cccv_current);
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
        (void) mini_snprintf(value, value_len, "%d", (pwrctl_vout_enabled() ? saved_u : cccv_voltage.value));
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", pwrctl_vout_enabled() ? saved_i : cccv_current.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void cccv_enable(bool enabled)
{
    emu_printf("[CCCV] %s output\n", enabled ? "Enable" : "Disable");
    if (enabled) {
        /** Display will now show the current values, keep the user setting saved */
        saved_u = cccv_voltage.value;
        saved_i = cccv_current.value;
        (void) pwrctl_set_vout(saved_u);
        (void) pwrctl_set_iout(saved_i);
        (void) pwrctl_set_ilimit(CONFIG_DPS_MAX_CURRENT);
        (void) pwrctl_set_vlimit(0xFFFF); /** Set the voltage limit to the maximum to prevent OVP (over voltage protection) firing */
        pwrctl_enable_vout(true);
    } else {
        pwrctl_enable_vout(false);
        /** Make sure we're displaying the settings and not the current
          * measurements when the power output is switched off */
        cccv_voltage.value = saved_u;
        cccv_voltage.ui.draw(&cccv_voltage.ui);
        cccv_current.value = saved_i;
        cccv_current.ui.draw(&cccv_current.ui);
        cccv_power.value = saved_i * saved_u / 1000;
        cccv_power.ui.draw(&cccv_power.ui);
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
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_U, (void*) &saved_u, 4 /* sizeof(cccv_voltage.value) */ )) {
        /** @todo: handle past write failures */
    }
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4 /* sizeof(cccv_current.value) */ )) {
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
        saved_u = cccv_voltage.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = cccv_current.value = *p;
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
static void cccv_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    /** Continously update max voltage output value
      * Max output voltage = Vin / VIN_VOUT_RATIO
      * Add 0.5f to ensure correct rounding when truncated */
    cccv_voltage.max = (float) pwrctl_calc_vin(v_in_raw) / VIN_VOUT_RATIO + 0.5f;
    if (pwrctl_vout_enabled()) {
        int32_t new_u = pwrctl_calc_vout(v_out_raw);
        int32_t new_i = pwrctl_calc_iout(i_out_raw);
        int32_t new_p = new_u * new_i / 1000;

        if (cccv_voltage.ui.has_focus) {
            /** If the voltage setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cccv_voltage.value != (int32_t) pwrctl_get_vout()) {
                cccv_voltage.value = pwrctl_get_vout();
                cccv_voltage.ui.draw(&cccv_voltage.ui);
            }
        } else {
            /** No focus, update display if necessary */
            if (new_u != cccv_voltage.value) {
                cccv_voltage.value = new_u;
                cccv_voltage.ui.draw(&cccv_voltage.ui);
            }
        }

        if (cccv_current.ui.has_focus) {
            /** If the current setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (cccv_current.value != saved_i) {
                cccv_current.value = saved_i;
                cccv_current.ui.draw(&cccv_current.ui);
            }
        } else {
            /** No focus, update display if necessary */
            if (new_i != cccv_current.value) {
                cccv_current.value = new_i;
                cccv_current.ui.draw(&cccv_current.ui);
            }
        }

        if (new_p != cccv_power.value) {
            cccv_power.value = new_p;
            cccv_power.ui.draw(&cccv_power.ui);
        }
    }
}

/**
 * @brief      Initialise the CV module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_cccv_init(uui_t *ui)
{
    cccv_voltage.value = 0; /** read from past */
    cccv_current.value = 0; /** read from past */
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    cccv_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */
    number_init(&cccv_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    cccv_voltage.cur_digit = 2;
    number_init(&cccv_current);
    number_init(&cccv_power);
    uui_add_screen(ui, &cccv_screen);
}
