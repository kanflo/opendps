/** Font generated from gfx/Ubuntu-C.ttf 18pt */

#ifndef __FONT_SMALL_H__
#define __FONT_SMALL_H__

#include <stdint.h>

#define FONT_SMALL_MAX_GLYPH_HEIGHT (12)
#define FONT_SMALL_MAX_GLYPH_WIDTH  (9)
#define FONT_SMALL_MAX_DIGIT_WIDTH  (6)
#define FONT_SMALL_DOT_WIDTH        (2)
#define FONT_SMALL_SPACING          (1)

extern const uint32_t font_small_height;
extern const uint32_t font_small_num_glyphs;
extern const uint8_t font_small_widths[13];
extern const uint8_t font_small_sizes[13];
extern const uint16_t font_small_offsets[13];
extern const uint8_t font_small_pixdata[234];

#endif // __FONT_SMALL_H__