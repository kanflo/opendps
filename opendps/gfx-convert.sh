#!/bin/bash
#
# Convert gfx/png/*.png to bgr565 formatted header files
#

if which magick >/dev/null; then
	gcc -o rgbtobgr565 rgbtobgr565.c

	cd gfx/png/

	echo "#ifndef __GRAPHICS_H__" > ../../graphics.h
	echo "#define __GRAPHICS_H__" >> ../../graphics.h

	for src in *.png ; do
	    base=`echo $src | cut -d. -f1`

	    dst=../../$base.h
	    echo Converting $src to $dst
		magick $src -depth 8 rgb:- | ../../rgbtobgr565 > $base
		width="_width = "`file $src | sed 's/\ //g' | cut -d, -f 2 | cut -dx -f1`
		height="_height = "`file $src | sed 's/\ //g' | cut -d, -f 2 | cut -dx -f2`

		xxd -c 16 -i $base | sed 's/unsigned char/const uint8_t/g' | sed 's/unsigned int/uint32_t/g' > $dst
		echo "uint32_t $base$width;" >> $dst
		echo "uint32_t $base$height;" >> $dst
		rm $base
		echo "#include \"$base.h\"" >> ../../graphics.h
	done
	echo "#endif // __GRAPHICS_H__" >> ../../graphics.h
else
	echo "Please install ImageMagick 7"
	exit
fi
