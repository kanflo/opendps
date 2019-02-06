/*
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ILI9163C driver based on TFT_ILI9163C: https://github.com/sumotoy/TFT_ILI9163C
 * Copyright (c) 2014, .S.U.M.O.T.O.Y., coded by Max MC Costa.
 *
 */

#ifndef _ILI9163C_H_
#define _ILI9163C_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>

#define BLACK       0x0000      /*   0,   0,   0 */
#define NAVY        0x000F      /*   0,   0, 128 */
#define DARKGREEN   0x03E0      /*   0, 128,   0 */
#define DARKCYAN    0x03EF      /*   0, 128, 128 */
#define MAROON      0x7800      /* 128,   0,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define OLIVE       0x7BE0      /* 128, 128,   0 */
#define LIGHTGREY   0xC618      /* 192, 192, 192 */
#define DARKGREY    0x7BEF      /* 128, 128, 128 */
#define BLUE        0x001F      /*   0,   0, 255 */
#define GREEN       0x07E0      /*   0, 255,   0 */
#define CYAN        0x07FF      /*   0, 255, 255 */
#define RED         0xF800      /* 255,   0,   0 */
#define MAGENTA     0xF81F      /* 255,   0, 255 */
#define YELLOW      0xFFE0      /* 255, 255,   0 */
#define WHITE       0xFFFF      /* 255, 255, 255 */
#define ORANGE      0xFD20      /* 255, 165,   0 */
#define GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define PINK        0xF81F


void ili9163c_init(void);
void ili9163c_get_geometry(uint16_t *width, uint16_t *height);
void ili9163c_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ili9163c_push_color(uint16_t color);
void ili9163c_fill_screen(uint16_t color);
void ili9163c_draw_pixel(int16_t x, int16_t y, uint16_t color);
void ili9163c_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,uint16_t color);
void ili9163c_set_rotation(uint8_t r);
void ili9163c_invert_display(bool i);
void ili9163c_display(bool on);
bool ili9163c_boundary_check(int16_t x,int16_t y);
void ili9163c_draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color);
void ili9163c_draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color);

#endif // _ILI9163C_H_
