/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Johan Kanflo (github.com/kanflo)
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

#include "ui.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "tft.h"
#include "tick.h"
#include "pwrctl.h"
#include "ili9163c.h"
#include "graphics.h"
#include "hw.h"
#include "font-0.h"
#include "font-1.h"
#include "gpio.h"
#include "past.h"


/** How ofter we update the measurements in the UI (ms) */
#define UI_UPDATE_INTERVAL_MS  (250)

/** Timeout for waiting for wifi connction (ms) */
#define WIFI_CONNECT_TIMEOUT  (10000)

/** How many 'edit' positions does each unit have? */
#define MAX_EDIT_POSITION_V (2) // a.bc V (note 'a' might be 2 digits)
#define MAX_EDIT_POSITION_A (3) // a.bcd A

/** Blit positions */
#define YPOS_VOLTAGE (15)
#define YPOS_AMPERE  (60)
#define XPOS_WIFI     (4)
#define XPOS_MODE    (28)
#define XPOS_LOCK    (52)
#define XPOS_VIN     (71)

/** Constants describing how certain things on the screen flash when needed */
#define WIFI_CONNECTING_FLASHING_PERIOD  (1000)
#define WIFI_ERROR_FLASHING_PERIOD        (500)
#define WIFI_UPGRADING_FLASHING_PERIOD    (250)
#define LOCK_FLASHING_PERIOD              (250)
#define LOCK_FLASHING_COUNTER               (4)
#define TFT_FLASHING_PERIOD               (100)
#define TFT_FLASHING_COUNTER                (2)

static void update_power_output(void);
static void update_power_mode_status(void);
static void update_input_voltage(void);
static void update_settings_ui(bool update_both);
static uint32_t get_voltage_increment(uint32_t edit_pos);
static uint32_t get_ampere_increment(uint32_t edit_pos);
static void ui_flash(void);
static void read_past_settings(void);
static void write_past_settings(void);
static void check_master_reset(void);

/** UI settings */
static uint16_t bg_color;
static uint32_t ui_width;
static uint32_t ui_height;

/** Currently displayed values in mV / mA */
static uint32_t cur_v_in;
static uint32_t cur_v_out;
static uint32_t cur_i_out;

/** Maximum v_out setting is the input voltge - V_IO_DELTA */
static uint32_t max_v_out;
/** Maximum i_limit setting is depends on your DPS model, eg 5A for the DPS5005 */
static uint32_t max_i_limit;

/** Used to make the screen flash */
static uint32_t tft_flashing_period;
static uint32_t tft_flash_counter;

/** Used for flashing the wifi icon */
static uint32_t wifi_status_flashing_period;
static bool wifi_status_visible;

/** Used for flashing the lock icon */
static uint32_t lock_flashing_period;
static bool lock_visible;
static uint32_t lock_flash_counter;

/** Current icon settings */
static wifi_status_t wifi_status;
static bool is_locked;
static bool is_enabled;

/** Time at wich V_out was enabled */
static uint64_t pwr_start;

/** Last settings written to past */
static uint32_t     last_vout_setting;
static uint32_t     last_ilimit_setting;
static bool         last_tft_inv_setting;
static power_mode_t last_power_mode_setting;

/** Our parameter storage */
static past_t g_past;

/** Power mode */
static power_mode_t power_mode = pm_constant_voltage;

/** Current U/I settings (mV/mA) */
static uint32_t v_setting;
static uint32_t i_setting;

/** X positions at which we will draw digits */
                                   //  1   2   .   3   4   V
const uint32_t voltage_positions[] = { 9, 30, 51, 63, 83, 105};
                                   //  0   .   1   2   3   A
const uint32_t ampere_positions[] =  {10, 30, 40, 60, 80, 105};

                                   //  1   2   .   3   V
const uint32_t bottom_positions[] =   {0,  7, 14, 19, 26}; /// + XPOS_VIN

/** Edit modes */
typedef enum {
    edit_none = 0,
    edit_voltage,
    edit_ampere
} edit_mode_t;

/** Edit state variables */
static edit_mode_t edit_mode;
static edit_mode_t last_edit_mode;
static uint32_t edit_position;

