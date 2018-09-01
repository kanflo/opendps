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
#include <inttypes.h>
#include <string.h>
#include <rcc.h>
#include <adc.h>
#include <dac.h>
#include <timer.h>
#include <gpio.h>
#include <nvic.h>
#include <exti.h>
#include "spi_driver.h"
#include "cli.h"
#include "hw.h"
#include "tft.h"
#include "ili9163c.h"
#include "font-18.h"
#include "font-24.h"
#include "font-48.h"
#include "dbg_printf.h"

static bool is_inverted;

/** Buffers for speeding up drawing */
static uint16_t blit_buffer[20*35]; // 20x35 pixels
static uint16_t black_buffer[20*35]; // 20x35 pixels

static void frame_glyph(uint32_t xpos, uint32_t ypos, uint32_t glyph_height, uint32_t glyph_width, uint16_t color);


/**
  * @brief Initialize the TFT module
  * @retval none
  */
void tft_init(void)
{
    ili9163c_init();
    ili9163c_set_rotation(3); // Specific for DPS5005
    ili9163c_fill_screen(BLACK);
}

/**
  * @brief Clear the TFT
  * @retval none
  */
void tft_clear(void)
{
    ili9163c_fill_screen(BLACK);
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
    ili9163c_set_window(x, y, x + width-1, y + height-1);
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    (void) spi_dma_transceive((uint8_t*) bits, 2*width*height, 0, 0);
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
void tft_putch(uint8_t size, char ch, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool highlight)
{
    uint32_t glyph_index, glyph_width, glyph_height, glyph_size;
    uint32_t xpos, ypos;
    uint16_t *glyph;
    switch(ch) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            glyph_index = ch - '0';
            break;
        case '.':
            glyph_index = 10;
            break;
        case 'V':
            glyph_index = 11;
            break;
        case 'A':
            glyph_index = 12;
            break;
        default:
            dbg_printf("Cannot print character '%c'\n", ch);
            return;
    }
    switch(size) {
        case 18:
            glyph_width = font_18_widths[glyph_index];
            glyph = (uint16_t*) font_18_pix[glyph_index];
            glyph_size = font_18_sizes[glyph_index];
            glyph_height = font_18_height;
            break;
        case 24:
            glyph_width = font_24_widths[glyph_index];
            glyph = (uint16_t*) font_24_pix[glyph_index];
            glyph_size = font_24_sizes[glyph_index];
            glyph_height = font_24_height;
            break;
        case 48:
            glyph_width = font_48_widths[glyph_index];
            glyph = (uint16_t*) font_48_pix[glyph_index];
            glyph_size = font_48_sizes[glyph_index];
            glyph_height = font_48_height;
            break;
        default:
            dbg_printf("Cannot print at size %d\n", (int) size);
            return;
    }

    if (highlight) {
        uint32_t *p = (uint32_t*) &blit_buffer;
        /** @todo Perform memcpy and inversion operation */
        memcpy(blit_buffer, glyph, glyph_size);
        uint32_t i;
        for (i = 0; i < glyph_size/4; i++) { // Invert 32 bits in each go
            p[i] = ~p[i];
        }
        if (glyph_size % 4) { // Bits remaining?
            blit_buffer[2*i] = ~blit_buffer[2*i];
        }
        glyph = blit_buffer;
    }

    if (w < glyph_width) {
        w = glyph_width + 2;
    }
    if (h < glyph_height) {
        h = glyph_height + 2;
    }
    xpos = x+(w-glyph_width)/2;
    ypos = y+(h-glyph_height)/2;

    ili9163c_set_window(xpos, ypos, xpos + glyph_width-1, ypos + glyph_height-1);
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    (void) spi_dma_transceive((uint8_t*) glyph, 2 * glyph_width * glyph_height, 0, 0);

    if (x < xpos) {
        tft_fill_pattern(x, y, xpos-1, y+h-1, (uint8_t*) black_buffer, sizeof(black_buffer));
    }
    if (xpos+glyph_width < x+w) {
        tft_fill_pattern(xpos+glyph_width, y, x+w, y+h-1, (uint8_t*) black_buffer, sizeof(black_buffer));
    }

    if (highlight) {
        // Add some more highlighting around the glyph
        frame_glyph(xpos, ypos, glyph_height, glyph_width, WHITE);
    } else if (size == 1) { /** @todo Ugly hack as only size 1 can get highlighted */
        // If not highlighted, make sure to erase the highlight (small font will never get highlighted)
        frame_glyph(xpos, ypos, glyph_height, glyph_width, BLACK);
    }
}

/**
  * @brief Fill area with specified pattern
  * @param x1,y1 top left corner
  * @param x2,y2 bottom right corner
  * @param fill buffer to fill from (needs not match the described area)
  * @param fill_size size of buffer
  * @retval none
  */
void tft_fill_pattern(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t *fill, uint32_t fill_size)
{
    uint32_t count = 2*(x2-x1+1)*(y2-y1+1);
    ili9163c_set_window(x1, y1, x2, y2);
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    while(count) {
        uint32_t c = count < fill_size ? count : fill_size;
        (void) spi_dma_transceive((uint8_t*) fill, c, 0, 0);
        count -= c;
    }
}

/**
  * @brief Fill area with specified color
  * @param x,y top left corner
  * @param w,h width and height
  * @param color in bgr565 format
  * @retval none
  * @todo Change prototype to match tft_fill_pattern (or vice versa)
  */
void tft_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
    uint8_t hi = color >> 8;
    uint8_t lo = color && 0xff;
    int32_t count = 2 * w * h;
    uint8_t fill[] = {hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo};
    ili9163c_set_window(x, y, x+w-1, y+h-1);
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    while (count > 0) {
        uint32_t len = (uint32_t) count > sizeof(fill) ? sizeof(fill) : (uint32_t) count;
        (void) spi_dma_transceive((uint8_t*) fill, len, 0, 0);
        count -= sizeof(fill);
    }
}

/**
  * @brief Invert display
  * @param invert true to invert, false to restore
  * @retval none
  */
void tft_invert(bool invert)
{
    ili9163c_invert_display(invert);
    is_inverted = invert;
}

/**
  * @brief Get invert mode
  * @retval true if TFT is inverted
  */
bool tft_is_inverted(void)
{
    return is_inverted;
}

/**
  * @brief Draw a highlight frame around a glyph
  * @param xpos x position
  * @param ypos y position
  * @param glyph_height height of frame
  * @param glyph_width width of frame
  * @param color color in bgr565 format
  * @retval none
  */
static void frame_glyph(uint32_t xpos, uint32_t ypos, uint32_t glyph_height, uint32_t glyph_width, uint16_t color)
{
    for (uint32_t i = xpos; i < xpos + glyph_width; i++) {
        ili9163c_draw_pixel(i, ypos-1, color);
        ili9163c_draw_pixel(i, ypos + glyph_height, color);
    }
    for (uint32_t i = ypos; i < ypos + glyph_height; i++) {
        ili9163c_draw_pixel(xpos-1, i, color);
        ili9163c_draw_pixel(xpos + glyph_width, i, color);
    }
}

