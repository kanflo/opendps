/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019
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
#include "func_settings.h"
#include "uui.h"
#include "uui_number.h"
#include "dbg_printf.h"
#include "mini-printf.h"
#include "dps-model.h"
#include "ili9163c.h"
#include "font-full_small.h"
#include "opendps.h"
#include "gfx-crosshair.h"
#include "pastunits.h"
#include "opendps.h"


#define SCREEN_ID  (7)


/*
 * This is the implementation of the Settings screen.
 */

static void settings_enable(bool _enable);
static void settings_tick(void);
static void settings_reset(void);

static void past_save(past_t *past);
static void past_restore(past_t *past);

static bool event(uui_t *ui, event_t event, uint8_t data);
static void field_changed(ui_number_t *item);
static void set_page(int8_t page);

static set_param_status_t set_parameter(char *name, char *value);
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len);

static bool select_mode;

// want to calibrate V_ADC,DAC  A_ADC,DAC   VIN_DAC   Brightness,  refresh timing
// so 12 fields in total...
// possibly want OCP, OVP, OPP?

static int32_t get_v_adc_k() {
    return v_adc_k_coef * 1000;
}
static void set_v_adc_k(ui_number_t *item) {
    v_adc_k_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}
static int32_t get_v_adc_c() {
    return v_adc_c_coef * 1000;
}
static void set_v_adc_c(ui_number_t *item) {
    v_adc_c_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}


static int32_t get_v_dac_k() {
    return v_dac_k_coef * 1000;
}
static void set_v_dac_k(ui_number_t *item) {
    v_adc_k_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}
static int32_t get_v_dac_c() {
    return v_dac_c_coef * 1000;
}
static void set_v_dac_c(ui_number_t *item) {
    v_adc_c_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}



static int32_t get_a_adc_k() {
    return a_adc_k_coef * 1000;
}
static void set_a_adc_k(ui_number_t *item) {
    a_adc_k_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}
static int32_t get_a_adc_c() {
    return a_adc_c_coef * 1000;
}
static void set_a_adc_c(ui_number_t *item) {
    a_adc_c_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}


static int32_t get_a_dac_k() {
    return a_dac_k_coef * 1000;
}
static void set_a_dac_k(ui_number_t *item) {
    a_adc_k_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}
static int32_t get_a_dac_c() {
    return a_dac_c_coef * 1000;
}
static void set_a_dac_c(ui_number_t *item) {
    a_adc_c_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}


static int32_t get_vin_adc_k() {
    return vin_adc_k_coef * 1000;
}
static void set_vin_adc_k(ui_number_t *item) {
    vin_adc_k_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}
static int32_t get_vin_adc_c() {
    return vin_adc_c_coef * 1000;
}
static void set_vin_adc_c(ui_number_t *item) {
    vin_adc_c_coef = item->value / (item->color == RED) ? -1000.0f : 1000.0f;
}


static int32_t get_brightness() {
    return (hw_get_backlight() / 1.28f) * 1000;
}
static void set_brightness(ui_number_t *item) {
    if (item->value > 100 * 1000) item->value = 100 * 1000;
    if (item->value < 0) item->value = 0;

    hw_set_backlight((item->value / 1000.0f) * 1.28f);
}

static int32_t get_refresh() {
    return opendps_update_interval * 1000;
}
static void set_refresh(ui_number_t *item) {
    // todo: make this persistent
    opendps_update_interval = item->value / 1000;

    // ensure sane values
    if (opendps_update_interval > 1000) {
        opendps_update_interval = 1000;
    }
    if (opendps_update_interval < 10) {
        opendps_update_interval = 10;
    }
}



typedef void (*set_func)(struct ui_number_t *item);
typedef int32_t (*get_func)();

#define ITEMS_PER_PAGE 6
#define ITEMS 12
#define PAGES 2 // 12 / 6  = 2 pages worth
#define ROW_HEIGHT 17 
#define FIELD_Y_OFFSET 0 // 15
#define FIELD_X_OFFSET 128 // 64 // half of screen

// which page we are currently on.
static int8_t current_page = 0;
static int8_t current_item = 0; // 0 through 4

get_func get_functions[] = {
    &get_v_adc_k,
    &get_v_adc_c,
    &get_v_dac_k,
    &get_v_dac_c,
    &get_a_adc_k,
    &get_a_adc_c,
    &get_a_dac_k,
    &get_a_dac_c,
    &get_vin_adc_k,
    &get_vin_adc_c,
    &get_brightness,
    &get_refresh
};

