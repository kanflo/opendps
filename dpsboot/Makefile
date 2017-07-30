BINARY = dpsboot

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
CFLAGS = -I. -I../opendps -DGIT_VERSION=\"$(GIT_VERSION)\"

OBJS = \
	common.o \
	hw.o

# Bootloader linker script
LDSCRIPT = stm32f100_boot.ld

#include ../stm32common/makefile.inc
include ../libopencm3.target.mk
