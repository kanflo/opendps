#!/bin/bash

# Builds OpenDPS binaries in docker and then copies it over to host machine

set -eEuo pipefail
umask 022

declare CONTAINER_ID
cleanup() {
    if [ -n ${CONTAINER_ID:-} ] ; then
        docker stop $CONTAINER_ID \
            || docker kill $CONTAINER_ID \
            || true
        docker rm $CONTAINER_ID \
            || true
    fi
}
trap cleanup EXIT HUP INT

PROJECT=$(git rev-parse --short HEAD)
echo "Building opendps git version: $PROJECT"

docker rmi --no-prune opendps_$PROJECT || true
docker build -t opendps_$PROJECT docker

CONTAINER_ID=$(docker run -td -u opendps -v $(readlink -f .):/parent:ro opendps_$PROJECT bash)
docker exec -u opendps $CONTAINER_ID /build.sh "$@"
docker cp $CONTAINER_ID:/home/opendps/code/opendps/opendps.elf ./opendps/opendps.elf
docker cp $CONTAINER_ID:/home/opendps/code/opendps/opendps.bin ./opendps/opendps.bin
docker cp $CONTAINER_ID:/home/opendps/code/dpsboot/dpsboot.elf ./dpsboot/dpsboot.elf
docker cp $CONTAINER_ID:/home/opendps/code/dpsboot/dpsboot.bin ./dpsboot/dpsboot.bin
