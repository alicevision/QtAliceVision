#!/usr/bin/env bash
set -euo pipefail

# Build the Rocky Linux qtAliceVision images (qt-av-deps + qtalicevision).
# See docker/build-image.sh for the build logic and overridable variables.

export OS=rocky
export OS_VERSION="${ROCKY_VERSION:-9}"

exec docker/build-image.sh
