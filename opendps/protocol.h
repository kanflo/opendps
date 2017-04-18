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


/* This module defines the serial interface protocol. Everything you can do via
 * the buttons and dial on the DPS can be instrumented via the serial port.
 *
 * The basic frame payload is [<cmd>] [<optional payload>]* to which the device
 * will respond [cmd_response | <cmd>] [success] [<response data>]*
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum { /** Make sure these match protocol.py */
    cmd_ping = 1,
    cmd_set_vout = 2,
    cmd_set_ilimit = 3,
    cmd_status = 4,
    cmd_power_enable = 5,
    cmd_wifi_status = 6,
    cmd_lock = 7,
    cmd_ocp_event = 8,
    cmd_output_mode = 9,
    cmd_response = 0x80
} command_t;

typedef enum {
    wifi_off = 0,
    wifi_connecting,
    wifi_connected,
    wifi_error,
    wifi_upgrading // Used by the ESP8266 when doing FOTA
} wifi_status_t;

#define MAX_FRAME_LENGTH (2*16) // Based on the cmd_status reponse frame (fullu escaped)

/*
 * Helpers for creating frames.
 *
 * On return, 'frame' will hold a complete frame ready for transmission and the
 * return value will be the length of the frame. If the specified 'length' of
 * the frame buffer is not sufficient, the return value will be zero and the
 * 'frame' buffer is left untouched.
 */
uint32_t protocol_create_response(uint8_t *frame, uint32_t length, command_t cmd, uint8_t success);
uint32_t protocol_create_ping(uint8_t *frame, uint32_t length);
uint32_t protocol_create_power_enable(uint8_t *frame, uint32_t length, uint8_t enable);
uint32_t protocol_create_vout(uint8_t *frame, uint32_t length, uint16_t vout_mv);
uint32_t protocol_create_ilimit(uint8_t *frame, uint32_t length, uint16_t ilimit_ma);
uint32_t protocol_create_status(uint8_t *frame, uint32_t length);
uint32_t protocol_create_status_response(uint8_t *frame, uint32_t length, uint16_t v_in, uint16_t v_out_setting, uint16_t v_out, uint16_t i_out, uint16_t i_limit, uint8_t power_enabled);
uint32_t protocol_create_wifi_status(uint8_t *frame, uint32_t length, wifi_status_t status);
uint32_t protocol_create_lock(uint8_t *frame, uint32_t length, uint8_t locked);
uint32_t protocol_create_ocp(uint8_t *frame, uint32_t length, uint16_t i_cut);

/*
 * Helpers for unpacking frames.
 *
 * These functions will unpack the content of the unframed payload and return
 * true. If the command byte of the frame does not match the expectation or the
 * frame is too short to unpack the expected payload, false will be returned.
 */
bool protocol_unpack_response(uint8_t *payload, uint32_t length, command_t *cmd, uint8_t *success);
bool protocol_unpack_power_enable(uint8_t *payload, uint32_t length, uint8_t *enable);
bool protocol_unpack_vout(uint8_t *payload, uint32_t length, uint16_t *vout_mv);
bool protocol_unpack_ilimit(uint8_t *payload, uint32_t length, uint16_t *ilimit_ma);
bool protocol_unpack_status_response(uint8_t *payload, uint32_t length, uint16_t *v_in, uint16_t *v_out_setting, uint16_t *v_out, uint16_t *i_out, uint16_t *i_limit, uint8_t *power_enabled);
bool protocol_unpack_wifi_status(uint8_t *payload, uint32_t length, wifi_status_t *status);
bool protocol_unpack_lock(uint8_t *payload, uint32_t length, uint8_t *locked);
bool protocol_unpack_ocp(uint8_t *payload, uint32_t length, uint16_t *i_cut);
bool protocol_unpack_output_mode(uint8_t *payload, uint32_t length, uint8_t *mode);

