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
#include "uframe.h"

typedef enum {
    cmd_ping = 1,
    __obsolete_cmd_set_vout, /** Kept for keeping enum and Python code in sync */
    __obsolete_cmd_set_ilimit,
    cmd_query,
    __obsolete_cmd_power_enable,
    cmd_wifi_status,
    cmd_lock,
    cmd_ocp_event,
    cmd_upgrade_start,
    cmd_upgrade_data,
    cmd_set_function,
    cmd_enable_output,
    cmd_list_functions,
    cmd_set_parameters,
    cmd_list_parameters,
    cmd_temperature_report,
    cmd_version,
    cmd_cal_report,
    cmd_set_calibration,
    cmd_clear_calibration,
    cmd_change_screen,
    cmd_set_brightness,
    cmd_response = 0x80
} command_t;

typedef enum {
    wifi_off = 0,
    wifi_connecting,
    wifi_connected,
    wifi_error,
    wifi_upgrading // Used by the ESP8266 when doing FOTA
} wifi_status_t;

typedef enum {
    upgrade_continue = 0, /** device sent go-ahead for continued upgrade */
    upgrade_bootcom_error, /** device found errors in the bootcom data */
    upgrade_crc_error, /** crc verification of downloaded upgrade failed */
    upgrade_erase_error, /** device encountered error while erasing flash */
    upgrade_flash_error, /** device encountered error while writing to flash */
    upgrade_overflow_error, /** downloaded image would overflow flash */
    upgrade_protocol_error, /** device received upgrade data but no upgrade start */
    upgrade_success = 16 /** device received entire firmware and crc, branch verification was successful */
} upgrade_status_t;

/** The boot will report why it entered upgrade mode */
typedef enum {
    reason_unknown = 0, /** No idea why I'm here */
    reason_forced, /** User forced via button */
    reason_past_failure, /** Past init failed */
    reason_bootcom, /** App told us via bootcom */
    reason_unfinished_upgrade, /** A previous unfinished sympathy, eh upgrade */
    reason_app_start_failed /** App returned */
} upgrade_reason_t;

/** Used in cmd_set_parameters responses */
typedef enum {
    sp_ok = 1,
    sp_unknown_parameter,
    sp_illegal_value
} set_parameter_status_t;

#define INVALID_TEMPERATURE (0xffff)

/*
 * Helpers for creating frames.
 *
 * On return, 'frame' will hold a complete frame ready for transmission and the 
 * return value will be the length of the frame. If the specified 'length' of 
 * the frame buffer is not sufficient, the return value will be zero and the 
 * 'frame' buffer is left untouched.
 */
void protocol_create_response(frame_t *frame, command_t cmd, uint8_t success);
void protocol_create_ping(frame_t *frame);
void protocol_create_power_enable(frame_t *frame, uint8_t enable);
void protocol_create_vout(frame_t *frame, uint16_t vout_mv);
void protocol_create_ilimit(frame_t *frame, uint16_t ilimit_ma);
void protocol_create_status(frame_t *frame);
void protocol_create_query_response(frame_t *frame, uint16_t v_in, uint16_t v_out_setting, uint16_t v_out, uint16_t i_out, uint16_t i_limit, uint8_t power_enabled);
void protocol_create_wifi_status(frame_t *frame, wifi_status_t status);
void protocol_create_lock(frame_t *frame, uint8_t locked);
void protocol_create_ocp(frame_t *frame, uint16_t i_cut);

/*
 * Helpers for unpacking frames.
 *
 * These functions will unpack the content of the unframed payload and return
 * true. If the command byte of the frame does not match the expectation or the
 * frame is too short to unpack the expected payload, false will be returned.
 */
bool protocol_unpack_response(frame_t *frame, command_t *cmd, uint8_t *success);
bool protocol_unpack_power_enable(frame_t *frame, uint8_t *enable);
bool protocol_unpack_vout(frame_t *frame, uint16_t *vout_mv);
bool protocol_unpack_ilimit(frame_t *frame, uint16_t *ilimit_ma);
bool protocol_unpack_query_response(frame_t *frame, uint16_t *v_in, uint16_t *v_out_setting, uint16_t *v_out, uint16_t *i_out, uint16_t *i_limit, uint8_t *power_enabled);
bool protocol_unpack_wifi_status(frame_t *frame, wifi_status_t *status);
bool protocol_unpack_lock(frame_t *frame, uint8_t *locked);
bool protocol_unpack_ocp(frame_t *frame, uint16_t *i_cut);
bool protocol_unpack_upgrade_start(frame_t *frame, uint16_t *chunk_size, uint16_t *crc);


