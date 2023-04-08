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
#include <dac.h>
#include "dbg_printf.h"
#include "dps-model.h"
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
 extern void dps_emul_send_frame(frame_t *frame);
#endif // DPS_EMULATOR

typedef enum {
    cmd_failed = 0,
    cmd_success,
    cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much,
} command_status_t;

static uint8_t frame_buffer[MAX_FRAME_LENGTH];
static uint32_t rx_idx = 0;
static bool receiving_frame = false;

/**
  * @brief Send a frame on the uart
  * @param frame the frame to send
  * @param length length of frame
  * @retval None
  */
static void send_frame(const frame_t *frame)
{
#ifdef DPS_EMULATOR
    dps_emul_send_frame(frame);
#else // DPS_EMULATOR
    for (uint32_t i = 0; i < frame->length; ++i)
        usart_send_blocking(USART1, frame->buffer[i]);
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
    int16_t temp1 = INVALID_TEMPERATURE, temp2 = INVALID_TEMPERATURE;
    bool temp_shutdown = 0;
#ifdef CONFIG_THERMAL_LOCKOUT
    opendps_get_temperature(&temp1, &temp2, &temp_shutdown);
#endif // CONFIG_THERMAL_LOCKOUT
//    uint32_t len = protocol_create_query_response(frame_buffer, sizeof(frame_buffer), v_in, v_out_setting, v_out, i_out, i_limit, power_enabled);

    frame_t frame;

    set_frame_header(&frame);
    pack8(&frame, cmd_response | cmd_query);

    
    pack8(&frame, 1); // Always success
    pack16(&frame, v_in);
    emu_printf("v_in = %d\n", v_in);
    pack16(&frame, v_out);
    emu_printf("v_out = %d\n", v_out);
    pack16(&frame, i_out);
    emu_printf("i_out = %d\n", i_out);
    pack8(&frame, output_enabled);
    emu_printf("output_enabled = %d\n", output_enabled);
    pack16(&frame, temp1);
    pack16(&frame, temp2);
    pack8(&frame, temp_shutdown);
    pack_cstr(&frame, curr_func);
    emu_printf("%s:\n", curr_func);
    for (uint32_t i=0; i < num_param; i++) {
        opendps_get_curr_function_param_value(params[i].name, value, sizeof(value));
        emu_printf(" %s = %s\n" , params[i].name, value);
        pack_cstr(&frame, params[i].name);
        pack_cstr(&frame, value);
    }
    end_frame(&frame);

    send_frame(&frame);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_set_function(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    uint32_t i = 0;
    char func_name[32];
    bool success = false;
    uint8_t ch;
    memset(func_name, 0, sizeof(func_name));
    {
        command_t cmd;
        start_frame_unpacking(frame);
        unpack8(frame, &cmd);
        (void) cmd;
        do {
            unpack8(frame, &ch);
            func_name[i++] = ch;
        } while(i < sizeof(func_name) - 1 && ch && frame->length);

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
        frame_t frame_resp;
        set_frame_header(&frame_resp);
        pack8(&frame_resp, cmd_response | cmd_set_function);
        pack8(&frame_resp, success); // Always success
        end_frame(&frame_resp);
        send_frame(&frame_resp);
    }
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_list_functions(void)
{
    emu_printf("%s\n", __FUNCTION__);
    char *names[OPENDPS_MAX_PARAMETERS];
    uint32_t num_funcs = opendps_get_function_names(names, OPENDPS_MAX_PARAMETERS);
    emu_printf("Got %d functions\n" , num_funcs);
    frame_t frame;
    set_frame_header(&frame);
    pack8(&frame, cmd_response | cmd_list_functions);
    pack8(&frame, 1); // Always success
    for (uint32_t i=0; i < num_funcs; i++) {
        emu_printf(" %s\n" , names[i]);
        pack_cstr(&frame, names[i]);
    }
    end_frame(&frame);
    send_frame(&frame);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_set_parameters(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    char *name = 0, *value = 0;
    command_t cmd;
    set_param_status_t stats[OPENDPS_MAX_PARAMETERS];
    uint32_t status_index = 0;
    {
        start_frame_unpacking(frame);
        unpack8(frame, &cmd);
        (void) cmd;
        do {
            /** Extract all occurences of <name>=<value>\0 ... */
            name = value = 0;
            /** This is quite ugly, please don't look */
            name = (char*) &frame->buffer[frame->unpack_pos];
            frame->unpack_pos += strlen(name) + 1;
            frame->length -= strlen(name) + 1;
            value = (char*) &frame->buffer[frame->unpack_pos];
            frame->unpack_pos += strlen(value) + 1;
            frame->length -= strlen(value) + 1;
            if (name && value) {
                stats[status_index++] = opendps_set_parameter(name, value);
            }
        } while(frame->length && status_index < OPENDPS_MAX_PARAMETERS);
    }

    {
        frame_t frame_resp;
        set_frame_header(&frame_resp);
        pack8(&frame_resp, cmd_response | cmd_set_parameters);
        pack8(&frame_resp, 1); // Always success
        for (uint32_t i = 0; i < status_index; i++) {
            pack8(&frame_resp, stats[i]);
        }
        end_frame(&frame_resp);
        send_frame(&frame_resp);
    }
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_set_calibration(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    char *name = 0;
    float *value = 0;
    command_t cmd;
    set_param_status_t stats[OPENDPS_MAX_PARAMETERS];
    uint32_t status_index = 0;
    {
        start_frame_unpacking(frame);
        unpack8(frame, &cmd);
        (void) cmd;
        do {
            /** Extract all occurences of <name>=<value> ... */
            name = 0;
            /** This is quite ugly (again), please don't look */
            name = (char*) &frame->buffer[frame->unpack_pos];
            frame->unpack_pos += strlen(name) + sizeof(char);
            frame->length -= strlen(name) + sizeof(char);
            value = (float*) &frame->buffer[frame->unpack_pos];
            frame->unpack_pos += sizeof(float);
            frame->length -= sizeof(float);
            if (name && value) {
                stats[status_index++] = opendps_set_calibration(name, value);
            }
        } while(frame->length && status_index < OPENDPS_MAX_PARAMETERS);
    }

    {
        frame_t frame_resp;
        set_frame_header(&frame_resp);
        pack8(&frame_resp, cmd_response | cmd_set_calibration);
        pack8(&frame_resp, 1); // Always success
        for (uint32_t i = 0; i < status_index; i++) {
            pack8(&frame_resp, stats[i]);
        }
        end_frame(&frame_resp);
        send_frame(&frame_resp);
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
    frame_t frame;
    set_frame_header(&frame);
    pack8(&frame, cmd_response | cmd_list_parameters);
    pack8(&frame, 1); // Always success
    /** Pack name of current function */
    pack_cstr(&frame, name);

    for (uint32_t i=0; i < num_param; i++) {
        emu_printf(" %s %d %d\n", params[i].name, params[i].unit, params[i].prefix);
        pack_cstr(&frame, params[i].name);
        pack8(&frame, params[i].unit);
        pack8(&frame, params[i].prefix);
    }
    end_frame(&frame);
    send_frame(&frame);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

static command_status_t handle_enable_output(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    uint8_t cmd;
    uint8_t enable_byte;
    start_frame_unpacking(frame);
    unpack8(frame, &cmd);
    (void) cmd;
    unpack8(frame, &enable_byte);
    bool enable = !!enable_byte;
    if (opendps_enable_output(enable)) {
        return cmd_success;
    } else {
        return cmd_failed;
    }
}

static command_status_t handle_set_brightness(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    uint8_t cmd;
    uint8_t brightness_pct;
    start_frame_unpacking(frame);
    unpack8(frame, &cmd);
    (void) cmd;
    unpack8(frame, &brightness_pct);
    hw_set_backlight(brightness_pct);
    return cmd_success;
}

#ifdef CONFIG_THERMAL_LOCKOUT
static command_status_t handle_temperature(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    command_t cmd;
    int16_t temp1, temp2;
    start_frame_unpacking(frame);
    unpack8(frame, &cmd);
    (void) cmd;
    unpack16(frame, (uint16_t*) &temp1);
    unpack16(frame, (uint16_t*) &temp2);
    opendps_set_temperature(temp1, temp2);
    return cmd_success;
}
#endif // CONFIG_THERMAL_LOCKOUT

static command_status_t handle_version(void)
{
    emu_printf("%s\n", __FUNCTION__);
    uint32_t boot_str_len, app_str_len;
    const char *boot_git_hash = 0;
    const char *app_git_hash = 0;

    /** Load and check both GIT hashes exist */ 
    boot_str_len = opendps_get_boot_git_hash(&boot_git_hash);
    app_str_len = opendps_get_app_git_hash(&app_git_hash);

    frame_t frame;
    set_frame_header(&frame);
    pack8(&frame, cmd_response | cmd_version);
    if (boot_str_len > 0 &&
        app_str_len > 0)
    {
        pack8(&frame, 1);
        pack_cstr(&frame, boot_git_hash);
        pack_cstr(&frame, app_git_hash);
    }
    else
    {
        pack8(&frame, 0);
        pack8(&frame, '\0'); pack8(&frame, '\0'); /** Pack two empty strings */ 
    }
    end_frame(&frame);

    send_frame(&frame);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

/**
  * @brief Handle a cal report
  * @retval command_status_t failed, success or "I sent my own frame"
  */
static command_status_t handle_cal_report(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);

    frame_t frame;
    set_frame_header(&frame);
    pack8(&frame, cmd_response | cmd_cal_report);
    pack8(&frame, 1); 
    pack16(&frame, v_out_raw);
    pack16(&frame, v_in_raw);
    pack16(&frame, i_out_raw);
    pack16(&frame, DAC_DHR12R2(DAC1));
    pack16(&frame, DAC_DHR12R1(DAC1));
    pack_float(&frame, a_adc_k_coef);
    pack_float(&frame, a_adc_c_coef);
    pack_float(&frame, a_dac_k_coef);
    pack_float(&frame, a_dac_c_coef);
    pack_float(&frame, v_adc_k_coef);
    pack_float(&frame, v_adc_c_coef);
    pack_float(&frame, v_dac_k_coef);
    pack_float(&frame, v_dac_c_coef);
    pack_float(&frame, vin_adc_k_coef);
    pack_float(&frame, vin_adc_c_coef);
    end_frame(&frame);
    send_frame(&frame);
    return cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much;
}

/**
  * @brief  Clear and set calibration values
  * @retval cmd_success on success else cmd_failed
  */
static command_status_t handle_clear_calibration(void)
{
    if (opendps_clear_calibration())
        return cmd_success;
    else
        return cmd_failed;
}

/**
  * @brief Handle a wifi status command
  * @param payload payload of command frame
  * @param payload_len length of payload
 * @retval command_status_t failed, success or "I sent my own frame"
  */
static command_status_t handle_wifi_status(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    command_status_t success = cmd_failed;
    wifi_status_t status;
    if (protocol_unpack_wifi_status(frame, &status)) {
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
static command_status_t handle_lock(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    command_status_t success = cmd_failed;
    uint8_t status;
    if (protocol_unpack_lock(frame, &status)) {
        success = cmd_success;
        opendps_lock(status);
    }
    return success;
}

/**
  * @brief Handle an upgrade start command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval false in case of errors, if successful the device reboots
  */
static command_status_t handle_upgrade_start(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    command_status_t success = cmd_failed;
    uint16_t chunk_size, crc;
    if (protocol_unpack_upgrade_start(frame, &chunk_size, &crc)) {
        bootcom_put(0xfedebeda, (chunk_size << 16) | crc);
        opendps_upgrade_start();
    }
    return success;
}

/**
  * @brief Handle a change screen command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval false in case of errors, if successful the device reboots
  */
static command_status_t handle_change_screen(frame_t *frame)
{
    emu_printf("%s\n", __FUNCTION__);
    uint8_t cmd, screen_id;
    start_frame_unpacking(frame);
    unpack8(frame, &cmd);
    (void) cmd;
    unpack8(frame, &screen_id);
    if (opendps_change_screen(screen_id)) {
        return cmd_success;
    } else {
        return cmd_failed;
    }
}

/**
  * @brief Handle a receved frame
  * @param frame the received frame
  * @param length length of frame
  * @retval None
  */
static void handle_frame(uint8_t *data, uint32_t length)
{
    command_status_t success = cmd_failed;
    command_t cmd = cmd_response;

    frame_t frame;

    int32_t payload_len = uframe_extract_payload(&frame, data, length);

    if (payload_len <= 0) {
        dbg_printf("Frame error %ld\n", payload_len);
    } else {
        cmd = frame.buffer[0];
        switch(cmd) {
            case cmd_ping:
                success = 1; // Response will be sent below
                emu_printf("Got pinged\n");
                opendps_handle_ping();
                break;
            case cmd_set_function:
                success = handle_set_function(&frame);
                break;
            case cmd_list_functions:
                success = handle_list_functions();
                break;
            case cmd_set_parameters:
                success = handle_set_parameters(&frame);
                break;
            case cmd_list_parameters:
                success = handle_list_parameters();
                break;
            case cmd_query:
                success = handle_query();
                break;
            case cmd_wifi_status:
                success = handle_wifi_status(&frame);
                break;
            case cmd_lock:
                success = handle_lock(&frame);
                break;
            case cmd_upgrade_start:
                success = handle_upgrade_start(&frame);
                break;
            case cmd_enable_output:
                success = handle_enable_output(&frame);
                break;
#ifdef CONFIG_THERMAL_LOCKOUT
            case cmd_temperature_report:
                success = handle_temperature(&frame);
                break;
#endif // CONFIG_THERMAL_LOCKOUT
            case cmd_version:
                success = handle_version();
                break;
            case cmd_cal_report:
                success = handle_cal_report();
                break;
            case cmd_set_calibration:
                success = handle_set_calibration(&frame);
                break;
            case cmd_clear_calibration:
                success = handle_clear_calibration();
                break;
            case cmd_change_screen:
                success = handle_change_screen(&frame);
                break;
            case cmd_set_brightness:
                success = handle_set_brightness(&frame);
                break;
            default:
                emu_printf("Got unknown command %d (0x%02x)\n", cmd, cmd);
                break;
        }
    }
    if (success != cmd_success_but_i_actually_sent_my_own_status_thank_you_very_much) {
        frame_t frame_resp;
        protocol_create_response(&frame_resp, cmd, success);
        if (frame_resp.length > 0 && cmd != cmd_response) {
            send_frame(&frame_resp);
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