/*
 *    *** Adding new commands ***
 *
 * opendps/protocol.h
 * ------------------
 *  Add command number in the command_t list
 *  Add a descrption of the command with parameters, response format etc. etc.
 *
 * dpsctl/protocol.py
 * ------------------
 *  Add the same command number in the cmd_* list
 *  Add command frame creeation (eg. cmd_set_my_thing(...))
 *  Add optional response frame in the same file.
 *
 * dpsctl/dpsctl.py
 * ----------------
 *  Add argument, eg. parser.add_argument('-q', '--quick', help="Quick command")
 *  Add command handler in handle_commands(...), eg. communicate(comms,
 *    protocol.cmd_quick_command(mode), args)
 *  Add optional response handler in handle_response(...)
 *
 * opendps/protocol.h
 * ------------------
 *  Add the same command number as in protocol.h
 *
 * opendps/protocol_handler.c
 * --------------------------
 *  Add command enum in handle_frame(...)
 *  Add command handler, eg. handle_quick(...)
 *  Add frame unpacker, eg. protocol_unpack_quick(...)
 *  Do your thing
 *
 * There are simple commands such as the 'lock' and 'power mode' commands and more
 * complex commands such as the 'status' command returning a bit if data.
 *
 *
 *    *** Command types ***
 *
 * === Pinging DPS ===
 * The ping command is sent by the host to check if the DPS is online.
 *
 *  HOST:   [cmd_ping]
 *  DPS:    [cmd_response | cmd_ping] [1]
 *
 *
 * === Setting desired out voltage ===
 * The voltage is specified in millivolts. The success response field will be
 * 0 if the requested voltage was outside of what the DPS can provide.
 *
 *  HOST:   [cmd_set_vout] [vout_mv(15:8)] [vout_mv(7:0)]
 *  DPS:    [cmd_response | cmd_set_vout] [<success>]
 *
 *
 * === Setting maximum current limit ===
 * The current is specified in milliampere. The success response field will be
 * 0 if the requested current was outside of what the DPS can provide.
 *
 *  HOST:   [cmd_set_ilimit] [ilimit_ma(15:8)] [ilimit_ma(7:0)]
 *  DPS:    [cmd_response | cmd_set_ilimit] [<success>]
 *
 *
 * === Setting CV or CC mode ===
 * The <mode> is 0 for CV and 1 for CC
 * 0 if the requested current was outside of what the DPS can provide.
 *
 *  HOST:   [cmd_output_mode] [<mode>]
 *  DPS:    [cmd_response | cmd_output_mode] [<success>]
 *
 *
 * === Reading the status of the DPS ===
 * This command retrieves V_in, V_out, I_out, I_limit, power enable. Voltage
 * and currents are all in the 'milli' range.
 *
 *  HOST:   [cmd_status]
 *  DPS:    [cmd_response | cmd_status] [1] [V_in(15:8)] [V_in(7:0)] [V_out_setting(15:8)] [V_out_setting(7:0)] [V_out(15:8)] [V_out(7:0)] [I_out(15:8)] [I_out(7:0)] [I_limit(15:8)] [I_limit(7:0)] [<power enable>]
 *
 *
 * === Enabling/disabling power output ===
 * This command is used to enable or disable power output. Enable = 1 will
 * obviously enable :)
 *
 *  HOST:   [cmd_power_enable] [<enable>]
 *  DPS:    [cmd_response | cmd_power_enable] [1]
 *
 *
 * === Setting wifi status ===
 * This command is used to set the wifi indicator on the screen. Status will be
 * one of the wifi_status_t enums
 *
 *  HOST:   [cmd_wifi_status] [<wifi_status_t>]
 *  DPS:    [cmd_response | cmd_wifi_status] [1]
 *
 *
 * === Locking the controls ===
 * This command is used to lock or unlock the controls.
 * lock = 1 will do just that
 *
 *  HOST:   [cmd_lock] [<lock>]
 *  DPS:    [cmd_response | cmd_lock] [1]
 *
 *
 * === Overcurrent protection event controls ===
 * If the DPS detects overcurrent, it will send this frame with the current
 * that caused the protection to kick in (in milliamperes).
 * The DPS does not expect a response
 *
 *  DPS:    [cmd_ocp_event] [I_cut(7:0)] [I_cut(15:8)]
 *  HOST:   none
 *
 */

#endif // __PROTOCOL_H__