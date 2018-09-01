#!/bin/bash

# Builds OpenDPS binaries in docker and then copies it over to host machine

set -e
git rev-parse --short HEAD
PROJECT=$(git rev-parse --short HEAD)
echo "Building opendps git version: $PROJECT"
docker build -t $PROJECT -f Dockerfile3 .

CONTAINER_ID=$(docker create $PROJECT bash)
echo "CONTAINER ID: $CONTAINER_ID"
docker cp $CONTAINER_ID:/home/opendps/code/opendps/opendps.elf ./opendps/opendps.elf
docker cp $CONTAINER_ID:/home/opendps/code/opendps/opendps.bin ./opendps/opendps.bin
docker cp $CONTAINER_ID:/home/opendps/code/dpsboot/dpsboot.elf ./dpsboot/dpsboot.elf
docker cp $CONTAINER_ID:/home/opendps/code/dpsboot/dpsboot.bin ./dpsboot/dpsboot.bin
docker rm $CONTAINER_ID
