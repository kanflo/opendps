#!/bin/bash

set -eEuo pipefail
umask 022

sudo mkdir -p /home/opendps/code
sudo cp -R /parent/{*,.git*} /home/opendps/code
sudo chown -R opendps:opendps /home/opendps

cd /home/opendps/code
git submodule init
git submodule update
make -j -C libopencm3
make -C opendps fonts "$@"
make -C opendps elf bin MODEL=DPS5005 "$@"
touch opendps/dps-model.h
make -C opendps elf bin MODEL=DPS5015 "$@"
touch opendps/dps-model.h
make -C opendps elf bin MODEL=DPS3005 "$@"
make -C dpsboot elf bin "$@"

cp -f /home/opendps/code/opendps/opendps*.elf /output/
cp -f /home/opendps/code/opendps/opendps*.bin /output/
cp -f /home/opendps/code/dpsboot/dpsboot.elf /output/
cp -f /home/opendps/code/dpsboot/dpsboot.bin /output/

