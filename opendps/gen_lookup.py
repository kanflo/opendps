#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import sys
import os
import socket
try:
    from PIL import Image
    from PIL import ImageDraw
    from PIL import ImageFont
    from PIL import ImageChops
except ImportError:
    print("%s depends on the Python Image Library, please install:" % sys.argv[0])
    print("pip install Pillow")
    sys.exit(0)

def rgb888_to_bgr565(r,g,b):
    bgr565 = (
        (int(r * 31.0 / 255.0)) |
        (int(g * 63.0 / 255.0)) << 5 |
        (int(b * 31.0 / 255.0)) << 11
    )
    return bgr565

"""
Convert an image to a packed mono 2bpp byte array
"""
def image_to_mono2bpp(im):
    image_bytes = im.tobytes("raw", "R") # Create a byte array from the red channel of the input image
    image_bytes += '\0\0\0' # Dummy value at the end for alignment

    # Convert 8bpp to packed 2bpp
    mono2array = []
    for x in range(len(image_bytes) / 4):
        mono2array.append(int(ord(image_bytes[x*4+3]) / 64.0) << 6 | int(ord(image_bytes[x*4+2]) / 64.0) << 4 | int(ord(image_bytes[x*4+1]) / 64.0) << 2 | int(ord(image_bytes[x*4]) / 64.0))

    return mono2array

"""
Convert an image to a bgr565 byte array
"""
def image_to_bgr565(im):
    image_bytes = im.tobytes("raw", "RGB") # Create a byte array in 24 bit RGB format from an image

    # Convert 24-bit RGB to 16-bit BGR
    bgr565array = []
    for x in range(len(image_bytes) / 3):
        bgr565 = rgb888_to_bgr565(ord(image_bytes[x*3]), ord(image_bytes[x*3+1]), ord(image_bytes[x*3+2]))
        bgr565array.append((bgr565 >> 8) & 0xFF)
        bgr565array.append(bgr565 & 0xFF)

    return bgr565array

"""
Generate the lookup table for bytes consisting of packed 2bpp pixels
"""
def generate_pixel_lookup_table(output_filename):
    print "Generating lookup table for pixels as %s.c/.h" % (output_filename)
    source_filename = "%s.c" % (output_filename)
    header_filename = "%s.h" % (output_filename)

    # Generate the C source file
    source_file = open(source_filename, "w")
    source_file.write("#include \"%s\"\n\n" % (header_filename))
    source_file.write("const uint32_t mono2bpp_lookup[16] = {")
    for n in range(0,16):
        if not(n % 4):
            source_file.write("\n   ")
        a = int(((n >> 2) & 0x3) * 255.0 / 3.0)
        b = int(((n >> 0) & 0x3) * 255.0 / 3.0)
        a565 = socket.htons(rgb888_to_bgr565(a, a, a))
        b565 = socket.htons(rgb888_to_bgr565(b, b, b))
        source_file.write(" 0x%04X%04X," % (a565,b565))
    source_file.write("\n};\n")

    # Generate the C header file
    header_file = open(header_filename, "w")
    header_file.write("#ifndef __LOOKUP_%s_H__\n" % (output_filename.upper()))
    header_file.write("#define __LOOKUP_%s_H__\n\n" % (output_filename.upper()))
    header_file.write("#include <stdint.h>\n\n")
    header_file.write("extern const uint32_t mono2bpp_lookup[16];\n\n")
    header_file.write("#endif // __LOOKUP_%s_H__\n" % (output_filename.upper()))

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
    for i in range(0, len(characters)):

        # Crop box around content, using the array we calulated in last loop
        images[i] = images[i].crop(font_canvas_size[i])

    return images