/** Parameters stored in flash */
typedef enum {
    /** stored as [I_limit:16] | [V_out:16] */
    past_power = 1,
    /** stored as 0 or 1 */
    past_tft_inversion,
    /** Current power output mode */
    past_power_mode
} parameter_id_t;


/**
  * @brief Initialize the UI module
  * @param width width of the UI
  * @param height height of the UI
  * @retval none
  */
void ui_init(uint32_t width, uint32_t height)
{
    g_past.blocks[0] = 0x0800f800;
    g_past.blocks[1] = 0x0800fc00;

    if (!past_init(&g_past)) {
        printf("Error: past init failed!\n");
        /** @todo Handle past init failure */
    }

    check_master_reset();

    bg_color = BLACK;
    ui_width = width;
    ui_height = height;
    max_i_limit = CONFIG_DPS_MAX_CURRENT;
    edit_mode = edit_none;

    read_past_settings();

    if (power_mode == pm_constant_current) {
        last_edit_mode = edit_ampere;
    } else {
        last_edit_mode = edit_voltage;
    }
}

/**
  * @brief Handle event
  * @param event the received event
  * @param data optional extra data
  * @retval none
  */
void ui_hande_event(event_t event, uint8_t data)
{
    if (event == event_rot_press && data == press_long) {
        ui_lock(!is_locked);
        return;
    } else if (event == event_button_sel && data == press_long) {
        if (pwrctl_vout_enabled()) {
            /** @todo: Blink or something */
        } else {
            ui_next_power_mode();
            if (ui_get_power_mode() == pm_constant_current && edit_mode == edit_voltage) {
                /** Cannot edit voltage in CC mode */
                edit_mode = edit_ampere;
            }
            last_edit_mode = edit_ampere;
            update_settings_ui(true);
            write_past_settings();
        }
//        tft_invert(!tft_is_inverted());
        return;
    }

    if (is_locked) {
        switch(event) {
            case event_button_m1:
            case event_button_m2:
            case event_button_sel:
            case event_rot_press:
            case event_rot_left:
            case event_rot_right:
            case event_button_enable:
                lock_flashing_period = LOCK_FLASHING_PERIOD;
                lock_flash_counter = LOCK_FLASHING_COUNTER;
                return;
            default:
                break;
        }
    }

    switch(event) {
        case event_ocp:
            {
#ifdef CONFIG_OCP_DEBUGGING
                uint16_t i_out_raw, v_in_raw, v_out_raw;
                hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
                (void) v_in_raw;
                (void) v_out_raw;
                uint16_t trig = hw_get_itrig_ma();
                printf("%10lu OCP: trig:%lumA limit:%lumA cur:%lumA\n", (uint32_t) (get_ticks() - pwr_start), pwrctl_calc_iout(trig), pwrctl_calc_iout(pwrctl_i_limit_raw), pwrctl_calc_iout(i_out_raw));
#endif // CONFIG_OCP_DEBUGGING
                ui_flash(); /** @todo When OCP kicks in, show last I_out on screen */
                ui_update_power_status(pwrctl_vout_enabled());
            }
            break;
        case event_button_enable:
            {
                bool new_state = !pwrctl_vout_enabled();
                if (new_state) {
                    write_past_settings();
                    pwr_start = get_ticks();
                }
                if (new_state) {
                    update_power_output();
                }
                pwrctl_enable_vout(new_state);
                ui_update_power_status(new_state);
            }
            break;
        case event_button_m1:
            if (edit_mode != edit_none && ui_get_power_mode() != pm_constant_current) {
                edit_mode = edit_voltage;
                if (edit_position > MAX_EDIT_POSITION_V) {
                    edit_position = MAX_EDIT_POSITION_V;
                }
                update_settings_ui(true);
            }
            break;
        case event_button_m2:
            if (edit_mode != edit_none) {
                edit_mode = edit_ampere;
                if (edit_position > MAX_EDIT_POSITION_A) {
                    edit_position = MAX_EDIT_POSITION_A;
                }
                update_settings_ui(true);
            }
            break;
        case event_button_sel:
            if (edit_mode == edit_none) {
                edit_mode = last_edit_mode;
                update_settings_ui(true);
            } else {
                write_past_settings();
                last_edit_mode = edit_mode;
                edit_mode = edit_none;
            }
            break;
        case event_rot_press:
#ifdef CONFIG_ADC_BENCHMARK
                hw_print_ticks();
#endif // CONFIG_ADC_BENCHMARK
            if (edit_mode != edit_none) {
                edit_position++;
                if (edit_mode == edit_voltage && edit_position > MAX_EDIT_POSITION_V) {
                    edit_position = 0;
                }
                else if (edit_mode == edit_ampere && edit_position > MAX_EDIT_POSITION_A) {
                    edit_position = 0;
                }
                update_settings_ui(false);
            }
            break;
        case event_rot_left:
            if (edit_mode == edit_voltage) {
                uint32_t increment = get_voltage_increment(edit_position);
                if (v_setting >= increment) {
                    v_setting -= increment;
                    update_power_output();
                    update_settings_ui(false);
                }
            } else if (edit_mode == edit_ampere) {
                uint32_t increment = get_ampere_increment(edit_position);
                if (i_setting >= increment) {
                    i_setting -= increment;
                    update_power_output();
                    update_settings_ui(false);
                }
            }
            break;
        case event_rot_right:
            if (edit_mode == edit_voltage) {
                uint32_t increment = get_voltage_increment(edit_position);
                if (v_setting + increment <= max_v_out) {
                    v_setting += increment;
                    update_power_output();
                    update_settings_ui(false);
                }
            } else if (edit_mode == edit_ampere) {
                uint32_t increment = get_ampere_increment(edit_position);
                if (i_setting + increment <= max_i_limit) {
                    i_setting += increment;
                    update_power_output();
                    update_settings_ui(false);
                }
            }
            break;
        default:
            break;
    }
}

