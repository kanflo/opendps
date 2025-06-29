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
#include <stdlib.h>
#include <string.h>
#include <libopencm3/cm3/cortex.h>
#include "ringbuf.h"
#include "event.h"

#define MAX_EVENTS	(64)

static ringbuf_t events;
static uint8_t buffer[2*MAX_EVENTS];


/**
  * @brief Initialize the event module
  * @retval None
  */
void event_init(void)
{
	ringbuf_init(&events, (uint8_t*) buffer, sizeof(buffer));
	memset(buffer, 0, sizeof(buffer));
}

/**
  * @brief Fetch next event in queue
  * @param event the type of event received or 'event_none' if no events in queue
  * @param data additional event data
  * @retval true if an event was found
  */
bool event_get(event_t *event, uint8_t *data)
{
	bool got_event = true;
	uint16_t e;
	cm_disable_interrupts();
	if (!ringbuf_get(&events, &e)) {
		*event = event_none;
		*data = 0;
		got_event = false;
	} else {
		*event = e >> 8;
		*data = e & 0xff;

	}
	cm_enable_interrupts();
	return got_event;
}

/**
  * @brief Place event in event fifo
  * @param event event type
  * @param data additional event data
  * @retval None
  */
bool event_put(event_t event, uint8_t data)
{
	cm_disable_interrupts();
	bool result = ringbuf_put(&events, (uint16_t) (event << 8 | data));
	cm_enable_interrupts();
	return result;
}
