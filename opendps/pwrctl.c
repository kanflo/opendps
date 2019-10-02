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
#include "pastunits.h"
#include <gpio.h>
#include <dac.h>

/** This module handles voltage and current calculations
  * Calculations based on measurements found at
  * https://docs.google.com/spreadsheets/d/1AhGsU_gvZjqZyr2ZYrnkz6BeUqMquzh9UNYoTqy_Zp4/edit?usp=sharing
  */

static uint32_t i_out, v_out, i_limit, v_limit;
static bool v_out_enabled;

float a_adc_k_coef = A_ADC_K;
float a_adc_c_coef = A_ADC_C;
float a_dac_k_coef = A_DAC_K;
float a_dac_c_coef = A_DAC_C;
float v_adc_k_coef = V_ADC_K;
float v_adc_c_coef = V_ADC_C;
float v_dac_k_coef = V_DAC_K;
float v_dac_c_coef = V_DAC_C;
float vin_adc_k_coef = VIN_ADC_K;
float vin_adc_c_coef = VIN_ADC_C;

/** not static as it is referred to from hw.c for performance reasons */
uint32_t pwrctl_i_limit_raw;
uint32_t pwrctl_v_limit_raw;

/**
  * @brief Initialize the power control module
  * @retval none
  */
void pwrctl_init(past_t *past)
{
    uint32_t length;
    float *p;

    /** Load default calibration constants */
    a_adc_k_coef = A_ADC_K;
    a_adc_c_coef = A_ADC_C;
    a_dac_k_coef = A_DAC_K;
    a_dac_c_coef = A_DAC_C;
    v_adc_k_coef = V_ADC_K;
    v_adc_c_coef = V_ADC_C;
    v_dac_k_coef = V_DAC_K;
    v_dac_c_coef = V_DAC_C;
    vin_adc_k_coef = VIN_ADC_K;
    vin_adc_c_coef = VIN_ADC_C;

    /** Load any calibration constants that maybe stored in non-volatile memory (past) */
    if (past_read_unit(past, past_A_ADC_K, (const void**) &p, &length))
        a_adc_k_coef = *p;
    if (past_read_unit(past, past_A_ADC_C, (const void**) &p, &length))
        a_adc_c_coef = *p;
    if (past_read_unit(past, past_A_DAC_K, (const void**) &p, &length))
        a_dac_k_coef = *p;
    if (past_read_unit(past, past_A_DAC_C, (const void**) &p, &length))
        a_dac_c_coef = *p;
    if (past_read_unit(past, past_V_ADC_K, (const void**) &p, &length))
        v_adc_k_coef = *p;
    if (past_read_unit(past, past_V_ADC_C, (const void**) &p, &length))
        v_adc_c_coef = *p;
    if (past_read_unit(past, past_V_DAC_K, (const void**) &p, &length))
        v_dac_k_coef = *p;
    if (past_read_unit(past, past_V_DAC_C, (const void**) &p, &length))
        v_dac_c_coef = *p;
    if (past_read_unit(past, past_VIN_ADC_K, (const void**) &p, &length))
        vin_adc_k_coef = *p;
    if (past_read_unit(past, past_VIN_ADC_C, (const void**) &p, &length))
        vin_adc_c_coef = *p;

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
    if (v_out_enabled) {
        /** Needed for the DPS5005 "communications version" (the one with BT/USB) */
        DAC_DHR12R1 = pwrctl_calc_vout_dac(v_out);
    } else {
        DAC_DHR12R1 = 0;
    }
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
    if (v_out_enabled) {
        DAC_DHR12R2 = pwrctl_calc_iout_dac(value_ma);
    } else {
        DAC_DHR12R2 = 0;
    }
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
  * @brief Set voltage limit
  * @param value_mv limit in millivolts
  * @retval true requested voltage was within specs
  */
bool pwrctl_set_vlimit(uint32_t value_mv)
{
    /** @todo Check with V_limit, currently filtered by ui.c */
    v_limit = value_mv;
    pwrctl_v_limit_raw = pwrctl_calc_vlimit_adc(v_limit);
    return true;
}

/**
  * @brief Get current limit setting
  * @retval current setting in milliampere
  */
uint32_t pwrctl_get_vlimit(void)
{
    return v_limit;
}

/**
  * @brief Enable or disable power output
  * @param enable true for enable, false for disable
  * @retval none
  */
void pwrctl_enable_vout(bool enable)
{
    v_out_enabled = enable;
    if (v_out_enabled) {
      (void) pwrctl_set_vout(v_out);
      (void) pwrctl_set_iout(i_out);
#ifdef DPS5015
        //gpio_clear(GPIOA, GPIO9); // this is power control on '5015
        gpio_set(GPIOB, GPIO11);    // B11 is fan control on '5015
        gpio_clear(GPIOC, GPIO13);  // C13 is power control on '5015
#else
        gpio_clear(GPIOB, GPIO11);  // B11 is power control on '5005
#endif
    } else {
#ifdef DPS5015
        //gpio_set(GPIOA, GPIO9);    // gpio_set(GPIOB, GPIO11);
        gpio_clear(GPIOB, GPIO11); // B11 is fan control on '5015
        gpio_set(GPIOC, GPIO13);   // C13 is power control on '5015
#else
        gpio_set(GPIOB, GPIO11);  // B11 is power control on '5005
#endif
      (void) pwrctl_set_vout(v_out);
      (void) pwrctl_set_iout(i_out);
    }
}

/**
  * @brief Return power output status
  * @retval true if power output is enabled
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
    float value = vin_adc_k_coef * raw + vin_adc_c_coef;
    if (value <= 0)
        return 0;
    else
        return value + 0.5f; /** Add 0.5f to value so it is correctly rounded when it is truncated */
}

