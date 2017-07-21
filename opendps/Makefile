BINARY = opendps

# Build with commandline interface rather than serial interface
COMMANDLINE := 0

# Include splash screen
SPLASH_SCREEN := 0

# Build wifi version (only change being that the wifi icon will start flashing
# on power up)
WIFI := 1

# Maximum current your DPS model can provide. Eg 5000mA for the DPS5005
MAX_CURRENT := 5000
# Please note that the UI currently does not handle settings larger that 9.99A

CFLAGS = -I. -DCONFIG_DPS_MAX_CURRENT=$(MAX_CURRENT)

# Output voltage and curren limit are persisted in flash,
# this is the default setting
CFLAGS += -DCONFIG_DEFAULT_VOUT=5000 -DCONFIG_DEFAULT_ILIMIT=500

OBJS = \
    ui.o \
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
	@./font-convert.py

graphics:
	@./gfx-convert.sh

test:
	@make -C tests

doxygen:
	@doxygen config.dox
