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

#ifndef __HW_H__
#define __HW_H__

#include "ringbuf.h"

#define BUTTON_SEL_PORT GPIOA
#define BUTTON_SEL_PIN  GPIO2


/**
  * @brief Initialize the hardware
  * @param usart_rx_buf pointer to the USART RX ringbuffer
  * @retval None
  */
void hw_init(ringbuf_t *usart_rx_buf);

/**
  * @brief Check if we are to enter forced upgrade
  * @retval true if so
  */
bool hw_check_forced_upgrade(void);

/**
  * @brief Reconfigure USART1 to the given baud rate (session only, not saved)
  * @param baud new baud rate (9600, 19200, 38400, 57600, or 115200)
  * @retval none
  */
void hw_set_baudrate_boot(uint32_t baud);

/**
  * @brief Check if baud rate is in the supported set
  * @param baud baud rate to check
  * @retval true if valid
  */
bool opendps_is_valid_baud(uint32_t baud);

#endif // __HW_H__
