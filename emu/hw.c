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

/**
  * @brief Initialize the hardware
  * @retval None
  */
void hw_init(void)
{
}

/**
  * @brief Read latest ADC mesurements
  * @param i_out_raw latest I_out raw value
  * @param v_in_raw latest V_in raw value
  * @param v_out_raw latest V_out raw value
  * @retval none
  */
void hw_get_adc_values(uint16_t *i_out_raw, uint16_t *v_in_raw, uint16_t *v_out_raw)
{
    *i_out_raw = 0;
    *v_in_raw = 0;
    *v_out_raw = 0;
}

/**
  * @brief Initialize TIM4 that drives the backlight of the TFT
  * @retval None
  */
void hw_enable_backlight(void)
{
}

/**
  * @brief Get the ADC valut that triggered the OCP
  * @retval Trivver value in mA
  */
uint16_t hw_get_itrig_ma(void)
{
    return 0;
}

/**
  * @brief Check if it current press is a long press, inject event if so
  * @retval None
  */
void hw_longpress_check(void)
{
}

/**
  * @brief Check if SEL button is pressed
  * @retval true if SEL button is pressed, false otherwise
  */
bool hw_sel_button_pressed(void)
{
    return false;
}

/**
  * @brief Set TFT backlight value
  * @retval None
  */
void hw_set_backlight(uint8_t brightness)
{
    (void) brightness;
}

/**
  * @brief Get TFT backlight value
  * @retval Brightness percentage
  */
uint8_t hw_get_backlight(void)
{
    return 0;
}

/**
  * @brief Set the output voltage DAC value
  * @param v_dac the value to set to
  * @retval none
  */
void hw_set_voltage_dac(uint16_t v_dac)
{
    (void) v_dac;
}

/**
  * @brief Set the output current DAC value
  * @param i_dac the value to set to
  * @retval none
  */
void hw_set_current_dac(uint16_t i_dac)
{
    (void) i_dac;
}