# syntax=docker/dockerfile:1.4

# =============================================================================
# Dockerfile
#
# Builds qtAliceVision on top of the qt-av-deps image (Dockerfile_rocky_deps),
# passed as DEPS_IMAGE.
#
# Built by docker/build-rocky.sh (which sets DEPS_IMAGE to the deps tag).
# =============================================================================

ARG DEPS_IMAGE

# Only what the build needs, so unrelated changes in the context do not
# invalidate the build cache. Bind-mounted below, never in the image.
FROM scratch AS sources
COPY CMakeLists.txt /
COPY src /src

FROM ${DEPS_IMAGE}

# One build tree per deps image (its CMake cache points into it); set by
# docker/build-rocky.sh.
ARG QTAV_BUILD_CACHE_ID=default

ENV QTAV_DEV=/opt/qtAliceVision_git         \
    QTAV_BUILD=/tmp/qtAliceVision_build     \
    QTAV_INSTALL=/opt/qtAliceVision_install

# What an application (Meshroom) needs to load the plugins.
ENV QML2_IMPORT_PATH="${QTAV_INSTALL}/qml" \
    QT_PLUGIN_PATH="${QTAV_INSTALL}"

WORKDIR "${QTAV_BUILD}"

# The sources are bind-mounted rather than COPY'd so they never end up in an
# image layer. Path hardcoded = ${QTAV_DEV}.
# The build tree lives in a BuildKit cache mount, kept across builds: a source
# change only recompiles what it affects. The install prefix is a regular layer.
# The bundle (in PATH) ships the same configs and libs as the install prefix:
# ignore it, CMake would otherwise mix both and fail to link.
RUN --mount=type=bind,from=sources,target=/opt/qtAliceVision_git \
    --mount=type=cache,id=qtalicevision-build-${QTAV_BUILD_CACHE_ID},target=${QTAV_BUILD},sharing=locked \
    cmake -DCMAKE_BUILD_TYPE=Release                    \
        -DCMAKE_GENERATOR=Ninja                         \
        -DCMAKE_INSTALL_PREFIX:PATH="${QTAV_INSTALL}"   \
        -DCMAKE_IGNORE_PREFIX_PATH:PATH="${AV_BUNDLE}"  \
        "${QTAV_DEV}" &&                                \
    cmake --build . --target install

WORKDIR /workspace
