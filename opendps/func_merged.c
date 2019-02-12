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
#include "gfx-cc.h"
#include "gfx-cv.h"
#include "gfx-merged.h"
#include "hw.h"
#include "func_merged.h"
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
 * Thid allows for ramping the voltage and obsering the current increase.
 */

static void merged_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void merged_tick(void);
static void deactivated(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

/* We need to keep copies of the user settings as the value in the UI will
 * be replaced with measurements when output is active
 */
static int32_t saved_u, saved_i;

enum {
    CUR_GFX_NOT_DRAWN, 
    CUR_GFX_CV,
    CUR_GFX_CC,
} current_mode_gfx; 

#define SCREEN_ID  (3)
#define PAST_U     (0)
#define PAST_I     (1)

/* This is the definition of the voltage item in the UI */
ui_number_t merged_voltage = {
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
ui_number_t merged_current = {
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
ui_screen_t merged_screen = {
    .id = SCREEN_ID,
    .name = "merged",
    .icon_data = (uint8_t *) gfx_merged,
    .icon_data_len = sizeof(gfx_merged),
    .icon_width = GFX_MERGED_WIDTH,
    .icon_height = GFX_MERGED_HEIGHT,
    .activated = NULL,
    .deactivated = &deactivated,
    .enable = &merged_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &merged_tick,
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
    .items = { (ui_item_t*) &merged_voltage, (ui_item_t*) &merged_current }
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
        if (ivalue < merged_voltage.min || ivalue > merged_voltage.max) {
            emu_printf("[MERGED] Voltage %d is out of range (min:%d max:%d)\n", ivalue, merged_voltage.min, merged_voltage.max);
            return ps_range_error;
        }
        emu_printf("[MERGED] Setting voltage to %d\n", ivalue);
        merged_voltage.value = ivalue;
        voltage_changed(&merged_voltage);
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < merged_current.min || ivalue > merged_current.max) {
            emu_printf("[MERGED] Current %d is out of range (min:%d max:%d)\n", ivalue, merged_current.min, merged_current.max);
            return ps_range_error;
        }
        emu_printf("[MERGED] Setting current to %d\n", ivalue);
        merged_current.value = ivalue;
        current_changed(&merged_current);
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
        (void) mini_snprintf(value, value_len, "%d", (pwrctl_vout_enabled() ? saved_u : merged_voltage.value));
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", pwrctl_vout_enabled() ? saved_i : merged_current.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void merged_enable(bool enabled)
{
    emu_printf("[MERGED] %s output\n", enabled ? "Enable" : "Disable");
    if (enabled) {
        /** Display will now show the current values, keep the user setting saved */
        saved_u = merged_voltage.value;
        saved_i = merged_current.value;
        (void) pwrctl_set_vout(merged_voltage.value);
        (void) pwrctl_set_iout(merged_current.value);
        (void) pwrctl_set_ilimit(0xFFFF); /** Set the current limit to the maximum to prevent OCP (over current protection) firing */
        pwrctl_enable_vout(true);
    } else {
        pwrctl_enable_vout(false);
        /** Make sure we're displaying the settings and not the current
          * measurements when the power output is switched off */
        merged_voltage.value = saved_u;
        merged_voltage.ui.draw(&merged_voltage.ui);
        merged_current.value = saved_i;
        merged_current.ui.draw(&merged_current.ui);

        /** Ensure the CC or CV logo has been cleared from the screen */
        if (current_mode_gfx == CUR_GFX_CV) {
            tft_fill(30, 128 - GFX_CV_HEIGHT, GFX_CV_WIDTH, GFX_CV_HEIGHT, BLACK);
        } else if (current_mode_gfx == CUR_GFX_CC) {
            tft_fill(30, 128 - GFX_CC_HEIGHT, GFX_CC_WIDTH, GFX_CC_HEIGHT, BLACK);
        }
        current_mode_gfx = CUR_GFX_NOT_DRAWN;
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
 * @brief      Do any required clean up before changing away from this screen
 */
static void deactivated(void)
{
    /** Ensure the CC or CV logo has been cleared from the screen */
    if (current_mode_gfx == CUR_GFX_CV) {
        tft_fill(30, 128 - GFX_CV_HEIGHT, GFX_CV_WIDTH, GFX_CV_HEIGHT, BLACK);
    } else if (current_mode_gfx == CUR_GFX_CC) {
        tft_fill(30, 128 - GFX_CC_HEIGHT, GFX_CC_WIDTH, GFX_CC_HEIGHT, BLACK);
    }
    current_mode_gfx = CUR_GFX_NOT_DRAWN;
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    /** @todo: past bug causes corruption for units smaller than 4 bytes (#27) */
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_U, (void*) &saved_u, 4 /* sizeof(merged_voltage.value) */ )) {
        /** @todo: handle past write failures */
    }
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4 /* sizeof(merged_current.value) */ )) {
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
        saved_u = merged_voltage.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = merged_current.value = *p;
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
static void merged_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    /** Continously update max voltage output value
      * Max output voltage = Vin / VIN_VOUT_RATIO
      * Add 0.5f to ensure correct rounding when truncated */
    merged_voltage.max = (float) pwrctl_calc_vin(v_in_raw) / VIN_VOUT_RATIO + 0.5f;
    if (pwrctl_vout_enabled()) {

        int32_t vout_actual = pwrctl_calc_vout(v_out_raw);
        int32_t cout_actual = pwrctl_calc_iout(i_out_raw);

        if (merged_voltage.ui.has_focus) {
            /** If the voltage setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (merged_voltage.value != saved_u) {
                merged_voltage.value = saved_u;
                merged_voltage.ui.draw(&merged_voltage.ui);
            }
        } else {
            /** No focus, update display if necessary */
            if (merged_voltage.value != vout_actual) {
                merged_voltage.value = vout_actual;
                merged_voltage.ui.draw(&merged_voltage.ui);
            }
        }

        if (merged_current.ui.has_focus) {
            /** If the current setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (merged_current.value != saved_i) {
                merged_current.value = saved_i;
                merged_current.ui.draw(&merged_current.ui);
            }
        } else {
            /** No focus, update display if necessary */
            if (merged_current.value != cout_actual) {
                merged_current.value = cout_actual;
                merged_current.ui.draw(&merged_current.ui);
            }
        }

        /** Determine if we are in CV or CC mode and display it */
        int32_t vout_diff = abs(saved_u - vout_actual);
        int32_t cout_diff = abs(saved_i - cout_actual);

        if (cout_diff < vout_diff) {
            if (current_mode_gfx != CUR_GFX_CC) {
                tft_blit((uint16_t*) gfx_cc, GFX_CC_WIDTH, GFX_CC_HEIGHT, 30, 128 - GFX_CC_HEIGHT);
                current_mode_gfx = CUR_GFX_CC;
            }
        } else {
            if (current_mode_gfx != CUR_GFX_CV) {
                tft_blit((uint16_t*) gfx_cv, GFX_CV_WIDTH, GFX_CV_HEIGHT, 30, 128 - GFX_CV_HEIGHT);
                current_mode_gfx = CUR_GFX_CV;
            }
        }
    }
}

/**
 * @brief      Initialise the MERGED module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_merged_init(uui_t *ui)
{
    merged_voltage.value = 0; /** read from past */
    merged_current.value = 0; /** read from past */
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    merged_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */
    number_init(&merged_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    merged_voltage.cur_digit = 2;
    number_init(&merged_current);
    uui_add_screen(ui, &merged_screen);
}
