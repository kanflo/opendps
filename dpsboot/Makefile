BINARY = dpsboot

CFLAGS = -I. -I../stm32common

OBJS = \
	hw.o \
	protocol_handler.o \
	past.o \
	tick.o \
	ringbuf.o \
	uframe.o \
	protocol.o \
	stdio.o \

# Bootloader linker script
LDSCRIPT = stm32f401_boot.ld

#include ../stm32common/makefile.inc
include ../libopencm3.target.mk
