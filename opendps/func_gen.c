/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Cyril Russo (github.com/X-Ryl669)
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
#include "gfx-sin.h"
#include "gfx-saw.h"
#include "gfx-square.h"
#include "hw.h"
#include "func_gen.h"
#include "uui.h"
#include "uui_number.h"
#include "uui_icon.h"
#include "dbg_printf.h"
#include "mini-printf.h"
#include "dps-model.h"
#include "ili9163c.h"
#include "font-full_small.h"

/* We are reacting on the ADC's IRQ which runs at ~20kHz.
 * As such we want at least 20 points per period so the shape of the function is 
 * "clean", so we limit generation to 1kHz. 
 */
#define MAX_FREQUENCY   999

/* The basic generator function that's selected at runtime */
typedef int32_t (*compute_func_t)(uint32_t time_in_us, uint32_t period_in_us, int32_t max);

/*
 * This is the implementation of the function generator screen. It has three editable values,
 * voltage, frequency and function type. */
static void    func_gen(void);
static int16_t sin1(int16_t angle);
static int32_t square_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max);
static int32_t saw_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max);
static int32_t sin_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max);

/* The basic generator function that's selected at runtime */
static compute_func_t compute_func = &square_gen;

static void funcgen_enable(bool _enable);
static void voltage_changed(ui_number_t *item);
static void frequency_changed(ui_number_t *item);
static void func_changed(ui_icon_t *item);
static void func_gen_tick(void);
static void compute_period_from_freq(int32_t freq);
static void activated(void);
static void deactivated(void);
static void past_save(past_t *past);
static void past_restore(past_t *past);
static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

/* We need to keep copies of the period to avoid recomputing it every time. */
static uint32_t period_us;

#define SCREEN_ID  (5)
#define PAST_U     (0)
#define PAST_P     (1)
#define PAST_F     (2)

/* This is the definition of the voltage item in the UI */
ui_number_t gen_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 120,
        .y = 15,
        .can_focus = true,
    },
    .font_size = FONT_METER_MEDIUM, /** The bigger one, try FONT_SMALL or FONT_MEDIUM for kicks */
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

/* This is the definition of the frequency item in the UI */
ui_number_t gen_freq = {
    {
        .type = ui_item_number,
        .id = 11,
        .x = 120,
        .y = 42,
        .can_focus = true,
    },
    .font_size = FONT_METER_MEDIUM,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = COLOR_AMPERAGE,
    .value = 0,
    .min = 0,
    .max = MAX_FREQUENCY * 10, /* In dHz */
    .si_prefix = si_deci,
    .num_digits = 3,
    .num_decimals = 1,
    .unit = unit_hertz,
    .changed = &frequency_changed,
};

/* This is the definition of the function item in the UI */
ui_icon_t gen_func = {
    {
        .type = ui_item_icon,
        .id = 12,
        .x = 120 - GFX_SQUARE_WIDTH,
        .y = 69,
        .can_focus = true,
    },
    .icons_data_len = sizeof(gfx_square),
    .icons_width = GFX_SQUARE_WIDTH,
    .icons_height = GFX_SQUARE_HEIGHT,
    .value = 0,
    .num_icons = 3,
    .changed = &func_changed,
    .icons = { gfx_square, gfx_saw, gfx_sin}
};

/* This is the screen definition */
ui_screen_t gen_screen = {
    .id = SCREEN_ID,
    .name = "funcgen",
    .icon_data = (uint8_t *) gfx_sin,
    .icon_data_len = sizeof(gfx_sin),
    .icon_width = GFX_SIN_WIDTH,
    .icon_height = GFX_SIN_HEIGHT,
    .activated = &activated,
    .deactivated = &deactivated,
    .enable = &funcgen_enable,
    .past_save = &past_save,
    .past_restore = &past_restore,
    .tick = &func_gen_tick,
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
            .name = "freq",
            .unit = unit_hertz,
            .prefix = si_deci
        },
        {
            .name = "func",
            .unit = unit_none,
            .prefix = si_none
        },
        {
            .name = {'\0'} /** Terminator */
        },
    },
    .items = { (ui_item_t*) &gen_voltage, (ui_item_t*) &gen_freq, (ui_item_t*) &gen_func }
};

/**
 * @brief      Compute a square signal as selected by the user
 *
 * @param[in]  time   the current clock in microsecond 
 * @param[in]  period period of the generated signal in microsecond (frequency in range 0 to 5KHz, period is in range 200 to 1<<31)
 * @param[in]  max    the amplitude of the voltage for this generator
 *
 * @retval     int32_t the voltage to apply
 */
