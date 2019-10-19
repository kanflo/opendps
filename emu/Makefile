TARGET = dpsemu
LIBS = -lm -lpthread
CC = gcc
CFLAGS = -m32 -g -Wall -I. -I../opendps -DCONFIG_DPS_MAX_CURRENT=5000 -Ddbg_printf=printf -DDPS5005 -DDPS_EMULATOR -DCONFIG_CC_ENABLE -DCOLOR_INPUT=WHITE -DCOLOR_VOLTAGE=WHITE -DCOLOR_AMPERAGE=WHITE -Wmissing-braces

.PHONY: default all clean

default: $(TARGET)
all: default

vpath %.c ../opendps

SRCS = opendps.c \
	dpsemul.c \
	event.c \
	past.c \
	flash.c \
	ringbuf.c \
	pwrctl.c \
	uui.c \
	uui_number.c \
	tft.c \
	hw.c \
	dac.c \
	bootcom.c \
	crc16.c \
	uframe.c \
	protocol.c \
	protocol_handler.c \
	func_cv.c \
	func_cc.c \
	misc.c \
    font-full_small.o \
    font-meter_small.o \
    font-meter_medium.o \
    font-meter_large.o \
	gfx-cc.o \
    gfx-crosshair.o \
    gfx-cv.o \
    gfx-cl.o \
    gfx-padlock.o \
    gfx-thermometer.o \
    gfx-wifi.o \
	gfx-power.o \
	settings_calibration.o \

#OBJECTS = $(patsubst ../%, %, $(patsubst %.c, %.o, $(SRCS)))
OBJECTS = $(patsubst %.c, %.o, $(SRCS))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
