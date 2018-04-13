#!/bin/bash
#
# Convert gfx/png/*.png to bgr565 formatted header files
#


if [ "$#" -ne 1 ]; then
    TINT=ffffff
else
	TINT=$1
fi

if which magick >/dev/null; then
	gcc -o rgbtobgr565 rgbtobgr565.c

	cd gfx/png/

	for src in *.png ; do
	    base=`echo $src | cut -d. -f1`

	    dst=../../$base.h
	    echo Converting $src to $dst
		magick $src -colorspace gray +level-colors ,#$TINT -depth 8 rgb:- | ../../rgbtobgr565 > $base
		width="_width  "`file $src | sed 's/\ //g' | cut -d, -f 2 | cut -dx -f1`
		height="_height  "`file $src | sed 's/\ //g' | cut -d, -f 2 | cut -dx -f2`
		xxd -c 16 -i $base | sed 's/unsigned char/const uint8_t/g' | grep -v "unsigned int" > $dst
		echo "#define $base$width" >> $dst
		echo "#define $base$height" >> $dst
		rm $base
	done
else
	echo "Please install ImageMagick 7"
	exit
fi
