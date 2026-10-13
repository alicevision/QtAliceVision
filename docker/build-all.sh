#!/usr/bin/env bash
set -euo pipefail

# Build all supported Docker images (AV_VERSION must be set, see docker/build-image.sh).

test -e docker/build-image.sh || {
  echo "This script must be run from the top level of the qtAliceVision tree" >&2
  exit 1
}

CUDA_VERSION=12.8.0 UBUNTU_VERSION=22.04 docker/build-ubuntu.sh
CUDA_VERSION=12.8.0 ROCKY_VERSION=9 docker/build-rocky.sh
