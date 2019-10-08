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

#ifndef __EVENT_H__
#define __EVENT_H__

typedef enum {
    event_none = 0,
    event_button_m1,
    event_button_m2,
    event_button_m1_and_m2,
    event_button_sel,

    event_button_sel_m1,
    event_button_sel_m2,

    event_button_enable,
    event_rot_left,
    event_rot_right,
    event_rot_left_set,
    event_rot_right_set,
    event_rot_left_m1,
    event_rot_right_m1,
    event_rot_left_m2,
    event_rot_right_m2,
    event_rot_left_down,
    event_rot_right_down,
    event_rot_press,

    event_uart_rx,
    event_ocp,
    event_ovp,
    event_opp,

    event_shutoff,
    event_timer
} event_t;

typedef enum {
    press_short = 0,
    press_long,
} button_press_t;


/**
  * @brief Initialize the event module
  * @retval None
  */
void event_init(void);

/**
  * @brief Fetch next event in queue
  * @param event the type of event received or 'event_none' if no events in queue
  * @param data additional event data
  * @retval true if an event was found
  */
bool event_get(event_t *event, uint8_t *data);

/**
  * @brief Place event in event fifo
  * @param event event type
  * @param data additional event data
  * @retval None
  */
bool event_put(event_t event, uint8_t data);

#endif // __EVENT_H__
