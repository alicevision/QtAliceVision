#!/bin/bash
set -ex

AVDEPS_VERSION=2026.03.30
ROCKY_VERSION=9
CUDA_VERSION=12.1.1
QT_VERSION=6.11.1  # 6.11.1 6.10.3 6.9.3 6.8.3

DOCKER_TAG=alicevision/qt-av-deps:qt${QT_VERSION}-avdeps${AVDEPS_VERSION}-rocky${ROCKY_VERSION}-cuda${CUDA_VERSION}

docker build \
    --rm \
    --progress=plain \
    --build-arg "QT_VERSION=${QT_VERSION}" \
    --tag "${DOCKER_TAG}" \
    -f Dockerfile_rocky .

echo "# To launch the docker image:"
echo docker run -it ${DOCKER_TAG} /bin/bash


