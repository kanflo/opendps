#!/bin/bash
#
# Dump past area to pastdump.bin. Requires OpenOCD
#
arm-none-eabi-gdb -ex "target remote localhost:3333" -ex "monitor reset halt" -ex "dump binary memory pastdump.bin 0x0800f800 0x08010000" -ex "monitor resume" -ex "quit" --batch
hexdump -C pastdump.bin
