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
#include "font-meter_large.h"
#include "hw.h"
#include "func_dpsmode.h"
#include "uui.h"
#include "uui_number.h"
#include "dbg_printf.h"
#include "mini-printf.h"
#include "dps-model.h"
#include "ili9163c.h"
#include "opendps.h"

/*
 * This is the implementation of the CV screen. It has two editable values,
 * constant voltage and current limit. When power is enabled it will continously
 * display the current output voltage and current draw. If the user edits one
 * of the values when power is eabled, the other will continue to be updated.
 * Thid allows for ramping the voltage and obsering the current increase.
 */

static void dpsmode_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void current_changed(ui_number_t *item);
static void dpsmode_tick(void);
static void activated(void);
static void deactivated(void);
static bool event(uui_t *ui, event_t event);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

static void clear_bars(void);
static void clear_ccbar(void);
static void clear_cvbar(void);

/* We need to keep copies of the user settings as the value in the UI will
 * be replaced with measurements when output is active
 */
static int32_t saved_v, saved_i;

// single edit mode, with M1/M2 buttons, not select.
// pressing any other button when in this mode will exit the edit mode
static bool single_edit_mode;
static bool select_mode;

enum {
    CUR_GFX_NOT_DRAWN = 0, 
    CUR_GFX_CV = 1,
    CUR_GFX_CC = 2,
} dpsmode_graphics; 

#define SCREEN_ID  (6)
#define PAST_U     (0)
#define PAST_I     (1)
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

ui_number_t dpsmode_power = {
    {
        .type = ui_item_number,
        .id = 12,
        .x = XPOS_METER,
        .y = YPOS_POWER,
        .can_focus = false, // TODO: OPP feature, with warning bar when approaching power limit
    },
    .font_size = FONT_METER_LARGE,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = COLOR_WATTAGE,
    .value = 0,
    .min = 0,
    .max = 0,
    .si_prefix = si_micro,
    .num_digits = 2,
    .num_decimals = 2,
    .unit = unit_watt,
    .changed = NULL,
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
    .items = { 
        (ui_item_t*) &dpsmode_voltage, 
        (ui_item_t*) &dpsmode_current, 
        (ui_item_t*) &dpsmode_power
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
            emu_printf("[CL] Voltage %d is out of range (min:%d max:%d)\n", ivalue, dpsmode_voltage.min, dpsmode_voltage.max);
            return ps_range_error;
        }
        emu_printf("[CL] Setting voltage to %d\n", ivalue);
        dpsmode_voltage.value = ivalue;
        voltage_changed(&dpsmode_voltage);
        return ps_ok;
    } else if (strcmp("current", name) == 0 || strcmp("i", name) == 0) {
        if (ivalue < dpsmode_current.min || ivalue > dpsmode_current.max) {
            emu_printf("[CL] Current %d is out of range (min:%d max:%d)\n", ivalue, dpsmode_current.min, dpsmode_current.max);
            return ps_range_error;
        }
        emu_printf("[CL] Setting current to %d\n", ivalue);
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
    emu_printf("[CL] %s output\n", enabled ? "Enable" : "Disable");

    if (enabled) {
        /** Display will now show the current values, keep the user setting saved */
        saved_v = dpsmode_voltage.value;
        saved_i = dpsmode_current.value;
        (void) pwrctl_set_vout(dpsmode_voltage.value);
        (void) pwrctl_set_iout(dpsmode_current.value);
        (void) pwrctl_set_ilimit(0xFFFF); /** Set the current limit to the maximum to prevent OCP (over current protection) firing */
        pwrctl_enable_vout(true);

    } else {
        pwrctl_enable_vout(false);
        /** Make sure we're displaying the settings and not the current
          * measurements when the power output is switched off */

        dpsmode_voltage.value = saved_v;
        dpsmode_voltage.color = COLOR_VOLTAGE;
        dpsmode_voltage.ui.draw(&dpsmode_voltage.ui);

        dpsmode_current.value = saved_i;
        dpsmode_current.color = COLOR_AMPERAGE;
        dpsmode_current.ui.draw(&dpsmode_current.ui);

        dpsmode_power.value = 0;
        dpsmode_power.color = COLOR_WATTAGE;
        dpsmode_power.ui.draw(&dpsmode_power.ui);

        clear_bars();
    }
}

/**
 * @brief      Callback for when value of the voltage item is changed
 *
 * @param      item  The voltage item
 */
static void voltage_changed(ui_number_t *item)
{
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
    saved_i = item->value;
    (void) pwrctl_set_iout(item->value);
}


