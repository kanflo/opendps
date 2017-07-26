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
#include <scb.h>
#include <gpio.h>
#include <stdlib.h>
#include <systick.h>
#include <usart.h>
#include <timer.h>
#include <flash.h>
#include "tick.h"
#include "hw.h"
#include "ringbuf.h"
#include "past.h"
#include "uframe.h"
#include "protocol.h"
#include "bootcom.h"
#include "crc16.h"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define RX_BUF_SIZE  (16)
#define MAX_CHUNK_SIZE (2*1024)

/** A past unit who's precense indicates we have a non finished upgrade and
    must not boot */
#define PAST_UNIT_UPGRADE_STARTED  (0xff)
/** Our parameter storage */
static past_t past;

/** Linker file symbols */
extern uint32_t *_app_start;
extern uint32_t *_app_end;
extern uint32_t *_past_start;
extern uint32_t *_past_end;
extern uint32_t *_bootcom_start;
extern uint32_t *_bootcom_end;

/** UART RX FIFO */
static ringbuf_t rx_buf;
static uint8_t buffer[2*RX_BUF_SIZE];

/** UART frame buffer */
static uint8_t frame_buffer[FRAME_OVERHEAD(MAX_CHUNK_SIZE)];
static uint32_t rx_idx = 0;
static bool receiving_frame = false;

/** For keeping track of flash writing */
static uint16_t chunk_size;
static uint32_t cur_flash_address =  (uint32_t) &_app_start;
static uint16_t fw_crc16;

static void handle_frame(uint8_t *frame, uint32_t length);
static void send_frame(uint8_t *frame, uint32_t length);

/**
  * @brief Handle firmware upgrade
  * @retval none
  */
