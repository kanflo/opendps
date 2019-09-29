/* 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Johan Kanflo (github.com/kanflo)
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

#ifndef __OPENDPS_H__
#define __OPENDPS_H__

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

/** Max number of parameters to a function */
#define OPENDPS_MAX_PARAMETERS  (8)

/** UI Width/Height */
#define TFT_WIDTH   (128)
#define TFT_HEIGHT  (128)

/**
 * @brief      Enable specified function
 *
 * @param      func_idx  index of function
 *
 * @return     True if successful
 */
bool opendps_enable_function_idx(uint32_t func_idx);

/**
 * @brief      List function names of device
 *
 * @param      names  Output vector holding pointers to the names
 * @param[in]  size   Number of items in the output array
 *
 * @return     Number of items returned
 */
uint32_t opendps_get_function_names(char* names[], size_t size);

/**
 * @brief      Get current function name
 *
 * @return     Current function name :)
 */
const char* opendps_get_curr_function_name(void);

/**
 * @brief      Get dpsboot GIT hash string address
 *
 * @return     Return string length on success, else 0
 */
uint32_t opendps_get_boot_git_hash(const char** git_hash);

/**
 * @brief      Get opendps GIT hash string address
 *
 * @return     Return string length on success, else 0
 */
uint32_t opendps_get_app_git_hash(const char** git_hash);

/**
 * @brief      List parameter names of current function
 *
 * @param      parameters  Output vector holding pointers to the parameters
 *
 * @return     Number of items returned
 */
uint32_t opendps_get_curr_function_params(ui_parameter_t **parameters);

/**
 * @brief      Return value of named parameter for current function 
 *
 * @param      name       Parameter name
 * @param[in]  value      String representation of parameter value
 * @param      value_len  Length of value buffer
 *
 * @return     true if param exists
 */
bool opendps_get_curr_function_param_value(char *name, char *value, uint32_t value_len);

/**
 * @brief      Set parameter to value
 *
 * @param      name   Name of parameter
 * @param      value  Value as a string
 *
 * @return     Status of the operation
 */
set_param_status_t opendps_set_parameter(char *name, char *value);

/**
 * @brief      Sets Calibration Data
 *
 * @param      name Name of calibration variable to set
 * @value      value Value to set it to
 *
 * @return     Status of the operation
 */
set_param_status_t opendps_set_calibration(char *name, float *value);

/**
 * @brief      Clear Calibration Data
 *
 * @return     True on success
 */
bool opendps_clear_calibration(void);

/**
 * @brief      Enable output of current function
 *
 * @param[in]  enable  Enable or disable
 *
 * @return     True if enable was successful
 */
bool opendps_enable_output(bool enable);

/**
  * @brief Update power enable status icon
  * @param enabled new power status
  * @retval none
  */
void opendps_update_power_status(bool enabled);

/**
  * @brief Update wifi status icon
  * @param status new wifi status
  * @retval none
  */
void opendps_update_wifi_status(wifi_status_t status);

/**
  * @brief Handle ping
  * @retval none
  */
void opendps_handle_ping(void);

/**
  * @brief Lock or unlock the UI
  * @param lock true for lock, false for unlock
  * @retval none
  */
void opendps_lock(bool lock);

/**
  * @brief Lock or unlock the UI due to a temperature alarm
  * @param lock true for lock, false for unlock
  * @retval none
  */
void opendps_temperature_lock(bool lock);

/**
  * @brief Set temperatures
  * @param temp1 first temperature we can deal with
  * @param temp2 second temperature we can deal with
  * @retval none
  */
void opendps_set_temperature(int16_t temp1, int16_t temp2);


/**
  * @brief Get temperatures
  * @param temp1 first temperature we can deal with
  * @param temp2 second temperature we can deal with
  * @param temp_shutdown true if shutdown due to high temperature
  * @retval none
  */
void opendps_get_temperature(int16_t *temp1, int16_t *temp2, bool *temp_shutdown);

/**
 * @brief      Upgrade was requested by the protocol handler
  * @param chunk_size size of data chunks as requested by the remote
 */
void opendps_upgrade_start(void);

/**
 * @brief      Change the current screen
 *
 * @param[in]  screen_id  The screen ID to change to
 *
 * @return     True if successful
 */
bool opendps_change_screen(uint8_t screen_id);

#endif // __OPENDPS_H__
