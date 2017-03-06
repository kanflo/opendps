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

#ifndef __PWRCTL_H__
#define __PWRCTL_H__

extern uint32_t pwrctl_i_limit_raw;

/**
  * @brief Initialize the power control module
  * @retval none
  */
void pwrctl_init(void);

/**
  * @brief Set voltage output
  * @param value_mv voltage in millivolt
  * @retval true requested voltage was within specs
  */
bool pwrctl_set_vout(uint32_t value_mv);

/**
  * @brief Get voltage output setting
  * @retval current setting in millivolt
  */
uint32_t pwrctl_get_vout(void);

/**
  * @brief Set current limit
  * @param value_ma limit in milliampere
  * @retval true requested current was within specs
  */
bool pwrctl_set_ilimit(uint32_t value_ma);

/**
  * @brief Get current limit setting
  * @retval current setting in milliampere
  */
uint32_t pwrctl_get_ilimit(void);

/**
  * @brief Enable or disable power output
  * @param enable true for enable, false for disable
  * @retval none
  */
void pwrctl_enable_vout(bool enable);

/**
  * @brief Return power output status
  * @retval true if power output is wnabled
  */
bool pwrctl_vout_enabled(void);

/**
  * @brief Calculate V_in based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding voltage in millivolt
  */
uint32_t pwrctl_calc_vin(uint16_t raw);

/**
  * @brief Calculate V_out based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding voltage in millivolt
  */
uint32_t pwrctl_calc_vout(uint16_t raw);

/**
  * @brief Calculate DAC setting for requested V_out
  * @param v_out_mv requested output voltage
  * @retval corresponding voltage in millivolt
  */
uint16_t pwrctl_calc_vout_dac(uint32_t v_out_mv);

/**
  * @brief Calculate I_out based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding current in milliampere
  */
uint32_t pwrctl_calc_iout(uint16_t raw);

/**
  * @brief Calculate expected raw ADC value based on selected I_limit
  * @param i_limit_ma selected I_limit
  * @retval expected raw ADC value
  */
uint16_t pwrctl_calc_ilimit_adc(uint16_t i_limit_ma);

#endif // __PWRCTL_H__