/**
  * @brief Update values on the UI
  * @param v_in input voltage (mV)
  * @param v_out current output volrage (mV)
  * @param i_out current current draw :) (mA)
  * @todo  merge with update_settings_ui(...)
  * @retval none
  */
void ui_update_values(uint32_t v_in, uint32_t v_out, uint32_t i_out)
{
    static uint64_t last = 0;
    /** Update on the first call and every UI_UPDATE_INTERVAL_MS ms */
    if (last > 0 && get_ticks() - last < UI_UPDATE_INTERVAL_MS) {
        return;
    }

    last = get_ticks();

    max_v_out = v_in - V_IO_DELTA;

    if (cur_v_in != v_in) {
        cur_v_in = v_in;
    }
    if (cur_v_out != v_out) {
        cur_v_out = v_out;
    }
    if (cur_i_out != i_out) {
        cur_i_out = i_out;
    }
    if (cur_v_out > max_v_out) {
        cur_v_out = max_v_out;
    }

    if (edit_mode == edit_none) {
        /** @todo Select the widest character instead of hard coding [4] */
        uint32_t w = font_1_widths[4] + 2;
        uint32_t h = font_1_height + 2;
        uint32_t temp;
        if (pwrctl_vout_enabled()) {
            temp = cur_v_out;
        } else {
            temp = v_setting;
        }

        // Print voltage
        if (power_mode == pm_constant_current && !pwrctl_vout_enabled()) {
            tft_fill(voltage_positions[0], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[1], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[2], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[3], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[4], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[5], YPOS_VOLTAGE, font_1_widths[font_1_num_glyphs-1], h, bg_color);
        } else {
            if (temp/10000) {
                tft_putch(1, '0'+temp/10000, voltage_positions[0], YPOS_VOLTAGE, w, h, false);
                temp %= 10000;
            } else {
                tft_fill(voltage_positions[0], YPOS_VOLTAGE, w, h, bg_color);
            }

            tft_putch(1, '0'+temp/1000, voltage_positions[1], YPOS_VOLTAGE, w, h, false);
            temp %= 1000;
            tft_putch(1, '.', voltage_positions[2], YPOS_VOLTAGE, 10, h, false);
            tft_putch(1, '0'+temp/100, voltage_positions[3], YPOS_VOLTAGE, w, h, false);
            temp %= 100;
            tft_putch(1, '0'+temp/10, voltage_positions[4], YPOS_VOLTAGE, w, h, false);
            temp %= 10;
            tft_putch(1, 'V', voltage_positions[5], YPOS_VOLTAGE, font_1_widths[font_1_num_glyphs-1], h, false);
        }

        // Print current current (mohahahaha) or selected I max
        if (pwrctl_vout_enabled()) {
            temp = cur_i_out;
        } else {
            temp = i_setting;
        }

        tft_putch(1, '0'+temp/1000, ampere_positions[0], YPOS_AMPERE, w, h, false);
        temp %= 1000;
        tft_putch(1, '.', ampere_positions[1], YPOS_AMPERE, 10, h, false);
        tft_putch(1, '0'+temp/100, ampere_positions[2], YPOS_AMPERE, w, h, false);
        temp %= 100;
        tft_putch(1, '0'+temp/10, ampere_positions[3], YPOS_AMPERE, w, h, false);
        temp %= 10;
        tft_putch(1, '0'+temp, ampere_positions[4], YPOS_AMPERE, w, h, false);
        tft_putch(1, 'A', ampere_positions[5], YPOS_AMPERE, font_1_widths[font_1_num_glyphs-1], h, false);

        // Print current input voltage
        update_input_voltage();
    }
}

