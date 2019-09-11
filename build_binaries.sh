#!/bin/bash

# Builds OpenDPS binaries in docker and then copies it over to host machine

set -eEuo pipefail
umask 022

PROJECT=$(git rev-parse --short HEAD)
echo "Building opendps git version: $PROJECT"

docker rmi --no-prune opendps_$PROJECT || true
docker build -t opendps_$PROJECT docker

mkdir -p output
chmod 777 output

docker run -t -u opendps -v $(readlink -f .):/parent:ro -v $(readlink -f .)/output:/output opendps_$PROJECT /parent/docker/build.sh "$@"
