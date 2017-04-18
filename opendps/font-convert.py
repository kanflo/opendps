#!/usr/bin/python

import subprocess
from subprocess import call
import sys


# file rusk.png
# rusk.png: PNG image data, 209 x 35, 8-bit/color RGB, non-interlaced
# convert -size 209x35 xc:#0f0 colormask.png
# composite rusk.png -compose Multiply colormask.png prick.png


# Execute cmd in a shell returning stdout
def shell(cmd):
	return subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout.read()

# Return size of named image as the tuple (width, height)
def image_size(fname):
	cmd = "file %s" % (fname)
#	info = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout.read().replace(" ", "")
	info = shell(cmd).replace(" ", "")
	(width, height) = info.split(",")[1].split("x")
	return (int(width), int(height))

def convert_font(font_fname, font_width_fname, characters, font_size):
	print "Converting %s to font-%d.c" % (font_fname, font_size)
	glyph_widths = []
	glyph_sizes = []
	(width, height) = image_size(font_width_fname)
	(fwidth, fheight) = image_size(font_fname)
	if fwidth != width or fheight != height:
		print("Something is fishy with your font.")
		print("The font pixmap and the glyph pixmap have different sizes.")
		sys.exit(1)

	cmd = "magick %s -depth 8 rgb:-" % (font_width_fname)
	rgb = shell(cmd)
#	rgb = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout.read()
	i = 0
	ch_width = 0
	total_width = 0;
	while True:
		ch_width += 1
		r = ord(rgb[i])
		g = ord(rgb[i+1])
		b = ord(rgb[i+2])
		if r == b == g == 0xff:
			glyph_widths.append(ch_width)
			total_width += ch_width
			ch_width = 0
		i += 3
		if i >= len(rgb):
			break

	if total_width != width or len(glyph_widths) != len(characters):
		print("Something is fishy with your font.")
		print("The width does not match the specified glyphs.")
		print("If you added a character you need to add it in the 'characters' array.")
		sys.exit(1)

	x_position = 0;
	shell("echo \"#include <stdint.h>\" > font-%d.c" % (font_size))
	shell("echo >> font-%d.h" % (font_size))
	shell("echo \"const uint32_t font_%d_height = %d;\" >> font-%d.c" % (font_size, height, font_size))
	shell("echo \"const uint32_t font_%d_num_glyphs = %d;\" >> font-%d.c" % (font_size, len(characters), font_size))
	shell("echo >> font-%d.c" % (font_size))


	for i in range(0, len(characters)):
		# Crop, convert to rgb888 and then to bgr565
		cmd = "convert -crop %dx%d+%d+0 %s - | magick - -depth 8 rgb:- | ./rgbtobgr565 > temp.565" % (glyph_widths[i], height, x_position, font_fname)
		shell(cmd)
		cmd = "xxd -c 16 -i temp.565 | sed 's/unsigned char/const uint8_t/g' | sed 's/unsigned int/const uint32_t/g' | sed 's/temp_565/font_%d_%s/' >> font-%d.c" % (font_size, characters[i], font_size)
		shell(cmd)
		shell("echo >> font-%d.c" % (font_size))

		x_position += glyph_widths[i]

	# Output glyph widths
	shell("echo \"const uint8_t font_%d_widths[%d] = {\" >> font-%d.c" % (font_size, len(characters), font_size))
	for i in range(0, len(characters)):
		shell("echo \"  %d, \" >> font-%d.c" % (glyph_widths[i], font_size))
	shell("echo \" };\" >> font-%d.c" % (font_size))
	shell("echo >> font-%d.c" % (font_size))

	# Output glyph sizes
	shell("echo \"const uint16_t font_%d_sizes[%d] = {\" >> font-%d.c" % (font_size, len(characters), font_size))
	for i in range(0, len(characters)):
		shell("echo \"  %d, \" >> font-%d.c" % (2 * height * glyph_widths[i], font_size))
	shell("echo \" };\" >> font-%d.c" % (font_size))
	shell("echo >> font-%d.c" % (font_size))

	# Output glyph pix pointers
	shell("echo \"const uint16_t *font_%d_pix[%d] = {\" >> font-%d.c" % (font_size, len(characters), font_size))
	for i in range(0, len(characters)):
		shell("echo \"  (uint16_t*) &font_%d_%s, \" >> font-%d.c" % (font_size, characters[i], font_size))
	shell("echo \" };\" >> font-%d.c" % (font_size))
	shell("echo >> font-%d.c" % (font_size))

	shell("cat font-%d.c | grep const | sed 's/const/extern const/g' | cut -d= -f1 | sed 's/ $/;/g' > font-%d.h" % (font_size, font_size))
	shell("rm temp.565")


characters = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'dot', 'v', 'a', 'dash']
convert_font("gfx/fonts/ubuntu_condensed_18.png", "gfx/fonts/ubuntu_condensed_18_width.png", characters, 0)
convert_font("gfx/fonts/ubuntu_condensed_48.png", "gfx/fonts/ubuntu_condensed_48_width.png", characters, 1)
print("Success! If you added glyphs, you need to fix tft_putch(...) in tft.c")