/**
  * @brief Lock or unlock the UI
  * @param lock true for lock, false for unlock
  * @retval none
  */
void ui_lock(bool lock)
{
    if (is_locked != lock) {
        is_locked = lock;
        lock_flashing_period = 0;
        if (is_locked) {
            lock_visible = true;
            tft_blit((uint16_t*) padlock, padlock_width, padlock_height, XPOS_LOCK, ui_height-padlock_height);
        } else {
            lock_visible = false;
            tft_fill(XPOS_LOCK, ui_height-padlock_height, padlock_width, padlock_height, bg_color);
        }
    }
}

/**
  * @brief Do periodical updates in the UI
  * @retval none
  */
void ui_tick(void)
{
    static uint64_t last_wifi_update = 0;
    static uint64_t last_tft_flash = 0;
    static uint64_t last_lock_flash = 0;

    if (wifi_status_flashing_period > 0 && get_ticks() - last_wifi_update > wifi_status_flashing_period) {
        last_wifi_update = get_ticks();
        if (wifi_status_visible) {
            tft_fill(XPOS_WIFI, ui_height-wifi_height, wifi_width, wifi_height, bg_color);
        } else {
            tft_blit((uint16_t*) wifi, wifi_width, wifi_height, XPOS_WIFI, ui_height-wifi_height);
        }
        wifi_status_visible = !wifi_status_visible;
    }

    if (lock_flashing_period > 0 && get_ticks() - last_lock_flash > lock_flashing_period) {
        last_lock_flash = get_ticks();
        lock_visible = !lock_visible;
        if (lock_visible) {
            tft_blit((uint16_t*) padlock, padlock_width, padlock_height, XPOS_LOCK, ui_height-padlock_height);
        } else {
            tft_fill(XPOS_LOCK, ui_height-padlock_height, padlock_width, padlock_height, bg_color);
        }
        lock_flash_counter--;
        if (lock_flash_counter == 0) {
            lock_visible = true;
            /** If the user hammers the locked buttons we might end up with an
                invisible locking symbol at the end of the flashing */
            tft_blit((uint16_t*) padlock, padlock_width, padlock_height, XPOS_LOCK, ui_height-padlock_height);
            lock_flashing_period = 0;
        }
    }

    if (tft_flashing_period > 0 && get_ticks() - last_tft_flash > tft_flashing_period) {
        last_tft_flash = get_ticks();
        tft_flash_counter--;
        tft_invert(!tft_is_inverted());
        if (tft_flash_counter == 0) {
            tft_flashing_period = 0;
        }
    }

    if (wifi_status == wifi_connecting && get_ticks() > WIFI_CONNECT_TIMEOUT) {
        ui_update_wifi_status(wifi_off);
    }
}

/**
  * @brief Handle ping
  * @retval none
  */
void ui_handle_ping(void)
{
    ui_flash();
}

/**
  * @brief Update wifi status icon
  * @param status new wifi status
  * @retval none
  */
