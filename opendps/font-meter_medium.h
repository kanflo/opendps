/** Font generated from `./gen_lookup.py -f gfx/Ubuntu-C.ttf -s 24 -o meter_medium` */

#ifndef __FONT_METER_MEDIUM_H__
#define __FONT_METER_MEDIUM_H__

#include <stdint.h>

#define FONT_METER_MEDIUM_MAX_GLYPH_HEIGHT (17)
#define FONT_METER_MEDIUM_MAX_GLYPH_WIDTH  (16)
#define FONT_METER_MEDIUM_MAX_DIGIT_WIDTH  (9)
#define FONT_METER_MEDIUM_DOT_WIDTH        (3)
#define FONT_METER_MEDIUM_SPACING          (3)
#define FONT_METER_MEDIUM_SPACE_WIDTH      (5)

extern const uint32_t font_meter_medium_height;
extern const uint8_t font_meter_medium_widths[96];
extern const uint8_t font_meter_medium_sizes[96];
extern const uint16_t font_meter_medium_offsets[96];
extern const uint8_t font_meter_medium_pixdata[512];

#endif // __FONT_METER_MEDIUM_H__