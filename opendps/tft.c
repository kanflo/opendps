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
#include "font-full_small.h"
#include "font-meter_small.h"
#include "font-meter_medium.h"
#include "font-meter_large.h"
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

static uint16_t blit_buffer[((4*FONT_METER_LARGE_MAX_GLYPH_WIDTH*FONT_METER_LARGE_MAX_GLYPH_HEIGHT)+3)/4]; // Alignment for being able to lay down uint64_t in one go, without dealing with padding

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
  * @brief Decode 2bpp glyph to TFT-native bgr565 format into the tft's blit_buffer
  * @param pixdata the input bytes from the font definition
  * @param nbytes number of bytes in the source glyph array
  * @param invert whether to invert the glyph
  * @param color color mask to use when decoding
  * @retval none
  */
void tft_decode_glyph(const uint8_t *pixdata, size_t nbytes, bool invert, uint16_t color)
{
    if(nbytes == 0) { /* we're attempting to draw a space */
        /** Wipe out the target buffer if we're drawing a space */
        memset(blit_buffer, (invert ? WHITE : BLACK) & 0xFF, sizeof(blit_buffer));
    }
    else {
        uint32_t *target32 = (uint32_t*)blit_buffer;
        if(invert) {
            for(size_t i = 0; i < nbytes; ++i) {
                *target32++ = ~mono2bpp_lookup[pixdata[i] & 0xF];
                *target32++ = ~mono2bpp_lookup[pixdata[i] >> 4];
            }
        }
        else if(color != WHITE) {
            uint32_t color_mask =
                ((uint32_t)ILI9163C_COLOR_TO_BITMASK(color) << 16) |
                ILI9163C_COLOR_TO_BITMASK(color);

            if(is_inverted)
                color_mask = ~color_mask;

            for(size_t i = 0; i < nbytes; ++i) {
                *target32++ = mono2bpp_lookup[pixdata[i] & 0xF] & color_mask;
                *target32++ = mono2bpp_lookup[pixdata[i] >> 4] & color_mask;
            }
        }
        else {
            for(size_t i = 0; i < nbytes; ++i) {
                *target32++ = mono2bpp_lookup[pixdata[i] & 0xF];
                *target32++ = mono2bpp_lookup[pixdata[i] >> 4];
            }
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
  * @brief Determine glyph spacing given the font size
  * @param size font size
  * @retval the spacing
  */
uint8_t tft_get_glyph_spacing(tft_font_size_t size)
{
    switch(size) {
        case FONT_FULL_SMALL:
            return FONT_FULL_SMALL_SPACING;
        case FONT_METER_SMALL:
            return FONT_METER_SMALL_SPACING;
        case FONT_METER_MEDIUM:
            return FONT_METER_MEDIUM_SPACING;
        case FONT_METER_LARGE:
            return FONT_METER_LARGE_SPACING;
        default:
            return 0;
    }
}

/**
  * @brief Determine glyph metrics given the supplied character and font size
  * @param size font size
  * @param ch the character (must be a supported character)
  * @param glyph_width (out) the width in pixels of the character
  * @param glyph_height (out) the height in pixels of the character
  * @retval none
  */
void tft_get_glyph_metrics(tft_font_size_t size, char ch, uint32_t *glyph_width, uint32_t *glyph_height)
{
    size_t idx = ch - 0x20;
    switch(size) {
        case FONT_FULL_SMALL:
            *glyph_width = font_full_small_widths[idx];
            *glyph_height = font_full_small_height;
            break;
        case FONT_METER_SMALL:
            *glyph_width = font_meter_small_widths[idx];
            *glyph_height = font_meter_small_height;
            break;
        case FONT_METER_MEDIUM:
            *glyph_width = font_meter_medium_widths[idx];
            *glyph_height = font_meter_medium_height;
            break;
        case FONT_METER_LARGE:
            *glyph_width = font_meter_large_widths[idx];
            *glyph_height = font_meter_large_height;
            break;
        default:
            dbg_printf("Cannot print at size %d\n", (int) size);
            return;
    }
}

/**
  * @brief Determine glyph pixel data given the supplied character and font size
  * @param size font size
  * @param ch the character (must be a supported character)
  * @param glyph_pixdata (out) the pointer to the pixel data for the glyph
  * @param glyph_size (out) the number of bytes taken up in pixdata for this glyph
  * @retval none
  */
void tft_get_glyph_pixdata(tft_font_size_t size, char ch, const uint8_t **glyph_pixdata, uint32_t *glyph_size)
{
    size_t idx = ch - 0x20;
    switch(size) {
        case FONT_FULL_SMALL:
            *glyph_pixdata = &font_full_small_pixdata[font_full_small_offsets[idx]];
            *glyph_size = font_full_small_sizes[idx];
            break;
        case FONT_METER_SMALL:
            *glyph_pixdata = &font_meter_small_pixdata[font_meter_small_offsets[idx]];
            *glyph_size = font_meter_small_sizes[idx];
            break;
        case FONT_METER_MEDIUM:
            *glyph_pixdata = &font_meter_medium_pixdata[font_meter_medium_offsets[idx]];
            *glyph_size = font_meter_medium_sizes[idx];
            break;
        case FONT_METER_LARGE:
            *glyph_pixdata = &font_meter_large_pixdata[font_meter_large_offsets[idx]];
            *glyph_size = font_meter_large_sizes[idx];
            break;
        default:
            dbg_printf("Cannot print at size %d\n", (int) size);
            return;
    }
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
  * @retval the width of the character drawn
  */
uint8_t tft_putch(tft_font_size_t size, char ch, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color, bool invert)
{
    uint32_t glyph_width, glyph_height, glyph_size;
    uint32_t xpos, ypos;
    const uint8_t *glyph_pixdata;

    /** Get the glyph metrics */
    tft_get_glyph_metrics(size, ch, &glyph_width, &glyph_height);

    /** Skip drawing if there's no character available */
    if(glyph_width == 0 || glyph_height == 0) {
        dbg_printf("Glyph 0x%02X does not exist in font size %d\n", (int) ch, (int) size);
        return 0;
    }

    /** Get the glyph data and decode it to the native TFT format */
    tft_get_glyph_pixdata(size, ch, &glyph_pixdata, &glyph_size);
    tft_decode_glyph(glyph_pixdata, glyph_size, invert, color);

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

    return glyph_width;
}

/**
  * @brief Determine string metrics given the supplied string and font size
  * @param size font size
  * @param str the string
  * @param string_width (out) the width in pixels of the string
  * @param string_height (out) the height in pixels of the string
  * @retval none
  */
void tft_get_string_metrics(tft_font_size_t size, const char *str, uint32_t *string_width, uint32_t *string_height)
{
    uint32_t w = 0, h = 0;
    uint8_t spacing = tft_get_glyph_spacing(size);
    bool first = true;

    while(str && *str) {
        uint32_t glyph_width, glyph_height;

        if(!first) {
            w += spacing;
        }

        tft_get_glyph_metrics(size, *str, &glyph_width, &glyph_height);
        h = glyph_height;
        w += glyph_width;

        first = false;
        ++str;
    }

    *string_width = w;
    *string_height = h;
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
    uint32_t width_remainder, screen_remainder, draw_remainder;
    uint16_t screen_w, screen_h;
    uint32_t xpos, ypos;
    uint32_t space_width, font_height;
    uint8_t spacing = tft_get_glyph_spacing(size);
    bool first = true;

    ili9163c_get_geometry(&screen_w, &screen_h);
    tft_get_glyph_metrics(size, ' ', &space_width, &font_height);
    xpos = x;
    ypos = y - font_height;

    while(str && *str) {
        uint32_t glyph_width, glyph_height, glyph_size;
        const uint8_t *glyph_pixdata;

        if(!first) {
            tft_fill(xpos, ypos, spacing, h, invert ? WHITE : BLACK);
            xpos += spacing;
        }

        /** Get the glyph metadata */
        tft_get_glyph_metrics(size, *str, &glyph_width, &glyph_height);

        /** Skip drawing if there's no character available */
        if(glyph_width == 0 || glyph_height == 0) {
            dbg_printf("Glyph 0x%02X does not exist in font size %d\n", (int) *str, (int) size);
            continue;
        }

        /** Check if this character would exceed the supplied width or screen width, blank the rest and drop out if so */
        width_remainder = w - (xpos - x);
        screen_remainder = screen_w - (xpos - x);
        draw_remainder = width_remainder < screen_remainder ? width_remainder : screen_remainder;
        if(glyph_width > draw_remainder) {
            tft_fill(xpos, ypos, draw_remainder, glyph_height, invert ? WHITE : BLACK);
            xpos += draw_remainder;
            return xpos - x;
        }

        /** Get the glyph data and decode it to the native TFT format */
        tft_get_glyph_pixdata(size, *str, &glyph_pixdata, &glyph_size);
        tft_decode_glyph(glyph_pixdata, glyph_size, invert, color);

        /** Draw the glyph */
        ili9163c_set_window(xpos, ypos, xpos + glyph_width-1, ypos + glyph_height-1);
        gpio_set(TFT_A0_PORT, TFT_A0_PIN);
        (void) spi_dma_transceive((uint8_t*) blit_buffer, sizeof(uint16_t) * glyph_width * glyph_height, 0, 0);

        xpos += glyph_width;

        first = false;
        ++str;
    }

    return xpos - x;
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
  * @brief Draw a one pixel rectangle
  * @param xpos x position
  * @param ypos y position
  * @param width width of frame
  * @param height height of frame
  * @param color color in bgr565 format
  */
void tft_rect(uint32_t xpos, uint32_t ypos, uint32_t width, uint32_t height, uint16_t color)
{
    ili9163c_draw_hline(xpos, ypos, width, color);
    ili9163c_draw_hline(xpos, ypos + height, width, color);
    ili9163c_draw_vline(xpos, ypos, height, color);
    ili9163c_draw_vline(xpos + width, ypos, height, color);
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
