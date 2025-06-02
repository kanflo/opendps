/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Johan Kanflo (github.com/kanflo)
 * Modifications made by Leo Leung <leo@steamr.com>
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

#ifndef __FUNC_DPSMODE_H__
#define __FUNC_DPSMODE_H__

#include "uui.h"


/**
 * @brief      Add the CV function to the UI
 *
 * @param      ui    The user interface
 */
void func_dpsmode_init(uui_t *ui);



/**
  * @brief Saves past data
  * @retval none
  */
void dpsmode_past_save(past_t *past);


#endif // __FUNC_DPSMODE_H__
