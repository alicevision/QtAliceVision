#!/usr/bin/env bash
set -euo pipefail

# Generic qtAliceVision Docker image builder, shared by build-rocky.sh and
# build-ubuntu.sh. It builds the qt-av-deps image (AliceVision + Qt), then the
# qtalicevision image.
#
# Required environment:
#   OS          rocky | ubuntu
#   OS_VERSION  e.g. 9 (rocky) or 22.04 (ubuntu)
#   AV_VERSION  tag version of the alicevision/alicevision base image
# Optional overrides:
#   CUDA_VERSION     (default 12.8.0)
#   QT_VERSION       (default 6.12.0)
#   QTAV_VERSION     (default: <branch>-<short-sha>)
#   REPO_OWNER       (default alicevision)
#   CONTAINER_ENGINE docker | podman (see docker/container-engine.sh)

: "${OS:?OS must be set (rocky|ubuntu)}"
: "${OS_VERSION:?OS_VERSION must be set}"

test -e "docker/Dockerfile_${OS}_deps" || {
  echo "This script must be run from the top level of the qtAliceVision tree" >&2
  exit 1
}

# shellcheck source=docker/container-engine.sh
. docker/container-engine.sh

: "${AV_VERSION:?AV_VERSION must be set (alicevision/alicevision:<AV_VERSION>-<os>...)}"
: "${CUDA_VERSION:=12.8.0}"
: "${QT_VERSION:=6.12.0}"  # 6.11.1 6.10.3 6.9.3 6.8.3
: "${REPO_OWNER:=alicevision}"

if [[ -z "${QTAV_VERSION:-}" ]]; then
  if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    # '/' (e.g. in "feat/..." branch names) is invalid in a Docker image tag.
    qtav_branch=$(git rev-parse --abbrev-ref HEAD)
    QTAV_VERSION="${qtav_branch//\//-}-$(git rev-parse --short HEAD)"
  else
    QTAV_VERSION="unknown"
  fi
fi

FLAVOUR="${OS}${OS_VERSION}-cuda${CUDA_VERSION}"
AV_IMAGE="${REPO_OWNER}/alicevision:${AV_VERSION}-${FLAVOUR}"
DEPS_DOCKER_TAG="${REPO_OWNER}/qt-av-deps:qt${QT_VERSION}-av${AV_VERSION}-${FLAVOUR}"
DOCKER_TAG="${REPO_OWNER}/qtalicevision:${QTAV_VERSION}-qt${QT_VERSION}-${FLAVOUR}"

echo "AV_IMAGE:     ${AV_IMAGE}"
echo "QT_VERSION:   ${QT_VERSION}"
echo "QTAV_VERSION: ${QTAV_VERSION}"

print_push() {
  echo ""
  echo "  To upload results:"
  for _tag in "$@"; do
    echo "  ${CONTAINER_ENGINE} push ${_tag}"
  done
  echo ""
}

echo "--== BUILD DEPENDENCIES ==--"
"${CONTAINER_ENGINE}" build \
  --rm \
  --progress=plain \
  --build-arg AV_IMAGE="${AV_IMAGE}" \
  --build-arg QT_VERSION="${QT_VERSION}" \
  --tag "${DEPS_DOCKER_TAG}" \
  -f "docker/Dockerfile_${OS}_deps" .

print_push "${DEPS_DOCKER_TAG}"

echo "--== BUILD QTALICEVISION ==--"
"${CONTAINER_ENGINE}" build \
  --rm \
  --progress=plain \
  --build-arg DEPS_IMAGE="${DEPS_DOCKER_TAG}" \
  --build-arg QTAV_BUILD_CACHE_ID="av${AV_VERSION}-qt${QT_VERSION}-${FLAVOUR}" \
  --tag "${DOCKER_TAG}" \
  -f docker/Dockerfile .

echo "  To launch the image:"
echo "  ${CONTAINER_ENGINE} run -it ${DOCKER_TAG}"
print_push "${DEPS_DOCKER_TAG}" "${DOCKER_TAG}"
