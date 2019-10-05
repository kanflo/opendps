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
#include <stdio.h>
#include <string.h>
#include "tft.h"

#define TFT_WIDTH   128
#define TFT_HEIGHT  128
uint8_t tft[TFT_WIDTH][TFT_HEIGHT];

/**
 * @brief Draw the tft on stdout
 * @retval none
 */
void emul_tft_draw(void)
{
    bool space = false;
    bool empty_line = true;
    for (uint32_t y = 0; y < TFT_HEIGHT; y++) {
        empty_line = true;
        space = false;
        for (uint32_t x = 0; x < TFT_WIDTH; x++) {
            if (!tft[x][y]) {
                space = true;
            } else {
                empty_line = false;
                if (space) {
//                    printf(" ");
                }
                printf("%c", tft[x][y]);
            }
        }
        if (!empty_line) {
            printf("\n");
        }
    }
}

/**
  * @brief Initialize the TFT module
  * @retval none
  */
void tft_init(void)
{
    tft_clear();
}

/**
  * @brief Clear the TFT
  * @retval none
  */
void tft_clear(void)
{
    memset(tft, 0, sizeof(tft));
}

/**
  * @brief Blit graphics on TFT
  * @param bits graphics in bgr565 format mathing the specified size
  * @param width width of data
  * @param height of data
  * @param x x position
  * @param y y position
  * @retval none
  */
void tft_blit(uint16_t *bits, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
    (void) bits;
    (void) width;
    (void) height;
    (void) x;
    (void) y;
}

/**
  * @brief Blit character on TFT
  * @param size size of character (0:small 1:large)
  * @param ch the character (must be a supported character)
  * @param x x position
  * @param y y position
  * @param w width of bounding box
  * @param h height of bounding box
  * @param highlight if true, the character will be inverted
  * @retval none
  */
uint8_t tft_putch(tft_font_size_t size, char ch, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color, bool invert)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        printf("Error: character '%c' put outside of screen (%d, %d)\n", ch, x, y);
    }
    tft[x][y] = ch;
    (void) size;
    (void) w;
    (void) h;
    (void) color;
    (void) invert;
}

/**
  * @brief Blit string on TFT, anchored to bottom-left
  * @param size size of character
  * @param str the string (must be a supported character)
  * @param x x position (left-side of string)
  * @param y y position (bottom of string)
  * @param w width of bounding box
  * @param h height of bounding box
  * @param color color of the string
  * @param invert if true, the string will be inverted
  * @retval the width of the string drawn
  */
uint16_t tft_puts(tft_font_size_t size, const char *str, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color, bool invert)
{
    (void) size;
    (void) str;
    (void) x;
    (void) y;
    (void) w;
    (void) h;
    (void) color;
    (void) invert;
}

/**
  * @brief Fill area with specified pattern
  * @param x1 y1 top left corner
  * @param x2 y2 bottom right corner
  * @param fill buffer to fill from (needs not match the described area)
  * @param fill_size size of buffer
  * @retval none
  */
void tft_fill_pattern(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t *fill, uint32_t fill_size)
{
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
    (void) fill;
    (void) fill_size;
}

/**
  * @brief Draw a one pixel rectangle
  * @param xpos x position
  * @param ypos y position
  * @param width width of frame
  * @param height height of frame
  * @param color color in bgr565 format
  * @retval none
  */
void tft_rect(uint32_t xpos, uint32_t ypos, uint32_t width, uint32_t height, uint16_t color)
{
    (void) xpos;
    (void) ypos;
    (void) width;
    (void) height;
    (void) color;
}

/**
  * @brief Fill area with specified color
  * @param x y top left corner
  * @param w h width and height
  * @param color in bgr565 format
  * @retval none
  * @todo Change prototype to match tft_fill_pattern (or vice versa)
  */
void tft_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
    (void) x;
    (void) y;
    (void) w;
    (void) h;
    (void) color;
}

/**
  * @brief Invert display
  * @param invert true to invert, false to restore
  * @retval none
  */
void tft_invert(bool invert)
{
    (void) invert;
}

/**
  * @brief Get invert mode
  * @retval true if TFT is inverted
  */
bool tft_is_inverted(void)
{
    return false;
}
