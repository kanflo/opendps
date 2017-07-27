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

#include <rcc.h>
#include <gpio.h>
#include <dma.h>
#include <nvic.h>
#include <spi.h>
#include <errno.h>
#include "spi_driver.h"

/** Used to keep track of the SPI DMA status */
typedef enum {
    spi_idle = 0,
    spi_tx_running = 1,
    spi_rx_running = 2
} spi_status_t;

static volatile spi_status_t dma_status;

/** The DPS5005 has NSS grounded meaning we do not have to toggle it */
#define SPI_NSS_GROUNDED

/**
  * @brief Initialize the SPI driver
  * @retval None
  */
void spi_init(void)
{
    dma_status = spi_idle;

    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_DMA1);
    spi_reset(SPI2);
    SPI2_I2SCFGR = 0;
    spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_2, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    spi_enable(SPI2);
    nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, 0);
    nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);
    nvic_set_priority(NVIC_DMA1_CHANNEL5_IRQ, 0);
    nvic_enable_irq(NVIC_DMA1_CHANNEL5_IRQ);
}

/**
  * @brief TX, and optionally RX data on the SPI bus
  * @param tx_buf transmit buffer
  * @param tx_len transmit buffer size
  * @param rx_buf receive buffer (may be NULL)
  * @param rx_len receive buffer size (may be 0)
  * @retval true if operation succeeded
  *         false if parameter or driver error
  */
bool spi_dma_transceive(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len)
{
    if (!rx_len && !tx_len) {
        return false;
    }

    dma_channel_reset(DMA1, DMA_CHANNEL4);
    dma_channel_reset(DMA1, DMA_CHANNEL5);

    volatile uint8_t temp __attribute__ ((unused));
    while (SPI_SR(SPI2) & (SPI_SR_RXNE | SPI_SR_OVR)) {
        temp = SPI_DR(SPI2);
    }

    dma_status = spi_idle;

    if (rx_len) {
        dma_status |= spi_rx_running;
        dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&SPI2_DR);
        dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)rx_buf);
        dma_set_number_of_data(DMA1, DMA_CHANNEL4, rx_len);
        dma_set_read_from_peripheral(DMA1, DMA_CHANNEL4);
        dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
        dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_8BIT);
        dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
        dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_VERY_HIGH);
        dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);
        dma_enable_channel(DMA1, DMA_CHANNEL4);
    }

    if (tx_len) {
        dma_status |= spi_tx_running;
        dma_set_peripheral_address(DMA1, DMA_CHANNEL5, (uint32_t)&SPI2_DR);
        dma_set_memory_address(DMA1, DMA_CHANNEL5, (uint32_t)tx_buf);
        dma_set_number_of_data(DMA1, DMA_CHANNEL5, tx_len);
        dma_set_read_from_memory(DMA1, DMA_CHANNEL5);
        dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL5);
        dma_set_peripheral_size(DMA1, DMA_CHANNEL5, DMA_CCR_PSIZE_8BIT);
        dma_set_memory_size(DMA1, DMA_CHANNEL5, DMA_CCR_MSIZE_8BIT);
        dma_set_priority(DMA1, DMA_CHANNEL5, DMA_CCR_PL_HIGH);
        dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL5);
        dma_enable_channel(DMA1, DMA_CHANNEL5);
    }

#ifndef SPI_NSS_GROUNDED
    gpio_clear(GPIOB, GPIO12);
#endif // SPI_NSS_GROUNDED

    // Enable DMA, effectivly starting the transfer
    if (rx_len) {
        spi_enable_rx_dma(SPI2);
    }
    if (tx_len) {
        spi_enable_tx_dma(SPI2);
    }

    // Wait until DMA completed in accordance with RM0008 (r16) p.713
    /** @todo Add timeout for SPI transmission */
    while (dma_status != spi_idle) ;
    while (!(SPI_SR(SPI2) & SPI_SR_TXE)) ;
    while (SPI_SR(SPI2) & SPI_SR_BSY) ;

#ifndef SPI_NSS_GROUNDED
    gpio_set(GPIOB, GPIO12);
#endif // SPI_NSS_GROUNDED

    return true;
}

/**
  * @brief SPI RX DMA handler
  * @retval None
  */
void dma1_channel4_isr(void)
{
    if ((DMA1_ISR &DMA_ISR_TCIF2) != 0) {
        DMA1_IFCR |= DMA_IFCR_CTCIF2;
    }
    dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);
    spi_disable_rx_dma(SPI2);
    dma_disable_channel(DMA1, DMA_CHANNEL4);
    dma_status &= ~spi_rx_running;
}

/**
  * @brief SPI TX DMA handler
  * @retval None
  */
void dma1_channel5_isr(void)
{
    if ((DMA1_ISR &DMA_ISR_TCIF3) != 0) {
        DMA1_IFCR |= DMA_IFCR_CTCIF3;
    }
    dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL5);
    spi_disable_tx_dma(SPI2);
    dma_disable_channel(DMA1, DMA_CHANNEL5);
    dma_status &= ~spi_tx_running;
}
