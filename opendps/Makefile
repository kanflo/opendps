BINARY = opendps

# Build with commandline interface rather than serial interface
COMMANDLINE := 0

# Set build model, defaults to DPS5005
MODEL := DPS5005

# Include splash screen
SPLASH_SCREEN := 0

# Build wifi version (only change being that the wifi icon will start flashing
# on power up)
WIFI := 1

# Maximum current your DPS model can provide. Eg 5000mA for the DPS5005
MAX_CURRENT := 5000
# Please note that the UI currently does not handle settings larger that 9.99A

# Color space for the DPS display. Most units use GBR but RGB has been spotted in the wild
COLORSPACE ?= 0

# Optional tinting for UI elements
TINT ?= ffffff

# Enable CC mode
CC_ENABLE ?= 1

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
CFLAGS = -I. -DCONFIG_DPS_MAX_CURRENT=$(MAX_CURRENT) -DGIT_VERSION=\"$(GIT_VERSION)\" -Wno-missing-braces

# Output voltage and current limit are persisted in flash,
# this is the default setting
CFLAGS += -DCONFIG_DEFAULT_VOUT=5000 -DCONFIG_DEFAULT_ILIMIT=500 -DCOLORSPACE=$(COLORSPACE) -D$(MODEL)

# Application linker script
LDSCRIPT = stm32f100_app.ld

OBJS = \
    flashlock.o \
    bootcom.o \
    crc16.o \
    uui.o \
    uui_number.o \
    func_cv.o \
    hw.o \
    pwrctl.o \
    event.o \
    past.o \
    tick.o \
    tft.o \
    spi_driver.o \
    ringbuf.o \
    ili9163c.o \
    dbg_printf.o \
    mini-printf.o \
    font-0.o \
    font-1.o

ifeq ($(CC_ENABLE),1)
	CFLAGS +=-DCONFIG_CC_ENABLE
	OBJS += func_cc.o
endif

ifeq ($(SPLASH_SCREEN),1)
	CFLAGS +=-DCONFIG_SPLASH_SCREEN
endif

ifeq ($(WIFI),1)
	CFLAGS +=-DCONFIG_WIFI
endif

ifeq ($(COMMANDLINE),1)
	CFLAGS +=-DCONFIG_COMMANDLINE
	OBJS += cli.o command_handler.o
else
	CFLAGS +=-DCONFIG_SERIAL_PROTOCOL
	OBJS += uframe.o protocol.o protocol_handler.o
endif

include ../libopencm3.target.mk

fonts:
	@./font-convert.py $(TINT)

graphics:
	@./gfx-convert.sh $(TINT)

test:
	@make -C tests

doxygen:
	@doxygen config.dox
