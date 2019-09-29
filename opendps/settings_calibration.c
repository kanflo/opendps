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
#include <dac.h>
#include "gfx-crosshair.h"
#include "hw.h"
#include "settings_calibration.h"
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

static void calibration_enable(bool _enable);
static void v_dac_changed(ui_number_t *item);
static void a_dac_changed(ui_number_t *item);
static void calibration_tick(void);
static void activated(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

#define SCREEN_ID  (3)

/* This is the definition of the voltage ADC item in the UI */
ui_number_t calibration_v_dac = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 121,
        .y = 10,
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = 0,
    .max = 4095,
    .si_prefix = si_none,
    .num_digits = 4,
    .num_decimals = 0,
    .unit = unit_none,
    .changed = &v_dac_changed,
};

/* This is the definition of the current DAC item in the UI */
ui_number_t calibration_a_dac = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 121,
        .y = 28,
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 2,
    .min = 0,
    .max = 4095,
    .si_prefix = si_none,
    .num_digits = 4,
    .num_decimals = 0,
    .unit = unit_none,
    .changed = &a_dac_changed,
};

/* This is the definition of the voltage ADC item in the UI */
ui_number_t calibration_vin_adc = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 121,
        .y = 46,
        .can_focus = false,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = 0,
    .max = 4095,
    .si_prefix = si_none,
    .num_digits = 4,
    .num_decimals = 0,
    .unit = unit_none,
    .changed = NULL,
};

/* This is the definition of the voltage ADC item in the UI */
ui_number_t calibration_v_adc = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 121,
        .y = 64,
        .can_focus = false,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = 0,
    .max = 4095,
    .si_prefix = si_none,
    .num_digits = 4,
    .num_decimals = 0,
    .unit = unit_none,
    .changed = NULL,
};

/* This is the definition of the current ADC item in the UI */
ui_number_t calibration_a_adc = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 121,
        .y = 82,
        .can_focus = false,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = 0,
    .max = 4095,
    .si_prefix = si_none,
    .num_digits = 5,
    .num_decimals = 0,
    .unit = unit_none,
    .changed = NULL,
};