void ui_update_wifi_status(wifi_status_t status)
{
    if (wifi_status != status) {
        wifi_status = status;
        switch(wifi_status) {
            case wifi_off:
                wifi_status_flashing_period = 0;
                wifi_status_visible = true;
                tft_fill(XPOS_WIFI, ui_height-wifi_height, wifi_width, wifi_height, bg_color);
                break;
            case wifi_connecting:
                wifi_status_flashing_period = WIFI_CONNECTING_FLASHING_PERIOD;
                break;
            case wifi_connected:
                wifi_status_flashing_period = 0;
                wifi_status_visible = false;
                tft_blit((uint16_t*) wifi, wifi_width, wifi_height, XPOS_WIFI, ui_height-wifi_height);
                break;
            case wifi_error:
                wifi_status_flashing_period = WIFI_ERROR_FLASHING_PERIOD;
                break;
            case wifi_upgrading:
                wifi_status_flashing_period = WIFI_UPGRADING_FLASHING_PERIOD;
                break;
        }
    }
}

/**
  * @brief Update power enable status icon
  * @param enabled new power status
  * @retval none
  */
void ui_update_power_status(bool enabled)
{
    if (is_enabled != enabled) {
        is_enabled = enabled;
        if (is_enabled) {
            tft_blit((uint16_t*) power, power_width, power_height, ui_width-power_width, ui_height-power_height);
        } else {
            tft_fill(ui_width-power_width, ui_height-power_height, power_width, power_height, bg_color);
        }
    }
}

/**
  * @brief Select next output mode (currently CV or CC)
  * @retval none
  */
void ui_next_power_mode(void)
{
    power_mode++;
    if (power_mode == pm_max) {
        power_mode = pm_min;
    }
    update_power_mode_status();
}

/**
  * @brief Get current output mode (currently CV or CC)
  * @retval current power mode
  */
power_mode_t ui_get_power_mode(void)
{
    return power_mode;
}

#ifdef CONFIG_SPLASH_SCREEN
/**
  * @brief Draw splash screen
  * @retval none
  */
void ui_draw_splash_screen(void)
{
    tft_blit((uint16_t*) logo, logo_width, logo_height, (ui_width-logo_width)/2, (ui_height-logo_height)/2);
}
#endif // CONFIG_SPLASH_SCREEN

/**
  * @brief Update power output according to current settings
  * @retval none
  */
static void update_power_output(void)
{
    switch(power_mode) {
        case pm_constant_voltage:
            pwrctl_set_vout(v_setting);
            pwrctl_set_ilimit(i_setting);
            pwrctl_set_iout(max_i_limit);
            break;
        case pm_constant_current:
            pwrctl_set_vout(max_v_out);
            pwrctl_set_ilimit(max_i_limit);
            pwrctl_set_iout(i_setting);
            break;
        default:
            break;
    }
}

/**
  * @brief Update power mode status icon
  * @retval none
  */
static void update_power_mode_status(void)
{
    switch(power_mode) {
        case pm_constant_current:
            tft_blit((uint16_t*) cc, cc_width, cc_height, XPOS_MODE, ui_height-cc_height);
            break;
        case pm_constant_voltage:
            tft_blit((uint16_t*) cv, cv_width, cv_height, XPOS_MODE, ui_height-cv_height);
            break;
        default:
            break;
    }
}

/**
  * @brief Update input voltage on UI
  * @retval none
  */
static void update_input_voltage(void)
{
    uint32_t w = font_0_widths[0];
    uint32_t temp = cur_v_in;
    if (temp/10000 > 0) {
        tft_putch(0, '0'+temp/10000, XPOS_VIN+bottom_positions[0], ui_height-font_0_height, w, font_0_height, false);
        temp %= 10000;
    } else {
        tft_fill(XPOS_VIN+bottom_positions[0], ui_height-font_0_height, w, font_0_height, bg_color);
    }

    tft_putch(0, '0'+temp/1000, XPOS_VIN+bottom_positions[1], ui_height-font_0_height, w, font_0_height, false);
    temp %= 1000;
    tft_putch(0, '.', XPOS_VIN+bottom_positions[2], ui_height-font_0_height, 4, font_0_height, false);
    tft_putch(0, '0'+temp/100, XPOS_VIN+bottom_positions[3], ui_height-font_0_height, w, font_0_height, false);
    tft_putch(0, 'V', XPOS_VIN+bottom_positions[4], ui_height-font_0_height, font_0_widths[font_0_num_glyphs-1], font_0_height, false);
}

