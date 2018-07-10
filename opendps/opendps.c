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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rcc.h>
#include <gpio.h>
#include <stdlib.h>
#include <systick.h>
#include <usart.h>
#include <timer.h>
#include "dbg_printf.h"
#include "tick.h"
#include "tft.h"
#include "event.h"
#include "hw.h"
#include "pwrctl.h"
#include "protocol.h"
#include "serialhandler.h"
#include "ili9163c.h"
#include "padlock.h"
#include "thermometer.h"
#include "power.h"
#include "wifi.h"
#include "font-0.h"
#include "font-1.h"
#include "gpio.h"
#include "past.h"
#include "pastunits.h"
#include "uui.h"
#include "uui_number.h"
#include "opendps.h"
#include "func_cv.h"
#include "my_assert.h"
#ifdef CONFIG_CC_ENABLE
#include "func_cc.h"
#endif // CONFIG_CC_ENABLE

#ifdef DPS_EMULATOR
#include "dpsemul.h"
#endif // DPS_EMULATOR

#define TFT_HEIGHT  (128)
#define TFT_WIDTH   (128)

/** How ofter we update the measurements in the UI (ms) */
#define UI_UPDATE_INTERVAL_MS  (250)

/** Timeout for waiting for wifi connction (ms) */
#define WIFI_CONNECT_TIMEOUT  (10000)

/** Blit positions */
#define XPOS_WIFI     (4)
#define XPOS_LOCK    (30)

/** Constants describing how certain things on the screen flash when needed */
#define WIFI_CONNECTING_FLASHING_PERIOD  (1000)
#define WIFI_ERROR_FLASHING_PERIOD        (500)
#define WIFI_UPGRADING_FLASHING_PERIOD    (250)
#define LOCK_FLASHING_PERIOD              (250)
#define LOCK_FLASHING_COUNTER               (4)
#define TFT_FLASHING_PERIOD               (100)
#define TFT_FLASHING_COUNTER                (2)

static void ui_flash(void);
static void read_past_settings(void);
static void write_past_settings(void);
static void check_master_reset(void);

/** UI settings */
static uint16_t bg_color;
static uint32_t ui_width;
static uint32_t ui_height;

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
static bool is_temperature_locked;
static bool is_enabled;

/** Last settings written to past */
static bool     last_tft_inv_setting;

/** Temperature readings, invalid at start */
static int16_t temp1 = INVALID_TEMPERATURE;
static int16_t temp2 = INVALID_TEMPERATURE;

#ifndef CONFIG_TEMPERATURE_ALERT_LEVEL
 #define CONFIG_TEMPERATURE_ALERT_LEVEL  (500)
#endif // CONFIG_TEMPERATURE_ALERT_LEVEL

/** Temperature when the DPS goes into shutdown mode,
    in x10 degrees whatever-temperature-unit-you-prefer */
static int16_t shutdown_temperature = CONFIG_TEMPERATURE_ALERT_LEVEL;

/** Our parameter storage */
static past_t g_past = {
    .blocks = {0x0800f800, 0x0800fc00}
};

/** The function UI displaying the current active function */
static uui_t func_ui;
/** The main UI displaying input voltage and such */
static uui_t main_ui;


static void main_ui_tick(void);

/* This is the definition of the voltage item in the UI */
ui_number_t input_voltage = {
    {
        .type = ui_item_number,
        .id = 10,
        .x = 0,
        .y = 0,
        .can_focus = false,
    },
    .font_size = 0,
    .value = 0,
    .min = 0,
    .max = 0,
    .num_digits = 2,
    .num_decimals = 1, /** 2 decimals => value is in decivolts */
    .unit = unit_volt,
};

/* This is the screen definition */
ui_screen_t main_screen = {
    .name = "main",
    .tick = &main_ui_tick,
    .num_items = 1,
    .items = { (ui_item_t*) &input_voltage }
};

/**
 * @brief      List function names of device
 *
 * @param      names  Output vector holding pointers to the names
 * @param[in]  size   Number of items in the output array
 *
 * @return     Number of items returned
 */
uint32_t opendps_get_function_names(char* names[], size_t size)
{
    uint32_t i;
    for (i = 0; i < func_ui.num_screens && i < size; i++) {
        names[i] = func_ui.screens[i]->name;
    }
    return i;
}

