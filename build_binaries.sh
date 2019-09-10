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

mkdir output
chmod 777 output

CONTAINER_ID=$(docker run -td -u opendps -v $(readlink -f .):/parent:ro -v $(readlink -f .)/output:/output opendps_$PROJECT bash)
docker exec -u opendps $CONTAINER_ID /build.sh "$@"
