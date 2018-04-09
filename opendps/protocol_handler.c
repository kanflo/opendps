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
#include "pwrctl.h"
#include "uframe.h"
#include "protocol.h"
#include "serialhandler.h"
#include "bootcom.h"

// From opendps.c
void ui_update_power_status(bool enabled);
void ui_update_wifi_status(wifi_status_t status);
void ui_handle_ping(void);
void ui_lock(bool lock);

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
    do {
        usart_send_blocking(USART1, *frame);
        frame++;
    } while(--length);
}

/**
  * @brief Handle a set V_out command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval true if frame was correct and command executed ok
  */
static bool handle_set_vout(uint8_t *payload, uint32_t payload_len)
{
    bool success = false;
    uint16_t setting;
    if (protocol_unpack_vout(payload, payload_len, &setting)) {
        success = pwrctl_set_vout(setting);
    }
    return success;
}

/**
  * @brief Handle a set I_limit command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval true if frame was correct and command executed ok
  */
static bool handle_set_ilimit(uint8_t *payload, uint32_t payload_len)
{
    bool success = false;
    uint16_t setting;
    if (protocol_unpack_ilimit(payload, payload_len, &setting)) {
        success = pwrctl_set_ilimit(setting);
    }
    return success;
}

/**
  * @brief Handle a status command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval true if frame was correct and command executed ok
  */
static bool handle_status(void)
{
    uint16_t i_out_raw, v_in_raw, v_out_raw;
    hw_get_adc_values(&i_out_raw, &v_in_raw, &v_out_raw);
    uint16_t v_in = pwrctl_calc_vin(v_in_raw);
    uint16_t v_out = pwrctl_calc_vout(v_out_raw);
    uint16_t i_out = pwrctl_calc_iout(i_out_raw);
    uint16_t v_out_setting = pwrctl_get_vout();
    uint16_t i_limit = pwrctl_get_ilimit();
    uint8_t power_enabled = pwrctl_vout_enabled();
    uint32_t len = protocol_create_status_response(frame_buffer, sizeof(frame_buffer), v_in, v_out_setting, v_out, i_out, i_limit, power_enabled);
    send_frame(frame_buffer, len);
    return true;
}

/**
  * @brief Handle a power enable command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval true if frame was correct and command executed ok
  */
static bool handle_power_enable(uint8_t *payload, uint32_t payload_len)
{
    bool success = false;
    uint8_t status;
    if (protocol_unpack_power_enable(payload, payload_len, &status)) {
        pwrctl_enable_vout(status);
        ui_update_power_status(status);
        success = true;
    }
    return success;
}

/**
  * @brief Handle a wifi status command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval true if frame was correct and command executed ok
  */
static bool handle_wifi_status(uint8_t *payload, uint32_t payload_len)
{
    bool success = false;
    wifi_status_t status;
    if (protocol_unpack_wifi_status(payload, payload_len, &status)) {
        success = true;
        ui_update_wifi_status(status);
    }
    return success;
}

/**
  * @brief Handle a lock command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval true if frame was correct and command executed ok
  */
static bool handle_lock(uint8_t *payload, uint32_t payload_len)
{
    bool success = false;
    uint8_t status;
    if (protocol_unpack_lock(payload, payload_len, &status)) {
        success = true;
        ui_lock(status);
    }
    return success;
}

/**
  * @brief Handle an upgrde start command
  * @param payload payload of command frame
  * @param payload_len length of payload
  * @retval false in case of errors, if successful the device reboots
  */
static bool handle_upgrade_start(uint8_t *payload, uint32_t payload_len)
{
    bool success = false;
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
    bool success = false;
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
                ui_handle_ping();
                break;
            case cmd_set_vout:
                success = handle_set_vout(payload, payload_len);
                break;
            case cmd_set_ilimit:
                success = handle_set_ilimit(payload, payload_len);
                break;
            case cmd_status:
                success = handle_status();
                break;
            case cmd_power_enable:
                success = handle_power_enable(payload, payload_len);
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
            default:
                break;
        }
    }
    if (cmd_status != cmd) { // The status command sends its own frame
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
    if (receiving_frame && rx_idx < sizeof(frame_buffer)) {
        frame_buffer[rx_idx++] = b;
        if (b == _EOF) {
            handle_frame(frame_buffer, rx_idx);
            receiving_frame = false;
        }
    }
}
