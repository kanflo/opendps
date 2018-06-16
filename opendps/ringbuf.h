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

#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef DPS_EMULATOR
 #include <pthread.h>
#endif // DPS_EMULATOR

typedef struct {
	uint16_t *buf;
	uint32_t size;
	uint32_t read;
	uint32_t write;
#ifdef DPS_EMULATOR
 pthread_mutex_t mutex;
#endif // DPS_EMULATOR
} ringbuf_t;

/**
  * @brief Initialize the ring buffer
  * @param ring pointer to ring buffer
  * @param buf buffer to store ring buffer data in
  * @param size size of buffer
  * @retval none
  */
void ringbuf_init(ringbuf_t *ring, uint8_t *buf, uint32_t size);

/**
  * @brief Put data into ring buffer
  * @param ring pointer to ring buffer
  * @param word the data to put into the buffer
  * @retval true if there was room in the buffer for the data
  */
bool ringbuf_put(ringbuf_t *ring, uint16_t word);

/**
  * @brief Get data from ring buffer
  * @param ring pointer to ring buffer
  * @param word the data pulled from the ring bufer
  * @retval false if the ring buffer was empty
  */
bool ringbuf_get(ringbuf_t *ring, uint16_t *word);

#endif // __RINGBUF_H__