/*
 *    *** Command types ***
 *
 * === Pinging DPS ===
 * The ping command is sent by the host to check if the DPS is online.
 *
 *  HOST:   [cmd_ping]
 *  DPS:    [cmd_response | cmd_ping] [1]
 *
 *
 * === Reading the status of the DPS ===
 * @todo this description is obsolete...
 *
 * This command retrieves V_in, V_out, I_out, I_limit, power enable. Voltage
 * and currents are all in the 'milli' range.
 *
 *  HOST:   [cmd_query]
 *  DPS:    [cmd_response | ccmd_query] [1] [V_in(15:8)] [V_in(7:0)] [V_out_setting(15:8)] [V_out_setting(7:0)] [V_out(15:8)] [V_out(7:0)] [I_out(15:8)] [I_out(7:0)] [I_limit(15:8)] [I_limit(7:0)] [<power enable>]
 *
 *
 * === Changing active function ===
 * Remember functions are the generic term for the functions your OpenDPS suports
 * (constant voltage, constant current, ...) and the cmd_set_function command
 * sets the active function. The change will be reflected on the display and
 * the current function will be turned off. The function name is sent in ascii
 * and must match a function of one of the registered screens.
 *
 *  HOST:   [cmd_set_function] [<function name>]
 *  DPS:    [cmd_response | cmd_set_function] [<status>]
 *
 * <status> will be 1 or 0 depending on if the function is available or not
 *
 *
 * === Listing available functions ===
 * This command is used to list the available functions on the OpenDPS.
 * <status> is always 1.
 *
 *  HOST:   [cmd_list_functions]
 *  DPS:    [cmd_response | cmd_list_functions]  [<status>] <func name 1> \0 <func name 2> \0 ...
 *
 *
 * === Setting function parameters ===
 * Each function can be controlled using a set of named parameters. The set
 * parameter command supports piggybacking of several parameters sent to the
 * current active function. Both names and values are sent in ascii.
 * The reponse will contain as many statuses as parmeters received informing
 * the host of whether the parameter was successfully set, was out of bounds
 * or did not exist at all. The notation of named parametes decouples dpsctl
 * from the OpenDPS device; you may add any functions and parameters without
 * the need for updating dpsctl.
 *
 *  HOST:   [cmd_set_parameters <param 1> \0 <value 1> \0 <param 2> \0 <value 2> ... ]
 *  DPS:    [cmd_response | cmd_set_parameters] <param 1 > <set_parameter_status_t 1> <param 2> <set_parameter_status_t 2> ...
 *
 *
 * === Listing function parameters ===
 * This command replaces the old cmd_status command and returns a list of
 * parameters associated with the current active function, their values and a
 * list of system parameters:
 *
 *   - Power out status (1/0)
 *   - Input voltage (V.VV)
 *   - Output voltage (V.VV)
 *   - Output current (A.AAA)
 *
 *  HOST:   [cmd_list_parameters]
 *  DPS:    [cmd_response | cmd_list_parameters] <param 1> \0 <value 1> \0 <param 2> \0 <value 2> ... ]
 *
 *
 * === Receiving a temperature report ===
 * This command is used by a wifi companion with the ability to measure
 * temperature. Two temperatures are included as signed 16 bit integers x10
 * and 0xffff indicates an illegal temperature. You can decide for yourself
 * if your temperature unit is Celcius, Farenheit, Kelvin or one you just made
 * up for fun.
 *
 *  HOST:   [cmd_temperature_report <temp1[15:8]> <temp1[7:0]> <temp2[15:8]> <temp2[7:0]> ]
 *  DPS:    [cmd_response | cmd_temperature_report] ]
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
 *
 * === DPS upgrade sessions ===
 * When the cmd_upgrade_start packet is received, the device prepares for
 * an upgrade session:
 *  1. The upgrade packet chunk size is determined based on the host's request
 *     and is written into the bootcom RAM in addition with the 16 bit crc of
 *     the new firmware. and the upgrade magick.
 *  2. The device restarts.
 *  3. The booloader detecs the upgrade magic in the bootcom RAM.
 *  4. The booloader sets the upgrade flag in the PAST.
 *  5. The bootloader initializes the UART, sends the cmd_upgrade_start ack and
 *     prepares for download.
 *  6. The bootloader receives the upgrade packets, writes the data to flash
 *     and acks each packet.
 *  7. When the last packet has been received, the bootloader clears the upgrade
 *     flag in the PAST and boots the app.
 *  8. The host pings the app to check the new firmware started.
 *
 *  HOST:     [cmd_upgrade_start] [chunk_size:16] [crc:16]
 *  DPS (BL): [cmd_response | cmd_upgrade_start] [<upgrade_status_t>] [<chunk_size:16>]  [<upgrade_reason_t:8>]
 *
 * The host will send packets of the agreed chunk size with the device 
 * acknowledging each packet once crc checked and written to flash. A packet
 * smaller than the chunk size or with zero payload indicates the end of the
 * upgrade session. The device will now return the outcome of the 32 bit crc
 * check of the new firmware and continue on step 7 above.
 *
 * The upgrade data packets have the following format with the payload size
 * expected to be equal to what was aggreed upon in the cmd_upgrade_start packet.
 *
 *  HOST:   [cmd_upgrade_data] [<payload>]+
 *  DPS BL: [cmd_response | cmd_upgrade_data] [<upgrade_status_t>]
 *
 */

#endif // __PROTOCOL_H__