static int32_t square_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max)
{
    if (!period_in_us) return max;
    /** The time is ticking at 1000000Hz so a unit is 1us. The production is 1 for t in [(t%T) ; (t%T+T/2)], 0 otherwise */
    uint32_t t = time_in_us % period_in_us;
    return t <= period_in_us / 2 ? max : 0;
}

/**
 * @brief      Compute a saw signal as selected by the user
 *
 * @param[in]  time   the current clock in microsecond 
 * @param[in]  period period of the generated signal in microsecond (frequency in range 0 to 5KHz, period is in range 200 to 1<<31)
 * @param[in]  max    the amplitude of the voltage for this generator
 *
 * @retval     int32_t the voltage to apply
 */
static int32_t saw_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max)
{
    if (!period_in_us) return max;
    /* The time is ticking at 1000000Hz so a unit is 1us. The production is (max*(t%T))/T */
    uint64_t t = time_in_us % period_in_us;
    /* Here, because the longest period is 10s and max is in millivolt, we can overflow an uint32_t
       Since CortexM3 has a hw 64 bits multiply and divider, it shouldn't cost more to use it */
    return (int32_t)((t * max) / period_in_us);
}

/*
 * The number of bits of our data type: here 16 (sizeof operator returns bytes).
 */
#define INT16_BITS  (8 * sizeof(int16_t))
#ifndef INT16_MAX
#define INT16_MAX   ((1<<(INT16_BITS-1))-1)
#endif
 
/*
 * "5 bit" large table = 32 values. The mask: all bit belonging to the table
 * are 1, the all above 0.
 */
#define TABLE_BITS  (5)
#define TABLE_SIZE  (1<<TABLE_BITS)
#define TABLE_MASK  (TABLE_SIZE-1)
 
/*
 * The lookup table is to 90DEG, the input can be -360 to 360 DEG, where negative
 * values are transformed to positive before further processing. We need two
 * additional bits (*4) to represent 360 DEG:
 */
#define LOOKUP_BITS (TABLE_BITS+2)
#define LOOKUP_MASK ((1<<LOOKUP_BITS)-1)
#define FLIP_BIT    (1<<TABLE_BITS)
#define NEGATE_BIT  (1<<(TABLE_BITS+1))
#define INTERP_BITS (INT16_BITS-1-LOOKUP_BITS)
#define INTERP_MASK ((1<<INTERP_BITS)-1)
 
/**
 * "5 bit" lookup table for the offsets. These are the sines for exactly
 * at 0deg, 11.25deg, 22.5deg etc. The values are from -1 to 1 in Q15.
 */
static int16_t sin90[TABLE_SIZE+1] = {
  0x0000,0x0647,0x0c8b,0x12c7,0x18f8,0x1f19,0x2527,0x2b1e,
  0x30fb,0x36b9,0x3c56,0x41cd,0x471c,0x4c3f,0x5133,0x55f4,
  0x5a81,0x5ed6,0x62f1,0x66ce,0x6a6c,0x6dc9,0x70e1,0x73b5,
  0x7640,0x7883,0x7a7c,0x7c29,0x7d89,0x7e9c,0x7f61,0x7fd7,
  0x7fff
};
 
/**
 * Sine calculation using interpolated table lookup.
 * Instead of radiants or degrees we use "turns" here. Means this
 * sine does NOT return one phase for 0 to 2*PI, but for 0 to 1.
 * Input: -1 to 1 as int16 Q15  == -32768 to 32767.
 * Output: -1 to 1 as int16 Q15 == -32768 to 32767.
 *
 * See the full description at www.AtWillys.de for the detailed
 * explanation.
 *
 * @param int16_t angle Q15
 * @return int16_t Q15
 */
static int16_t sin1(int16_t angle)
{
  int16_t v0, v1;
  if(angle < 0) { angle += INT16_MAX; angle += 1; }
  v0 = (angle >> INTERP_BITS);
  if(v0 & FLIP_BIT) { v0 = ~v0; v1 = ~angle; } else { v1 = angle; }
  v0 &= TABLE_MASK;
  v1 = sin90[v0] + (int16_t) (((int32_t) (sin90[v0+1]-sin90[v0]) * (v1 & INTERP_MASK)) >> INTERP_BITS);
  if((angle >> INTERP_BITS) & NEGATE_BIT) v1 = -v1;
  return v1;
}


