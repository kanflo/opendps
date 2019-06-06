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

#ifndef __TFT_H__
#define __TFT_H__

typedef enum
{
    FONT_FULL_SMALL,
    FONT_METER_SMALL,
    FONT_METER_MEDIUM,
    FONT_METER_LARGE
} tft_font_size_t;

/**
  * @brief Initialize the TFT module
  * @retval none
  */
void tft_init(void);

/**
  * @brief Clear the TFT
  * @retval none
  */
void tft_clear(void);

/**
  * @brief Determine glyph spacing given the font size
  * @param size font size
  * @retval the spacing
  */
uint8_t tft_get_glyph_spacing(tft_font_size_t size);

/**
  * @brief Determine glyph metrics given the supplied character and font size
  * @param size font size
  * @param ch the character (must be a supported character)
  * @param glyph_width (out) the width in pixels of the character
  * @param glyph_height (out) the height in pixels of the character
  * @retval none
  */
void tft_get_glyph_metrics(tft_font_size_t size, char ch, uint32_t *glyph_width, uint32_t *glyph_height);

/**
  * @brief Determine glyph pixel data given the supplied character and font size
  * @param size font size
  * @param ch the character (must be a supported character)
  * @param glyph_pixdata (out) the pointer to the pixel data for the glyph
  * @param glyph_size (out) the number of bytes taken up in pixdata for this glyph
  * @retval none
  */
void tft_get_glyph_pixdata(tft_font_size_t size, char ch, const uint8_t **glyph_pixdata, uint32_t *glyph_size);

/**
  * @brief Decode 2bpp glyph to TFT-native bgr565 format into the tft's blit_buffer
  * @param pixdata the input bytes from the font definition
  * @param nbytes number of bytes in the source glyph array
  * @param invert whether to invert the glyph
  * @param color color mask to use when decoding
  * @retval none
  */
void tft_decode_glyph(const uint8_t *pixdata, size_t nbytes, bool invert, uint16_t color);

/**
  * @brief Blit graphics on TFT
  * @param bits graphics in bgr565 format mathing the specified size
  * @param width width of data
  * @param height of data
  * @param x x position
  * @param y y position
  * @retval none
  */
void tft_blit(uint16_t *bits, uint32_t width, uint32_t height, uint32_t x, uint32_t y);

/**
  * @brief Blit character on TFT
  * @param size font size used
  * @param ch the character (must be a supported character)
  * @param x x position
  * @param y y position
  * @param w width of bounding box
  * @param h height of bounding box
  * @param color color of the glyph
  * @param invert if true, the character will be inverted
  * @retval the width of the character drawn
  */
uint8_t tft_putch(tft_font_size_t size, char ch, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color, bool invert);

/**
  * @brief Determine string metrics given the supplied string and font size
  * @param size font size
  * @param str the string
  * @param string_width (out) the width in pixels of the string
  * @param string_height (out) the height in pixels of the string
  * @retval none
  */
void tft_get_string_metrics(tft_font_size_t size, const char *str, uint32_t *string_width, uint32_t *string_height);

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
uint16_t tft_puts(tft_font_size_t size, const char *str, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color, bool invert);

/**
  * @brief Fill area with specified pattern
  * @param x1,y1 top left corner
  * @param x2,y2 bottom right corner
  * @param fill buffer to fill from (needs not match the described area)
  * @param fill_size size of buffer
  * @retval none
  */
void tft_fill_pattern(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t *fill, uint32_t fill_size);

/**
  * @brief Fill area with specified color
  * @param x,y top left corner
  * @param w,h width and height
  * @param color in bgr565 format
  * @retval none
  */
void tft_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);

/**
  * @brief Draw a one pixel rectangle
  * @param xpos x position
  * @param ypos y position
  * @param width width of frame
  * @param height height of frame
  * @param color color in bgr565 format
  * @retval none
  */
void tft_rect(uint32_t xpos, uint32_t ypos, uint32_t width, uint32_t height, uint16_t color);

/**
  * @brief Invert display
  * @param invert true to invert, false to restore
  * @retval none
  */
void tft_invert(bool invert);

/**
  * @brief Get invert mode
  * @retval true if TFT is inverted
  */
bool tft_is_inverted(void);

#ifdef DPS_EMULATOR
void emul_tft_draw(void);
#endif // DPS_EMULATOR

#endif // __TFT_H__
