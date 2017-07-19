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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rcc.h>
#include <gpio.h>
#include <stdlib.h>
#include <systick.h>
#include <usart.h>
#include <timer.h>
#include "tick.h"
#include "tft.h"
#include "event.h"
#include "ui.h"
#include "hw.h"
#include "pwrctl.h"
#include "serialhandler.h"

#define TFT_HEIGHT  (128)
#define TFT_WIDTH   (128)

static void update_measurements(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    uint32_t v_in = pwrctl_calc_vin(v_in_raw);
    uint32_t v_out = pwrctl_calc_vout(v_out_raw);
    uint32_t i_out = pwrctl_calc_iout(i_out_raw);
    ui_update_values(v_in, v_out, i_out);

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
            update_measurements();
            ui_tick();
        } else {
            switch(event) {
                case event_none:
                    printf("Weird, should not receive 'none events'\n");
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
    printf("\nWelcome to OpenDPS!\n");
    printf("Try 'help;' for, well, help (note the semicolon).\n");
#endif // CONFIG_COMMANDLINE

    tft_init();
    delay_ms(50); // Without this delay we will observe some flickering
    tft_clear();
    ui_init(TFT_WIDTH, TFT_HEIGHT);

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