/**
 * @brief      Compute a sin signal as selected by the user
 *
 * @param[in]  time   the current clock in microsecond 
 * @param[in]  period period of the generated signal in microsecond (frequency in range 0 to 5KHz, period is in range 200 to 1<<31)
 * @param[in]  max    the amplitude of the voltage for this generator
 *
 * @retval     int32_t the voltage to apply
 */
static int32_t sin_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max)
{
    if (!period_in_us) return max;
    /** The time is ticking at 1000000Hz so a unit is 1us. The production is (max/2) * sin((t%T)/T*2*PI) + (max/2) */
    uint64_t t = time_in_us % period_in_us;
    
    /* Here, because the longest period is 10s and max is in millivolt, we can overflow an uint32_t
       Since CortexM3 has a hw 64 bits multiply and divider, it shouldn't cost more to use it.
       There's a small error in amplitude here to avoid dividing by 32767 */
    return (max/2) * sin1((int16_t)((t * 32768) / period_in_us)) / 32768 + max/2; 
}

/**
 * @brief     Get the new output to apply to the DAC.
 *            This is called in a ISR context, so we have to be as fast as possible here.
 */ 
static void func_gen(void)
{
    /* period_us is updated atomically (it's a word) in the UI's event code and so is max value */
    int32_t v = (*compute_func)(cur_time_us(), period_us, gen_voltage.value);
    (void) pwrctl_set_vout(v);
//    pwrctl_enable_vout(v > 0);
}


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
        if (ivalue < gen_voltage.min || ivalue > gen_voltage.max) {
            emu_printf("[FNCGEN] Voltage %d is out of range (min:%d max:%d)\n", ivalue, gen_voltage.min, gen_voltage.max);
            return ps_range_error;
        }
        emu_printf("[FNCGEN] Setting voltage to %d\n", ivalue);
        gen_voltage.value = ivalue;
        voltage_changed(&gen_voltage);
        return ps_ok;
    } else if (strcmp("freq", name) == 0 || strcmp("f", name) == 0) {
        if (ivalue < gen_freq.min || ivalue > gen_freq.max) {
            emu_printf("[FNCGEN] Frequency %d is out of range (min:%d max:%d)\n", ivalue, gen_freq.min, gen_freq.max);
            return ps_range_error;
        }
        emu_printf("[FNCGEN] Setting frequency to %d\n", ivalue);
        gen_freq.value = ivalue;
        frequency_changed(&gen_freq);
        return ps_ok;
    } else if (strcmp("func", name) == 0 || strcmp("n", name) == 0) {
        if ((uint32_t)ivalue >= gen_func.num_icons) {
            emu_printf("[FNCGEN] Function %d is out of range (min:0 max:%d)\n", ivalue, gen_func.num_icons - 1);
            return ps_range_error;
        }
        emu_printf("[FNCGEN] Setting mode to %d\n", ivalue);
        gen_func.value = ivalue;
        func_changed(&gen_func);
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
        (void) mini_snprintf(value, value_len, "%d", gen_voltage.value);
        return ps_ok;
    } else if (strcmp("freq", name) == 0 || strcmp("f", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", gen_freq.value);
        return ps_ok;
    } else if (strcmp("func", name) == 0 || strcmp("n", name) == 0) {
        (void) mini_snprintf(value, value_len, "%d", gen_func.value);
        return ps_ok;
    }
    return ps_unknown_name;
}

/**
 * @brief       Compute the period in microsecond from the given frequency 
 * @param[in]   freq    Frequency in dHz
 */
static void compute_period_from_freq(int32_t freq)
{
    /* Compute the period to use in microseconds, that's 1e6/freq (and since the frequency is in dHz, needs Hz here) */
    period_us = (uint32_t)(1000000.0 / (freq * 0.1) + .5);
}

/**
 * @brief      Callback for when the function is enabled
 *
 * @param[in]  enabled  true when function is enabled
 */
static void funcgen_enable(bool enabled)
{
    emu_printf("[FNCGEN] %s output\n", enabled ? "Enable" : "Disable");
    if (enabled) {
        compute_period_from_freq(gen_freq.value);
        func_changed(&gen_func);
        /* Draw the current function to the expected position */
        tft_blit((uint16_t*) gen_func.icons[gen_func.value], gen_func.icons_width, gen_func.icons_height, XPOS_ICON, 128 - GFX_SIN_HEIGHT);
        (void) pwrctl_set_vout(gen_voltage.value);
        (void) pwrctl_set_iout(CONFIG_DPS_MAX_CURRENT);
        (void) pwrctl_set_vlimit(0xFFFF);
        (void) pwrctl_set_ilimit(0xFFFF); /** Set the current limit to the maximum to prevent OCP (over current protection) firing */
        pwrctl_enable_vout(true);
        funcgen_tick = &func_gen;
    } else {
        funcgen_tick = &fg_noop;
        (void) pwrctl_set_vout(0);
        pwrctl_enable_vout(false);
        /** Ensure the function logo has been cleared from the screen */
        tft_fill(XPOS_ICON, 128 - GFX_SIN_HEIGHT, GFX_SIN_WIDTH, GFX_SIN_HEIGHT, BLACK);
    }
}

