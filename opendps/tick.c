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
#include <systick.h>
#include <nvic.h>
#include "tick.h"

static volatile uint32_t ticks_lower;
static volatile uint32_t ticks_upper;

/**
  * @brief Initialize the systick module
  * @retval none
  */
void systick_init(void)
{
    // 48MHz / 8 => 6000000 counts per second
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

    // 6000000/6000 = 1000 overflows per second - every 1ms one interrupt
    // SysTick interrupt every N clock pulses: set reload to N-1
    systick_set_reload(5999);

    systick_interrupt_enable();
    systick_counter_enable();
}

/**
  * @brief Busy wait for a given time
  * @param delay time in milliseconds
  * @retval none
  */
void delay_ms(uint32_t delay)
{
    uint32_t start = ticks_lower;

    while ((ticks_lower - start) < delay)
    {}
}

/**
  * @brief Get systick
  * @retval number of milliseconds since powerup
  */
uint64_t get_ticks(void)
{
    systick_interrupt_disable();
    uint64_t ticks = ((uint64_t) ticks_upper << 32) | ((uint64_t) ticks_lower);
    systick_interrupt_enable();
    return ticks;
}

/**
  * @brief STM32 systick handler
  * @retval none
  */
void sys_tick_handler(void)
{
    ticks_lower++;
    if (ticks_lower == 0) // If an overflow has occured
        ticks_upper++;
}

