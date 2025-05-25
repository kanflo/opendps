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
#include <string.h>
#include <timer.h>
#include <rcc.h>
#include <gpio.h>
#include <nvic.h>
#include <usart.h>
#include <stdio.h>
#include "tick.h"
#include "hw.h"
#include "ringbuf.h"

static void clock_init(void);
static void usart_init(void);
static void gpio_init(void);

static ringbuf_t *rx_buf;

/**
  * @brief Initialize the hardware
  * @retval None
  */
void hw_init(ringbuf_t *usart_rx_buf)
{
    rx_buf = usart_rx_buf;
    clock_init();
    systick_init();
    gpio_init();
    usart_init();
}

/**
  * @brief Check if we are to enter forced upgrade
  * @retval true if so
  */
bool hw_check_forced_upgrade(void)
{
    /** What the heck? Without this delay we will always read "button pressed"
      * for the first milli seconds or so. Constantly printing GPIO_IDR(PORTA)
      * reveals it changes after some time on cold boots. */
    static bool first_call = false;
    if (!first_call) {
        delay_ms(100);
        first_call = true;
    }
    return gpio_get(BUTTON_SEL_PORT, BUTTON_SEL_PIN) != BUTTON_SEL_PIN;
}

void usart1_isr(void)
{
    if ((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0 &&
        (USART_SR(USART1) & USART_SR_RXNE) != 0) {
        uint16_t ch = usart_recv(USART1);
        if ((USART_SR(USART1) & USART_SR_ORE) == 0 &&
            (USART_SR(USART1) & USART_SR_FE) == 0 &&
            (USART_SR(USART1) & USART_SR_PE) == 0) {
            if (!ringbuf_put(rx_buf, ch)) {
                //printf("ASSERT:usart1_isr:%d\n", __LINE__);
            }
        } else {
            //printf("ASSERT:usart1_isr:%d\n", __LINE__);
        }
    }
}

/**
  * @brief Enable clocks
  * @retval None
  */
static void clock_init(void)
{
    rcc_clock_setup_in_hsi_out_48mhz();
    // rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_48MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_AFIO);
}

/**
  * @brief Initialize USART1
  * @retval None
  */
static void usart_init(void)
{
    rcc_periph_clock_enable(RCC_USART1);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

    nvic_enable_irq(NVIC_USART1_IRQ);
    usart_set_baudrate(USART1, CONFIG_BAUDRATE); /** Baudrate set in makefile */
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    // Enable USART1 Receive interrupt.
    USART_CR1(USART1) |= USART_CR1_RXNEIE;

    usart_enable(USART1);
}

/**
  * @brief Initialize GPIO
  * @retval None
  */
static void gpio_init(void)
{
    gpio_set_mode(BUTTON_SEL_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, BUTTON_SEL_PIN);
    gpio_set(BUTTON_SEL_PORT, BUTTON_SEL_PIN);
}
