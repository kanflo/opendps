PROGRAM=dpsproxy

# The baudrate used for serial communications, defaults to 115200
BAUDRATE ?= 115200

OTA=1
EXTRA_COMPONENTS=extras/rboot-ota extras/stdin_uart_interrupt
PROGRAM_INC_DIR = . ./../opendps ./uhej
PROGRAM_SRC_DIR=. ./uhej
PROGRAM_CFLAGS+=-DCONFIG_BAUDRATE=$(BAUDRATE)
include esp-open-rtos/common.mk
