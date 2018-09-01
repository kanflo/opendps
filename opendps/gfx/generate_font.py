# -*- coding: utf-8 -*-

from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont
from PIL import ImageChops

canvas = Image.new("RGB", (1000, 100), (0,0,0))
canvas_width = Image.new("RGB", (1000, 100), (0,0,0))
draw = ImageDraw.Draw(canvas)

font = ImageFont.truetype("Ubuntu-C.ttf", 24)

content = "0123456789.VA"

def meassure(im, border):
  bg = Image.new(im.mode, im.size, border)
  diff = ImageChops.difference(im, bg)
  return diff.getbbox()

font_canvas = [None] * len(content)
font_canvas_size = [None] * len(content)
for i in range(0, len(content)):
    # Draw a letter into a new 100x100 box
    font_canvas[i] = Image.new("RGB", (100,100), (0, 0, 0))
    font_draw = ImageDraw.Draw(font_canvas[i])
    font_draw.text((0, 0), content[i], (255, 255, 255), font=font)
    font_canvas_size[i] = meassure(font_canvas[i], (0,0,0))

for i in range(0, len(content)):
    # left, upper, right, lower
    font_canvas_size[i] = (
            font_canvas_size[i][0], 
            min(x[1] for x in font_canvas_size), 
            font_canvas_size[i][2], 
            max(x[3] for x in font_canvas_size), 
    )

left = 0;
for i in range(0, len(content)):

    # Crop box around content, using the array we calulated in last loop
    font_canvas[i] = font_canvas[i].crop(font_canvas_size[i])

    # Add to sprites next to each other.
    width, height = font_canvas[i].size
    canvas.paste(font_canvas[i], (left, canvas.size[1] - height))
    left += width
    canvas_width.putpixel((left-1, 0), (255,255,255))

canvas = canvas.crop(meassure(canvas, (0,0,0,0)))
canvas_width = canvas_width.crop(meassure(canvas, (0,0,0,0)))
canvas.save("fonts/ubuntu_condensed_24.png")
canvas_width.save("fonts/ubuntu_condensed_24_width.png")