/**
 * @brief      Get current function name
 *
 * @return     Current function name :)
 */
const char* opendps_get_curr_function_name(void)
{
    return (const char*) func_ui.screens[func_ui.cur_screen]->name;
}

/**
 * @brief      List parameter names of current function
 *
 * @param      parameters  Output vector holding pointers to the parameters
 *
 * @return     Number of items returned
 */
uint32_t opendps_get_curr_function_params(ui_parameter_t **parameters)
{
    uint32_t i = 0;
    *parameters = (ui_parameter_t*) &func_ui.screens[func_ui.cur_screen]->parameters;
    while ((*parameters)[i].name[0] != 0) {
        i++;
    }
    return i;
}

/**
 * @brief      Return value of named parameter for current function
 *
 * @param      name       Parameter name
 * @param[in]  value      String representation of parameter value
 * @param      value_len  Length of value buffer
 *
 * @return     true if param exists
 */
bool opendps_get_curr_function_param_value(char *name, char *value, uint32_t value_len)
{
    if (func_ui.screens[func_ui.cur_screen]->get_parameter) {
        return ps_ok == func_ui.screens[func_ui.cur_screen]->get_parameter(name, value, value_len);
    }
    return false;
}

/**
 * @brief      Set parameter to value
 *
 * @param      name   Name of parameter
 * @param      value  Value as a string
 *
 * @return     Status of the operation
 */
set_param_status_t opendps_set_parameter(char *name, char *value)
{
    set_param_status_t status = ps_not_supported;
    if (func_ui.screens[func_ui.cur_screen]->set_parameter) {
        status = func_ui.screens[func_ui.cur_screen]->set_parameter(name, value);
        if (status == ps_ok) {
            uui_refresh(&func_ui, true);
        }
    }
    return status;
}

/**
 * @brief      Enable output of current function
 *
 * @param[in]  enable  Enable or disable
 *
 * @return     True if enable was successul
 */
bool opendps_enable_output(bool enable)
{
    if (!is_temperature_locked && func_ui.screens[func_ui.cur_screen]->enable) {
        if (func_ui.screens[func_ui.cur_screen]->is_enabled != enable) {
            event_put(event_button_enable, press_short); /** @todo: call directly as this will not work for temperature alarm */
        }
    } else {
        emu_printf("Output enable failed %s\n", is_temperature_locked ? "due to high temperature" : "");
        return false;
    }
    return true;
}

bool opendps_enable_function_idx(uint32_t index)
{
    if (is_temperature_locked) {
        return false;
    } else {
        uui_set_screen(&func_ui, index);
        return true; /** @todo: handle failure */
    }
}

/**
 * @brief      Main UI UUI tick handler :)
 */
static void main_ui_tick(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    (void) i_out_raw;
    (void) v_out_raw;
    input_voltage.value = pwrctl_calc_vin(v_in_raw) / 100;
    input_voltage.ui.draw(&input_voltage.ui);
}

/**
  * @brief Initialize the UI
  * @retval none
  */
static void ui_init(void)
{
    bg_color = BLACK;
    ui_width = TFT_WIDTH;
    ui_height = TFT_HEIGHT;

    uui_init(&func_ui, &g_past);
    func_cv_init(&func_ui);
#ifdef CONFIG_CC_ENABLE
    func_cc_init(&func_ui);
#endif // CONFIG_CC_ENABLE
    uui_activate(&func_ui);

    uui_init(&main_ui, &g_past);
    number_init(&input_voltage);
    input_voltage.ui.x = 105;
    input_voltage.ui.y = ui_height - font_0_height;
    uui_add_screen(&main_ui, &main_screen);
    uui_activate(&main_ui);
}

/**
  * @brief Handle event
  * @param event the received event
  * @param data optional extra data
  * @retval none
  */