/**
  * @brief Update the part of settings UI currently being edited
  *        (voltage or current) or both
  * @param update_both force update of both voltage and current settings
  * @retval none
  */
static void update_settings_ui(bool update_both)
{
    uint32_t w = font_1_widths[4] + 2;
    uint32_t h = font_1_height + 2;
    uint32_t temp = v_setting;

    // Print desired voltage
    /** @todo: print dashes as voltage when in editing mode and constant current mode */
    if (edit_mode == edit_voltage || update_both) {
        if (power_mode == pm_constant_current) {
            tft_fill(voltage_positions[0], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[1], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[2], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[3], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[4], YPOS_VOLTAGE, w, h, bg_color);
            tft_fill(voltage_positions[5], YPOS_VOLTAGE, font_1_widths[font_1_num_glyphs-1], h, bg_color);
        } else {
            if (temp/10000 > 0) {
                tft_putch(1, '0'+temp/10000, voltage_positions[0], YPOS_VOLTAGE, w, h, false);
                temp %= 10000;
            } else {
                tft_fill(voltage_positions[0], YPOS_VOLTAGE, w, h, bg_color);
            }

            tft_putch(1, '0'+temp/1000, voltage_positions[1], YPOS_VOLTAGE, w, h, (edit_mode == edit_voltage && edit_position == 0));
            temp %= 1000;
            tft_putch(1, '.', voltage_positions[2], YPOS_VOLTAGE, 10, h, false);
            tft_putch(1, '0'+temp/100, voltage_positions[3], YPOS_VOLTAGE, w, h, (edit_mode == edit_voltage && edit_position == 1));
            temp %= 100;
            tft_putch(1, '0'+temp/10, voltage_positions[4], YPOS_VOLTAGE, w, h, (edit_mode == edit_voltage && edit_position == 2));
            temp %= 10;
            tft_putch(1, 'V', voltage_positions[5], YPOS_VOLTAGE, font_1_widths[font_1_num_glyphs-1], h, false);
        }
    }

    if (edit_mode == edit_ampere || update_both) {
        // Print desired current
        temp = i_setting;
        tft_putch(1, '0'+temp/1000, ampere_positions[0], YPOS_AMPERE, w, h, (edit_mode == edit_ampere && edit_position == 0));
        temp %= 1000;
        tft_putch(1, '.', ampere_positions[1], YPOS_AMPERE, 10, h, false);
        tft_putch(1, '0'+temp/100, ampere_positions[2], YPOS_AMPERE, w, h, (edit_mode == edit_ampere && edit_position == 1));
        temp %= 100;
        tft_putch(1, '0'+temp/10, ampere_positions[3], YPOS_AMPERE, w, h, (edit_mode == edit_ampere && edit_position == 2));
        temp %= 10;
        tft_putch(1, '0'+temp, ampere_positions[4], YPOS_AMPERE, w, h, (edit_mode == edit_ampere && edit_position == 3));
        tft_putch(1, 'A', ampere_positions[5], YPOS_AMPERE, font_1_widths[font_1_num_glyphs-1], h, false);
    }
    // Print current input voltage
    update_input_voltage();
}

/**
  * @brief Get voltage increment given current editing position
  * @param edit_pos the current editing position [0..2]
  * @retval none
  */
static uint32_t get_voltage_increment(uint32_t edit_pos)
{
    switch(edit_pos) {
        case 0:
            return 1000;
            break;
        case 1:
            return 100;
            break;
        case 2:
            return 10;
            break;
        default:
            return 0;
    }
}

/**
  * @brief Get current increment given current editing position
  * @param edit_pos the current editing position [0..3]
  * @retval none
  */
static uint32_t get_ampere_increment(uint32_t edit_pos)
{
    switch(edit_pos) {
        case 0:
            return 1000;
            break;
        case 1:
            return 100;
            break;
        case 2:
            return 10;
            break;
        case 3:
            return 1;
            break;
        default:
            return 0;
    }
}