"""
Convert the specified font to a pair of .c/.h C language lookup tables in mono 2bpp format
"""
def convert_font_to_c(font_fname, characters, font_size, font_spacing, output_filename):
    print "Converting %s %dpt to font-%s.c/h" % (font_fname, font_size, output_filename)

    character_images = generate_font_images(font_fname, characters, font_size)
    (_, character_heights) = character_images[0].size

    # Convert all character images to bgr565 byte lists
    character_data = []
    for i in range(len(characters)):
        character_data.append(image_to_mono2bpp(character_images[i]))

    # Generate the output filenames
    font_source_filename = "font-%s.c" % (output_filename)
    font_header_filename = "font-%s.h" % (output_filename)

    # Generate the C source file
    font_source_file = open(font_source_filename, "w")

    font_source_file.write("/** Font generated from %s %dpt */\n\n" % (font_fname, font_size))
    font_source_file.write("#include \"%s\"\n\n" % (font_header_filename))
    font_source_file.write("const uint32_t font_%s_height = %d;\n" % (output_filename, character_heights))
    font_source_file.write("const uint32_t font_%s_num_glyphs = %d;\n\n" % (output_filename, len(characters)))

    # Work out how many bytes all characters are going to use
    total_byte_count = 0
    for i in range(len(characters)):
        total_byte_count += len(character_data[i])

    # Generate character data lookup tables
    character_offsets = []
    current_offset = 0
    font_source_file.write("const uint8_t font_%s_pixdata[%d] = {" % (output_filename, total_byte_count))
    for i in range(len(characters)):
        font_source_file.write("\n  // %s\n " % (characters[i]))
        character_offsets.append(current_offset)
        current_offset += len(character_data[i])
        count = 0
        for j in character_data[i]:
            font_source_file.write(" {0:#04x},".format(j))
            count += 1
            if count < len(character_data[i]):
                if not (count % 16): # Place a new line every 16 values
                    font_source_file.write("\n ")

    font_source_file.write("\n};\n\n")

    # Generate glyph widths
    font_source_file.write("const uint8_t font_%s_widths[%d] = {\n  " % (output_filename, len(characters)))
    character_max_width = 0
    digit_max_width = 0
    count = 0
    for i in range(len(characters)):
        (width, _) = character_images[i].size
        font_source_file.write("%d" % (width))
        if width > character_max_width:
            character_max_width = width # Find the maximum glyph width
        if width > digit_max_width and characters[i].isdigit():
            digit_max_width = width # Find the maximum digit width
        count += 1
        if count < len(characters):
            font_source_file.write(",\n  ")
    font_source_file.write("\n};\n\n")

    # If no spacing parameter is set then generate one based on the max glyph width
    if not args.font_spacing:
        args.font_spacing = int(round(0.223 * float(character_max_width) - 0.863)) # This is fairly rough
        if args.font_spacing < 1:
            args.font_spacing = 1 # Spacing should be at least one pixel

    # Generate glyph sizes
    font_source_file.write("const uint8_t font_%s_sizes[%d] = {\n  " % (output_filename, len(characters)))
    count = 0
    for i in range(len(characters)):
        font_source_file.write("%d" % (len(character_data[i])))
        count += 1
        if count < len(characters):
            font_source_file.write(",\n  ")
    font_source_file.write("\n};\n\n")

    # Generate glyph pix pointers
    font_source_file.write("const uint16_t font_%s_offsets[%d] = {" % (output_filename, len(character_offsets)))
    for i in range(len(character_offsets)):
        font_source_file.write("\n  %d," % (character_offsets[i]))
    font_source_file.write("\n};\n\n")

    font_source_file.close()

    # Generate the C header file
    font_header_file = open(font_header_filename, "w")

    font_header_file.write("/** Font generated from %s %dpt */\n\n" % (font_fname, font_size))

    font_header_file.write("#ifndef __FONT_%s_H__\n" % (output_filename.upper()))
    font_header_file.write("#define __FONT_%s_H__\n\n" % (output_filename.upper()))

    font_header_file.write("#include <stdint.h>\n\n")

    font_header_file.write("#define FONT_%s_MAX_GLYPH_HEIGHT (%d)\n" % (output_filename.upper(), character_heights))
    font_header_file.write("#define FONT_%s_MAX_GLYPH_WIDTH  (%d)\n" % (output_filename.upper(), character_max_width))
    font_header_file.write("#define FONT_%s_MAX_DIGIT_WIDTH  (%d)\n" % (output_filename.upper(), digit_max_width))

    # If a dot is in the lookup tables make a #define for its width
    if '.' in characters:
        dot_index = characters.index('.')
        (dot_width, _) = character_images[dot_index].size
        font_header_file.write("#define FONT_%s_DOT_WIDTH        (%d)\n" % (output_filename.upper(), dot_width))

    font_header_file.write("#define FONT_%s_SPACING          (%d)\n\n" % (output_filename.upper(), args.font_spacing))

    font_header_file.write("extern const uint32_t font_%s_height;\n" % (output_filename))
    font_header_file.write("extern const uint32_t font_%s_num_glyphs;\n" % (output_filename))
    font_header_file.write("extern const uint8_t font_%s_widths[%d];\n" % (output_filename, len(characters)))
    font_header_file.write("extern const uint8_t font_%s_sizes[%d];\n" % (output_filename, len(characters)))
    font_header_file.write("extern const uint16_t font_%s_offsets[%d];\n" % (output_filename, len(characters)))
    font_header_file.write("extern const uint8_t font_%s_pixdata[%d];\n\n" % (output_filename, total_byte_count))

    font_header_file.write("#endif // __FONT_%s_H__" % (output_filename.upper()))

    font_header_file.close()

