BINARY = dpsboot

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
CFLAGS = -I. -DGIT_VERSION=\"$(GIT_VERSION)\"

OBJS = \
	flashlock.o \
	hw.o \
	past.o \
	tick.o \
	ringbuf.o \
	uframe.o \
	protocol.o \
	stdio.o \
	bootcom.o \
	crc16.o

# Bootloader linker script
LDSCRIPT = stm32f100_boot.ld

#include ../stm32common/makefile.inc
include ../libopencm3.target.mk
