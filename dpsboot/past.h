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

#ifndef __PAST_H__
#define __PAST_H__

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t past_id_t;

/** A structure describing a past instance. The user is expected to fill out the
  * blocks array before calling past_init(...). The other fields must not be
  * touched.
  */
typedef struct {
    uint32_t blocks[2];
    uint32_t _cur_block;
    uint32_t _counter;
    uint32_t _end_addr;
    bool _valid;
} past_t;

/**
  * @brief Initialize the past, format or garbage collect if needed
  * @param past A past structure with the block[] vector initialized
  * @retval true if the past could be initialized
  *         false if init falied
  */
bool past_init(past_t *past);

/**
  * @brief Read unit from past
  * @param past An initialized past structure
  * @param id Unit id to read
  * @param data Pointer to data read
  * @param length Length of data read
  * @retval true if the unit was found and could be read
  *         false if the unit was not found
  */
bool past_read_unit(past_t *past, past_id_t id, const void **data, uint32_t *length);

/**
  * @brief Write unit to past
  * @param past An initialized past structure
  * @param id Unit id to write
  * @param data Data to write
  * @param length Size of data
  * @retval true if the unit was written
  *         false if writing failed or the past was full
  */
bool past_write_unit(past_t *past, past_id_t id, void *data, uint32_t length);

/**
  * @brief Erase unit
  * @param past An initialized past structure
  * @param id Unit id to erase
  * @retval true if the unit was found and erasing was successful
  *         false if erasing failed or the unit was not found
  */
bool past_erase_unit(past_t *past, past_id_t id);

/**
  * @brief Format the past area (both blocks) and initialize the first one
  * @param past pointer to an initialized past structure
  * @retval True if formatting was successful
  *         False in case of unrecoverable errors
  */
bool past_format(past_t *past);

#endif // __PAST_H__
