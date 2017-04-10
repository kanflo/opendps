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
#include <usart.h>
#include <string.h>
#include <stdlib.h>
#include "cli.h"
#include "ui.h"
#include "hw.h"
#include "pwrctl.h"
#include "serialhandler.h"

static void stat_cmd(uint32_t argc, char *argv[]);
static void on_cmd(uint32_t argc, char *argv[]);
static void off_cmd(uint32_t argc, char *argv[]);
static void v_cmd(uint32_t argc, char *argv[]);


static const cli_command_t commands[] = {
    {
        .cmd = "on",
        .handler = &on_cmd,
        .min_arg = 0, .max_arg = 0,
        .help = "Power output on",
        .usage = "",
    },
    {
        .cmd = "off",
        .handler = &off_cmd,
        .min_arg = 0, .max_arg = 0,
        .help = "Power output off",
        .usage = "",
    },
    {
        .cmd = "stat",
        .handler = &stat_cmd,
        .min_arg = 0, .max_arg = 0,
        .help = "Print DPS status, V_in, V_out, I_out etc.",
        .usage = "",
    },
    {
        .cmd = "v",
        .handler = &v_cmd,
        .min_arg = 1, .max_arg = 1,
        .help = "Set output <voltage> mV",
        .usage = "<millivolt>",
    },
  };


void serial_handle_rx_char(char c)
{
    static char buffer[80];
    static uint32_t i = 0;
    if (i == 0) {
        memset(buffer, 0, sizeof(buffer));
    }

    if (c == '\b' || c == 0x7f) {
        if (i) {
            printf("\b \b");
            i--;
        }
    } else if (i >= sizeof(buffer) - 1) {
        usart_send_blocking(USART1, '\a');
    } else if (c == ';') {
        usart_send_blocking(USART1, '\n');
        if (strlen(buffer) > 0) {
            cli_run(commands, sizeof(commands) / sizeof(cli_command_t), (char*) buffer);
        }
        i = 0;
    } else if (c == '\r' || c == '\n') {
        // nada
    } else if (c < 0x20) {
        // Ignore other control characters
    } else {
        buffer[i++] = c;
        usart_send_blocking(USART1, c);
    }
}

static void on_cmd(uint32_t argc, char *argv[])
{
    (void) argc;
    (void) argv;
    printf("Power on\n");
    pwrctl_enable_vout(true);
    ui_update_power_status(true);
}

static void off_cmd(uint32_t argc, char *argv[])
{
    (void) argc;
    (void) argv;
    printf("Power off\n");
    pwrctl_enable_vout(false);
    ui_update_power_status(false);
}

static void stat_cmd(uint32_t argc, char *argv[])
{
    (void) argc;
    (void) argv;
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    uint32_t v_in = pwrctl_calc_vin(v_in_raw);
    uint32_t v_out = pwrctl_calc_vout(v_out_raw);
    uint32_t i_out = pwrctl_calc_iout(i_out_raw);
    printf(" V_in  : %2lu.%02lu V\n", v_in/1000,  (v_in%1000)/10);
    printf(" V_out : %2lu.%02lu V (%s)\n", v_out/1000, (v_out%1000)/10, pwrctl_vout_enabled() ? "enabled" : "disabled");
    printf(" I_out : %2lu.%03lu A\n", i_out/1000, i_out%1000);
}

static void v_cmd(uint32_t argc, char *argv[])
{
    (void) argc;
    uint32_t v_out = atoi(argv[1]);
    printf("Setting V_out to %lumv\n", v_out);
    pwrctl_set_vout(v_out);
}
