/* fontwidth - calculate font widths from convert 24-bit RGB pixels

  Base on work by by Glenn Randers-Pehrson <glennrp@users.sf.net>

  Reads an RBG image from stdin and looks for white pixels representing
  character widths.

  Use with ImageMagick or GraphicsMagick to convert 24-bit RGB pixels
  to 16-bit BGR565 pixels, e.g.,

    % identify gfx/png/ubuntu-condensed-48-widths.png 
    gfx/png/ubuntu-condensed-48-width.png PNG 209x35 209x35+0+0 8-bit sRGB 1.32KB 0.000u 0:00.009
    % magick gfx/png/ubuntu-condensed-48-width.png -depth 8 rgb:- | ./fontwidth 209
    [ 15, 11, 16, 16, 18, 16, 16, 17, 16, 16, 7, 22, 22 ]

  To the extent possible under law, the author has dedicated all copyright
  and related and neighboring rights to this software to the public domain
  worldwide. This software is distributed without any warranty.
  See <http://creativecommons.org/publicdomain/zero/1.0/>. 

  Note that the 16-bit pixels are written in network byte order (most
  significant byte first), with blue in the most significant bits and
  red in the least significant bits.
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    int red, green, blue;
    uint32_t image_width, image_height;
    uint32_t char_width = 0;
    uint32_t total_char_width = 0;
    bool first = true;
    bool found_width = false;

    if (argc != 3) {
        printf("You need to specify the image width and height (oh, and provide a 24 bit RGB image on stdin).\n");
        return 1;
    }

    image_width = atoi(argv[1]);
    image_height = atoi(argv[2]);

    while (image_width) {
        red = getchar(); if (red == EOF) return (0);
        green = getchar(); if (green == EOF) return (1);
        blue = getchar(); if (blue == EOF) return (1);
        char_width++;
        if (red == 0xff && green == 0xff && blue == 0xff) {
            found_width = true;
#if 1
            printf("convert -crop %ux%u+%u+0 font.png rusk.png\n", char_width, image_height, total_char_width);
#else
            if (first) {
                first = false;
                printf("uint8_t font_width[] = { %2d", char_width);
            } else {
                printf(", %2d", char_width);
            }
#endif
            total_char_width += char_width;
            char_width = 0;
        }
        image_width--;
    }
    if (found_width) {
        printf(" };\n");
        if (total_char_width != atoi(argv[1])) {
            printf("Not sure if this image is correct\n");
        }
    } else {
        printf("Are you sure this is a font width file?\n");
    }
    return 0;
}