static bool event(uui_t *ui, event_t event) {

    switch(event) {
        case event_button_m1:
        case event_button_m2:
        case event_button_sel:

            if (single_edit_mode) {
                single_edit_mode = false;

                // toggle focus on anything that is in focus
                if (dpsmode_voltage.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_voltage);
                if (dpsmode_current.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_current);
                return true;
            }

        default:
            break;
    }


    switch(event) {
        case event_button_m1:
            // if in normal select mode, let parent handle it
            if (select_mode) return false;

            // otherwise, enter single edit mode
            single_edit_mode = true;

            // focus on voltage if not already focused
            if ( ! dpsmode_voltage.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_voltage);

            // we handled it, parent should do nothing
            return true;

        case event_button_m2:
            if (select_mode) return false;
            single_edit_mode = true;
            if ( ! dpsmode_current.ui.has_focus) uui_focus(ui, (ui_item_t*) &dpsmode_current);
            return true;

        case event_button_sel:
            // toggle select mode, so parent can deal with other UI elements
            // keep track, so this screen will not do anything until we leave this mode
            select_mode = ! select_mode;
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
}


/**
 * @brief      Do any required clean up before changing away from this screen
 */
static void deactivated(void)
{
    clear_bars();
    tft_clear();
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    /** @todo: past bug causes corruption for units smaller than 4 bytes (#27) */
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_U, (void*) &saved_v, 4 /* sizeof(dpsmode_voltage.value) */ )) {
        /** @todo: handle past write failures */
    }
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_I, (void*) &saved_i, 4 /* sizeof(dpsmode_current.value) */ )) {
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
        saved_v = dpsmode_voltage.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_I, (const void**) &p, &length)) {
        saved_i = dpsmode_current.value = *p;
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
static void dpsmode_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    /** Continously update max voltage output value
      * Max output voltage = Vin / VIN_VOUT_RATIO
      * Add 0.5f to ensure correct rounding when truncated */
    dpsmode_voltage.max = (float) pwrctl_calc_vin(v_in_raw) / VIN_VOUT_RATIO + 0.5f;

    if (pwrctl_vout_enabled()) {
        int32_t vout_actual = pwrctl_calc_vout(v_out_raw);
        int32_t cout_actual = pwrctl_calc_iout(i_out_raw);

        dpsmode_power.value = vout_actual * cout_actual;
        dpsmode_power.color = COLOR_WATTAGE;
        dpsmode_power.ui.draw(&dpsmode_power.ui);

        if (dpsmode_voltage.ui.has_focus) {
            /** If the voltage setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (dpsmode_voltage.value != saved_v) {
                dpsmode_voltage.value = saved_v;
                dpsmode_voltage.ui.draw(&dpsmode_voltage.ui);
            }
        } else {
            /** No focus, update display if necessary */
            if (dpsmode_voltage.value != vout_actual) {
                dpsmode_voltage.value = vout_actual;
                dpsmode_voltage.color = COLOR_VOLTAGE;
                dpsmode_voltage.ui.draw(&dpsmode_voltage.ui);
            }
        }

        if (dpsmode_current.ui.has_focus) {
            /** If the current setting has focus, make sure we're displaying
              * the desired setting and not the current output value. */
            if (dpsmode_current.value != saved_i) {
                dpsmode_current.value = saved_i;
                dpsmode_current.ui.draw(&dpsmode_current.ui);
            }
        } else {
            /** No focus, update display if necessary */
            if (dpsmode_current.value != cout_actual) {
                dpsmode_current.value = cout_actual;
                dpsmode_current.color = COLOR_AMPERAGE;
                dpsmode_current.ui.draw(&dpsmode_current.ui);
            }
        }

        /** Determine if we are in CV or CC mode and display it */
        int32_t vout_diff = abs(saved_v - vout_actual);
        int32_t cout_diff = abs(saved_i - cout_actual);

        if (cout_diff < vout_diff) {
            // current diff smaller than voltage diff (constant current)
            if (dpsmode_graphics & CUR_GFX_CV) {
                clear_cvbar();
            }

            if ((dpsmode_graphics & CUR_GFX_CC) == 0) {
                dpsmode_graphics |= CUR_GFX_CC;

                // mine, draw cc bar beside current
                tft_blit((uint16_t*) gfx_ccbar,
                        GFX_CCBAR_WIDTH, GFX_CCBAR_HEIGHT,
                        TFT_WIDTH - GFX_CCBAR_WIDTH,
                        YPOS_CURRENT + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CCBAR_HEIGHT );
            }

        } else {
            // current diff larger than voltage diff (constant voltage)

            if (dpsmode_graphics & CUR_GFX_CC) {
                clear_ccbar();
            }

            if ((dpsmode_graphics & CUR_GFX_CV) == 0) {
                dpsmode_graphics |= CUR_GFX_CV;

                // mine, draw cv bar beside voltage
                tft_blit((uint16_t*) gfx_cvbar,
                        GFX_CVBAR_WIDTH, GFX_CVBAR_HEIGHT,
                        TFT_WIDTH - GFX_CVBAR_WIDTH,
                        YPOS_VOLTAGE + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CVBAR_HEIGHT );
            }
        }
    }
}


static void clear_bars() {
    /** Ensure the CC or CV logo has been cleared from the screen */
    if (dpsmode_graphics & CUR_GFX_CV) {
        clear_cvbar();
    }
    if (dpsmode_graphics & CUR_GFX_CC) {
        clear_ccbar();
    }
    dpsmode_graphics = CUR_GFX_NOT_DRAWN;
}

static void clear_ccbar() {
    // clear cc bar
    tft_fill(TFT_WIDTH - GFX_CCBAR_WIDTH, YPOS_CURRENT + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CCBAR_HEIGHT,
                GFX_CCBAR_WIDTH, GFX_CCBAR_HEIGHT,
                BLACK);
    dpsmode_graphics = dpsmode_graphics & ~CUR_GFX_CC;
}

static void clear_cvbar() {
    // clear cv bar
    tft_fill(TFT_WIDTH - GFX_CVBAR_WIDTH, YPOS_VOLTAGE + FONT_METER_LARGE_MAX_GLYPH_HEIGHT - GFX_CVBAR_HEIGHT,
                GFX_CVBAR_WIDTH, GFX_CVBAR_HEIGHT,
                BLACK);
    dpsmode_graphics = dpsmode_graphics & ~CUR_GFX_CV;
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
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;

    dpsmode_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */

    number_init(&dpsmode_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    dpsmode_voltage.cur_digit = 2;

    number_init(&dpsmode_current);
    number_init(&dpsmode_power);

    uui_add_screen(ui, &dpsmode_screen);
}
