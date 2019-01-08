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
make -C opendps elf bin "$@"
make -C dpsboot elf bin "$@"
