#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import sys
import os
try:
    from PIL import Image
    from PIL import ImageDraw
    from PIL import ImageFont
    from PIL import ImageChops
except ImportError:
    print("%s depends on the Python Image Library, please install:" % sys.argv[0])
    print("pip install Pillow")
    sys.exit(0)

"""
Convert an image to a bgr565 byte array
"""
def image_to_bgr565(im):
    image_bytes = im.tobytes("raw", "RGB") # Create a byte array in 24 bit RGB format from an image

    # Convert 24-bit RGB to 16-bit BGR
    bgr565array = []
    for x in range(len(image_bytes) / 3):
        bgr565 = (
            (int(ord(image_bytes[x*3]) * 31.0 / 255.0)) |
            (int(ord(image_bytes[x*3+1]) * 63.0 / 255.0)) << 5 |
            (int(ord(image_bytes[x*3+2]) * 31.0 / 255.0)) << 11
        )

        bgr565array.append((bgr565 >> 8) & 0xFF)
        bgr565array.append(bgr565 & 0xFF)

    return bgr565array

"""
Measure the size of an image and return the coordinates of the (left, upper, right, lower) bounding box
"""
def measure(im, border=(0,0,0,0)):
    bg = Image.new(im.mode, im.size, border)
    diff = ImageChops.difference(im, bg)
    diff = ImageChops.add(diff, diff, 2.0, -100)
    return diff.getbbox()

"""
Generate and return seperate images for each character in the characters string for the font and size specified
"""
def generate_font_images(font_fname, characters, font_size):
    font = ImageFont.truetype(font_fname, font_size)

    # Create an image of each character
    images = [None] * len(characters)
    font_canvas_size = [None] * len(characters)
    for i in range(0, len(characters)):
        images[i] = Image.new("RGB", (100,100), (0, 0, 0)) # Draw a letter into a new 100x100 box
        font_draw = ImageDraw.Draw(images[i])
        font_draw.text((0, 0), characters[i], (255, 255, 255), font=font)
        font_canvas_size[i] = measure(images[i])

    # Find the size of each character but keeping all heights the same
    for i in range(0, len(characters)):
        font_canvas_size[i] = ( # left, upper, right, lower
                font_canvas_size[i][0],
                min(x[1] for x in font_canvas_size),
                font_canvas_size[i][2],
                max(x[3] for x in font_canvas_size),
        )

    # Crop all characters to the same height but a different width
    character_widths = []
    for i in range(0, len(characters)):

        # Crop box around content, using the array we calulated in last loop
        images[i] = images[i].crop(font_canvas_size[i])

        # Add to sprites next to each other.
        width, height = images[i].size
        character_widths.append(width)

    return images

"""
Convert the specified font to a pair of .c/.h C language lookup tables in BGR565 format
"""
def convert_font_to_c(font_fname, characters, character_strings, font_size):
    print "Converting %s to font-%d.c/h" % (font_fname, font_size)

    if len(characters) != len(character_strings):
        print("characters and character_strings must have matching length")
        sys.exit(1)

    character_images = generate_font_images(font_fname, characters, font_size)
    (_, character_heights) = character_images[0].size

    # Convert all character images to bgr565 byte lists
    character_data = []
    for i in range(len(characters)):
        character_data.append(image_to_bgr565(character_images[i]))

    # Generate the C source file
    file_name = "font-%d.c" % (font_size)
    font_source_file = open(file_name, "w")

    font_source_file.write("/** Font generated from %s */\n\n" % (font_fname))
    font_source_file.write("#include <stdint.h>\n")
    font_source_file.write("const uint32_t font_%d_height = %d;\n" % (font_size, character_heights))
    font_source_file.write("const uint32_t font_%d_num_glyphs = %d;\n\n" % (font_size, len(character_strings)))

    # Generate character data lookup tables
    for i in range(len(characters)):
        font_source_file.write("const uint8_t font_%d_%s[] = {\n  " % (font_size, character_strings[i]))

        count = 0
        for j in character_data[i]:
            font_source_file.write("{0:#04x}".format(j))
            count += 1
            if count < len(character_data[i]):
                font_source_file.write(", ")
                if not (count % 16): # Place a new line every 16 values
                    font_source_file.write("\n  ")

        font_source_file.write("\n};\n")
        font_source_file.write("const uint32_t font_%d_%s_len = %d;\n\n" % (font_size, character_strings[i], len(character_data[i])))

    # Generate glyph widths
    font_source_file.write("const uint8_t font_%d_widths[%d] = {\n  " % (font_size, len(character_strings)))
    count = 0
    for i in range(len(characters)):
        (width, _) = character_images[i].size
        font_source_file.write("%d" % (width))
        count += 1
        if count < len(characters):
            font_source_file.write(",\n  ")
    font_source_file.write("\n};\n\n")

    # Generate glyph sizes
    font_source_file.write("const uint16_t font_%d_sizes[%d] = {\n  " % (font_size, len(character_strings)))
    count = 0
    for i in range(len(characters)):
        font_source_file.write("%d" % (len(character_data[i])))
        count += 1
        if count < len(characters):
            font_source_file.write(",\n  ")
    font_source_file.write("\n};\n\n")

    # Generate glyph pix pointers
    font_source_file.write("const uint16_t *font_%d_pix[%d] = {\n  " % (font_size, len(character_strings)))
    count = 0
    for i in range(len(characters)):
        font_source_file.write("(uint16_t*) &font_%d_%s" % (font_size, character_strings[i]))
        count += 1
        if count < len(characters):
            font_source_file.write(",\n  ")
    font_source_file.write("\n};\n\n")

    font_source_file.close()

    # Generate the C header file
    file_name = "font-%d.h" % (font_size)
    font_header_file = open(file_name, "w")

    font_header_file.write("extern const uint32_t font_%d_height;\n" % (font_size))
    font_header_file.write("extern const uint32_t font_%d_num_glyphs;\n" % (font_size))

    for i in range(len(characters)):
        font_header_file.write("extern const uint8_t font_%d_%s[];\n" % (font_size, character_strings[i]))
        font_header_file.write("extern const uint32_t font_%d_%s_len;\n" % (font_size, character_strings[i]))

    font_header_file.write("extern const uint8_t font_%d_widths[%d];\n" % (font_size, len(character_strings)))
    font_header_file.write("extern const uint16_t font_%d_sizes[%d];\n" % (font_size, len(character_strings)))
    font_header_file.write("extern const uint16_t *font_%d_pix[%d];\n" % (font_size, len(character_strings)))

    font_header_file.close()

def main():
    global args
    parser = argparse.ArgumentParser(description='Generate font lookup tables for the OpenDPS firmware')
    parser.add_argument('-f', '--font', type=str, required=True, dest="fontfile", help="The font to use")
    args, unknown = parser.parse_known_args()

    # Check font file actually exists
    if not os.path.exists(args.fontfile):
        print("Can't find file %s" % (args.fontfile))
        sys.exit(1)

    characters = "0123456789.VA" # The characters to generate a lookup table of
    character_strings = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'dot', 'v', 'a'] # The textual reference to each character

    convert_font_to_c(args.fontfile, characters, character_strings, 18)
    convert_font_to_c(args.fontfile, characters, character_strings, 24)
    convert_font_to_c(args.fontfile, characters, character_strings, 48)

if __name__ == "__main__":
    main()