set_func set_functions[] = {
    &set_v_adc_k,
    &set_v_adc_c,
    &set_v_dac_k,
    &set_v_dac_c,
    &set_a_adc_k,
    &set_a_adc_c,
    &set_a_dac_k,
    &set_a_dac_c,
    &set_vin_adc_k,
    &set_vin_adc_c,
    &set_brightness,
    &set_refresh
};

// fields that can be changed
const char* const field_label[] = {
    "V ADC K",
    "V ADC C",
    "V DAC K",
    "V DAC C",
    "I ADC K",
    "I ADC C",
    "I DAC K",
    "I DAC C",
    "Vin ADC C",
    "Vin ADC K",
    "Scr-LED",
    "Refresh",
};


ui_number_t settings_field[] = {
{
    {
        .type = ui_item_number,
        .id = 10,
        .x = FIELD_X_OFFSET,
        .y = FIELD_Y_OFFSET,
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = -1000000,
    .max = 1000000,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 3,
    .unit = unit_none,
    .changed = &field_changed,
},
{
    {
        .type = ui_item_number,
        .id = 11,
        .x = FIELD_X_OFFSET,
        .y = FIELD_Y_OFFSET + (1 * ROW_HEIGHT),
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = -1000000,
    .max = 1000000,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 3,
    .unit = unit_none,
    .changed = &field_changed,
},
{
    {
        .type = ui_item_number,
        .id = 12,
        .x = FIELD_X_OFFSET,
        .y = FIELD_Y_OFFSET + (2*ROW_HEIGHT),
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = -1000000,
    .max = 1000000,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 3,
    .unit = unit_none,
    .changed = &field_changed,
},
{
    {
        .type = ui_item_number,
        .id = 13,
        .x = FIELD_X_OFFSET,
        .y = FIELD_Y_OFFSET + (3*ROW_HEIGHT),
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = -1000000,
    .max = 1000000,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 3,
    .unit = unit_none,
    .changed = &field_changed,
},
{
    {
        .type = ui_item_number,
        .id = 14,
        .x = FIELD_X_OFFSET,
        .y = FIELD_Y_OFFSET + (4*ROW_HEIGHT),
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = -1000000,
    .max = 1000000,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 3,
    .unit = unit_none,
    .changed = &field_changed,
},
{
    {
        .type = ui_item_number,
        .id = 15,
        .x = FIELD_X_OFFSET,
        .y = FIELD_Y_OFFSET + (5*ROW_HEIGHT),
        .can_focus = true,
    },
    .font_size = FONT_FULL_SMALL,
    .alignment = ui_text_right_aligned,
    .pad_dot = false,
    .color = WHITE,
    .value = 0,
    .min = -1000000,
    .max = 1000000,
    .si_prefix = si_milli,
    .num_digits = 3,
    .num_decimals = 3,
    .unit = unit_none,
    .changed = &field_changed,
}
};


ui_screen_t settings_screen = {
    .id = SCREEN_ID,
    .name = "settings",
    .icon_data = (uint8_t *) gfx_crosshair,
    .icon_data_len = sizeof(gfx_crosshair),
    .icon_width = GFX_CROSSHAIR_WIDTH,
    .icon_height = GFX_CROSSHAIR_HEIGHT,
    .event = event,
    .activated = NULL,
    .deactivated = NULL,
    .enable = &settings_enable,
    .past_save = &past_save,
    .past_restore = NULL,
    .tick = &settings_tick,
    .num_items = ITEMS_PER_PAGE,
    .items = {
        (ui_item_t*) &settings_field[0], 
        (ui_item_t*) &settings_field[1], 
        (ui_item_t*) &settings_field[2], 
        (ui_item_t*) &settings_field[3], 
        (ui_item_t*) &settings_field[4],
        (ui_item_t*) &settings_field[5]
    },
    .set_parameter = &set_parameter,
    .get_parameter = &get_parameter,
    .parameters = {
        {
            .name = {'\0'} /** Terminator */
        },
    },
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
static set_param_status_t get_parameter(char *name, char *value, uint32_t value_len) {
    return ps_unknown_name;
}


static bool event(uui_t *ui, event_t event, uint8_t data) {
    switch (event) {
        case event_button_m1:
        case event_button_m2:
            if ( ! select_mode) {
                return false;
            }

            // go up
            if ( event == event_button_m1 ) {
                // not first item, move up one
                if (current_item > 0) {
                    current_item--;
                    return false;
                }

                // we are left with current_item == 0.
                
                if (current_page == 0) {
                    // do nothing for first page/first item
                    return true;
                }

                // current_item == 0, and current_page != 0
                set_page(current_page - 1);
                current_item = ITEMS_PER_PAGE - 1;
                return false;

            // go down
            } else {
                // not last item, go down
                if (current_item < ITEMS_PER_PAGE - 1) {
                    // do nothing if we are on the last item already
                    if ((current_page * ITEMS_PER_PAGE) + current_item >= ITEMS - 1)
                        return true;

                    // otherwise, go down one item
                    current_item++;
                    return false;
                }

                // we are left with current_item == last item

                if (current_page == PAGES - 1) {
                    // last page. do nothing
                    return true;
                }

                // last item, but not last page
                set_page(current_page + 1);
                current_item = 0;
                return false;
            }

        case event_button_sel:
            // long SET press will reest all values
            if (data == press_long) {
                settings_reset();
                // update page to update values
                set_page(current_page);
                return true;
            }

            select_mode = ! select_mode;
            return false;

        default:
            return false;
    }

    return false;
}

// generic field change function called by all fields
// the current item is used to determine the actual field being changed
static void field_changed(ui_number_t *item) {
    int8_t page_offset = current_page * ITEMS_PER_PAGE;

    if (page_offset + current_item >= ITEMS) {
        // nope
        return;
    }

    // call the appropriate set function
    set_functions[page_offset + current_item](item);
}

// called whenever the page is changed
static void set_page(int8_t page) {
    current_page = page;

    int8_t page_offset = current_page * ITEMS_PER_PAGE;

    // draw each field with its corresponding label
    for (uint8_t i = 0; i < ITEMS_PER_PAGE; i++) {
        // if greater than total number of items, clear the area
        if (page_offset + i >= ITEMS) {
            break;
        }
       
        // update field value using value from the get function
        int32_t value = get_functions[page_offset + i]();
        if (value < 0) {
            // because uui_number doesn't support signed values (i think??)
            // color represents the sign
            settings_field[i].value = -value;
            settings_field[i].color = RED;
        } else {
            settings_field[i].value = value;
            settings_field[i].color = WHITE;
        }
    }

    // debug:
    tft_clear();
}

static void settings_enable(bool enabled) {
}

static void settings_tick(void) {
    int8_t page_offset = current_page * ITEMS_PER_PAGE;

    // draw each field with its corresponding label
    for (uint8_t i = 0; i < ITEMS_PER_PAGE; i++) {
        uint16_t x = settings_field[i].ui.x;
        uint16_t y = settings_field[i].ui.y;


        // if greater than total number of items, clear the area
        if (page_offset + i >= ITEMS) {
            tft_fill(0 /* x */, i * ROW_HEIGHT /* y */,
                TFT_WIDTH, ROW_HEIGHT,
                BLACK);
        } else {
            tft_puts(FONT_FULL_SMALL, 
                    field_label[page_offset + i], 
                    0 /* x */,    (i * ROW_HEIGHT) + FONT_FULL_SMALL_MAX_GLYPH_HEIGHT  /* y */ ,
                    TFT_WIDTH/2, FONT_FULL_SMALL_MAX_GLYPH_HEIGHT,
                    WHITE, false);

            settings_field[i].ui.draw(&settings_field[i].ui);
        }
    }
}


static void settings_reset() {
    a_adc_k_coef = A_ADC_K;
    a_adc_c_coef = A_ADC_C;
    a_dac_k_coef = A_DAC_K;
    a_dac_c_coef = A_DAC_C;
    v_adc_k_coef = V_ADC_K;
    v_adc_c_coef = V_ADC_C;
    v_dac_k_coef = V_DAC_K;
    v_dac_c_coef = V_DAC_C;
    vin_adc_k_coef = VIN_ADC_K;
    vin_adc_c_coef = VIN_ADC_C;
}

static void past_save(past_t *past) {
    pwrctl_past_save(past);
}


/**
 * @brief      Initialise the CV module and add its screen to the UI
 *
 * @param      ui    The user interface
 */
void func_settings_init(uui_t *ui) {
    for (uint8_t i = 0; i < ITEMS_PER_PAGE; i++) {
        number_init(&settings_field[i]); 
    }

    // init to page 0
    set_page(0);
    current_item = 0;

    uui_add_screen(ui, &settings_screen);
}
