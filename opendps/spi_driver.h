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

#ifndef __SPI_DRIVER_H__
#define __SPI_DRIVER_H__

/**
  * @brief Initialize the SPI driver
  * @retval None
  */
void spi_init(void);

/**
  * @brief TX, and optionally RX data on the SPI bus
  * @param tx_buf transmit buffer
  * @param tx_len transmit buffer size
  * @param rx_buf receive buffer (may be NULL)
  * @param rx_len receive buffer size (may be 0)
  * @retval true if operation succeeded
  *         false if parameter or driver error
  */
bool spi_dma_transceive(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len);

#endif // __SPI_DRIVER_H__
