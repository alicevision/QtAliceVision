#!/usr/bin/env bash
set -euo pipefail

# Build the Ubuntu qtAliceVision images (qt-av-deps + qtalicevision).
# See docker/build-image.sh for the build logic and overridable variables.

export OS=ubuntu
export OS_VERSION="${UBUNTU_VERSION:-22.04}"

exec docker/build-image.sh