"""
Convert the specified font to a pair of .c/.h C language lookup tables in BGR565 format
"""
def convert_graphic_to_c(graphic_fname, output_filename):
    print "Converting %s to gfx-%s.c/h" % (graphic_fname, output_filename)

    graphic_image = Image.open(graphic_fname)

    # Get image dimensions
    (width, height) = graphic_image.size

    # Convert image to bgr565 byte list
    graphic_data = image_to_bgr565(graphic_image)

    # Generate the output filenames
    gfx_source_filename = "gfx-%s.c" % (output_filename)
    gfx_header_filename = "gfx-%s.h" % (output_filename)

    # Generate the C source file
    gfx_source_file = open(gfx_source_filename, "w")

    gfx_source_file.write("/** Gfx generated from %s */\n\n" % (graphic_fname))

    gfx_source_file.write("#include \"%s\"\n\n" % (gfx_header_filename))

    # Generate image data lookup table
    gfx_source_file.write("const uint8_t gfx_%s[%d] = {\n  " % (output_filename, len(graphic_data)))
    count = 0
    for j in graphic_data:
        gfx_source_file.write("{0:#04x}".format(j))
        count += 1
        if count < len(graphic_data):
            gfx_source_file.write(", ")
            if not (count % 16): # Place a new line every 16 values
                gfx_source_file.write("\n  ")
    gfx_source_file.write("\n};")

    gfx_source_file.close()

    # Generate the C header file
    gfx_header_file = open(gfx_header_filename, "w")

    gfx_header_file.write("/** Gfx generated from %s */\n\n" % (graphic_fname))

    gfx_header_file.write("#ifndef __GFX_%s_H__\n" % (output_filename.upper()))
    gfx_header_file.write("#define __GFX_%s_H__\n\n" % (output_filename.upper()))

    gfx_header_file.write("#include <stdint.h>\n\n")

    gfx_header_file.write("#define GFX_%s_HEIGHT (%d)\n" % (output_filename.upper(), height))
    gfx_header_file.write("#define GFX_%s_WIDTH  (%d)\n\n" % (output_filename.upper(), width))

    gfx_header_file.write("extern const uint8_t gfx_%s[%d];\n\n" % (output_filename, len(graphic_data)))

    gfx_header_file.write("#endif // __GFX_%s_H__" % (output_filename.upper()))

    gfx_header_file.close()

def main():
    global args
    parser = argparse.ArgumentParser(description='Generate font lookup tables for the OpenDPS firmware')

    parser.add_argument('-s',  '--font_size',    type=int, help="The font pt size to use")
    parser.add_argument('-sp', '--font_spacing', type=int, help="The number of pixels to space characters by")
    parser.add_argument('-o',  '--output',       type=str, required=True, help="The output file name")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-f',  '--font_file',    type=str, help="The font to use")
    group.add_argument('-i',  '--image_file',   type=str, help="The image file to use")
    group.add_argument('-l',  '--lookup_table',           help="Generate the lookup table", dest="lookup_table", action="store_true")
    group.set_defaults(lookup_table=False)

    args, unknown = parser.parse_known_args()

    if args.font_file:

        # Check font file actually exists
        if not os.path.exists(args.font_file):
            print("Can't find file %s" % (args.font_file))
            sys.exit(1)

        # If this is a font file ensure that a font_size has been specified
        if args.font_size:
            characters = "0123456789.VA" # The characters to generate a lookup table of
            convert_font_to_c(args.font_file, characters, args.font_size, args.font_spacing, args.output)
        else:
            print("error: argument -s/--font_size is required for fonts")
            sys.exit(1)

    elif args.image_file:

        # Check image file actually exists
        if not os.path.exists(args.image_file):
            print("Can't find file %s" % (args.image_file))
            sys.exit(1)

        convert_graphic_to_c(args.image_file, args.output)

    elif args.lookup_table:

        generate_pixel_lookup_table(args.output)

if __name__ == "__main__":
    main()
