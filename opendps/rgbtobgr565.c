/* rgbtobgr565 - convert 24-bit RGB pixels to 16-bit BGR565 pixels

  Written in 2016 by Glenn Randers-Pehrson <glennrp@users.sf.net>

  To the extent possible under law, the author has dedicated all copyright
  and related and neighboring rights to this software to the public domain
  worldwide. This software is distributed without any warranty.
  See <http://creativecommons.org/publicdomain/zero/1.0/>. 

  Use with ImageMagick or GraphicsMagick to convert 24-bit RGB pixels
  to 16-bit BGR565 pixels, e.g.,

      magick file.png -depth 8 rgb:- | rgbtobgr565 > file.bgr565

  Note that the 16-bit pixels are written in network byte order (most
  significant byte first), with blue in the most significant bits and
  red in the least significant bits.

  ChangLog:
  Jan 2017: changed bgr565 from int to unsigned short (suggested by
             Steven Valsesia)
*/

#include <stdio.h>

int main()
{
    int red,green,blue;
    unsigned short bgr565;
    while (1) {
        red=getchar(); if (red == EOF) return (0);
        green=getchar(); if (green == EOF) return (1);
        blue=getchar(); if (blue == EOF) return (1);
        bgr565 = (unsigned short)(red * 31.0 / 255.0) |
            (unsigned short)(green * 63.0 / 255.0) << 5 |
            (unsigned short)(blue * 31.0 / 255.0) << 11;
        putchar((bgr565 >> 8) & 0xFF);
        putchar(bgr565 & 0xFF);
    }
}
