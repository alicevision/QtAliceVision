message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.11)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.1.0)


# Build options
option(BUILD_IMAGEIO "Build qtAliceVisionImageIO plugin" ON)
option(BUILD_DEPTHMAPENTITY "Build depthMapEntity plugin" ON)
option(BUILD_SFM "Build qtAliceVision and qmlSfmData plugin" ON)

message(STATUS "BUILD_IMAGEIO: ${BUILD_IMAGEIO}")
message(STATUS "BUILD_DEPTHMAPENTITY: ${BUILD_DEPTHMAPENTITY}")
message(STATUS "BUILD_SFM: ${BUILD_SFM}")


# CMake Find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Qt
find_package(Qt6Core REQUIRED)
find_package(Qt6Quick REQUIRED)
find_package(Qt6Qml REQUIRED)
find_package(Qt6Charts REQUIRED)
find_package(Qt6Gui REQUIRED)
# Some Qt distributions (notably aqtinstall framework builds on macOS) do not auto-create the
# private-module targets via find_package(Qt6Gui); request Qt6GuiPrivate explicitly so that
# Qt6::GuiPrivate exists. Harmless where it is already provided (Windows/Linux official builds).
find_package(Qt6GuiPrivate REQUIRED)
find_package(Qt6 COMPONENTS ShaderTools)


set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)
add_definitions(-DQT_NO_KEYWORDS)

# comment to get qDebug outputs
add_definitions(-DQT_NO_DEBUG_OUTPUT)

# for performance reasons let Qt handle jpeg, png and ico formats
add_definitions(-DQT_ALICEVISIONIMAGEIO_USE_FORMATS_BLACKLIST)


# AliceVision dependency
find_package(AliceVision REQUIRED)


# macOS: the QML / imageformats plugins link AliceVision frameworks and their
# dependencies (e.g. libceres) via @rpath but installed no rpath of their own, so at
# load time dyld fell back to system/Homebrew copies — notably a libceres lacking the
# glog symbols statically bundled into AliceVision's — and the plugins failed to load
# ("Symbol not found"). Set relocatable @loader_path rpaths pointing at AliceVision's
# lib dir as laid out in a Meshroom standalone install (the plugins under
# qtPlugins/qml/<Module>/ and qtPlugins/imageformats/ sit beside aliceVision/lib).
# Relative paths keep the binaries redistributable — no absolute or system-specific
# paths are baked in. No-op where rpath is unused.
if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH
        "@loader_path/../../../aliceVision/lib"   # qml/<Module>/  -> <root>/aliceVision/lib
        "@loader_path/../../aliceVision/lib"      # imageformats/  -> <root>/aliceVision/lib
        "@loader_path"
        "@loader_path/..")
endif()


# Builds
add_subdirectory(src)
