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

#ifndef __PASTUNITS_H__
#define __PASTUNITS_H__

/** Parameters stored in flash */
typedef enum {
    /** stored as [I_limit:16] | [V_out:16] */
    past_power = 1,
    /** stored as 0 or 1 */
    past_tft_inversion,
    /** stored as strings */
    past_boot_git_hash, /** WARN: Moving past_boot_git_hash requires a recompile and flash of DPSBoot! */
    past_app_git_hash,
    /** stored as floats */
    past_A_ADC_K,
    past_A_ADC_C,
    past_A_DAC_K,
    past_A_DAC_C,
    past_V_DAC_K,
    past_V_DAC_C,
    past_V_ADC_K,
    past_V_ADC_C,
    past_VIN_ADC_K,
    past_VIN_ADC_C,
    past_tft_brightness,
    /** A past unit who's precense indicates we have a non finished upgrade and
    must not boot */
    past_upgrade_started = 0xff
} parameter_id_t;


#endif // __PASTUNITS_H__
