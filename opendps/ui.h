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
#include "event.h"
#include "protocol.h"

#ifndef __UI_H__
#define __UI_H__

/** Supported power out modes */
typedef enum {
        pm_min = 0,
        pm_constant_voltage = pm_min,
        pm_constant_current,
        pm_current_limit,
        pm_max
} power_mode_t;


/**
  * @brief Initialize the UI module
  * @param width width of the UI
  * @param height height of the UI
  * @retval none
  */
void ui_init(uint32_t width, uint32_t height);

/**
  * @brief Handle event
  * @param event the received event
  * @param data optional extra data
  * @retval none
  */
void ui_hande_event(event_t event, uint8_t data);

/**
  * @brief Update values on the UI
  * @param v_in input voltage (mV)
  * @param v_out current output volrage (mV)
  * @param i_out current current draw :) (mA)
  * @retval none
  */
void ui_update_values(uint32_t v_in, uint32_t v_out, uint32_t i_out);

/**
  * @brief Lock or unlock the UI
  * @param lock true for lock, false for unlock
  * @retval none
  */
void ui_lock(bool lock);

/**
  * @brief Do periodical updates in the UI
  * @retval none
  */
void ui_tick(void);

/**
  * @brief Handle ping
  * @retval none
  */
void ui_handle_ping(void);

/**
  * @brief Update wifi status icon
  * @param status new wifi status
  * @retval none
  */
void ui_update_wifi_status(wifi_status_t status);

/**
  * @brief Update power enable status icon
  * @param enabled new power status
  * @retval none
  */
void ui_update_power_status(bool enabled);

/**
  * @brief Set output mode
  * @param mode new output mode
  * @retval true if change was successful
  */
bool ui_set_power_mode(power_mode_t mode);

/**
  * @brief Set output voltage
  * @param voltage_mv new output voltage
  * @retval true if change was successful
  */
bool ui_set_voltage(uint32_t voltage_mv);

/**
  * @brief Set output current
  * @param current_ma new output current
  * @retval true if change was successful
  */
bool ui_set_current(uint32_t current_ma);

/**
  * @brief Enable output power
  * @param enable true if enable
  * @retval none
  */
void ui_enable_power(bool enable);

/**
  * @brief Select next output mode
  * @retval none
  */
void ui_next_power_mode(void);

/**
  * @brief Get current output mode
  * @retval current output mode
  */
power_mode_t ui_get_power_mode(void);

#ifdef CONFIG_SPLASH_SCREEN
/**
  * @brief Draw splash screen
  * @retval none
  */
void ui_draw_splash_screen(void);
#endif // CONFIG_SPLASH_SCREEN

#endif // __UI_H__