static void ui_hande_event(event_t event, uint8_t data)
{
    if (event == event_rot_press && data == press_long) {
        opendps_lock(!is_locked);
        return;
    } else if (event == event_button_sel && data == press_long) {
        tft_invert(!tft_is_inverted());
        write_past_settings();
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
                dbg_printf("%10u OCP: trig:%umA limit:%umA cur:%umA\n", (uint32_t) (get_ticks()), pwrctl_calc_iout(trig), pwrctl_calc_iout(pwrctl_i_limit_raw), pwrctl_calc_iout(i_out_raw));
#endif // CONFIG_OCP_DEBUGGING
                ui_flash(); /** @todo When OCP kicks in, show last I_out on screen */
                opendps_update_power_status(false);
                uui_handle_screen_event(&func_ui, event);
            }
            break;
        case event_button_enable:
            write_past_settings();
            /** Deliberate fallthrough */

        case event_button_m1:
        case event_button_m2:
        case event_button_sel:
        case event_rot_press:
        case event_rot_left:
        case event_rot_right:
        case event_rot_left_set:
        case event_rot_right_set:
            uui_handle_screen_event(&func_ui, event);
            uui_refresh(&func_ui, false);
            break;
        default:
            break;
    }
}

/**
  * @brief Lock or unlock the UI
  * @param lock true for lock, false for unlock
  * @retval none
  */
void opendps_lock(bool lock)
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
  * @brief Lock or unlock the UI due to a temperature alarm
  * @param lock true for lock, false for unlock
  * @retval none
  */
void opendps_temperature_lock(bool lock)
{
    if (is_temperature_locked != lock) {
        is_temperature_locked = lock;
        if (is_temperature_locked) {
            emu_printf("DPS disabled due to temperature\n");
            /** @todo Right now we cannot use opendps_enable_output here */
            uui_disable_cur_screen(&func_ui);
            tft_clear();
            uui_show(&func_ui, false);
            uui_show(&main_ui, false);
            tft_blit((uint16_t*) thermometer, thermometer_width, thermometer_height, 1+(ui_width-thermometer_width)/2, 30);
        } else {
            emu_printf("DPS enabled due to temperature\n");
            tft_clear();
            uui_show(&func_ui, true);
            uui_show(&main_ui, true);
            uui_refresh(&func_ui, true);
            uui_refresh(&main_ui, true);
        }
    }
}

/**
  * @brief Do periodical updates in the UI
  * @retval none
  */
static void ui_tick(void)
{
    static uint64_t last_wifi_update = 0;
    static uint64_t last_tft_flash = 0;
    static uint64_t last_lock_flash = 0;

    static uint64_t last = 0;
    /** Update on the first call and every UI_UPDATE_INTERVAL_MS ms */
    if (last > 0 && get_ticks() - last < UI_UPDATE_INTERVAL_MS) {
        return;
    }

    last = get_ticks();
    uui_tick(&func_ui);
    uui_tick(&main_ui);

#ifndef CONFIG_SPLASH_SCREEN
    {
        // Light up the display now that the UI has been drawn
        static bool first = true;
        if (first) {
            hw_enable_backlight();
            first = false;
        }
    }
#endif // CONFIG_SPLASH_SCREEN

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
        opendps_update_wifi_status(wifi_off);
    }
}

/**
  * @brief Handle ping
  * @retval none
  */
void opendps_handle_ping(void)
{
    ui_flash();
}

/**
  * @brief Update wifi status icon
  * @param status new wifi status
  * @retval none
  */
void opendps_update_wifi_status(wifi_status_t status)
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
void opendps_update_power_status(bool enabled)
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
  * @brief Set temperatures
  * @param temp1 first temperature we can deal with
  * @param temp2 second temperature we can deal with
  * @retval none
  */
void opendps_set_temperature(int16_t _temp1, int16_t _temp2)
{
    temp1 = _temp1;
    temp2 = _temp2;
    bool alert = temp1 > shutdown_temperature || temp2 > shutdown_temperature;
    opendps_temperature_lock(alert);
    emu_printf("Got temperature %d and %d %s\n", temp1, temp2, alert ? "[ALERT]" : "");
}

/**
  * @brief Get temperatures
  * @param temp1 first temperature we can deal with
  * @param temp2 second temperature we can deal with
  * @retval none
  */
void opendps_get_temperature(int16_t *_temp1, int16_t *_temp2, bool *temp_shutdown)
{
    assert(_temp1);
    assert(_temp2);
    assert(temp_shutdown);
    *_temp1 = temp1;
    *_temp2 = temp2;
    *temp_shutdown = is_temperature_locked;
}

#ifdef CONFIG_SPLASH_SCREEN
/**
  * @brief Draw splash screen
  * @retval none
  */
