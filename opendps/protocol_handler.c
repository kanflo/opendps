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
#include <usart.h>
#include <string.h>
#include <stdlib.h>
#include <scb.h>
#include "dbg_printf.h"
#include "hw.h"
#include "uui.h"
#include "pwrctl.h"
#include "uframe.h"
#include "protocol.h"
#include "serialhandler.h"
#include "bootcom.h"
#include "uframe.h"
#include "opendps.h"

#ifdef DPS_EMULATOR
 extern void dps_emul_send_frame(uint8_t *frame, uint32_t length);
#endif // DPS_EMULATOR

typedef enum {
    cmd_failed = 0,
    cmd_success,
    cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much,
} command_status_t;

static uint8_t frame_buffer[FRAME_OVERHEAD(MAX_FRAME_LENGTH)];
static uint32_t rx_idx = 0;
static bool receiving_frame = false;

/**
  * @brief Send a frame on the uart
  * @param frame the frame to send
  * @param length length of frame
  * @retval None
  */
static void send_frame(uint8_t *frame, uint32_t length)
{
#ifdef DPS_EMULATOR
    dps_emul_send_frame(frame, length);
#else // DPS_EMULATOR
    do {
        usart_send_blocking(USART1, *frame);
        frame++;
    } while(--length);
#endif // DPS_EMULATOR
  }

/**
  * @brief Handle a query command
 * @retval command_status_t failed, success or "I sent my own frame"
  */