static void handle_upgrade(void)
{
    if (fw_crc16) { /** dpsctl.py is expecting a response */
        DECLARE_FRAME(MAX_FRAME_LENGTH);
        PACK8(cmd_response | cmd_upgrade_start);
        PACK8(upgrade_continue);
        PACK16(chunk_size);
        FINISH_FRAME();
        flash_unlock();
        uint32_t setting = 1;
        (void) past_write_unit(&past, PAST_UNIT_UPGRADE_STARTED, (void*) &setting, sizeof(setting));
        send_frame(_buffer, _length);
    }

    while(1) {
        uint16_t b;
        if (ringbuf_get(&rx_buf, &b)) {
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
    }
}

/**
  * @brief Branch to main application
  * @retval false if app start failed
  */
static bool start_app(void)
{
    /** Branch to the address in the reset entry of the app's exception vector */
    uint32_t *app_start = (uint32_t*) (4 + (uint32_t) &_app_start);
    if (((*app_start) & 0xffff0000) == 0x08000000) { /** Is there something there we can branch to? */
        ((void (*)(void))*app_start)();
    }
    return false;
}

/**
  * @brief Write 32 bits to flash
  * @param address word aligned address to write to
  * @param data well, data
  * @retval true if write was successful
  */
static inline bool flash_write32(uint32_t address, uint32_t data)
{
    bool success = false;
    if (address % 4 == 0) {
        uint32_t *p = (uint32_t*) address;
        flash_program_word(address, data);
        success = FLASH_SR_EOP & flash_get_status_flags();
        success &= *p == data; // Verify write
    }
    return success;
}

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
  * @brief Handle a receved frame
  * @param frame the received frame
  * @param length length of frame
  * @retval None
  */
static void handle_frame(uint8_t *frame, uint32_t length)
{
    command_t cmd = cmd_response;
    uint8_t *payload;
    upgrade_status_t status;
    int32_t payload_len = uframe_extract_payload(frame, length);
    payload = frame; // Why? Well, frame now points to the payload
    if (payload_len > 0) {
        cmd = frame[0];
        switch(cmd) {
            case cmd_upgrade_start:
            {
                cur_flash_address = (uint32_t) &_app_start;
                {
                    DECLARE_UNPACK(payload, length);
                    UNPACK8(cmd);
                    UNPACK16(chunk_size);
                    chunk_size = MIN(MAX_CHUNK_SIZE, chunk_size);
                    UNPACK16(fw_crc16);
                }
                {
                    DECLARE_FRAME(MAX_FRAME_LENGTH);
                    PACK8(cmd_response | cmd_upgrade_start);
                    PACK8(upgrade_continue);
                    PACK16(chunk_size);
                    FINISH_FRAME();
                    flash_unlock();
                    uint32_t setting = 1;
                    (void) past_write_unit(&past, PAST_UNIT_UPGRADE_STARTED, (void*) &setting, sizeof(setting));
                    send_frame(_buffer, _length);
                }
                break;
            }
            case cmd_upgrade_data:
                if (payload_len < 0) {
                    status = upgrade_crc_error;
                } else if (cur_flash_address >= (uint32_t) &_app_end) {
                    status = upgrade_overflow_error;
                } else {
                    if (payload_len > 0) {
                        flash_erase_page(cur_flash_address);
                        if (!(FLASH_SR_EOP & flash_get_status_flags())) {
                            status = upgrade_erase_error;
                        } else {
                            uint32_t word;
                            status = upgrade_continue; // Think positive
                            /** Note, payload contains 1 frame type byte and N bytes data */
                            for (uint32_t i = 0; i < (uint32_t) payload_len-1; i+=4) {
                                word = payload[i+4] << 24 | payload[i+3] << 16 | payload[i+2] << 8 | payload[i+1];
                                /** @todo: Handle binaries not size aliged to 4 bytes */
                                if (!flash_write32(cur_flash_address+i, word)) {
                                    status = upgrade_flash_error;
                                    break;
                                }
                            }
                            cur_flash_address += payload_len-1; // -1 is the frame type of the payload
                        }
                    }
                    if (payload_len < chunk_size) {
                        uint16_t calc_crc = crc16((uint8_t*) &_app_start, cur_flash_address - (uint32_t) &_app_start);
                        status = fw_crc16 == calc_crc ? upgrade_success : upgrade_crc_error;
                        flash_lock();
                    }
                }
                {
                    DECLARE_FRAME(MAX_FRAME_LENGTH);
                    PACK8(cmd_response | cmd_upgrade_data);
                    PACK8(status);
                    FINISH_FRAME();
                    send_frame(_buffer, _length);
                    if (status == upgrade_success) {
                        usart_wait_send_ready(USART1); /** make sure FIFO is empty */
                        (void) past_erase_unit(&past, PAST_UNIT_UPGRADE_STARTED);
                        if (!start_app()) {
                            handle_upgrade(); /** Try again... */
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
}

/**
  * @brief Ye olde main
  * @retval preferably none
  */
int main(void)
{
    uint32_t magic = 0, temp = 0;
    uint32_t past_start = (uint32_t) &_past_start;

    ringbuf_init(&rx_buf, (uint8_t*) buffer, sizeof(buffer));
    hw_init(&rx_buf);

    past.blocks[0] = past_start;
    past.blocks[1] = past_start + 1024;
    if (!past_init(&past)) {
        handle_upgrade(); /** Not much we can do */
    }

    /** @todo implement host sync, enabling us to enter upgrade mode from cold/warm boot - arduino style */

    if (bootcom_get(&magic, &temp) && magic == 0xfedebeda) {
        chunk_size = temp >> 16;
        fw_crc16 = temp & 0xffff;
        handle_upgrade();
    }

    void *data;
    uint32_t length;
    if (past_read_unit(&past, PAST_UNIT_UPGRADE_STARTED, (const void**) &data, &length)) {
        handle_upgrade(); /** We have a non finished upgrade */
    }

    if (!start_app()) {
        handle_upgrade(); /** In case we somehow returned from the app */
    }

    return 0;
}