static void ui_draw_splash_screen(void)
{
    tft_blit((uint16_t*) logo, logo_width, logo_height, (ui_width-logo_width)/2, (ui_height-logo_height)/2);
}
#endif // CONFIG_SPLASH_SCREEN

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
    bool     inverse_setting = false;
    uint32_t length;
    uint32_t *p = 0;
    if (past_read_unit(&g_past, past_tft_inversion, (const void**) &p, &length)) {
        if (p) {
            inverse_setting = !!(*p);
        }
    }
    tft_invert(inverse_setting);

#ifdef GIT_VERSION
    /** Update app git hash in past if needed */
    char *ver = 0;
    bool exists = past_read_unit(&g_past, past_app_git_hash, (const void**) &ver, &length);
    if (!exists || strncmp((char*) ver, GIT_VERSION, 32 /* probably never longer than 32 bytes */) != 0) {
        if (!past_write_unit(&g_past, past_app_git_hash, (void*) &GIT_VERSION, strlen(GIT_VERSION))) {
            /** @todo Handle past write errors */
            dbg_printf("Error: past write app git hash failed!\n");
        }
    }
#endif // GIT_VERSION
}

/**
  * @brief Write changed settings to past. Checked when exiting edit mode,
  *        enabling power out or inverting the display.
  * @retval none
  */
static void write_past_settings(void)
{

    if (tft_is_inverted() != last_tft_inv_setting) {
        last_tft_inv_setting = tft_is_inverted();
        uint32_t setting = last_tft_inv_setting;
        if (!past_write_unit(&g_past, past_tft_inversion, (void*) &setting, sizeof(setting))) {
            /** @todo Handle past write errors */
            dbg_printf("Error: past write inv failed!\n");
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
        dbg_printf("Master reset\n");
        if (!past_format(&g_past)) {
            /** @todo Handle past format errors */
            dbg_printf("Error: past formatting failed!\n");
        }
    }
}

/**
  * @brief This is the app event handler, pulling an event off the event queue
  *        and reacting on it.
  * @retval None
  */
static void event_handler(void)
{
    while(1) {
        event_t event;
        uint8_t data = 0;
        if (!event_get(&event, &data)) {
            hw_longpress_check();
            ui_tick();
        } else {
            if (event) {
                emu_printf(" Event %d 0x%02x\n", event, data);
            }
            switch(event) {
                case event_none:
                    dbg_printf("Weird, should not receive 'none events'\n");
                    break;
                case event_uart_rx:
                    serial_handle_rx_char(data);
                    break;
                case event_ocp:
                    break;
                default:
                    break;
            }
            ui_hande_event(event, data);
        }
    }
}

/**
  * @brief Ye olde main
  * @retval preferably none
  */
int main(int argc, char const *argv[])
{
    hw_init();
    pwrctl_init(); // Must be after DAC init
    event_init();

#ifdef CONFIG_COMMANDLINE
    dbg_printf("Welcome to OpenDPS!\n");
    dbg_printf("Try 'help;' for, well, help (note the semicolon).\n");
#endif // CONFIG_COMMANDLINE

    tft_init();
    delay_ms(50); // Without this delay we will observe some flickering
    tft_clear();
#ifdef DPS_EMULATOR
    dps_emul_init(&g_past, argc, argv);
#else // DPS_EMULATOR
    (void) argc;
    (void) argv;
    g_past.blocks[0] = 0x0800f800;
    g_past.blocks[1] = 0x0800fc00;
#endif // DPS_EMULATOR
    if (!past_init(&g_past)) {
        dbg_printf("Error: past init failed!\n");
        /** @todo Handle past init failure */
    }

    check_master_reset();
    read_past_settings();
    ui_init();

#ifdef CONFIG_WIFI
    /** Rationale: the ESP8266 could send this message when it starts up but
      * the current implementation spews wifi/network related messages on the
      * UART meaning this message got lost. The ESP will however send the
      * wifi_connected status when it connects but if that does not happen, the
      * ui module will turn off the wifi icon after 10s to save the user's
      * sanity
      */
    opendps_update_wifi_status(wifi_connecting);
#endif // CONFIG_WIFI

#ifdef CONFIG_SPLASH_SCREEN
    ui_draw_splash_screen();
    hw_enable_backlight();
    delay_ms(750);
    tft_clear();
#endif // CONFIG_SPLASH_SCREEN
    event_handler();
    return 0;
}