/* This is the screen definition */
ui_screen_t calibration_screen = {
    .id = SCREEN_ID,
    .name = "calibration",
    .icon_data = (uint8_t *) gfx_crosshair,
    .icon_data_len = sizeof(gfx_crosshair),
    .icon_width = GFX_CROSSHAIR_WIDTH,
    .icon_height = GFX_CROSSHAIR_HEIGHT,
    .activated = &activated,
    .deactivated = NULL,
    .enable = &calibration_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &calibration_tick,
    .set_parameter = &set_parameter,
    .get_parameter = &get_parameter,
    .num_items = 5,
    .parameters = {
        {
            .name = "V_DAC",
            .unit = unit_none,
            .prefix = si_none
        },
        {
            .name = "A_DAC",
            .unit = unit_none,
            .prefix = si_none
        },
        {
            .name = {'\0'} /** Terminator */
        },
    },
    .items = { (ui_item_t*) &calibration_v_dac,
               (ui_item_t*) &calibration_a_dac,
               (ui_item_t*) &calibration_vin_adc,
               (ui_item_t*) &calibration_v_adc,
               (ui_item_t*) &calibration_a_adc }
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
    if (strcmp("V_DAC", name) == 0) {
        if (ivalue < calibration_v_dac.min || ivalue > calibration_v_dac.max) {
            emu_printf("[Calibration] V_DAC %d is out of range (min:%d max:%d)\n", ivalue, calibration_v_dac.min, calibration_v_dac.max);
            return ps_range_error;
        }
        emu_printf("[Calibration] Setting V_DAC to %d\n", ivalue);
        calibration_v_dac.value = ivalue;
        v_dac_changed(&calibration_v_dac);
        return ps_ok;
    } else if (strcmp("A_DAC", name) == 0) {
        if (ivalue < calibration_a_dac.min || ivalue > calibration_a_dac.max) {
            emu_printf("[Calibration] A_DAC %d is out of range (min:%d max:%d)\n", ivalue, calibration_a_dac.min, calibration_a_dac.max);
            return ps_range_error;
        }
        emu_printf("[Calibration] Setting A_DAC to %d\n", ivalue);
        calibration_a_dac.value = ivalue;
        a_dac_changed(&calibration_a_dac);
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
    if (strcmp("V_DAC", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", calibration_v_dac.value);
        return ps_ok;
    } else if (strcmp("A_DAC", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", calibration_a_dac.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void calibration_enable(bool enabled)
{
    emu_printf("[Calibration] %s output\n", enabled ? "Enable" : "Disable");
    if (enabled) {
        pwrctl_set_vlimit(0xFFFF); /** Set the voltage limit to the maximum to prevent OVP (over voltage protection) firing */
        pwrctl_set_ilimit(0xFFFF); /** Set the current limit to the maximum to prevent OCP (over current protection) firing */
        pwrctl_enable_vout(true);
        hw_set_voltage_dac(calibration_v_dac.value);
        hw_set_current_dac(calibration_a_dac.value);
    } else {
        pwrctl_enable_vout(false);
    }
}

/**
 * @brief      Callback for when value of the voltage DAC item is changed
 *
 * @param      item  The voltage item
 */
static void v_dac_changed(ui_number_t *item)
{
    if (calibration_screen.is_enabled)
        hw_set_voltage_dac(item->value);
}

/**
 * @brief      Callback for when value of the current DAC item is changed
 *
 * @param      item  The current item
 */
static void a_dac_changed(ui_number_t *item)
{
    if (calibration_screen.is_enabled)
        hw_set_current_dac(item->value);
}

/**
 * @brief      Set up any static graphics when the screen is first drawn
 */
static void activated(void)
{
    tft_puts(FONT_FULL_SMALL, "Vout DAC:", 6, 22, 64, 20, WHITE, false);
    tft_puts(FONT_FULL_SMALL, "Iout DAC:", 6, 40, 64, 20, WHITE, false);
    tft_puts(FONT_FULL_SMALL, "Vin ADC:" , 6, 58, 64, 20, WHITE, false);
    tft_puts(FONT_FULL_SMALL, "Vout ADC:", 6, 76, 64, 20, WHITE, false);
    tft_puts(FONT_FULL_SMALL, "Iout ADC:", 6, 94, 64, 20, WHITE, false);
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    (void) past;
}

/**
 * @brief      Restore persistent parameters
 *
 * @param      past  The past
 */
static void past_restore(past_t *past)
{
    (void) past;
}

/**
 * @brief      Update the UI. We need to be careful about the values shown
 *             as they will differ depending on the current state of the UI
 *             and the current power output mode.
 *             Power off: always show current setting
 *             Power on : show current output value unless the item has focus
 *                        in which case we shall display the current setting.
 */
static void calibration_tick(void)
{
    /** Continously update the ADC displayed values */
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);

    if (v_in_raw != calibration_vin_adc.value) {
        calibration_vin_adc.value = v_in_raw;
        calibration_vin_adc.ui.draw(&calibration_vin_adc.ui);
    }

    if (v_out_raw != calibration_v_adc.value) {
        calibration_v_adc.value = v_out_raw;
        calibration_v_adc.ui.draw(&calibration_v_adc.ui);
    }

    if (i_out_raw != calibration_a_adc.value) {
        calibration_a_adc.value = i_out_raw;
        calibration_a_adc.ui.draw(&calibration_a_adc.ui);
    }
}

/**
 * @brief      Initialise the CV module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void settings_calibration_init(uui_t *ui)
{
    number_init(&calibration_v_dac);
    number_init(&calibration_a_dac);
    number_init(&calibration_vin_adc);
    number_init(&calibration_v_adc);
    number_init(&calibration_a_adc);

    uui_add_screen(ui, &calibration_screen);
}
