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
#include "power.h"
#include "wifi.h"
#include "font-0.h"
#include "font-1.h"
#include "gpio.h"
#include "past.h"
#include "pastunits.h"
#include "uui.h"
#include "uui_number.h"
#include "func_cv.h"
#ifdef CONFIG_CC_ENABLE
#include "func_cc.h"
#endif // CONFIG_CC_ENABLE

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

/** Last settings written to past */
static bool     last_tft_inv_setting;

/** Our parameter storage */
static past_t g_past = {
    .blocks = {0x0800f800, 0x0800fc00}
};

/** The UI */
static uui_t func_ui;
static uui_t main_ui;


static void main_ui_tick(void);
void ui_update_power_status(bool enabled);
void ui_update_wifi_status(wifi_status_t status);
void ui_handle_ping(void);
void ui_lock(bool lock);

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
    max_i_limit = CONFIG_DPS_MAX_CURRENT;

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
        ui_lock(!is_locked);
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
                ui_update_power_status(false);
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
int main(void)
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
    g_past.blocks[0] = 0x0800f800;
    g_past.blocks[1] = 0x0800fc00;
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
    ui_update_wifi_status(wifi_connecting);
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