/**
  * @brief Calculate V_out based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding voltage in milli volt
  */
uint32_t pwrctl_calc_vout(uint16_t raw)
{
    float value = v_adc_k_coef * raw + v_adc_c_coef;
    if (value <= 0)
        return 0;
    else
        return value + 0.5f; /** Add 0.5f to value so it is correctly rounded when it is truncated */
}

/**
  * @brief Calculate DAC setting for requested V_out
  * @param v_out_mv requested output voltage
  * @retval corresponding 12 bit DAC value
  */
uint16_t pwrctl_calc_vout_dac(uint32_t v_out_mv)
{
    float value = v_dac_k_coef * v_out_mv + v_dac_c_coef;
    if (value <= 0)
        return 0;
    else if (value >= 0xfff)
        return 0xfff; /** 12 bits */
    else
        return value + 0.5f; /** Add 0.5f to value so correct rounding is done when truncated */
}

/**
  * @brief Calculate I_out based on raw ADC measurement
  * @param raw value from ADC
  * @retval corresponding current in milliampere
  */
uint32_t pwrctl_calc_iout(uint16_t raw)
{
    float value = a_adc_k_coef * raw + a_adc_c_coef;
    if (value <= 0)
        return 0;
    else
        return value + 0.5f; /** Add 0.5f to value so correct rounding is done when truncated */
}

/**
  * @brief Calculate expected raw ADC value based on selected I_limit
  * @param i_limit_ma selected I_limit
  * @retval expected raw ADC value
  */
uint16_t pwrctl_calc_ilimit_adc(uint16_t i_limit_ma)
{
    float value = (i_limit_ma - a_adc_c_coef) / a_adc_k_coef + 1;
    if (value <= 0)
        return 0;
    else
        return value + 0.5f; // Add 0.5 so it is correctly rounded when it is truncated
}

/**
  * @brief Calculate expected raw ADC value based on selected V_limit
  * @param v_limit_mv selected V_limit
  * @retval expected raw ADC value
  */
uint16_t pwrctl_calc_vlimit_adc(uint16_t v_limit_mv)
{
    float value = (v_limit_mv - v_adc_c_coef) / v_adc_k_coef + 1;
    if (value <= 0)
        return 0;
    else
        return value + 0.5f; // Add 0.5 so it is correctly rounded when it is truncated
}

/**
  * @brief Calculate DAC setting for constant current mode
  * @param i_out_ma requested constant current
  * @retval corresponding 12 bit DAC value
  * @note this formula is valid for the DPS5005 and would probably need changes
  *       for DPS:es capable of higher current output.
  */
uint16_t pwrctl_calc_iout_dac(uint32_t i_out_ma)
{
    float value = a_dac_k_coef * i_out_ma + a_dac_c_coef;
    if (value <= 0)
        return 0;
    else if (value >= 0xfff)
        return 0xfff; /** 12 bits */
    else
        return value + 0.5f; /** Add 0.5f to value so correct rounding is done when truncated */
}