/**
 * @brief      Callback for when value of the voltage item is changed
 *
 * @param      item  The voltage item
 */
static void voltage_changed(ui_number_t * item)
{
    (void)item;
}

/**
 * @brief      Callback for when value of the frequency item is changed
 *
 * @param      item  The current item
 */
static void frequency_changed(ui_number_t *item)
{
    compute_period_from_freq(item->value);
}

/**
 * @brief      Callback for when value of the function item is changed
 *
 * @param      item  The current item
 */
static void func_changed(ui_icon_t *item)
{
    static compute_func_t funcs[] = { &square_gen, &saw_gen, &sin_gen, 0 };
    compute_func = funcs[item->value];
}

/**
 * @brief      Do any required clean up before changing away from this screen
 */
static void activated(void)
{
    /** The screen is different here, let's clear it */
    tft_clear();
    for (uint32_t i = 0; i < gen_screen.num_items; i++) {
        gen_screen.items[i]->draw(gen_screen.items[i]);
    }

    tft_puts(FONT_FULL_SMALL, "Vout:", 6, 15+FONT_FULL_SMALL_MAX_GLYPH_HEIGHT, 64, 20, WHITE, false);
    tft_puts(FONT_FULL_SMALL, "Freq:", 6, 42+FONT_FULL_SMALL_MAX_GLYPH_HEIGHT, 64, 20, WHITE, false);
    tft_puts(FONT_FULL_SMALL, "Func:", 6, 69+FONT_FULL_SMALL_MAX_GLYPH_HEIGHT, 64, 20, WHITE, false);
}


/**
 * @brief      Do any required clean up before changing away from this screen
 */
static void deactivated(void)
{
    /** Ensure the sin logo has been cleared from the screen */
    tft_clear();
}

/**
 * @brief      Save persistent parameters
 *
 * @param      past  The past
 */
static void past_save(past_t *past)
{
    int32_t t = gen_voltage.value;
    /** @todo: past bug causes corruption for units smaller than 4 bytes (#27) */
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_U, (void*) &t, 4 /* sizeof(gen_voltage.value) */ )) {
        /** @todo: handle past write failures */
    }
    t = gen_freq.value;
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_P, (void*) &t, 4 /* sizeof(gen_freq.value) */ )) {
        /** @todo: handle past write failures */
    }
    t = gen_func.value;
    if (!past_write_unit(past, (SCREEN_ID << 24) | PAST_F, (void*) &t, 4 /* sizeof(gen_freq.value) */ )) {
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
        gen_voltage.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_P, (const void**) &p, &length)) {
        gen_freq.value = *p;
        (void) length;
    }
    if (past_read_unit(past, (SCREEN_ID << 24) | PAST_F, (const void**) &p, &length)) {
        gen_func.value = *p;
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
static void func_gen_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    /** Continously update max voltage output value
      * Max output voltage = Vin / VIN_VOUT_RATIO
      * Add 0.5f to ensure correct rounding when truncated */
    gen_voltage.max = (float) pwrctl_calc_vin(v_in_raw) / VIN_VOUT_RATIO + 0.5f;
 //   if (gen_voltage.value > gen_voltage.max) 
 //       gen_voltage.value = gen_voltage.max;
}

/**
 * @brief      Initialise the CL module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_gen_init(uui_t *ui)
{
    gen_voltage.value = 0; /** read from past */
    gen_freq.value = 0; /** read from past */
    gen_func.value = 0;
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    gen_voltage.max = pwrctl_calc_vin(v_in_raw); /** @todo: subtract for LDO */
    number_init(&gen_voltage); /** @todo: add guards for missing init calls */
    /** Start at the second most significant digit preventing the user from
        accidentally cranking up the setting 10V or more */
    gen_voltage.cur_digit = 2;
    number_init(&gen_freq);
    icon_init(&gen_func);
    uui_add_screen(ui, &gen_screen);
}
