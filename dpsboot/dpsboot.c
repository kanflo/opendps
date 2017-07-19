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
#include "hw.h"
#include "ringbuf.h"
#include "past.h"

#define MAX_EVENTS  (8)

static ringbuf_t rx_buf;
static uint8_t buffer[2*MAX_EVENTS];

/** Out parameter storage */
static past_t past;

extern uint32_t *_app_start;
extern uint32_t *_app_end;
extern uint32_t *_past_start;
extern uint32_t *_past_end;
extern uint32_t *_bootcom_start;
extern uint32_t *_bootcom_end;


/**
  * @brief Ye olde main
  * @retval preferably none
  */
int main(void)
{
    ringbuf_init(&rx_buf, (uint8_t*) buffer, sizeof(buffer));
//    memset(buffer, 0, sizeof(buffer));
    hw_init(&rx_buf);
    uint32_t past_start = (uint32_t) &_past_start;

    past.blocks[0] = past_start;
    past.blocks[1] = past_start + 1024;

    if (!past_init(&past)) {
        printf("Past init failed!\n");
        // Weeeell...
    }
    printf("Hello World!\n");
    printf("app     0x%08lx (%lu bytes)\n", (uint32_t) &_app_start, (uint32_t) &_app_end - (uint32_t) &_app_start);
    printf("past    0x%08lx (%lu bytes)\n", (uint32_t) &_past_start, (uint32_t) &_past_end - (uint32_t) &_past_start);
    printf("bootcom 0x%08lx (%lu bytes)\n", (uint32_t) &_bootcom_start, (uint32_t) &_bootcom_end - (uint32_t) &_bootcom_start);

    uint32_t *p;
    uint32_t length;
    if (past_read_unit(&past, 1 /*past_power*/, (const void**) &p, &length)) {
        if (p) {
            printf("V_limit : %lu\n", (*p)&0xffff);
            printf("I_limit : %lu\n", ((*p)>>16)&0xffff);
        } else {
            printf("No past power 1\n");
        }
    } else {
        printf("No past power 2\n");
    }


#if 0
// App needs to:
    /* Relocate by software the vector table to the internal SRAM at 0x20000000 ***/
 
    /* Copy the vector table from the Flash (mapped at the base of the application
      load address 0x08003000) to the base address of the SRAM at 0x20000000. */
    for(uint32_t i = 0; i < 48; i++) {
        VectorTable[i] = *(__IO uint32_t*)(APPLICATION_ADDRESS + (i<<2));
    }
 
    /* Enable the SYSCFG peripheral clock*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // Not Reset, Clock
 
    /* Remap SRAM at 0x00000000 */
    SYSCFG_CFGR1 &= SYSCFG_CFGR1_MEM_MODE;
    SYSCFG_CFGR1 |= SYSCFG_CFGR1_MEM_MODE_SRAM;

#endif

    usart_send_blocking(USART1, '@');
    usart_send_blocking(USART1, '@');
    usart_send_blocking(USART1, '@');
    usart_send_blocking(USART1, '\n');
    delay_ms(750);
    while(1) {
        uint16_t b;
        if (ringbuf_get(&rx_buf, &b)) {
            //printf("%c", b);
            usart_send_blocking(USART1, b & 0xff);
        }
    }

    ((void (*)(void))_app_start)();

    return 0;
}
