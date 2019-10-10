PROGRAM=dpsproxy

# The baudrate used for serial communications, defaults to 9600
BAUDRATE ?= 9600

OTA=1
EXTRA_COMPONENTS=extras/rboot-ota extras/stdin_uart_interrupt
PROGRAM_INC_DIR = . ./../opendps ./uhej
PROGRAM_SRC_DIR=. ./uhej
PROGRAM_CFLAGS+=-DCONFIG_BAUDRATE=$(BAUDRATE) -std=gnu99
include esp-open-rtos/common.mk