/**
  * @brief Flash the TFT once
  * @retval none
  */
static void ui_flash(void)
{
    tft_flashing_period = TFT_FLASHING_PERIOD;
    tft_flash_counter = TFT_FLASHING_COUNTER;
}

/**
  * @brief Read settings from past
  * @retval none
  */
static void read_past_settings(void)
{
    uint32_t *p;
    bool     inverse_setting = false;
    uint32_t length;
    if (past_read_unit(&g_past, past_power, (const void**) &p, &length)) {
        if (p) {
            v_setting = *p & 0xffff;
            i_setting = (*p >> 16) & 0xffff;
        }
    }
    if (!v_setting || !i_setting) {
#if !defined(CONFIG_DEFAULT_VOUT) || !defined(CONFIG_DEFAULT_ILIMIT)
        #error "Please define CONFIG_DEFAULT_VOUT and CONFIG_DEFAULT_ILIMIT (in mV/mA)"
#endif
        v_setting = CONFIG_DEFAULT_VOUT;
        i_setting = CONFIG_DEFAULT_ILIMIT;
    }

    // Set limits, with sanity check
    if (!pwrctl_set_vout(v_setting)) {
        v_setting = CONFIG_DEFAULT_VOUT;
        (void) pwrctl_set_vout(v_setting);
    }
    if (!pwrctl_set_ilimit(i_setting)) {
        i_setting = CONFIG_DEFAULT_ILIMIT;
        (void) pwrctl_set_ilimit(i_setting);
    }

    if (past_read_unit(&g_past, past_tft_inversion, (const void**) &p, &length)) {
        if (p) {
            inverse_setting = !!(*p);
        }
    }

    /** stored power_mode_t */
    if (past_read_unit(&g_past, past_power_mode, (const void**) &p, &length)) {
        if (p) {
            power_mode = (power_mode_t) *p;
        } else {
            power_mode = pm_constant_voltage;
        }
    } else {
        power_mode = pm_constant_voltage;
    }
    if (power_mode >= pm_max) {
        power_mode = pm_min;
    }
    update_power_mode_status();

    //tft_invert(inverse_setting);

    last_vout_setting = v_setting;
    last_ilimit_setting = i_setting;
    last_tft_inv_setting = inverse_setting;
    last_power_mode_setting = power_mode;
}

/**
  * @brief Write changed settings to past. Checked when exiting edit mode,
  *        enabling power out or inverting the display.
  * @retval none
  */
static void write_past_settings(void)
{
    if (v_setting != last_vout_setting || i_setting != last_ilimit_setting) {
        last_vout_setting = v_setting;
        last_ilimit_setting = i_setting;
        uint32_t setting = (last_ilimit_setting & 0xffff) << 16 | (last_vout_setting & 0xffff);
        if (!past_write_unit(&g_past, past_power, (void*) &setting, sizeof(setting))) {
            /** @todo Handle past write errors */
            printf("Error: past write pwr failed!\n");
        }
    }
#if 0
    if (tft_is_inverted() != last_tft_inv_setting) {
        last_tft_inv_setting = tft_is_inverted();
        uint32_t setting = last_tft_inv_setting;
        if (!past_write_unit(&g_past, past_tft_inversion, (void*) &setting, sizeof(setting))) {
            /** @todo Handle past write errors */
            printf("Error: past write inv failed!\n");
        }
    }
#endif
    if (last_power_mode_setting != power_mode) {
        uint32_t setting = (uint32_t) power_mode;
        if (!past_write_unit(&g_past, past_power_mode, (void*) &setting, sizeof(setting))) {
            /** @todo Handle past write errors */
            printf("Error: past write power mode failed!\n");
        }
    }
}

/**
  * @brief Check if user wants master reset, resetting the past area
  * @retval none
  */
static void check_master_reset(void)
{
    if (hw_sel_button_pressed()) {
        printf("Master reset\n");
        if (!past_format(&g_past)) {
            /** @todo Handle past format errors */
            printf("Error: past formatting failed!\n");
        }
    }
}
