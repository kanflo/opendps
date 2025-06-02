/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Leo Leung <leo@steamr.com>
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

#ifndef __FUNC_SETTINGS_H__
#define __FUNC_SETTINGS_H__

#include "uui.h"

extern bool dpsmode_cfg_initstate;

// Settings
enum SettingsStates {
    SCREEN_LOCKED_WHEN_ON = 1,
};

enum PopupMessage {
    POPUP_MESSAGE_NONE = 0,
    POPUP_MESSAGE_SAVED = 1,
    POPUP_MESSAGE_RESET = 2,
};


/**
 * @brief      Add the SETTINGS function to the UI
 *
 * @param      ui    The user interface
 */
void func_settings_init(uui_t *ui);

#endif // __FUNC_SETTINGS_H__