static command_status_t handle_query(void)
{
    emu_printf("%s\n", __FUNCTION__);
    ui_parameter_t *params;
    char value[16];
    uint32_t num_param = opendps_get_curr_function_params(&params);
    
    const char* curr_func = opendps_get_curr_function_name();

    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    uint16_t v_in = pwrctl_calc_vin(v_in_raw);
    uint16_t v_out = pwrctl_calc_vout(v_out_raw);
    uint16_t i_out = pwrctl_calc_iout(i_out_raw);
    uint8_t output_enabled = pwrctl_vout_enabled();
    int16_t temp1, temp2;
    bool temp_shutdown;
    opendps_get_temperature(&temp1, &temp2, &temp_shutdown);
//    uint32_t len = protocol_create_query_response(frame_buffer, sizeof(frame_buffer), v_in, v_out_setting, v_out, i_out, i_limit, power_enabled);
    DECLARE_FRAME(64);
    PACK8(cmd_response | cmd_query);
    PACK8(1); // Always success
    PACK16(v_in);
    emu_printf("v_in = %d\n", v_in);
    PACK16(v_out);
    emu_printf("v_out = %d\n", v_out);
    PACK16(i_out);
    emu_printf("i_out = %d\n", i_out);
    PACK8(output_enabled);
    emu_printf("output_enabled = %d\n", output_enabled);
    PACK16(temp1);
    PACK16(temp2);
    PACK8(temp_shutdown);
    PACK_CSTR(curr_func);
    emu_printf("%s:\n", curr_func);
    for (uint32_t i=0; i < num_param; i++) {
        opendps_get_curr_function_param_value(params[i].name, value, sizeof(value));
        emu_printf(" %s = %s\n" , params[i].name, value);
        PACK_CSTR(params[i].name);
        PACK_CSTR(value);
    }
    FINISH_FRAME();

    send_frame(_buffer, _length);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_set_function(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    uint32_t i = 0;
    char func_name[32];
    bool success = false;
    uint8_t ch;
    memset(func_name, 0, sizeof(func_name));
    {
        command_t cmd;
        DECLARE_UNPACK(payload, payload_len);
        UNPACK8(cmd);
        (void) cmd;
        do {
            UNPACK8(ch);
            func_name[i++] = ch;
        } while(i < sizeof(func_name) - 1 && ch && _remain);

        char *names[8];
        uint32_t num_funcs = opendps_get_function_names(names, 8);
        for (i = 0; i < num_funcs; i++) {
            if (strcmp(names[i], func_name) == 0) {
                success = true;
                break;
            }
        }
    }
    if (success) {
        emu_printf("Changing to function %s\n", func_name);
        success = opendps_enable_function_idx(i);
    } else {
        emu_printf("Function %s not available\n", func_name);
    }
    
    {
        DECLARE_FRAME(MAX_FRAME_LENGTH);
        PACK8(cmd_response | cmd_set_function);
        PACK8(success); // Always success
        FINISH_FRAME();
        send_frame(_buffer, _length);
    }
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_list_functions(void)
{
    emu_printf("%s\n", __FUNCTION__);
    char *names[OPENDPS_MAX_PARAMETERS];
    uint32_t num_funcs = opendps_get_function_names(names, OPENDPS_MAX_PARAMETERS);
    emu_printf("Got %d functions\n" , num_funcs);
    DECLARE_FRAME(MAX_FRAME_LENGTH);
    PACK8(cmd_response | cmd_list_functions);
    PACK8(1); // Always success
    for (uint32_t i=0; i < num_funcs; i++) {
        emu_printf(" %s\n" , names[i]);
        PACK_CSTR(names[i]);
    }
    FINISH_FRAME();
    send_frame(_buffer, _length);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_set_parameters(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    char *name = 0, *value = 0;
    command_t cmd;
    set_param_status_t stats[OPENDPS_MAX_PARAMETERS];
    uint32_t status_index = 0;
    {
        DECLARE_UNPACK(payload, payload_len);
        UNPACK8(cmd);
        (void) cmd;
        do {
            /** Extract all occurences of <name>=<value>\0 ... */
            name = value = 0;
            /** This is quite ugly, please don't look */
            name = (char*) &_buffer[_pos];
            _pos += strlen(name) + 1;
            _remain -= strlen(name) + 1;
            value = (char*) &_buffer[_pos];
            _pos += strlen(value) + 1;
            _remain -= strlen(value) + 1;
            if (name && value) {
                stats[status_index++] = opendps_set_parameter(name, value);
            }
        } while(_remain && status_index < OPENDPS_MAX_PARAMETERS);
    }

    {
        DECLARE_FRAME(MAX_FRAME_LENGTH);
        PACK8(cmd_response | cmd_set_parameters);
        PACK8(1); // Always success
        for (uint32_t i = 0; i < status_index; i++) {
            PACK8(stats[i]);
        }
        FINISH_FRAME();
        send_frame(_buffer, _length);
    }
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_list_parameters(void)
{
    emu_printf("%s\n", __FUNCTION__);
    ui_parameter_t *params;
    uint32_t num_param = opendps_get_curr_function_params(&params);

    const char* name = opendps_get_curr_function_name();
    emu_printf("Got %d parameters for %s\n" , num_param, name);
    DECLARE_FRAME(MAX_FRAME_LENGTH);
    PACK8(cmd_response | cmd_list_parameters);
    PACK8(1); // Always success
    /** Pack name of current function */
    PACK_CSTR(name);

    for (uint32_t i=0; i < num_param; i++) {
        emu_printf(" %s %d %d\n", params[i].name, params[i].unit, params[i].prefix);
        PACK_CSTR(params[i].name);
        PACK8(params[i].unit);
        PACK8(params[i].prefix);
    }
    FINISH_FRAME();
    send_frame(_buffer, _length);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_enable_output(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    uint8_t cmd;
    bool enable;
    DECLARE_UNPACK(payload, payload_len);
    UNPACK8(cmd);
    (void) cmd;
    UNPACK8(enable);
    enable = !!enable;
    if (opendps_enable_output(enable)) {
        return cmd_success;
    } else {
        return cmd_failed;
    }
}

static command_status_t handle_temperature(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    command_t cmd;
    int16_t temp1, temp2;
    DECLARE_UNPACK(payload, payload_len);
    UNPACK8(cmd);
    (void) cmd;
    UNPACK16(temp1);
    UNPACK16(temp2);
    opendps_set_temperature(temp1, temp2);
    return cmd_success;
}

/**
  * @brief Handle a wifi status command
  * @param payload payload of command frame
  * @param payload_len length of payload
 * @retval command_status_t failed, success or "I sent my own frame"
  */
static command_status_t handle_wifi_status(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    command_status_t success = cmd_failed;
    wifi_status_t status;
    if (protocol_unpack_wifi_status(payload, payload_len, &status)) {
        success = cmd_success;
        opendps_update_wifi_status(status);
    }
    return success;
}

/**
  * @brief Handle a lock command
  * @param payload payload of command frame
  * @param payload_len length of payload
 * @retval command_status_t failed, success or "I sent my own frame"
  */
static command_status_t handle_lock(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    command_status_t success = cmd_failed;
    uint8_t status;
    if (protocol_unpack_lock(payload, payload_len, &status)) {
        success = cmd_success;
        opendps_lock(status);
    }
    return success;
}

/**
  * @brief Handle an upgrde start command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval false in case of errors, if successful the device reboots
  */
static command_status_t handle_upgrade_start(uint8_t *payload, uint32_t payload_len)
{
    emu_printf("%s\n", __FUNCTION__);
    command_status_t success = cmd_failed;
    uint16_t chunk_size, crc;
    if (protocol_unpack_upgrade_start(payload, payload_len, &chunk_size, &crc)) {
        bootcom_put(0xfedebeda, (chunk_size << 16) | crc);
        scb_reset_system();
    }
    return success;
}

/**
  * @brief Handle a receved frame
  * @param frame the received frame
  * @param length length of frame
  * @retval None
  */
static void handle_frame(uint8_t *frame, uint32_t length)
{
    command_status_t success = cmd_failed;
    command_t cmd = cmd_response;
    uint8_t *payload;
    int32_t payload_len = uframe_extract_payload(frame, length);
    payload = frame; // Why? Well, frame now points to the payload
    if (payload_len <= 0) {
        dbg_printf("Frame error %ld\n", payload_len);
    } else {
        cmd = frame[0];
        switch(cmd) {
            case cmd_ping:
                success = 1; // Response will be sent below
                emu_printf("Got pinged\n");
                opendps_handle_ping();
                break;
            case cmd_set_function:
                success = handle_set_function(payload, payload_len);
                break;
            case cmd_list_functions:
                success = handle_list_functions();
                break;
            case cmd_set_parameters:
                success = handle_set_parameters(payload, payload_len);
                break;
            case cmd_list_parameters:
                success = handle_list_parameters();
                break;
            case cmd_query:
                success = handle_query();
                break;
            case cmd_wifi_status:
                success = handle_wifi_status(payload, payload_len);
                break;
            case cmd_lock:
                success = handle_lock(payload, payload_len);
                break;
            case cmd_upgrade_start:
                success = handle_upgrade_start(payload, payload_len);
                break;
            case cmd_enable_output:
                success = handle_enable_output(payload, payload_len);
                break;
            case cmd_temperature_report:
                success = handle_temperature(payload, payload_len);
                break;
            default:
                emu_printf("Got unknown command %d (0x%02x)\n", cmd, cmd);
                break;
        }
    }
    if (success != cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much) {
        length = protocol_create_response(frame_buffer, sizeof(frame_buffer), cmd, success);
        if (length > 0 && cmd != cmd_response) {
            send_frame(frame_buffer, length);
        }
    }
}

/**
  * @brief Handle received character
  * @param c well, the received character
  * @retval None
  */
void serial_handle_rx_char(char c)
{
    uint8_t b = (uint8_t) c;
    if (b == _SOF) {
        receiving_frame = true;
        rx_idx = 0;
    }
    if (receiving_frame) {
        if (rx_idx < sizeof(frame_buffer)) {
            frame_buffer[rx_idx++] = b;
            if (b == _EOF) {
                handle_frame(frame_buffer, rx_idx);
                receiving_frame = false;
            }
        } else {
            dbg_printf("Error: RX buffer overflow!\n");
        }
    }
}
