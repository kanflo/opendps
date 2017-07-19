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
#include "hw.h"
#include "uframe.h"
#include "protocol.h"
#include "serialhandler.h"

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
    if (payload_len > 0) {
        cmd = frame[0];
        switch(cmd) {
            case cmd_ping:
                success = 1; // Response will be sent below
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
