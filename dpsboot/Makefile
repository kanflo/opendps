BINARY = dpsboot

# The baudrate used for serial communications, defaults to 9600
BAUDRATE ?= 9600

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
CFLAGS = -I. -I../opendps -DGIT_VERSION=\"$(GIT_VERSION)\" -DCONFIG_PAST_NO_GC -DCONFIG_BAUDRATE=$(BAUDRATE)
# Future optimisation: saves ~600 bytes but does not work for gcc <= 7
#CFLAGS += -flto

EXT_DIR = ../opendps/

DPS_OBJ_DIR = dpsobj
DPS_OBJS = \
	$(DPS_OBJ_DIR)/protocol.o \
	$(DPS_OBJ_DIR)/uframe.o \
	$(DPS_OBJ_DIR)/crc16.o \
	$(DPS_OBJ_DIR)/bootcom.o \
	$(DPS_OBJ_DIR)/flashlock.o \
	$(DPS_OBJ_DIR)/past.o \
	$(DPS_OBJ_DIR)/ringbuf.o \
	$(DPS_OBJ_DIR)/tick.o

OBJS = \
	hw.o \
	dpsboot.o \
	$(DPS_OBJS)

# Bootloader linker script
LDSCRIPT = stm32f100_boot.ld

#include ../stm32common/makefile.inc
include ../libopencm3.target.mk

$(DPS_OBJ_DIR)/%.o : $(EXT_DIR)/%.c
	@mkdir -p $(DPS_OBJ_DIR)
	@printf "  CC      $(*).c\n"
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

