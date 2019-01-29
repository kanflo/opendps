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
#include "font-small.h"
#include "font-medium.h"
#include "font-large.h"
#include "dbg_printf.h"
#include "gfx_lookup.h"

static bool is_inverted;

#define ILI9163C_COLORSPACE_TWIDDLE(color) \
        (((COLORSPACE) == 0) \
            ? (((color) & 0xF800) >> 11) | ((color) & 0x07E0) | (((color) & 0x001F) << 11) \
            : (color))

#define ILI9163C_COLOR_TO_BITMASK(color) ( \
    ((ILI9163C_COLORSPACE_TWIDDLE(color) & 0xFF) << 8) | \
    ((ILI9163C_COLORSPACE_TWIDDLE(color) >> 8) & 0xFF) )

/** Buffers for speeding up drawing */

static uint16_t blit_buffer[((4*FONT_LARGE_MAX_GLYPH_WIDTH*FONT_LARGE_MAX_GLYPH_HEIGHT)+3)/4]; // +1 for uint64_t alignment, if odd number of pixels

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
  * @brief Decode 4bpp glyph to TFT-native bgr565 format
  * @param target the buffer to write the resulting image data to
  * @param source the input bytes from the font definition
  * @param nbytes number of bytes in the source glyph array
  * @param invert whether to invert the glyph
  * @param color color mask to use when decoding
  * @retval none
  */
void tft_decode_glyph(uint16_t *target, const uint8_t *source, size_t nbytes, bool invert, uint16_t color)
{
    uint32_t *target32 = (uint32_t*)target;
    if(invert)
    {
        for(size_t i = 0; i < nbytes; ++i)
        {
            *target32++ = ~mono2bpp_lookup[source[i] & 0xF];
            *target32++ = ~mono2bpp_lookup[source[i] >> 4];
        }
    }
    else if(color != WHITE)
    {
        uint32_t color_mask =
            ((uint32_t)ILI9163C_COLOR_TO_BITMASK(color) << 16) |
            ILI9163C_COLOR_TO_BITMASK(color);
        if(is_inverted)
            color_mask = ~color_mask;
        for(size_t i = 0; i < nbytes; ++i)
        {
            *target32++ = mono2bpp_lookup[source[i] & 0xF] & color_mask;
            *target32++ = mono2bpp_lookup[source[i] >> 4] & color_mask;
        }
    }
    else
    {
        for(size_t i = 0; i < nbytes; ++i)
        {
            *target32++ = mono2bpp_lookup[source[i] & 0xF];
            *target32++ = mono2bpp_lookup[source[i] >> 4];
        }
    }
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
  * @param size size of character
  * @param ch the character (must be a supported character)
  * @param x x position
  * @param y y position
  * @param w width of bounding box
  * @param h height of bounding box
  * @param color color of the glyph
  * @param highlight if true, the character will be inverted
  * @retval none
  */
void tft_putch(tft_font_size_t size, char ch, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color, bool invert)
{
    uint32_t glyph_index, glyph_width, glyph_height, glyph_size;
    uint32_t xpos, ypos;
    const uint8_t *glyph;
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
        case FONT_SMALL:
            glyph_width = font_small_widths[glyph_index];
            glyph = &font_small_pixdata[font_small_offsets[glyph_index]];
            glyph_size = font_small_sizes[glyph_index];
            glyph_height = font_small_height;
            break;
        case FONT_MEDIUM:
            glyph_width = font_medium_widths[glyph_index];
            glyph = &font_medium_pixdata[font_medium_offsets[glyph_index]];
            glyph_size = font_medium_sizes[glyph_index];
            glyph_height = font_medium_height;
            break;
        case FONT_LARGE:
            glyph_width = font_large_widths[glyph_index];
            glyph = &font_large_pixdata[font_large_offsets[glyph_index]];
            glyph_size = font_large_sizes[glyph_index];
            glyph_height = font_large_height;
            break;
        default:
            dbg_printf("Cannot print at size %d\n", (int) size);
            return;
    }

    tft_decode_glyph(blit_buffer, glyph, glyph_size, invert, color);

    /** Position glyph in center of region */
    xpos = x+(w-glyph_width)/2;
    ypos = y+(h-glyph_height)/2;

    /** Draw the glyph */
    ili9163c_set_window(xpos, ypos, xpos + glyph_width-1, ypos + glyph_height-1);
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    (void) spi_dma_transceive((uint8_t*) blit_buffer, sizeof(uint16_t) * glyph_width * glyph_height, 0, 0);

    /** If our glyph hasn't filled the entire region fill the remainder in with black or white depending on if we're inverting */
    uint16_t fill_color = invert ? WHITE : BLACK;
    if (x < xpos) {
        tft_fill(x, y, xpos-x, h, fill_color);
    }
    if (xpos+glyph_width < x+w) {
        tft_fill(xpos+glyph_width, y, (w-glyph_width+1)/2, h, fill_color);
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
    uint8_t lo = color & 0xff;
    int32_t count = 2 * w * h;
    uint8_t fill[] = {hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo};
    ili9163c_set_window(x, y, x+w-1, y+h-1);
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    while (count > 0) {
        uint32_t len = (uint32_t) count > sizeof(fill) ? sizeof(fill) : (uint32_t) count;
        (void) spi_dma_transceive((uint8_t*) fill, len, 0, 0);
        count -= len;
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
