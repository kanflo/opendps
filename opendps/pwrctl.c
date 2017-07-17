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

#include "pwrctl.h"
#include "dps-model.h"
#include <stdio.h>
#include <gpio.h>
#include <dac.h>

/** This module handles voltege and current calculations on the DPS5005
  * Calculations based on measurements found at
  * https://docs.google.com/spreadsheets/d/1AhGsU_gvZjqZyr2ZYrnkz6BeUqMquzh9UNYoTqy_Zp4/edit?usp=sharing
  */

static uint32_t i_out, v_out, i_limit;
static bool v_out_enabled;

/** not static as it is referred to from hw.c for performance reasons */
uint32_t pwrctl_i_limit_raw;

/**
  * @brief Initialize the power control module
  * @retval none
  */
void pwrctl_init(void)
{
    pwrctl_enable_vout(false);
}

/**
  * @brief Set voltage output
  * @param value_mv voltage in milli volt
  * @retval true requested voltage was within specs
  */
bool pwrctl_set_vout(uint32_t value_mv)
{
    /** @todo Check with max Vout, currently filtered by ui.c */
    v_out = value_mv;
    DAC_DHR12R1 = pwrctl_calc_vout_dac(v_out);
    return true;
}

/**
  * @brief Set current output
  * @param current_ma current in milli ampere
  * @retval true requested current was within specs
  */
bool pwrctl_set_iout(uint32_t value_ma)
{
    i_out = value_ma;
    DAC_DHR12R2 = pwrctl_calc_iout_dac(value_ma);
    return true;
}

/**
  * @brief Get current output setting
  * @retval current setting in milli amps
  */
uint32_t pwrctl_get_iout(void)
{
    return i_out;
}



/**
  * @brief Get voltage output setting
  * @retval current setting in milli volt
  */
uint32_t pwrctl_get_vout(void)
{
    return v_out;
}

/**
  * @brief Set current limit
  * @param value_ma limit in milliampere
  * @retval true requested current was within specs
  */
bool pwrctl_set_ilimit(uint32_t value_ma)
{
    /** @todo Check with I_limit, currently filtered by ui.c */
    i_limit = value_ma;
    pwrctl_i_limit_raw = pwrctl_calc_ilimit_adc(i_limit);
    return true;
}

/**
  * @brief Get current limit setting
  * @retval current setting in milliampere
  */
uint32_t pwrctl_get_ilimit(void)
{
    return i_limit;
}

/**
  * @brief Enable or disable power output
  * @param enable true for enable, false for disable
  * @retval none
  */
void pwrctl_enable_vout(bool enable)
{
  v_out_enabled = enable;
  if (v_out_enabled)
  {
#ifdef DPS5015
    //gpio_clear(GPIOA, GPIO9); // this is power control on '5015
    gpio_set(GPIOB, GPIO11);    // B11 is fan control on '5015
    gpio_clear(GPIOC, GPIO13);  // C13 is power control on '5015
#else
    gpio_clear(GPIOB, GPIO11);  // B11 is power control on '5005
#endif
  }
  else
  {
#ifdef DPS5015
    //gpio_set(GPIOA, GPIO9);    // gpio_set(GPIOB, GPIO11);
    gpio_clear(GPIOB, GPIO11); // B11 is fan control on '5015
    gpio_set(GPIOC, GPIO13);   // C13 is power control on '5015
#else
    gpio_set(GPIOB, GPIO11);  // B11 is power control on '5005
#endif
  }
}

/**
  * @brief Return power output status
  * @retval true if power output is wnabled
  */
bool pwrctl_vout_enabled(void)
{
    return v_out_enabled;
}

/**
  * @brief Calculate V_in based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding voltage in milli volt
  */
uint32_t pwrctl_calc_vin(uint16_t raw)
{
    return 16.746*(raw-1) + 64.112; // -1 becuse the value needed trimming
}

/**
  * @brief Calculate V_out based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding voltage in milli volt
  */
uint32_t pwrctl_calc_vout(uint16_t raw)
{
    return 13.164*raw - 100.751;
}

/**
  * @brief Calculate DAC setting for requested V_out
  * @param v_out_mv requested output voltage
  * @retval corresponding voltage in milli volt
  */
uint16_t pwrctl_calc_vout_dac(uint32_t v_out_mv)
{
    uint32_t dac = 0.072*v_out_mv + 1.85;
    return dac & 0xfff; /** 12 bits */
}

/**
  * @brief Calculate I_out based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding current in milliampere
  */
uint32_t pwrctl_calc_iout(uint16_t raw)
{
    return CUR_ADC_K*raw + CUR_ADC_C;
}

/**
  * @brief Calculate expected raw ADC value based on selected I_limit
  * @param i_limit_ma selected I_limit
  * @retval expected raw ADC value
  */
uint16_t pwrctl_calc_ilimit_adc(uint16_t i_limit_ma)
{
    return (i_limit_ma - CUR_ADC_C) / CUR_ADC_K + 1;
}

/**
  * @brief Calculate DAC setting for constant current mode
  * @param i_out_ma requested constant current
  * @retval corresponding DAC value
  * @note this formula is valid for the DPS5005 and would probably need changes
  *       for DPS:es capable of higher current output.
  */
uint16_t pwrctl_calc_iout_dac(uint32_t i_out_ma)
{
    uint32_t dac = CUR_DAC_K * i_out_ma + CUR_DAC_C;
    return dac & 0xfff; /** 12 bits */
}


